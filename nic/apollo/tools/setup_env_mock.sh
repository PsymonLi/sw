#! /bin/bash

if [ -z "$1" ]
then
    echo "Pipeline argument not supplied"
    exit 1
fi

export ASIC="${ASIC:-capri}"
export PDSPKG_TOPDIR='/sw/nic/'
export PIPELINE=$1
export BUILD_DIR=${PDSPKG_TOPDIR}/build/x86_64/${PIPELINE}/${ASIC}
export CONFIG_PATH=$PDSPKG_TOPDIR/conf/
export COVFILE=$PDSPKG_TOPDIR/coverage/sim_bullseye_hal.cov
export PATH=${PATH}:${BUILD_DIR}/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PDSPKG_TOPDIR/third-party/metaswitch/output/x86_64/
if [ -z "$2" ]; then
    export PERSISTENT_LOG_DIR=${PDSPKG_TOPDIR}
    export NON_PERSISTENT_LOG_DIR=${PDSPKG_TOPDIR}
else
    export PERSISTENT_LOG_DIR="$2/"
    export NON_PERSISTENT_LOG_DIR="$2/"
fi
export LOG_DIR=$NON_PERSISTENT_LOG_DIR

export ASIC_MOCK_MODE=1
export ASIC_MOCK_MEMORY_MODE=1
if [ $PIPELINE != 'apulu' ]; then
    export IPC_MOCK_MODE=1
fi

if [ $ASIC == 'elba' ]; then
    export ELBA_SKIP_PLL_INIT=1
fi
if [ -n "$DEFAULT_PF_STATE" ]; then
    CMDARGS=" --default-pf-state=$DEFAULT_PF_STATE "
fi

#GDB='gdb --args'
#VALGRIND='valgrind --leak-check=full --show-leak-kinds=all --gen-suppressions=all --error-limit=no --verbose --log-file=valgrind-out.txt --track-origins=yes'
