#!/bin/sh

export NIC_DIR='/nic'
export LD_LIBRARY_PATH=$NIC_DIR/lib
ulimit -c unlimited
cd /
ifconfig lo up
# start pciemgrd
/nic/bin/pciemgrd-gold &
# start memtun
/nic/bin/memtun &

# load kernel modules for mnics
insmod /nic/bin/ionic_mnic.ko &> /var/log/pensando/ionic_mnic_load.log
[[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnic driver!" && exit 1

insmod /nic/bin/mnet_uio_pdrv_genirq.ko &> /var/log/pensando/mnet_uio_pdrv_genirq_load.log
[[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet_uio_pdrv_genirq driver!" && exit 1

insmod /nic/bin/mnet.ko &> /var/log/pensando/mnet_load.log
[[ $? -ne 0 ]] && echo "Aborting sysinit, failed to load mnet driver!" && exit 1

# start agent
if [[ -f $NIC_DIR/tools/start-agent.sh ]]; then
    echo "Launching agent ..."
    $NIC_DIR/tools/start-agent.sh &
    [[ $? -ne 0 ]] && echo "Aborting sysinit, failed to start agent!" && exit 1
fi

# bring up oob
$NIC_DIR/tools/bringup_mgmt_ifs.sh &>/var/log/pensando/mgmt_if.log

# start vpp
if [[ -f $NIC_DIR/tools/start-vpp.sh ]]; then
    echo "Launching VPP ..."
    $NIC_DIR/tools/start-vpp.sh &
    [[ $? -ne 0 ]] && echo "Aborting Sysinit - VPP failed to start!" && exit 1
fi

echo "Launched all applications ..."
