#!/bin/bash

cd /sw/iota
cat /warmd.json

./iota.py --testbed /warmd.json $@
ret=$?

if [ "$ret" = "124" ];then
    #run triage script
    timeout 300 ./harness/infra/triage_failure.py --testbed ../warmd.json --destdir testcase_result_export --username vm --password vm
fi

cp -v testsuite_*_results.json /testcase_result_export

exit $ret
