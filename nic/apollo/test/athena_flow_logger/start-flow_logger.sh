#! /bin/bash -e

TOOLS_DIR=`dirname $0`
ABS_TOOLS_DIR=`readlink -f $TOOLS_DIR`
NIC_DIR=`dirname $ABS_TOOLS_DIR/../../../../`
export ASIC="${ASIC:-capri}"
export CONFIG_PATH=$NIC_DIR/conf/
export HAL_CONFIG_PATH=$NIC_DIR/conf/
export LOG_DIR=$NIC_DIR/
export PERSISTENT_LOG_DIR=$NIC_DIR/
export PDSPKG_TOPDIR=$NICDIR
export ZMQ_SOC_DIR=${NIC_DIR}

echo "Starting Flow Logger: `date +%x_%H:%M:%S:%N`"
BUILD_DIR=$NIC_DIR/build/x86_64/athena/${ASIC}
echo $NIC_DIR
$GDB $BUILD_DIR/bin/athena_flow_logger -d $NIC_DIR -t 5
