#! /bin/bash

set -e
TOPDIR=`git rev-parse --show-toplevel`
export NICDIR="$TOPDIR/nic"
export HAL_LOG_DIR=${NICDIR}
export ZMQ_SOC_DIR=${NICDIR}
export CAPRI_MOCK_MODE=1
export CAPRI_MOCK_MEMORY_MODE=1
export SKIP_VERIFY=1
export BUILD_DIR=${NICDIR}/build/x86_64/apollo/
export GEN_TEST_RESULTS_DIR=${BUILD_DIR}/gtest_results
export HAL_CONFIG_PATH=${NICDIR}/conf
export CONFIG_PATH=${NICDIR}/conf
export COVFILE=${NICDIR}/coverage/sim_bullseye_hal.cov
#export GDB='gdb --args'

if [[ "$1" ==  --coveragerun ]]; then
    export COVFILE=${NICDIR}/coverage/sim_bullseye_hal.cov
fi

#function finish {
#   echo "===== Collecting logs ====="
#   ${NICDIR}/apollo/test/tools/savelogs.sh
#}
#trap finish EXIT

rm -rf core.*
export PATH=${PATH}:${BUILD_DIR}/bin
#$GDB flow_test -c hal.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml" $*
flow_test -c hal.json -f apollo $* > run.log; grep "flow_gtest" run.log
#$GDB flow_test -c hal.json -f apollo $*
#perf record --call-graph fp flow_test -c hal.json $* > run.log; grep flow_gtest run.log
#valgrind --track-origins=yes --xml=yes --xml-file=out.xml apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale_test/scale_cfg.json

sed -n '/TimeProfileID/,$p' run.log
