#!/bin/sh

#Huge-pages for DPDK
echo 256 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir /dev/hugepages
mount -t hugetlbfs nodev /dev/hugepages

#VPP Partial init env variables
export NIC_DIR=/nic/
export CONFIG_PATH=$NIC_DIR/conf/
export VPP_LOG_FILE=/var/log/pensando/vpp.log
#VPP has 2.5 MB for logs, so per file 1.25 mb = 1.25*1024*1024 bytes
export VPP_LOG_FILE_SIZE=1310720
export DATAPATH_MNIC="cpu_mnic0"

ulimit -c unlimited

counter=600
start_vpp=0
output=""
while [ $counter -gt 0 ]
do
    output=$(cat /sys/class/uio/uio*/name | grep cpu_mnic0)
    if [ $output == "cpu_mnic0" ]; then
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

exec $NIC_DIR/bin/vpp -c $CONFIG_PATH/vpp/vpp_3_workers.conf
