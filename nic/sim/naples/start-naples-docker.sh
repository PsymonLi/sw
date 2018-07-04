#!/bin/bash

VER=v1
NAPLES_DIR=$HOME/naples/data
AGENT_PORT=9007
HAL_PORT=50054
CONTAINER="pensando/naples:$VER"

# create a dir so we can mount logs etc. from NAPLES into this
mkdir -p $NAPLES_DIR

echo "Purging old/dangling images, if any ..."
# remove any dangling images
# docker rmi -f $(docker images -q -f dangling=true)
docker images purge
docker rmi -f naples:$VER

echo "Loading docker image into registry ..."
docker load --input naples-docker-$VER.tgz

if [ "$1" == "--qemu" ]
  then
    echo "Running the NAPLES container in Qemu mode ..."
    docker run --rm -d --name naples-$VER --privileged -ti -p $AGENT_PORT:$AGENT_PORT  -p $HAL_PORT:$HAL_PORT --mount type=bind,source=$NAPLES_DIR,target=/naples/data -e WITH_QEMU=1 "$CONTAINER" 
else
    echo "Running the NAPLES container in stand-alone mode ..."
    docker run --rm -d --name naples-$VER --privileged -ti -p $AGENT_PORT:$AGENT_PORT  -p $HAL_PORT:$HAL_PORT --mount type=bind,source=$NAPLES_DIR,target=/naples/data "$CONTAINER"
fi

echo "NAPLES bring up completed"
