#!/bin/sh

#VPP Partial init env variables
export NIC_DIR=/nic/
export CONFIG_PATH=$NIC_DIR/conf/
export VPP_LOG_FILE=/var/log/pensando/vpp.log
#VPP has 2.5 MB for logs, so per file 1.25 mb = 1.25*1024*1024 bytes
export VPP_LOG_FILE_SIZE=1310720

ulimit -c unlimited

source /nic/tools/upgmgr_core_base.sh

dom=$( upgmgr_init_domain )
if [[ $dom == $UPGRADE_DOMAIN_B ]];then
    CPU_MNIC="cpu_mnic2"
    VPP_CONF="vpp_dom_b_2_workers.conf"
else
    CPU_MNIC="cpu_mnic0"
    VPP_CONF="vpp_2_workers.conf"
fi

export DATAPATH_MNIC=$CPU_MNIC

counter=600
start_vpp=0
output=""
while [ $counter -gt 0 ]
do
    output=$(cat /sys/class/uio/uio*/name | grep $CPU_MNIC)
    if [[ $output == $CPU_MNIC ]]; then
        start_vpp=1
        break
    fi
    sleep 1
    counter=$(( $counter - 1 ))
done
if [ $start_vpp == 0 ]; then
    echo "UIO device not created, not starting VPP!!"
    exit 1
fi
exec $NIC_DIR/bin/vpp -c $CONFIG_PATH/vpp/$VPP_CONF
