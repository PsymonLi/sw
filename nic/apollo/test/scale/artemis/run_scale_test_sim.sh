#! /bin/bash

set -e
export NICDIR=`pwd`
export HAL_LOG_DIR=${NICDIR}
export ZMQ_SOC_DIR=${NICDIR}
export BUILD_DIR=${NICDIR}/build/x86_64/artemis/
export GEN_TEST_RESULTS_DIR=${BUILD_DIR}/gtest_results
export HAL_CONFIG_PATH=${NICDIR}/conf
#export GDB='gdb --args'
export APOLLO_TEST_TEP_ENCAP=vxlan
export SKIP_VERIFY=1
export PATH=${PATH}:${BUILD_DIR}/bin

cfgfile=artemis/scale_cfg.json
if [[ "$1" ==  --cfg ]]; then
    cfgfile=$2
fi

$GDB apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/$cfgfile -f artemis --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml"
