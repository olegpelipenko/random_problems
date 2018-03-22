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

func isError() bool {
	return false
}

var refreshLockTimeout func(string) error

func main() {
	redisQueueLock := "queue_lock"
	redisMessagesQueue := "messages"
	redisLockTimeout := time.Duration(5 * time.Second)
	messageSleepTimeout := time.Duration(500 * time.Millisecond)
	subscriberPopTimeout := time.Duration(500 * time.Millisecond)

	addr := flag.String("addr", "", "Connection string, format: host:port")
	timeoutArg := flag.Int("message_sleep_timeout", 0, "Generation timeout for message queue")
	//timeoutArg := flag.Int("getErrors", 0, "Retrieves error messages from db")
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
			log.Fatal("Failed to lock redis marker")
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
						log.Printf("failed to push message to queue %v", err)
					}

					log.Printf("Message '%s' was pushed, queue length: %d\n", message, val)
				} else {
					log.Println("I was lose a lock")
					break
				}
			}
		} else {
			log.Println("I'm subscriber")

			res, err := client.BLPop(subscriberPopTimeout, redisMessagesQueue).Result()
			if err != nil {
				log.Printf("Error while poping message: %v", err)
				continue
			}

			log.Println("Message:", res)
		}
	}
}
