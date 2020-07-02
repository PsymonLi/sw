#! /bin/bash

set -x

MY_DIR=$( readlink -f $( dirname $0 ))
export DOL_ARGS='--pipeline apulu  --topo hostvxlan --feature upgrade --sub upgradebringup'
export PIPELINE=apulu
source $MY_DIR/../../../tools/setup_env_sim.sh $PIPELINE
source $PDSPKG_TOPDIR/sdk/upgrade/core/upgmgr_core_base.sh

function remove_shm_files () {
    rm -f /dev/shm/pds_* /dev/shm/ipc_* /dev/shm/metrics_* /dev/shm/alerts
    rm -f /dev/shm/nicmgr_shm /dev/shm/sysmgr /dev/shm/vpp /dev/shm/upgrade*
}

# clear any upgrade specific data from previous failed run
rm -rf /update/* /share/* /obfl # upgrade init mode
rm -rf /root/.pcie*             # pciemgrd saves here in sim mode
rm -rf /sw/nic/*.log /sw/nic/core.*
mkdir -p /update /share /obfl
rm -rf /tmp/dom*
rm -rf /.upgrade*  # TODO move all the above to a function
remove_shm_files

function terminate() {
    pid=`pgrep -f "$1"`
    if [ -n "$pid" ];then
        kill -TERM $pid
        sleep $2
    fi
}

function check_status() {
    file=$1
    count=0
    while [ ! -f $file ];do
        #echo "waiting for file $f"
        sleep 1
        count=`expr $count + 1`
        if [ $count -ge 240 ];then
            echo "Time exceeded"
            exit 1
        fi
    done
}

function trap_finish () {
    terminate 'start_upgrade_hitless_process.sh' 5
    terminate 'cap_model' 0
    terminate 'main.py' 0
    terminate 'dhcpd' 0
    rm -rf /update/* /.upgrade* /share/*
}
trap trap_finish EXIT SIGTERM

# start the model
# model used the current directory in sim. this cannot be avoided
$MY_DIR/start-model.sh &
sudo $MY_DIR/../../../tools/$PIPELINE/start-dhcpd-sim.sh -p apulu

# remove synchronization files
rm -f /tmp/agent_up
rm -f /tmp/start_new
# poll by dol for config replay ready
rm -f /tmp/config_replay_ready
# set by dol after config replay is done
rm -f /tmp/config_replay_done
# set by dol to indicate that it has done the pre-uggrade test
# and waiting for config replay ready indication
rm -f /tmp/config_replay_ready_wait

# set the domain. but this bringup is in default mode
sh $MY_DIR/start_upgrade_hitless_process.sh $UPGRADE_DOMAIN_A "0" &
# wait for this to be up
check_status '/tmp/agent_up'

# start DOL now
# dol required the below for device config and log access
export PDSPKG_TOPDIR="/tmp/$UPGRADE_DOMAIN_A"
# comment out DOL_ARGS above for debugging any application where config
# is not needed
pid=0
if [ -z "$DOL_ARGS" ];then
    touch /tmp/config_replay_done
    # wait for all processes to be up
    sleep 20
else
    /sw/dol/main.py $DOL_ARGS 2>&1 &
    pid=$!

    # wait for dol up ran a packet test on A
    check_status /tmp/config_replay_ready_wait
fi

# run upgrade
$BUILD_DIR/bin/pdsupgclient -m hitless &

# wait for second process request
check_status /tmp/start_new
dom=`cat /tmp/start_new`
sh $MY_DIR/start_upgrade_hitless_process.sh $dom "1" &

if [ $pid -ne 0 ];then
    wait $pid
    [[ $? -ne 0 ]] && echo "Packet test failed" && exit 1
else
    sleep 300
fi
exit 0
