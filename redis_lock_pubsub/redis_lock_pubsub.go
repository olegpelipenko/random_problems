package main

import (
	redis "github.com/go-redis/redis"
	"flag"
	"time"
	"log"
	"math/rand"
	"strconv"
	"errors"
	"net"
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
	redisClientsChannelsPattern := "client*"
	redisErrorsQueue := "errors"
	redisQueueLock := "queue_lock"
	messageSleepTimeout := time.Duration(500 * time.Millisecond)
	redisLockTimeout := time.Duration(5 * time.Second)

	addr := flag.String("addr", "", "Connection string, format: host:port")
	timeoutArg := flag.Int("message_sleep_timeout", 0, "Generation timeout for message queue")
	getError := flag.Bool("getErrors", false, "Returns errors")

	flag.Parse()

	if *addr == "" {
		log.Fatal("addr is not set")
	}
	if *timeoutArg != 0 {
		messageSleepTimeout = time.Duration(*timeoutArg)
		log.Println(messageSleepTimeout)
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

	// Should be unique between all clients
	redisMyId := "client_" + generateRandomString(6) + "_" + strconv.FormatInt(time.Now().Unix(), 10)
	log.Println("Trying to acquire lock, my id:", redisMyId)

	for {
		cmd := client.SetNX(redisQueueLock, redisMyId, redisLockTimeout)
		if cmd == nil {
			log.Fatal("Failed to lock")
		}

		if cmd.Val() == true {
			log.Println("I'm publisher")
			pubsub := client.Subscribe(redisClientsChannelsPattern)
			defer pubsub.Close() // LEAK?!

			go func() {
				// Lock refresher
				for {
					time.Sleep(time.Second)
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
			}()

			for {
				msg, err := pubsub.ReceiveMessage()
				if err != nil {
					log.Println("Receive new client error:", err)
					continue
				}

				log.Println("received", msg.Payload, "from", msg.Channel)

				// Create goroutine for each client
				channel := msg.Payload
				go func() {
					for {
						time.Sleep(messageSleepTimeout)
						message := generateRandomString(6)
						res, err := client.Publish(channel, message).Result()
						if err != nil {
							log.Fatal("Failed to publish message", err)
						}

						if res == 0 {
							log.Println("Client doesn't received message, stop sending")
							break;
						} else {
							log.Println("Message", message, "for", channel, "was sent", res)
						}
					}
				}()
			}
		} else {
			log.Println("I'm subscriber")

			_, err := client.Publish(redisClientsChannelsPattern, redisMyId).Result()
			if err != nil {
				log.Fatal("Failed to register:", err)
			}

			pubsub := client.Subscribe(redisMyId)
			defer pubsub.Close()  // LEAK!!!???

			for {
				msg, err := pubsub.ReceiveTimeout(time.Second)
				if err, ok := err.(*net.OpError); ok {
					// Timeout occurred, try to get a lock
					cmd := client.SetNX(redisQueueLock, redisMyId, redisLockTimeout)
					if cmd == nil {
						log.Fatal("Failed to lock")
					}

					if cmd.Val() == true {
						break
					} else {
						continue
					}
				} else {
					log.Println("Receive new message error:", err)
					continue
				}

				log.Println("Message received", msg)
			}
		}
	}
}
