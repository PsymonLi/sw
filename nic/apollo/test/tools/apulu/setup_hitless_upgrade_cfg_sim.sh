#! /bin/bash

set -x

MY_DIR=$( readlink -f $( dirname $0 ))

export PIPELINE=apulu
source $MY_DIR/../../../tools/setup_env_sim.sh $PIPELINE
source $MY_DIR/../setup_upgrade_gtests.sh

rm -f /tmp/upgrade_start_new_instance.txt

upg_setup $BUILD_DIR/gen/upgrade_hitless_sim.json upgrade_hitless.json

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

# wait for second process start request
check_status /tmp/upgrade_start_new_instance.txt
dom=`cat /tmp/upgrade_start_new_instance.txt`
exec $MY_DIR/start_upgrade_hitless_process.sh $dom "1"
