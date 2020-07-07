#! /bin/bash

if [ -z "$1" ]
then
    echo "Pipeline argument not supplied"
    exit 1
fi

export ASIC="${ASIC:-capri}"
export PDSPKG_TOPDIR=${PDSPKG_TOPDIR:-'/sw/nic/'}
export DOLDIR=${DOLDIR:-'/sw/dol/'}
export PIPELINE=$1
export BUILD_DIR=${PDSPKG_TOPDIR}/build/x86_64/${PIPELINE}/${ASIC}
export BIN_PATH=$BUILD_DIR/bin/
export LIB_PATH=$BUILD_DIR/lib/
export CONFIG_PATH=$PDSPKG_TOPDIR/conf/
export PIPELINE_CONFIG_PATH=$PDSPKG_TOPDIR/conf/${PIPELINE}/
export COVFILE=$PDSPKG_TOPDIR/coverage/sim_bullseye_hal.cov
export PATH=${PATH}:${BIN_PATH}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_PATH:$PDSPKG_TOPDIR/third-party/metaswitch/output/x86_64/
export OPERD_REGIONS=$PIPELINE_CONFIG_PATH/operd-regions.json
if [ -z "$2" ]; then
    export PERSISTENT_LOG_DIR=$PDSPKG_TOPDIR
    export NON_PERSISTENT_LOG_DIR=$PDSPKG_TOPDIR
else
    export PERSISTENT_LOG_DIR="$2/"
    export NON_PERSISTENT_LOG_DIR="$2/"
fi
export LOG_DIR=$NON_PERSISTENT_LOG_DIR

export ZMQ_SOC_DIR=${ZMQ_SOC_DIR:-$PDSPKG_TOPDIR}


if [[ -n ${DEFAULT_PF_STATE} ]]; then
    echo "Default PF State $DEFAULT_PF_STATE"
    CMDARGS+=" --default-pf-state=$DEFAULT_PF_STATE "
fi

#GDB='gdb --args'
#VALGRIND='valgrind --leak-check=full --show-leak-kinds=all --gen-suppressions=all --error-limit=no --verbose --log-file=valgrind-out.txt --track-origins=yes'

