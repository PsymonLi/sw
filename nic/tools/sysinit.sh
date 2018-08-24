#!/bin/sh

export NIC_DIR='/nic'
export HAL_CONFIG_PATH=$NIC_DIR/conf/
export LD_LIBRARY_PATH=$NIC_DIR/lib:$NIC_DIR/conf/sdk:$NIC_DIR/conf/linkmgr:$NIC_DIR/conf/sdk/external:/usr/local/lib:/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
export HAL_PBC_INIT_CONFIG="2x100_hbm"
rm -f hal.log*
ulimit -c unlimited

# check for all the binaries
if [[ ! -f $NIC_DIR/bin/hal ]]; then
    echo "HAL binary not found"
    exit 1
fi

if [[ ! -f $NIC_DIR/bin/netagent ]]; then
    echo "netagent binary not found"
    exit 1
fi

if [[ ! -f /platform/bin/nicmgrd ]]; then
    echo "nicmgr binary not found"
    exit 1
fi

# bring up HAL
$GDB $NIC_DIR/bin/hal -c hal_haps.json &

# MAX_RETRIES guards the maximum time we wait for HAL to come up. It waits for 2**(MAX_RETRIES) - 1 seconds
MAX_RETRIES=10
HAL_GRPC_PORT="${HAL_GRPC_PORT:-50054}"
HAL_SERVER="localhost:$HAL_GRPC_PORT"
HAL_UP=0

# wait for HAL
#timeout=1
#while [ "$HAL_UP" -eq 0 ]
#do
  #echo "Waiting for HAL GRPC server to be up. Sleeping for $timeout seconds..."
  #sleep "$timeout"
  #curl "$HAL_SERVER"
  #HAL_UP=$?
  #timeout=`expr $timeout \\* 2`
  #i=`expr $i + 1`
  #if [ $i -eq $MAX_RETRIES ]; then
    #echo "HAL server failed to come up"
    #exit 1
  #fi
#done

echo "HAL WAIT BEGIN: `date +%x_%H:%M:%S:%N`"

while [ 1 ]
do
    OUTPUT="$(tail hal.log 2>&1 | grep "gRPC server listening on" | cut -d ' ' -f 5-8)"
    if [ "$OUTPUT" == "gRPC server listening on" ]; then
	break
    fi
    sleep 3
done

echo "HAL UP: `date +%x_%H:%M:%S:%N`"

# start nicmgr
LD_LIBRARY_PATH=/platform/lib:/nic/lib:/nic/conf/sdk:$LD_LIBRARY_PATH /platform/bin/nicmgrd > nicmgr.log 2>&1 &

# start netagent
$NIC_DIR/bin/netagent -datapath hal -logtofile /tmp/agent.log -hostif lo &

echo "All processes brought up, please check ..."
