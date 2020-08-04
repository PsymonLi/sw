#! /bin/bash

export PIPELINE=athena
export NICDIR=`pwd`

# initial setup
source ${NICDIR}/apollo/test/tools/setup_gtests.sh
setup

# run all gtests

if [[ "$1" ==  --coveragerun ]]; then
    # run sdk tests for code coverage
    run_sdk_gtest
fi

run_gtest vlan_to_vnic
run_gtest mpls_label_to_vnic
run_gtest conntrack
#run_gtest policer
run_gtest epoch
run_gtest dnat
run_gtest flow_cache
run_gtest l2_flow_cache
run_gtest flow_session_info
run_gtest flow_session_rewrite
run_gtest flow_cache_scale
run_gtest l2_flow_cache_scale
run_gtest dnat_scale
run_gtest flow_cache_scale_profile
run_gtest flow_cache_scale_profile_1

# end of script
clean_exit
