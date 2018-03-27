package main

import (
	redis "github.com/go-redis/redis"
	"flag"
	"time"
	"log"
	"math/rand"
	"strconv"
	"errors"
	"runtime"
)

var symbols = []rune("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890")

func generateRandomString(length int) string{
	rand.Seed(time.Now().UTC().UnixNano())
	b := make([]rune, length)
	for i := range b {
		b[i] = symbols[rand.Intn(len(symbols))]
	}
	return string(b)
}

var refreshLockTimeout func(string) error
var popAllErrors func(errRange * []string) error

func tryToBePublisher(client * redis.Client, redisQueueLock string, roleSwitchChannel * chan struct{}) {
	for {
		ttl, err := client.TTL(redisQueueLock).Result()
		if err != nil {
			log.Println("Error in TTL command:", err)
		}

		if ttl == 0 {
			// There is a gap when lock is destroyed by timeout. During this gap we are receiving 0 and then <0 value.
			// To reduce number of queries let's sleep for a second.
			time.Sleep(time.Second)
		} else {
			time.Sleep(ttl)
		}

		currentLock, err := client.Exists(redisQueueLock).Result()
		if err != nil {
			log.Println("Error getting lock", err)
			continue
		}
		if currentLock == 0 {
			log.Println("I'm trying to became a pulisher")
			close(*roleSwitchChannel)
			break
		}
	}
}


func hearthbeat(client * redis.Client, redisQueueLock string, redisLockTimeout time.Duration, redisMyId string) {
	for {
		time.Sleep(redisLockTimeout - time.Second)

		// Transaction to refresh my timeout on a lock
		refreshLockTimeout := func(myId string) error {
			err := client.Watch(func(tx *redis.Tx) error {
				holderId, err := tx.Get(redisQueueLock).Bytes()
				if err != nil && err != redis.Nil {
					return err
				}

				if string(holderId) == myId {
					_, err = tx.Pipelined(func(pipe redis.Pipeliner) error {
						pipe.Expire(redisQueueLock, redisLockTimeout)
						log.Println("Lock was refreshed")
						return nil
					})
				} else {
					log.Println("Lock holder changed")
					return errors.New("lock holder changed")
				}
				return err
			}, myId)
			if err == redis.TxFailedErr {
				return refreshLockTimeout(myId)
			}
			return err
		}

		refreshLockTimeout(redisMyId)
	}
}

func runWorkers(stopWorkers * chan struct{},
				client * redis.Client,
				redisMessagesQueue, redisErrorsQueue string) {
	for i := 0; i < runtime.NumCPU(); i++ {
		go func(stopWorkers * chan struct{}) {
			log.Println("Worker started")

			for {
				select {
				case _, ok := <-*stopWorkers:
					if !ok {
						stopWorkers = nil
					}
				default:
					message, err := client.BLPop(time.Second, redisMessagesQueue).Result()
					if err != nil {
						log.Printf("Error while poping message, maybe timeout: %v", err)
						continue
					}

					log.Println("Received message:", message)

					isError := rand.Intn(100) < 5
					if !isError {
						continue
					}

					log.Println("This message is an error")

					if len(message) != 2 {
						log.Println("Wrong number of args from pop")
						continue
					}

					_, err = client.LPush(redisErrorsQueue, message[1]).Result()
					if err != nil {
						log.Println("failed to push error", err)
					}
				}
				if stopWorkers == nil {
					break
				}
			}

			log.Println("Worker stopped")
		}(stopWorkers)
	}
}

func printErrors(client * redis.Client, redisErrorsQueue string) {
	var errRange []string
	popAllErrors := func(errRange * []string) error {
		err := client.Watch(func(tx *redis.Tx) error {
			var err error
			*errRange, err = tx.LRange(redisErrorsQueue, 0, -1).Result()
			if err != nil && err != redis.Nil {
				return err
			}

			_, err = tx.Pipelined(func(pipe redis.Pipeliner) error {
				pipe.Del(redisErrorsQueue)
				return nil
			})
			return err
		})
		if err == redis.TxFailedErr {
			return popAllErrors(errRange)
		}
		return err
	}

	if err := popAllErrors(&errRange); err != nil {
		log.Fatalf("error while getting errors list: '%v'", err)
	}

	log.Println("Errors:")
	for _, error := range errRange {
		log.Println(error)
	}
}

func main() {
	redisQueueLock := "queue_lock"
	redisErrorsQueue := "errors"
	redisMessagesQueue := "messages"
	redisLockTimeout := time.Duration(5 * time.Second)
	messageSleepTimeout := time.Duration(500 * time.Millisecond)

	addr := flag.String("addr", "", "Connection string, format: host:port")
	timeoutArg := flag.Int("message_sleep_timeout", 0, "Generation timeout for message queue")
	getError := flag.Bool("getErrors", false, "Returns errors")

	flag.Parse()

	if *addr == "" {
		log.Fatal("addr is not set")
	}
	if *timeoutArg != 0 {
		messageSleepTimeout = time.Duration(*timeoutArg)
	}

	client := redis.NewClient(&redis.Options{Addr: *addr})
	if client == nil {
		log.Fatal("can't run new redis client, probably wrong addr or max number of clients is reached")
	}
	defer client.Close()

	if *getError {
		printErrors(client, redisErrorsQueue)
		return
	}

	// Should be unique between all clients
	redisMyId := generateRandomString(6) + "_" + strconv.FormatInt(time.Now().Unix(), 10)
	log.Println("Trying to acquire lock, my id:", redisMyId)
	for {
		cmd := client.SetNX(redisQueueLock, redisMyId, redisLockTimeout)
		if cmd == nil {
			log.Fatal("Failed to lock")
		}

		if cmd.Val() == true {
			log.Println("I'm publisher")

			go hearthbeat(client, redisQueueLock, redisLockTimeout, redisMyId)

			// This instance should be publisher until it will be killed or it loses a lock
			for {
				// In a real life this operations may take a long time
				time.Sleep(messageSleepTimeout)
				message := generateRandomString(6)

				val, err := client.LPush(redisMessagesQueue, message).Result()
				if err != nil {
					log.Println("failed to push message", err)
				}

				log.Printf("Message '%s' was pushed, queue length: %d\n", message, val)
			}
		} else {
			log.Println("I'm subscriber")

			roleSwitchChannel := make(chan struct{})
			go tryToBePublisher(client, redisQueueLock, &roleSwitchChannel)

			stopWorkers := make(chan struct{})
			runWorkers(&stopWorkers, client, redisMessagesQueue, redisErrorsQueue)

			select {
			case _, ok := <- roleSwitchChannel:
				if !ok {
					close(stopWorkers)
					roleSwitchChannel = nil
				}
			}
		}
	}
}
