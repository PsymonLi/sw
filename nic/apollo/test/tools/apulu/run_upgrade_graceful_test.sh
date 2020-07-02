#! /bin/bash

MY_DIR=$( readlink -f $( dirname $0 ))
UPGRADE_STATUS_FILE='/update/pds_upg_status.txt'

# clear any upgrade specific data from previous failed run
mkdir -p /update
rm -rf /update/*       # upgrade init mode
rm -rf /root/.pcie*    # pciemgrd saves here in sim mode
rm -rf /.upgrade*

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

# override trap
function trap_finish () {
    rm -rf /update/*     # upgrade init mode
    rm -rf /.upgrade*
    upg_finish upgmgr
    stop_processes
    stop_model
}
trap trap_finish EXIT

# start DOL now
# comment out below for debugging any application where config
# is not needed
$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
status=${PIPESTATUS[0]}

sleep 2

# run client
pdsupgclient
[[ $? -ne 0 ]] && echo "upgrade command failed" && exit 1

#sleep 10
#grep -i "success" $UPGRADE_STATUS_FILE
#[[ $? -ne 0 ]] && echo "upgrade command failed" && exit 1

echo "upgrade command successful"

# kill testing services
echo "stopping processes including upgrademgr"
stop_processes
sleep 2

# some of these can be moved to upgrade_graceful_mock.sh
# cleanup the old logs/data
echo "respawning processes, delete all previously generated files, dpdk uses it"
rm -f $PDSPKG_TOPDIR/conf/gen/dol_agentcfg.json
rm -f $PDSPKG_TOPDIR/conf/gen/device_info.txt
remove_shm_files
remove_ipc_files
remove_db
mv ./nicmgr.log ./nicmgr_old.log

# respawn the services
echo "starting new"
upg_operd_init
start_processes
upg_wait_for_pdsagent
start_upgrade_manager
# wait for the upgmgr to exit
sleep 30
counter=`upg_wait_for_process_exit pdsupgmgr 300`
if [ $counter -eq 0 ];then
    echo "Pdsupgmgr not exited"
    exit 1
fi

# dol always tries to connect to upgmgr. so  just spawn once
rm /update/*
start_upgrade_manager
sleep 5
$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
status=${PIPESTATUS[0]}

# end of script
rm -rf $PDSPKG_TOPDIR/model.log
stop_upgrade_manager
exit $status
