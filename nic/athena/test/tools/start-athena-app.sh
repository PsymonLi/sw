#! /bin/bash

export PIPELINE=athena

CUR_DIR=$( readlink -f $( dirname $0 ) )
if [ "$1" == "mock_mode" ]
then
source $CUR_DIR/../../../tools/setup_env_mock.sh $PIPELINE
shift
else
source $CUR_DIR/../../../tools/setup_env_sim.sh $PIPELINE
fi

OPERD_METRICS_PATH=${PDSPKG_TOPDIR}/infra/operd/metrics/cloud/

function stop_model() {
    pkill cap_model
}

function start_model () {
    $PDSPKG_TOPDIR/athena/test/tools/start-model.sh &
}

function remove_ipc_files () {
    rm -f /tmp/pen_*
}

function remove_shm_files () {
    rm -f /dev/shm/pds_* /dev/shm/ipc_* /dev/shm/metrics_* /dev/shm/event
    rm -f /dev/shm/nicmgr_shm /dev/shm/sysmgr /dev/shm/upgradelog
}

function remove_metrics_conf_files () {
    find ${OPERD_METRICS_PATH} -name "*.json" -printf "unlink $CONFIG_PATH/%P > /dev/null 2>&1 \n" | sh | echo -n ""
}

function remove_conf_files () {
    sudo rm -f $CONFIG_PATH/pipeline.json
    remove_metrics_conf_files
}

function remove_stale_files () {
    remove_ipc_files
    remove_shm_files
    remove_conf_files
}

function finish () {
    stop_model
    # unmount hugetlbfs
    umount /dev/hugepages || { echo "ERR: Failed to unmount hugetlbfs"; }
    remove_stale_files
}
trap finish EXIT

function setup_metrics_conf_files () {
    ln -s ${OPERD_METRICS_PATH}/*.json $CONFIG_PATH/ | echo -n ""
}

function setup_conf_files () {
    ln -s $PDSPKG_TOPDIR/conf/athena/pipeline.json $CONFIG_PATH/ | echo -n ""
    setup_metrics_conf_files
}

function setup () {
    # Cleanup of previous run if required
    stop_model
    # remove stale files from older runs
    remove_stale_files
    setup_conf_files
}
setup

start_model

#Huge-pages for DPDK
echo 2048 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir -p /dev/hugepages
mount -t hugetlbfs nodev /dev/hugepages

echo "Timestamp : `date +%x_%H:%M:%S:%N`"
echo "Calling athena app with args \"$*\""
$GDB $BUILD_DIR/bin/athena_app -c hal.json $* 2>&1

