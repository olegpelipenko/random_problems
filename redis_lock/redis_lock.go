package main

import (
	redis "github.com/go-redis/redis"
	"flag"
	"time"
	"log"
	"math/rand"
	"strconv"
	"errors"
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

// Returns a non-negative pseudo-random number in [0,hi)
func generateRandomNumber(hi int) int {
	rand.Seed(time.Now().UTC().UnixNano())
	return rand.Intn(hi)
}

func isError() bool {
	return false
}

var refreshLockTimeout func(string) error
var popAllErrors func(errRange * []string) error

func main() {
	redisQueueLock := "queue_lock"
	redisErrorsQueue := "errors"
	redisMessagesQueue := "messages"
	redisLockTimeout := time.Duration(5 * time.Second)
	messageSleepTimeout := time.Duration(500 * time.Millisecond)
	subscriberPopTimeout := time.Duration(1 * time.Second)

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
		var errRange []string
		popAllErrors := func(errRange * []string) error {
			err := client.Watch(func(tx *redis.Tx) error {
				var err error
				*errRange, err = tx.LRange(redisErrorsQueue, 0, -1).Result()
				if err != nil && err != redis.Nil {
					return err
				}

				_, err = tx.Pipelined(func(pipe redis.Pipeliner) error {
					pipe.LRem(redisErrorsQueue, 0, -1)
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

		return
	}

	// Transaction to refresh my timeout on a lock
	refreshLockTimeout := func(myId string) error {
		err := client.Watch(func(tx *redis.Tx) error {
			holderId, err := tx.Get(redisQueueLock).Bytes()
			if err != nil && err != redis.Nil {
				return err
			}

			if string(holderId) == myId {
				_, err = tx.Pipelined(func(pipe redis.Pipeliner) error {
					pipe.Set(redisQueueLock, myId, redisLockTimeout)
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

			// This instance should be publisher until it will be killed or it loses a lock
			for {
				// In a real life this operations may take a long time
				time.Sleep(messageSleepTimeout)
				message := generateRandomString(6)

				// Check that i'm still holder of lock
				if err := refreshLockTimeout(redisMyId); err == nil {
					log.Println("I'm still publisher, timeout was refreshed")

					val, err := client.LPush(redisMessagesQueue, message).Result()
					if err != nil {
						log.Println("failed to push message", err)
					}

					log.Printf("Message '%s' was pushed, queue length: %d\n", message, val)
				} else {
					log.Println("I was lose a lock")
					break
				}
			}
		} else {
			log.Println("I'm subscriber")

			message, err := client.BLPop(subscriberPopTimeout, redisMessagesQueue).Result()
			if err != nil {
				log.Printf("Error while poping message: %v", err)
				continue
			}

			isError := generateRandomNumber(19) == 0
			if isError {
				log.Println("This is an error:", message)
				
				if len(message) != 2 {
					log.Println("Wrong number of args from pop")
					continue
				}

				_, err = client.LPush(redisErrorsQueue, message[1]).Result()
				if err != nil {
					log.Println("failed to push error", err)
				}
			}

			log.Println("Message:", message)
		}
	}
}
