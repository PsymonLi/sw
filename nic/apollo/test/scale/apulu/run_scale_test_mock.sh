#! /bin/bash

export NICDIR=`pwd`
export PIPELINE=apulu
source $NICDIR/apollo/test/tools/setup_gtests.sh

export ZMQ_SOC_DIR=${NICDIR}
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PDSPKG_TOPDIR/third-party/metaswitch/output/x86_64/
#export GDB='gdb --args'

cfgfile=${PIPELINE}/scale_cfg.json
if [[ "$1" ==  --cfg ]]; then
    cfgfile=$2
fi

function finish {
    ${PDSPKG_TOPDIR}/tools/print-cores.sh
    echo "===== Collecting logs ====="
    ${NICDIR}/apollo/test/tools/savelogs.sh
    sudo pkill -9 vpp
    sudo pkill -9 dhcpd
    sudo rm -f /tmp/*.db /tmp/pen_* /dev/shm/pds_* /dev/shm/ipc_*
    remove_conf_files
}
trap finish EXIT

setup_conf_files

export PATH=${PATH}:${BUILD_DIR}/bin

echo "Starting VPP"
sudo $NICDIR/vpp/tools/start-vpp-mock.sh --pipeline ${PIPELINE}
if [[ $? != 0 ]]; then
    echo "Failed to bring up VPP"
    exit -1
fi

echo "Starting dhcpd"
sudo $NICDIR/apollo/tools/${PIPELINE}/start-dhcpd-sim.sh -p ${PIPELINE}
if [[ $? != 0 ]]; then
    echo "Failed to bring up dhcpd"
    exit -1
fi

$GDB ${PIPELINE}_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/$cfgfile --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/${PIPELINE}_scale_test.xml"
if [ $? -eq 0 ]
then
    rm -f ${PIPELINE}_scale_test.log
else
    tail -100 ${PIPELINE}_scale_test.log
fi
clean_exit
#$GDB ${PIPELINE}_scale_test -p p1 -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_p1.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/${PIPELINE}_scale_test.xml"
#$GDB ${PIPELINE}_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_v4_only.json --gtest_output="xml:${GEN_TEST_RESULTS_DIR}/${PIPELINE}_scale_test.xml"
#valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --gen-suppressions=all --verbose --error-limit=no --log-file=valgrind-out.txt ${PIPELINE}_scale_test -c hal.json -i ${NICDIR}/apollo/test/scale/scale_cfg_v4_only.json
