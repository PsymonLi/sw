#!/bin/bash
DUT=$1
TESTER=$2

# Set up the DTS configuration files
echo
echo "[>>>] Configuring DTS on $TESTER..."
cd pen-conf

DUT_IP=`host $DUT | cut -d ' ' -f 4`
TESTER_IP=`host $TESTER | cut -d ' ' -f 4`

dts_filter() {
    sed -i "s/XXX_DUT/$DUT_IP/" $1
    sed -i "s/XXX_TESTER/$TESTER_IP/" $1
    sed -i "s/XXX_PWD/docker/" $1
}

dts_filter conf/crbs.cfg
dts_filter conf/ports.cfg
dts_filter execution.cfg
dts_filter hello_world_execution.cfg
dts_filter unit_test_execution.cfg
dts_filter feature_execution.cfg
dts_filter fwd_execution.cfg
dts_filter pkt_execution.cfg
dts_filter pmd_execution.cfg

readarray -t INTEL_PORTS <<<`lspci | grep Ethernet | tail -n 2 | cut -d ' ' -f 1`
sed -i "s/XXX_DT0/${INTEL_PORTS[0]}/" conf/ports.cfg
sed -i "s/XXX_DT1/${INTEL_PORTS[1]}/" conf/ports.cfg

readarray -t NAPLES_PORTS <<<`sshpass -p docker ssh root@$DUT_IP "lspci | grep 1dd8:1002 | cut -d ' ' -f 1"`
sed -i "s/XXX_DP0/${NAPLES_PORTS[0]}/" conf/ports.cfg
sed -i "s/XXX_DP1/${NAPLES_PORTS[1]}/" conf/ports.cfg

cd ..
sed -i "s/XXX_DUT/${DUT}/" run.sh

echo
echo "[>>>] Configured network ports in $TESTER:dpdk-test/pen-conf/conf/ports.cfg:"
echo "    $DUT:${NAPLES_PORTS[0]} <-> $TESTER:${INTEL_PORTS[0]}"
echo "    $DUT:${NAPLES_PORTS[1]} <-> $TESTER:${INTEL_PORTS[1]}"
echo "PLEASE CONFIRM THAT THIS IS CORRECT BEFORE STARTING DTS"
echo
echo "[>>>] Configure DTS tests in:"
echo "    $TESTER:dpdk-test/pen-conf/execution.cfg"
echo
echo "[>>>] DTS can be started on $TESTER with:"
echo " # cd dpdk-test && ./run.sh"

