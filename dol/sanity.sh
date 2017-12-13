#! /bin/bash
export DIR_DOL=`dirname $0`
cd $DIR_DOL

function ErrorCheckNExit() {
    if [ "$1" = "0" ];then
        return
    fi

    echo "ERROR: $2 FAILED"
    exit 1
}

# Build the minimum targets required for dryrun.
#cd ../nic/utils/host_mem && make && cd -
cd /sw && make pull-assets && cd -
cd ../nic/proto/ && make && cd -
bazel build --deleted_packages=nic/sdk/include,nic/sdk,nic/sdk/obj //nic/utils/host_mem:host_mem
#bazel build //nic/proto:all
#bazel build //nic/proto/hal:all
#bazel build //nic/proto/agents:all

function DryRunSanity()
{
    topo=$1
    feature=$2
    ./main.py --dryrun --topo $topo --feature $feature
    ErrorCheckNExit $? "$topo topo"
}

DryRunSanity dos dos
DryRunSanity fte fte
DryRunSanity norm norm
DryRunSanity eth eth
DryRunSanity acl acl
DryRunSanity proxy proxy
#DryRunSanity proxy ipsec
DryRunSanity up2up networking
DryRunSanity vxlan vxlan
DryRunSanity eth ipsg
DryRunSanity firewall firewall
DryRunSanity telemetry telemetry
DryRunSanity admin admin
DryRunSanity rdma rdma
DryRunSanity eth dolut
DryRunSanity alg alg
exit 0
