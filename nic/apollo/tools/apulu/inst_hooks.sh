#!/bin/sh

INSTROOT=$1
ACTION=$2
NON_PERSISTENT_LOGDIR=$INSTROOT/var/log

function unload_drivers {
    # todo; fixme; stavros
    rmmod -f mnet
    rmmod -f mnet_uio_pdrv_genirq
    rmmod -f ionic_mnic
}

function load_drivers {
    KO_CACHE=/tmp/ko_cache

    mkdir -p $KO_CACHE

    cp $INSTROOT/nic/bin/ionic_mnic.ko $KO_CACHE/
    cp $INSTROOT/nic/bin/mnet_uio_pdrv_genirq.ko $KO_CACHE/
    cp $INSTROOT/nic/bin/mnet.ko $KO_CACHE/
    
    # load kernel modules for mnics
    insmod $KO_CACHE/ionic_mnic.ko &> $NON_PERSISTENT_LOGDIR/ionic_mnic_load.log
    [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnic driver!" && exit 1
   
    insmod $KO_CACHE/mnet_uio_pdrv_genirq.ko &> $NON_PERSISTENT_LOGDIR/mnet_uio_pdrv_genirq_load.log
    [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet_uio_pdrv_genirq driver!" && exit 1
    
    insmod $KO_CACHE/mnet.ko &> $NON_PERSISTENT_LOGDIR/mnet_load.log
    [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet driver!" && exit 1
}

if [ "$ACTION" = "mount-active" ]
then
    load_drivers
elif [ "$ACTION" = "mount-standby" ]
then
    :;
elif [ "$ACTION" = "post-shutdown" ]
then
    :;
elif [ "$ACTION" = "pre-switchover" ]
then
    :;
else
    echo "Unknown action '$ACTION'"
fi
