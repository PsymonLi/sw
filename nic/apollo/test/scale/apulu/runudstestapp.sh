#! /bin/bash -x

TOPDIR=`git rev-parse --show-toplevel`
NICDIR="$TOPDIR/nic"
export PIPELINE=apulu
source $NICDIR/apollo/test/tools/setup_gtests.sh

# for mock runs, point obfl logging to /tmp
export PERSISTENT_LOG_DIR=/tmp/
export AGENT_TRANSPORT_UDS=1

cleanup() {
    pkill agent
    # remove pdsctl gen files
    sudo rm -f $NICDIR/out.sh
    # remove uds socket
    sudo rm -f /var/run/pds_svc_server_sock
    # stop vpp
    # sudo pkill -9 vpp
    sudo pkill -9 dhcpd
    sudo rm -f /tmp/*.db /tmp/pen_* /dev/shm/pds_* /dev/shm/ipc_*
    remove_conf_files
}
trap cleanup EXIT

setup_conf_files

$NICDIR/apollo/tools/${PIPELINE}/start-agent-mock.sh > agent.log 2>&1 &
# wait till agent opens up gRPC service port
sleep 10

#echo "Starting VPP"
#sudo $NICDIR/vpp/tools/start-vpp-mock.sh --pipeline ${PIPELINE}
#if [[ $? != 0 ]]; then
    #echo "Failed to bring up VPP"
    #exit -1
#fi

echo "Starting dhcpd"
sudo $NICDIR/apollo/tools/${PIPELINE}/start-dhcpd-sim.sh -p ${PIPELINE}
if [[ $? != 0 ]]; then
    echo "Failed to bring up dhcpd"
    exit -1
fi

$NICDIR/build/x86_64/${PIPELINE}/${ASIC}/bin/udstestapp -i $NICDIR/apollo/test/scale/${PIPELINE}/scale_cfg.json
linecount=`$NICDIR/build/x86_64/${PIPELINE}/${ASIC}/bin/pdsctl show vpc --transport uds | grep "TENANT" | wc -l`
if [[ $linecount -eq 0 ]]; then
    echo "testapp failure"
    exit 1
fi
clean_exit
