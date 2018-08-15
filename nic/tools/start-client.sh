#!/bin/bash -e
#cd `dirname $0`        
#NIC_DIR=$PWD      
TOOLS_DIR=`dirname $0`
ABS_TOOLS_DIR=`readlink -f $TOOLS_DIR`
NIC_DIR=`dirname $ABS_TOOLS_DIR`
# export LD_LIBRARY_PATH=$NIC_DIR/model_sim/:$NIC_DIR/model_sim/libs:/home/asic/tools/src/0.25/x86_64/lib64:$LD_LIBRARY_PATH        
SUDO=

#$SUDO $GDB $NIC_DIR/../bazel-bin/nic/e2etests/proxy/nic_proxy-e2etest_tcp-client 80 $NIC_DIR/e2etests/hello-world  2>&1 | tee $NIC_DIR/client.log
$GDB $NIC_DIR/../bazel-bin/nic/e2etests/proxy/nic_proxy-e2etest_tcp-client -p 80 -d $NIC_DIR/e2etests/proxy/hello-world  -m from-host 2>&1 | tee $NIC_DIR/client.log


##! /bin/bash -e
#TOOLS_DIR=`dirname $0`
#ABS_TOOLS_DIR=`readlink -f $TOOLS_DIR`
#NIC_DIR=`dirname $ABS_TOOLS_DIR`
#export LD_LIBRARY_PATH=$NIC_DIR/model_sim/:$NIC_DIR/model_sim/libs:/home/asic/tools/src/0.25/x86_64/lib64:$LD_LIBRARY_PATH        
#$GDB $NIC_DIR/model_sim/build/cap_model +plog=info +model_debug=$NIC_DIR/build/iris/gen/datapath/p4/dbg_out/model_debug.json 2>&1 | tee $NIC_DIR/model.log
