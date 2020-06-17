#! /bin/bash

DRYRUN=0
NO_STOP=0
DEBUGMODE=0
DSCAGENTMODE=0
# max wait time for system to come up
SETUP_WAIT_TIME=600
# list of processes
SIM_PROCESSES=("vpp_main" "operd" "pdsagent" "pciemgrd" "sysmgr" "dhcpd")
TS_NAME=nic_sanity_logs.tar.gz

NAPLES_INTERFACES=("dsc0" "dsc1" "eth1" "oob_mnic0")

# set file size limit to user configured value or 10GB
if [[ -z "${LOG_SIZE}" ]]; then
    LOG_SIZE=$((10*1024*1024))
fi
ulimit -f $LOG_SIZE

CMDARGS=""
argc=$#
argv=($@)
for (( j=0; j<argc; j++ )); do
    [ "${argv[j]}" != '--nostop' ] && CMDARGS+="${argv[j]} "
    if [ ${argv[j]} == '--pipeline' ];then
        PIPELINE=${argv[j+1]}
    elif [ ${argv[j]} == '--netagent' ];then
        DSCAGENTMODE=1
    elif [[ ${argv[j]} =~ .*'--dry'.* ]];then
        DRYRUN=1
    elif [[ ${argv[j]} =~ .*'--debug'.* ]];then
        DEBUGMODE=1
    elif [[ ${argv[j]} == '--nostop' ]];then
        NO_STOP=1
    fi
done

CUR_DIR=$( readlink -f $( dirname $0 ) )
if [[ "$BASH_SOURCE" != "$0" ]]; then
    CUR_DIR=$( readlink -f $( dirname $BASH_SOURCE ) )
fi
source $CUR_DIR/setup_env_sim.sh $PIPELINE

if [[ $DSCAGENTMODE == 1 ]]; then
    SIM_PROCESSES+=("nmd" "netagent")
    METRICS_CONF_DIR=$PDSPKG_TOPDIR/operd/metrics/venice/
else
    METRICS_CONF_DIR=$PDSPKG_TOPDIR/operd/metrics/cloud/
fi

function stop_model() {
    pkill cap_model
}

function stop_processes () {
    echo "======> Setting Phasers to kill"
    for process in ${SIM_PROCESSES[@]}; do
        #dump backtrace of process to file, useful for debugging if process hangs
        pstack `pgrep -x ${process}` > $PDSPKG_TOPDIR/${process}_bt.log 2>&1
        printf "\n *** Nuking ${process}"
        pkill -9 ${process} || { echo -n " - already dead"; }
    done
    # kill vpp scripts if DOL was interrupted in the middle
    pkill -9 vpp || { echo ""; }
    echo "======> Setting Phasers to stun"
}

function start_dhcp_server() {
    sudo $PDSPKG_TOPDIR/apollo/tools/$PIPELINE/start-dhcpd-sim.sh -p $PIPELINE > dhcpd.log 2>&1
    if [[ $? != 0 ]]; then
        echo "ERR: Failed to start dhcpd!"
        exit 1
    fi
}

function start_sysmgr () {
    if [[ $DSCAGENTMODE == 1 ]]; then
        SYSMGR_CONF=pipeline-venice-dol.json
    else
        SYSMGR_CONF=pipeline-dol.json
    fi
    PENLOG_LOCATION=/var/log/pensando/ sysmgr $PDSPKG_TOPDIR/sysmgr/src/$PIPELINE/$SYSMGR_CONF &
}

function start_model () {
    if [[ $DEBUGMODE == 0 ]]; then
        MODEL_LOG=/dev/null
    else
        MODEL_LOG=$PDSPKG_TOPDIR/model.log
    fi
    $PDSPKG_TOPDIR/apollo/test/tools/$PIPELINE/start-model.sh $MODEL_LOG &
}

function start_processes () {
    echo "======> Laying in the course"
    start_sysmgr
    start_dhcp_server
}

function start_upgrade_manager () {
    if [ $PIPELINE == 'apulu' ];then
        echo "======> Starting Upgrade Manager"
        $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade/start-upgmgr-mock.sh -t $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade > upgrade_mgr.log &
    fi
}

function stop_upgrade_manager () {
    if [ $PIPELINE == 'apulu' ];then
        pkill pdsupgmgr
    fi
}

