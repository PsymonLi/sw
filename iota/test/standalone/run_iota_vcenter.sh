#!/bin/bash

set -ex

cd /sw/iota

/sw/iota/scripts/modify_warmd.sh --vcenter-version ${VCENTER_VERSION}

./iota.py --testbed /sw/iota/warmd_vcenter.json $@
ret=$?

cp -v testsuite_*_results.json /testcase_result_export

exit $ret
