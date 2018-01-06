#! /bin/bash
TOOLS_DIR=`dirname $0`
ABS_TOOLS_DIR=`readlink -f $TOOLS_DIR`
NIC_DIR=`dirname $ABS_TOOLS_DIR`

export ZMQ_SOC_DIR=$NIC_DIR
export HAL_CONFIG_PATH=$NIC_DIR/conf/
CAPRI_MOCK_MODE=1 LD_LIBRARY_PATH=$NIC_DIR/obj:$NIC_DIR/asic/capri/model/capsim-gen/lib:$NIC_DIR/third-party/lkl/export/bin/ $NIC_DIR/obj/hal_stub -c hal_hostpin.json 2>&1 | tee $NIC_DIR/hal.log
