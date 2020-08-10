#!/bin/sh

CUR_DIR=$( readlink -f $( dirname $0 ) )
source $CUR_DIR/setup_env_hw.sh
source $CUR_DIR/upgmgr_base.sh

export GOTRACEBACK='crash'
export GODEBUG=madvdontneed=1
export GOGC=50
export PERSISTENT_LOG_DIR='/obfl/'
UPGRADE_INIT_INSTANCE_FILE='/share/upgrade_init_instance.txt'

ulimit -c unlimited

function local_mounts {
    mkdir -p /var/log
    mount -t tmpfs -o size=100M tmpfs /var/log    
}

function initial_boot_action {

    # POST
   if [[ -f $SYSCONFIG/post_disable ]]; then
       echo "Skipping Power On Self Test (POST)"
   else
       echo "Running Power On Self Test (POST) ..."
       ($PDSPKG_TOPDIR/bin/diag_test post 2>&1 > $NON_PERSISTENT_LOGDIR/post_report_`date +"%Y%m%d-%T"`.txt; echo > /tmp/.post_done) &
   fi

   cd /
   ifconfig lo up

   sysctl -w net.ipv4.ip_local_reserved_ports=11357,11358,9007,9008,11359,11360

   # set core file pattern
   CORE_MIN_DISK=512
   [[ "$IMAGE_TYPE" = "diag" ]] && CORE_MIN_DISK=1
   mkdir -p /data/core
   #echo "|$PDSPKG_TOPDIR/bin/coremgr -P /data/core -p %p -e %e -m $CORE_MIN_DISK" > /proc/sys/kernel/core_pattern
   echo "/data/core/core_%p_%e" > /proc/sys/kernel/core_pattern

   # Verify BSM (this sould be moved inside sysmgr eventually)
   echo 1 > /sys/firmware/pensando/bsm/success

   # cleanup if there are any stale upgrade files due to incomplete upgrades
   rm -rf /update/*_upg*
   rm -rf /update/upgmgr_init_mode.txt
}

function load_drivers {
   
   # # load kernel modules for mnics
   # insmod $PDSPKG_TOPDIR/bin/ionic_mnic.ko &> $NON_PERSISTENT_LOGDIR/ionic_mnic_load.log
   # [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnic driver!" && exit 1
   
   # insmod $PDSPKG_TOPDIR/bin/mnet_uio_pdrv_genirq.ko &> $NON_PERSISTENT_LOGDIR/mnet_uio_pdrv_genirq_load.log
   # [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet_uio_pdrv_genirq driver!" && exit 1
   
   # insmod $PDSPKG_TOPDIR/bin/mnet.ko &> $NON_PERSISTENT_LOGDIR/mnet_load.log
   # [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet driver!" && exit 1


   # start memtun
   [ -f $SYSCONFIG/memtun_enable ] && (/$PDSPKG_TOPDIR/bin/memtun &)

   # if not already present, create a cache file recording the firmware inventory
   # at boot-time.  will be preserved across a live-update, and so always
   # provides a record of how things looked when we booted (seen with fwupdate -L)
   if [[ ! -r /var/run/fwupdate.cache ]]; then
       $PDSPKG_TOPDIR/tools/fwupdate -C &
   fi

   # Sync the boot fault log
   if [[ -f $PDSPKG_TOPDIR/tools/bflog_sync.sh ]]; then
    $PDSPKG_TOPDIR/tools/bflog_sync.sh &
   fi
}

function unload_drivers {
    :;
}

function start_sysmgr { 
    # start sysmgr
    PENLOG_LOCATION=$PERSISTENT_LOGDIR $PDSPKG_TOPDIR/bin/sysmgr &
}

function bringup_intfs {
    # bring up oob
    echo "Bringing up internal mnic interfaces ..."
    $PDSPKG_TOPDIR/tools/bringup_mgmt_ifs.sh &> $NON_PERSISTENT_LOGDIR/mgmt_if.log
}

function start_cronjobs {
    # start cronjobs
    nice crond -c $PDSPKG_TOPDIR/conf/apollo/crontabs
}

function setup_hugepages {
    set_nr_pages=$1
    if [[ $set_nr_pages == "1" ]];then
        #Huge-pages for DPDK considering both instances
        echo 128 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    fi
    mkdir -p /dev/hugepages
    mount -t hugetlbfs nodev /dev/hugepages
}

function setup_upgrade_domain {
    # set the domain for new based on the active domain
    instance=`cat $UPGRADE_INIT_INSTANCE_FILE`
    if [[ -f "$instance" ]];then
        # init boot instance is always dom_a and the next one is dom_b
        # for all processses including upgrade manager
        upgmgr_set_init_domain $UPGRADE_DOMAIN_A
    else
        upgmgr_set_init_domain $UPGRADE_DOMAIN_B
    fi
}

function setup_upgrade_defaults {
    upgmgr_clear_init_mode
    upgmgr_clear_init_domain
    # domain is different from filesystem instance id. domain is to
    # identify it is a regular boot vs upgrade boot.
    # save the init instance for future load
    instance="$(ls /.instance_*)"
    echo $instance > $UPGRADE_INIT_INSTANCE_FILE;
}


function start_ssh {
    /etc/init.d/S50sshd start
}

if [ "$1" = "init" ]
then
    local_mounts
    initial_boot_action
    load_drivers
    setup_hugepages "1"
    setup_upgrade_defaults
    start_sysmgr
    bringup_intfs
    start_cronjobs
    start_ssh
elif [ "$1" = "standby" ]
then
    local_mounts
    setup_hugepages "0"
    setup_upgrade_domain
    start_sysmgr
elif [ "$1" = "switchover" ]
then
    bringup_intfs
    start_cronjobs
    start_ssh
elif [ "$1" = "shutdown" ]
then
    echo "Nothing to do"
else
    echo "Unknown action: $1"
fi

    
