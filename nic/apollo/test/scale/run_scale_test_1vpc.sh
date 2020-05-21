#! /bin/bash

set -e
export ASIC="${ASIC:-capri}"
export NICDIR=`pwd`
export NON_PERSISTENT_LOG_DIR=${NICDIR}
export ZMQ_SOC_DIR=${NICDIR}
export SKIP_VERIFY=1
export BUILD_DIR=${NICDIR}/build/x86_64/apollo/${ASIC}
export GEN_TEST_RESULTS_DIR=${BUILD_DIR}/gtest_results
export CONFIG_PATH=${NICDIR}/conf
#export GDB='gdb --args'

export PATH=${PATH}:${BUILD_DIR}/bin
rm -f $NICDIR/conf/pipeline.json
ln -s $NICDIR/conf/apollo/pipeline.json $NICDIR/conf/pipeline.json
$GDB apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_1vpc.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/apollo_scale_test.xml"
rm -f $NICDIR/conf/pipeline.json

# Valgrind with XML output
#valgrind --track-origins=yes --xml=yes --xml-file=out.xml apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_1vpc.json

# Valgrind with text output
#valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all apollo_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_1vpc.json
