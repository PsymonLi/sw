#! /bin/bash

set -x
MY_DIR=$( readlink -f $( dirname $0 ))
HITLESS_DOM=$1
SET_DOM=$2

export PDSPKG_TOPDIR=/tmp/$HITLESS_DOM/
export ZMQ_SOC_DIR=/sw/nic/
source $MY_DIR/../../../tools/setup_env_sim.sh $PIPELINE

# cannot use from setup_upgrade because of the error check (set -e)
function terminate() {
    pid=`pgrep -f "$1"`
    if [ -n "$pid" ];then
        kill -TERM $pid
    fi
}

# setup upgrade
source $MY_DIR/../setup_upgrade_gtests.sh
hitless_copy_files
upg_operd_init

# use 8G configuration
cp $CONFIG_PATH/catalog_hw_68-0004.json $CONFIG_PATH/$PIPELINE/catalog.json
if [[ $SET_DOM = "1" ]];then
    # set upgrade mode
    source $PDSPKG_TOPDIR/sdk/upgrade/core/upgmgr_core_base.sh
    upgmgr_set_init_domain $HITLESS_DOM
    export IPC_SUFFIX="_${HITLESS_DOM}"
fi

function trap_finish () {
    terminate "vpp -c ${PDSPKG_TOPDIR}"
    terminate "$BUILD_DIR/bin/pciemgrd"
    terminate "$BUILD_DIR/bin/operd"
    terminate "$BUILD_DIR/bin/pdsupgmgr"
    terminate "$PDSPKG_TOPDIR/apollo/tools/apulu/start-agent-sim.sh"
    terminate "$BUILD_DIR/bin/pdsagent"
    terminate "$PDSPKG_TOPDIR/vpp/tools/start-vpp-sim.sh"
    rm /sw/nic/conf/vpp_startup.conf
}
trap trap_finish EXIT


# start processes
$BUILD_DIR/bin/pciemgrd -d &
$BUILD_DIR/bin/operd $CONFIG_PATH/apulu/operd.json $CONFIG_PATH/apulu/operd-decoders.json &
$PDSPKG_TOPDIR/apollo/tools/$PIPELINE/start-agent-sim.sh > $PDSPKG_TOPDIR/agent.log 2>&1 &

upg_wait_for_pdsagent

$PDSPKG_TOPDIR/vpp/tools/start-vpp-sim.sh ${DOL_ARGS} &

# start upgrade manager
upg_setup $BUILD_DIR/gen/upgrade_hitless_sim.json upgrade_hitless.json
$BUILD_DIR/bin/pdsupgmgr -t $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade > $PDSPKG_TOPDIR/upgrade_mgr.log 2>&1 &

touch /tmp/agent_up

wait
