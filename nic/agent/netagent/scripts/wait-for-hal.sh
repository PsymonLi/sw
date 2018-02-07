#!/bin/bash

# MAX_RETRIES guards the maxium time we wait for HAL to come up. It waits for 2**(MAX_RETRIES) - 1 seconds
MAX_RETRIES=8
HAL_GRPC_PORT="${HAL_GRPC_PORT:-50054}"
HAL_SERVER="localhost:$HAL_GRPC_PORT"
HAL_UP=-1
until (( HAL_UP == 0 )) || (( i == MAX_RETRIES ))
do
	timeout="$((2 ** i))"
	echo "Waiting for HAL GRPC server to be up. Sleeping for $timeout seconds..."
	sleep "$timeout"
	curl "$HAL_SERVER"
	HAL_UP="$?"
	let "i++"
done
if [ $i -eq $MAX_RETRIES ]; then
	echo "HAL server failed to come up"
	exit 1
fi
