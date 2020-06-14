#! /bin/bash

export PIPELINE=apulu
CUR_DIR=$( readlink -f $( dirname $0 ) )

# initial setup
source ${CUR_DIR}/../setup_gtests.sh
setup

# run all upg gtests
mkdir -p /update/ # TODO : currently need to run from root
rm -rf /update/*
rm -rf /.upgrade* # TODO : include setup_upgrade, call the functions

if [[ "$1" ==  --coveragerun ]]; then
    # run sdk tests for code coverage
    run_sdk_gtest
fi

run_gtest nh_upg
run_gtest nh_group_upg
run_gtest mapping_upg
run_gtest tep_upg
run_gtest vnic_upg
run_gtest vpc_upg

# end of script
clean_exit
