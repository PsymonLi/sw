#! /bin/bash

set -e
export ASIC="${ASIC:-capri}"
export NICDIR=`pwd`
export PDSPKG_TOPDIR=$NICDIR
export NON_PERSISTENT_LOG_DIR=${NICDIR}
export ZMQ_SOC_DIR=${NICDIR}
export ASIC_MOCK_MODE=1
export ASIC_MOCK_MEMORY_MODE=1
export VPP_IPC_MOCK_MODE=1
export SKIP_VERIFY=1
export BUILD_DIR=${NICDIR}/build/x86_64/apollo/${ASIC}
export GEN_TEST_RESULTS_DIR=${BUILD_DIR}/gtest_results
export CONFIG_PATH=${NICDIR}/conf
#export GDB='gdb --args'

cfgfile=scale_cfg.json
if [[ "$1" ==  --cfg ]]; then
    cfgfile=$2
fi

export PATH=${PATH}:${BUILD_DIR}/bin
rm -f $NICDIR/conf/pipeline.json
ln -s $NICDIR/conf/apollo/pipeline.json $NICDIR/conf/pipeline.json
apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/$cfgfile --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml" > apollo_scale_test.log
#$GDB apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/$cfgfile --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml"
rm -f $NICDIR/conf/pipeline.json
if [ $? -eq 0 ]
then
    rm -f apollo_scale_test.log
else
    tail -100 apollo_scale_test.log
fi
#$GDB apollo_scale_test -p p1 -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_p1.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml"
#$GDB apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_v4_only.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml"
#valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --gen-suppressions=all --verbose --error-limit=no --log-file=valgrind-out.txt apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_v4_only.json
