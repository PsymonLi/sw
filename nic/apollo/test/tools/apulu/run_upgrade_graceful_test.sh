#! /bin/bash

MY_DIR=$( readlink -f $( dirname $0 ))

# setup dol
DOL_ARGS='--pipeline apulu --topo learn --feature learn'
source $MY_DIR/../../../tools/setup_dol.sh $DOL_ARGS

set -x

# setup upgrade
source $MY_DIR/../setup_upgrade_gtests.sh
upg_operd_init

upg_wait_for_pdsagent

# override trap
function trap_finish () {
    rm -rf /update/*     # upgrade init mode
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

# start upgrade manager
upg_setup $BUILD_DIR/gen/upgrade_graceful_sim.json upgrade_graceful.json
$BUILD_DIR/bin/pdsupgmgr -t $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade > upgrade_mgr.log 2>&1 &
sleep 2

# run client
pdsupgclient
[[ $? -ne 0 ]] && echo "upgrade command failed" && exit 1
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
rm -f /tmp/pen_* /dev/shm/ipc_* /dev/shm/metrics_*
remove_db
mv ./nicmgr.log ./nicmgr_old.log

# respawn the services
echo "starting new"
start_processes
upg_wait_for_pdsagent

# spawn upgrade mgr to continue the post restart states
# ideally spawning upgmgr should be moved up, which requires seperate service timeout
# for sim and h/w mode. This can be taken up as an enhancement
echo "respawning upgrademgr"
$BUILD_DIR/bin/pdsupgmgr -t $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade >> upgrade_mgr.log 2>&1 &
sleep 2

$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
status=${PIPESTATUS[0]}

# end of script
rm -rf $PDSPKG_TOPDIR/model.log
exit $status