function remove_db () {
    rm -f $PDSPKG_TOPDIR/pdsagent.db
    rm -f $PDSPKG_TOPDIR/pdsagent.db-lock
    rm -f /tmp/*.db
}

function remove_ipc_files () {
    rm -f /tmp/pen_*
}

function remove_shm_files () {
    rm -f /dev/shm/pds_* /dev/shm/ipc_* /dev/shm/metrics_* /dev/shm/alerts
    rm -f /dev/shm/nicmgr_shm /dev/shm/sysmgr /dev/shm/vpp /dev/shm/upgradelog
}

function remove_interfaces () {
    for INTF in ${NAPLES_INTERFACES[@]}; do
        ip link del ${INTF} type dummy >/dev/null 2>&1
    done
}

function remove_metrics_conf_files () {
    find $METRICS_CONF_DIR -name "*.json" -printf "unlink $CONFIG_PATH/%P > /dev/null 2>&1 \n" | sh | echo -n ""
}

function remove_conf_files () {
    sudo rm -f $CONFIG_PATH/pipeline.json
    sudo rm -f $CONFIG_PATH/vpp_startup.conf
    remove_metrics_conf_files
}

function remove_stale_files () {
    echo "======> Cleaning debris"
    rm -f $PDSPKG_TOPDIR/out.sh
    rm -f $PDSPKG_TOPDIR/ipstrc.bak
    rm -f $PDSPKG_TOPDIR/conf/gen/dol_agentcfg.json
    rm -f $PDSPKG_TOPDIR/conf/gen/device_info.txt
    rm -f /var/run/pds_svc_server_sock
    remove_db
    remove_ipc_files
    remove_shm_files
    remove_conf_files
    if [[ $DSCAGENTMODE == 1 ]]; then
        remove_interfaces
    fi
}

function remove_logs () {
    # NOT to be used post run
    echo "======> Incinerating logs from unknown stardate"
    sudo rm -rf ${PDSPKG_TOPDIR}/*log* ${PDSPKG_TOPDIR}/core* ${PDSPKG_TOPDIR}/*Techsupport*
    sudo rm -rf /var/log/pensando/ /obfl/ /data/ /sysconfig/
    sudo rm -rf /update/*
    # pciemgrd saves here in sim mode
    sudo rm -rf /root/.pcie*
}

function collect_techsupport () {
    echo "======> Recording captain's logs, stardate `date +%x_%H:%M:%S:%N`"
    if ls ${PDSPKG_TOPDIR}/core.* 1> /dev/null 2>&1; then
        echo " *** waiting for the core to be produced"
        sleep 60
    fi
    echo " *** Collecting Techsupport ${PDSPKG_TOPDIR}/${TS_NAME} - Please wait..."
    techsupport -c ${CONFIG_PATH}/${PIPELINE}/techsupport_dol.json -d ${PDSPKG_TOPDIR} -o ${TS_NAME} >/dev/null 2>&1
    [[ $? -ne 0 ]] && echo "ERR: Failed to collect techsupport" && return
}

function finish () {
    if [ $NO_STOP == 1 ]; then
         return
    fi
    echo "======> Starting shutdown sequence"
    collect_techsupport
    if [ $DRYRUN == 0 ]; then
        stop_upgrade_manager
        stop_processes
        stop_model
        # unmount hugetlbfs
        umount /dev/hugepages || { echo "ERR: Failed to unmount hugetlbfs"; }
    fi
    ${PDSPKG_TOPDIR}/tools/print-cores.sh
    remove_stale_files
}
trap finish EXIT

function check_health () {
    echo "======> Initiating launch sequence"
    echo " *** Please wait until all processes (${SIM_PROCESSES[@]}) are up"
    # loop for SETUP_WAIT_TIME seconds and break if all processes are UP
    counter=$SETUP_WAIT_TIME
    while [ $counter -gt 0 ]; do
        sleep 1
        ((counter--))
        for process in ${SIM_PROCESSES[@]}; do
            if ! pgrep -x $process > /dev/null
            then
                # continue while loop if process isn't up yet
                continue 2
            fi
        done
        echo "======> All systems go! Let's punch it"
        if [[ $DSCAGENTMODE == 1 ]]; then
            # wait for netagent to come up - TODO: convert to poll
            sleep 30
        fi
        return
    done
    ps -ef > $PDSPKG_TOPDIR/health.log
    echo "ERR: Not all processes in (${SIM_PROCESSES[@]}) are up - Please check $PDSPKG_TOPDIR/health.log"
    exit 1
}

function setup_metrics_conf_files () {
    ln -s $METRICS_CONF_DIR/*.json $CONFIG_PATH/ | echo -n ""
}

function setup_conf_files () {
    # TODO Remove this once agent code is fixed
    # Create dummy device.conf - agent is trying to update it when device object is updated.
    # Without this, pdsagent crashes since config file is not found.
    sudo touch /sysconfig/config0/device.conf
    setup_metrics_conf_files
}

function setup_interfaces () {
    echo "======> Hailing frequencies"
    # nmd waits for these interfaces
    for INTF in ${NAPLES_INTERFACES[@]}; do
        ip link add ${INTF} type dummy
    done
}

function setup_fru () {
    fru_json='{
        "manufacturing-date": "1539734400",
        "manufacturer": "PENSANDO SYSTEMS INC.",
        "product-name": "NAPLES 100",
        "serial-number": "SIM18440000",
        "part-number": "68-0003-02 01",
        "engineering-change-level": "00",
        "board-id": "1000",
        "num-mac-address": "24",
        "mac-address": "02:22:22:11:11:11"
    }'

    if [[ $SKIP_FRU_SETUP != 1 ]]; then
        echo " *** Setting up fru"
        echo $fru_json > /tmp/fru.json
    fi
}

function setup_hugepages () {
    echo 4096 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    mkdir /dev/hugepages
    mount -t hugetlbfs nodev /dev/hugepages
}

function setup_env () {
    sudo mkdir -p /var/log/pensando/ /obfl/ /sysconfig/config0/ /data/
    setup_conf_files
    setup_fru
    if [[ $DSCAGENTMODE == 1 ]]; then
        setup_interfaces
    fi
    setup_hugepages
}

function setup () {
    # Cleanup of previous run if required
    stop_upgrade_manager
    stop_processes
    stop_model
    # remove stale files from older runs
    remove_stale_files
    remove_logs
    # setup env
    setup_env
}
setup

if [ $PIPELINE == 'artemis' ];then
    export AGENT_TEST_HOOKS_LIB='libdolagenthooks.so'
fi
