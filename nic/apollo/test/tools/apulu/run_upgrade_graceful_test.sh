#! /bin/bash

MY_DIR=$( readlink -f $( dirname $0 ))
UPGRADE_STATUS_FILE='/update/pds_upg_status.txt'

# setup dol
DOL_ARGS='--pipeline apulu --topo learn --feature learn'
source $MY_DIR/../../../tools/setup_dol.sh $DOL_ARGS

# setup upgrade
source $MY_DIR/../setup_upgrade_gtests.sh
upg_operd_init
upg_setup $BUILD_DIR/gen/upgrade_graceful_sim.json upgrade_graceful.json

start_model
start_processes
start_upgrade_manager

set -x

upg_wait_for_pdsagent

function wait_for_upgmgr_exit() {
    sleep 10
    counter=`upg_wait_for_process_exit pdsupgmgr 300`
    if [ $counter -eq 0 ];then
        echo "Pdsupgmgr not exited"
        exit 1
    fi
}

function check_upgrade_status() {
    counter=300
    while [ $counter -gt 0 ];do
        grep -i "success" $UPGRADE_STATUS_FILE
        if [ $? -eq 0 ];then
            break
        fi
        counter=$(( $counter - 1 ))
        sleep 1
    done
    [[ $counter -eq 0 ]] && echo "upgrade command failed" && exit 1
}

# start DOL now
# comment out below for debugging any application where config
# is not needed
#$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
#status=${PIPESTATUS[0]}

# run client
pdsupgclient
[[ $? -ne 0 ]] && echo "upgrade command failed" && exit 1

wait_for_upgmgr_exit
echo "upgrade command successful"

# kill all processes
# upgrade manager should be exited by itself after switchover
echo "stopping processes"
stop_processes

# some of these can be moved to upgrade_graceful_mock.sh
# cleanup the old logs/data
echo "respawning processes, delete all previously generated files, dpdk uses it"
mv ./nicmgr.log ./nicmgr_old.log
remove_stale_files

# respawn the services
echo "starting new"
setup_env
upg_operd_init
start_processes
upg_wait_for_pdsagent
start_upgrade_manager
# wait for the upgmgr to exit
wait_for_upgmgr_exit
check_upgrade_status

# dol always tries to connect to upgmgr. so  just spawn once in default mode
rm /update/upgrade*
start_upgrade_manager
sleep 5
#$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
#status=${PIPESTATUS[0]}

# end of script
rm -rf $PDSPKG_TOPDIR/model.log
stop_upgrade_manager
exit $status
