package main

import (
	redis "github.com/go-redis/redis"
	"flag"
	"time"
	"log"
	"math/rand"
	"strconv"
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
	redisMessagesChannel := "messages_channel"
	redisErrorsQueue := "errors"
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
	redisMyId := generateRandomString(6) + "_" + strconv.FormatInt(time.Now().Unix(), 10)
	log.Println("Trying to acquire lock, my id:", redisMyId)

	channels, err := client.PubSubChannels(redisMessagesChannel).Result()
	if err != nil {
		log.Fatal("Failed to list channels", err)
	}
	log.Println(channels)
}
