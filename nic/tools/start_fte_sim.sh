#! /bin/bash -e
export ASIC="${ASIC:-capri}"
TOOLS_DIR=`dirname $0`
ABS_TOOLS_DIR=`readlink -f $TOOLS_DIR`
NIC_DIR=`dirname $ABS_TOOLS_DIR`
SNORT_EXPORT_DIR=$NIC_DIR/hal/third-party/snort3/export
#GDB='gdb --args'
export ZMQ_SOC_DIR=$NIC_DIR
export CONFIG_PATH=$NIC_DIR/conf/
export SNORT_LUA_PATH=$SNORT_EXPORT_DIR/lua/
export LUA_PATH="$SNORT_EXPORT_DIR/lua/?.lua;;"
export SNORT_DAQ_PATH=$SNORT_EXPORT_DIR/x86_64/lib/

echo "STARTING fte_sim: `date +%x_%H:%M:%S:%N`"
/go/bin/tmagent -resolver-urls $* &
$NIC_DIR/build/x86_64/iris/${ASIC}/bin/fte_sim > /var/log/ftesim.$$ 2>&1
