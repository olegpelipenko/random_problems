#!/bin/bash
for (( i=0; i<100; i++ )); 
do
   ./redis_lock_pubsub -addr=localhost:6379 &
done
