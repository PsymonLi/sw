#! /bin/bash -e

set -exo pipefail

cd `dirname $0`
cd `/bin/pwd`

export NIC_DIR=$PWD
export ASIC_SRC=$NIC_DIR/sdk/asic_repo/asic
export ASIC_GEN=$NIC_DIR/asic_gen
export PATH=$ASIC_SRC/common/tools/bin/:$PATH
export COVFILE=$NIC_DIR/coverage/bullseye_model.cov

BULLSEYE_PATH=/home/asic/tools/eda/bullseye/bin

echo "NIC_DIR:  $NIC_DIR"
echo "ASIC_SRC: $ASIC_SRC"
echo "ASIC_GEN: $ASIC_GEN"
echo $PATH

cd $ASIC_SRC/capri/model/cap_top

if [ $# -eq 0 ]; then
    echo "Starting ASIC build"
    gen_rtl -n -v NOSKNOBS_PATH=1 OPT=-O2 -j$(grep -c processor /proc/cpuinfo)
elif [ $1 == "--coverage" ]; then
    echo "Starting ASIC build with coverage"
    $BULLSEYE_PATH/cov01 -1
    gen_rtl -n -v -- --coverage=1 NOSKNOBS_PATH=1 OPT=-O2 EXT_CC_OVERRIDE=$BULLSEYE_PATH/g++  -j $(grep -c processor /proc/cpuinfo)
    $BULLSEYE_PATH/cov01 -0
elif [ $1 == "--clean" ]; then
    echo "Starting ASIC clean build"
    gen_rtl -n -v -c
    rm -f $COVFILE
else
    echo "Invalid args. Use --coverage or --clean"
    exit
fi

$NIC_DIR/sdk/model_sim/cp_asic_bins.sh

# TODO build capsim-master. Required?
# cd $ASIC_SRC/capri/model/capsim-master
# make
# make ARCH=aarch64

# change dir back to NIC
cd $NIC_DIR
