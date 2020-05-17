#!/bin/sh

NICDIR=$1
export PDSPKG_TOPDIR=$NICDIR
PIPELINE=$2
APP_ID=$3

#set -x
#echo $NICDIR

#Huge-pages for DPDK
echo 4096 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir /dev/hugepages
mount -t hugetlbfs nodev /dev/hugepages

#VPP Partial init env variables
export HAL_CONFIG_PATH=$NICDIR/conf/
export CONFIG_PATH=$NICDIR/conf/
export ZMQ_SOC_DIR='/sw/nic'
#This is used in DPDK to chosing DESC/Pkt buffer memory pool
export DPDK_SIM_APP_ID=$APP_ID
export VPP_LOG_FILE=$NICDIR/vpp.log

ulimit -c unlimited

VPP_PKG_DIR=$NICDIR/sdk/third-party/vpp-pkg/x86_64

#echo "$VPP_PKG_DIR"
#setup libs required for vpp
rm -rf /usr/lib/vpp_plugins/*
mkdir -p /usr/lib/vpp_plugins/
ln -s $VPP_PKG_DIR/lib/vpp_plugins/dpdk_plugin.so /usr/lib/vpp_plugins/dpdk_plugin.so
rm -f $NICDIR/conf/vpp_startup.conf
ln -s $NICDIR/vpp/conf/startup_$PIPELINE.conf $NICDIR/conf/vpp_startup.conf
#Create softlink for all vpp plugins
find $NICDIR/vpp/ -name *.mk | xargs grep MODULE_TARGET | grep "\.lib" | awk '{ print $3 }' |  sed 's/\.lib/.so/g' | xargs -I@ bash -c "ln -s $NICDIR/build/x86_64/$PIPELINE/lib/@ /usr/lib/vpp_plugins/@"

