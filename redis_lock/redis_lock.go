package main

import (
	redis "github.com/go-redis/redis"
	"fmt"
	"flag"
)


func main() {
	addr := flag.String("addr", "", "Connection string, format: host:port")
	flag.Parse()

	if *addr == "" {
		fmt.Errorf("addr is not set")
	}

	client := redis.NewClient(&redis.Options{Addr: *addr})
	if client == nil {
		fmt.Errorf("can't run new redis client, probably wrong addr or max number of clients is reached")
	}

	// setex pages:about 0.1 '<h1>about us</h1>....'
	err := client.SetNX("key", "value", 1).Err()
	fmt.Println(err)
}
