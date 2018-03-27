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

var refreshLockTimeout func(string) error
var popAllErrors func(errRange * []string) error

// Goroutine for each client to generate messages for subscription
func sendMessagesToClient(channel string, client * redis.Client, messageSleepTimeout time.Duration) {
	for {
		time.Sleep(messageSleepTimeout)
		message := generateRandomString(6)

		res, err := client.Publish(channel, message).Result()
		if err != nil {
			log.Println("Failed to publish message", err)
			continue
		}

		if res == 0 {
			log.Println("Client doesn't receive message, stop sending")
			break
		} else {
			log.Println("Message", message, "for", channel, "was sent")
		}
	}
}

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

func main() {
	redisClientsChannelsPattern := "client*"
	redisErrorsQueue := "errors"
	redisQueueLock := "queue_lock"
	redisLockTimeout := time.Duration(2 * time.Second)
	messageSleepTimeout := time.Duration(500 * time.Millisecond)

	addr := flag.String("addr", "", "Connection string, format: host:port")
	getError := flag.Bool("getErrors", false, "Returns errors")

	flag.Parse()

	if *addr == "" {
		log.Fatal("addr is not set")
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
	redisMyId := "client_" + generateRandomString(6) + "_" + strconv.FormatInt(time.Now().Unix(), 10)
	log.Println("Trying to became publisher, my id:", redisMyId)

	for {
		cmd := client.SetNX(redisQueueLock, redisMyId, redisLockTimeout)
		if cmd == nil {
			log.Fatal("Failed to lock")
		}

		if cmd.Val() == true {
			log.Println("I'm publisher")

			go hearthbeat(client, redisQueueLock, redisLockTimeout, redisMyId)

			// Run message sender for already existing clients
			channels, err := client.PubSubChannels(redisClientsChannelsPattern).Result()
			if err != nil {
				log.Fatal("Errror listing channels", err)
			}
			for _, channel := range channels {
				go sendMessagesToClient(channel, client, messageSleepTimeout)
			}

			newClientsSub := client.Subscribe(redisClientsChannelsPattern)
			for {
				msg, err := newClientsSub.ReceiveMessage()
				if err != nil {
					log.Println("Receiving client error:", err)
					continue
				}

				log.Println("New client", msg.Payload)
				go sendMessagesToClient(msg.Payload, client, messageSleepTimeout)
			}

			newClientsSub.Unsubscribe(redisClientsChannelsPattern)
			newClientsSub.Close()
		} else {
			log.Println("I'm subscriber")

			// Registering myself in a clients subscription
			_, err := client.Publish(redisClientsChannelsPattern, redisMyId).Result()
			if err != nil {
				log.Fatal("Failed to register:", err)
			}

			roleSwitchChannel := make(chan struct{})
			go tryToBePublisher(client, redisQueueLock, &roleSwitchChannel)

			// My personal subscription
			msgSub := client.Subscribe(redisMyId)

			// Receiving messages until publisher exists
			for {
				select {
				case _, ok := <- roleSwitchChannel:
					if !ok {
						roleSwitchChannel = nil
					}
				default:
					msg, err := msgSub.ReceiveTimeout(time.Second)
					if err != nil {
						log.Println("Receiving message error:", err)
						continue
					}
					log.Println("Received message", msg)

					// We are receiving subscription object on a first call, let's ignore it
					_, ok := msg.(*redis.Message)
					if !ok {
						continue
					}

					isError := rand.Intn(100) < 5
					if isError {
						log.Println("This message is an error:", msg)

						_, err = client.LPush(redisErrorsQueue, msg.(*redis.Message).Payload).Result()
						if err != nil {
							log.Println("failed to push error", err)
						}
					}
				}

				if roleSwitchChannel == nil {
					msgSub.Unsubscribe(redisMyId)
					msgSub.Close()
					break
				}
			}
		}
	}
}
