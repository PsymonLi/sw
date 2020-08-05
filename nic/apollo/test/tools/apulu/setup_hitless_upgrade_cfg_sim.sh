#! /bin/bash

set -x

MY_DIR=$( readlink -f $( dirname $0 ))

export PIPELINE=apulu
source $MY_DIR/../../../tools/setup_env_sim.sh $PIPELINE
source $MY_DIR/../setup_upgrade_gtests.sh

rm -f /tmp/upgrade_start_new_instance.txt

upg_setup $BUILD_DIR/gen/upgrade_hitless_sim.json upgrade_hitless.json

FAILURE_STAGE=$1
FAILURE_REASON=$2

function copy_json() {
    local file=$CONFIG_PATH/gen/upgrade_hitless.json

    cp $file ${file}.org
    # add upgtestapp to the discovery and serial list
    awk ' BEGIN { found=0;line=0 }
      /"upg_svc"/ { found=1;line=0 }
      /.*/ { if (found == 1 && line == 1) print "    \"upgtestapp\"," }
      /.*/ { print $0;line=line+1 } ' ${file}.org >  $file

    sed -i 's/"svc_sequence" : "\([a-z:].*\)"/"svc_sequence" : "\1:upgtestapp"/' $file
    rm -f ${file}.org
}

if [ ! -z "$FAILURE_STAGE" ];then
    copy_json
    $BUILD_DIR/bin/fsm_test -s upgtestapp -i 60 -e $FAILURE_REASON -f $FAILURE_STAGE > $PDSPKG_TOPDIR/fsm_test.log 2>&1 &
fi

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
exec $MY_DIR/start_upgrade_hitless_process.sh $dom "1" "$1" "$2"
