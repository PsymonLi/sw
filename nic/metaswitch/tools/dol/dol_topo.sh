#! /bin/bash
#set -x
NICDIR="${PWD%/nic/*}/nic"
NIC_CONF_PATH="$NICDIR/conf"
PIPELINE="apulu"
SW_DIR=`dirname $NICDIR`
SUBNET=Test_Subnet
SUBNET1=10.1.1.0/24
SUBNET2=11.1.1.0/24
SUBNET3=20.1.1.0/24
CONTAINER=CTR
DOL_CFG=/sw/nic/metaswitch/config/dol_ctr
DOL_CFG_UPG=/sw/nic/metaswitch/config/dol_ctr1/ht-upg
MIB_PY=/sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py
PDSAGENT=/sw/nic/apollo/tools/apulu/start-agent-mock.sh
OPERD_METRICS_PATH=${NICDIR}/infra/operd/metrics/cloud/

###################################################
#
#                           Cntr 3
#                             |  10.3 (210.210.3.3)
#                             |
#                  10.1 --------------- 10.2
#   (100.100.1.1) Cntr 1               Cntr 2  (200.200.2.2)
#                  11.1 --------------- 11.2
#
#
###################################################

RR=0
DOL_RUN=0
DOL_TEST=0
UNDERLAY=0
UPG_TEST_A=0
UPG_FAIL=0
ORF_TEST=0

CMDARGS=""
argc=$#
argv=($@)
for (( j=0; j<argc; j++ )); do
    if [ ${argv[j]} == '--rr' ];then
        RR=1
    elif [ ${argv[j]} == '--dolrun' ];then
        DOL_RUN=1
    elif [ ${argv[j]} == '--doltest' ];then
        DOL_TEST=1
    elif [ ${argv[j]} == '--underlay' ];then
        UNDERLAY=1
    elif [ ${argv[j]} == '--grupg' ];then
        GRACEFUL_UPG=1
    elif [ ${argv[j]} == '--hiupg' ];then
        HITLESS_UPG=1
    elif [ ${argv[j]} == '--hiupg-test-a' ];then
        HITLESS_UPG=1
        UPG_TEST_A=1
    elif [ ${argv[j]} == '--hiupg-fail' ];then
        UPG_FAIL=1
    elif [ ${argv[j]} == '--orf' ];then
        ORF_TEST=1
        RR=1
    else
        CMDARGS+="${argv[j]} "
    fi
done

function remove_conf_files () {
    find ${OPERD_METRICS_PATH} -name "*.json" -printf "rm -f ${NIC_CONF_PATH}/%P \n" | sh
}

function setup_metrics_conf_files () {
    cp ${OPERD_METRICS_PATH}/*.json ${NIC_CONF_PATH}/
}

function setup_upgrade_version_files() {
    sudo rm -f ${NIC_CONF_PATH}/upgrade_cc_*
    cp ${NIC_CONF_PATH}/$PIPELINE/upgrade_cc_graceful_version.json ${NIC_CONF_PATH}/upgrade_cc_graceful_version.json
    cp ${NIC_CONF_PATH}/$PIPELINE/upgrade_cc_hitless_version.json ${NIC_CONF_PATH}/upgrade_cc_hitless_version.json
}

setup_metrics_conf_files
setup_upgrade_version_files

if [ $DOL_RUN == 1 ]; then
   DUTDIR=$NICDIR
else
   DUTDIR=$NICDIR/metaswitch/config/dol_ctr1
fi

if [ $RR = 1 ]; then
    CLIENTAPP='/sw/nic/build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test'
    CLIENTARG=''
elif [ $UNDERLAY = 1 ]; then
    CLIENTAPP='/sw/nic/build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test'
    CLIENTARG='underlay'
else
    CLIENTAPP='/sw/nic/build/x86_64/apulu/capri/bin/pds_ms_uecmp_grpc_test'
    CLIENTARG=''
fi

function amx-open {
   docker exec -it -e CONFIG_PATH="$DOL_CFG"$1 "$CONTAINER"$1 sh -c "$CLIENTAPP amx-open"
}
function amx-close {
   docker exec -it -e CONFIG_PATH="$DOL_CFG"$1 "$CONTAINER"$1 sh -c "$CLIENTAPP amx-close"
}

function start-pdsagent-dut {
    echo "start pdsagent in "$CONTAINER"1"
    ret=0

#   TODO: Controlplane sim testing with underlay in IPC MOCK MODE is not supported
#         since L3 interface requires IPC. Temporarily switch to PDS_MOCK_MODE until
#         we get it to work in IPC mode which requires full setup including VPP similar
#         to DOL
#    docker exec -it "$CONTAINER"1 sh -c "/sw/nic/metaswitch/tools/dol/manual_test.sh" || ret=$?
    docker exec -dit -w $1 "$CONTAINER"1 sh -c "PDS_MOCK_MODE=1 CSS_SNAPSHOT_DIR=${DOL_CFG}1 $PDSAGENT --log-dir $1" || ret=$?

    if [ $ret -ne 0 ]; then
        echo "failed to start pdsagent in "$CONTAINER"1: $ret"
    fi
    echo "sleep 10 seconds to let n-base to start in containers"
    t_s=10
    while [ $t_s -gt 0 ]; do
       echo -ne "$t_s\033[0K\r"
       sleep 1
       : $((t_s--))
    done
}

# Function to configure nodes using gRPC Client App
function client-app-cfg {
    if [ $UNDERLAY == 1 ] && [ "$1" = "3" ]; then
        echo "Skip configuring CTR3 in Underlay mode"
        return
    fi
    ret=0
    echo "push "$DOL_CFG"$1/evpn.json config to "$CONTAINER"$1"
    docker exec -it -e CONFIG_PATH="$DOL_CFG"$1  "$CONTAINER"$1 sh -c "$CLIENTAPP $CLIENTARG" || ret=$?

    if [ $ret -ne 0 ]; then
        echo "failed to push config to "$CONTAINER"$1: $ret"
    fi
}

if [ $HITLESS_UPG == 1 ]; then
    echo "Simulate Hitless upgrade Start event"
    docker exec -it -e CONFIG_PATH="$DOL_CFG"1 "$CONTAINER"1 sh -c "$CLIENTAPP htupg-start"
    echo "Check Domain A datapath not disturbed"
    sleep 5
    # Simulate 1 BGP Peer fail during upgrade
    # docker exec -it -e CONFIG_PATH="$DOL_CFG"2  "$CONTAINER"2 sh -c "$CLIENTAPP bgp-upeer-del 2" || ret=$?
    if [ $UPG_TEST_A == 0 ]; then
       echo "Kill Domain A pdsagent"
       docker exec -it "$CONTAINER"1 sh -c "pkill pdsagent"
       sleep 2
       #echo "Starting pegasus in "$CONTAINER"$i"
       #docker exec -dit -w "$DOL_CFG_UPG" -e LD_LIBRARY_PATH=/sw/nic/third-party/metaswitch/output/x86_64/ "$CONTAINER"1 sh -c '/sw/nic/build/x86_64/apulu/capri/bin/pegasus' || ret=$?
       echo "Starting Domain B pdsagent in "$CONTAINER"$i"
       start-pdsagent-dut $DOL_CFG_UPG
    fi
    if [ $UPG_TEST_A == 0 ]; then
       client-app-cfg 1
       if [ $UPG_FAIL == 1 ]; then
          echo "Simulate upgrade failure - send rollback to B"
          docker exec -it -e CONFIG_PATH="$DOL_CFG"1 "$CONTAINER"1 sh -c "$CLIENTAPP htupg-rollbk"
       else    
          echo "Simulate Hitless upgrade Domain B Sync config replay in 5 seconds"
          sleep 5
          docker exec -it -e CONFIG_PATH="$DOL_CFG"1 "$CONTAINER"1 sh -c "$CLIENTAPP htupg-sync"
          sleep 10
       fi
    else
       if [ $UPG_FAIL == 1 ]; then
          echo "Simulate upgrade failure - send repeal to A"
          docker exec -it -e CONFIG_PATH="$DOL_CFG"1 "$CONTAINER"1 sh -c "$CLIENTAPP htupg-repeal"
       fi
    fi
    exit
fi

if [ $GRACEFUL_UPG == 1 ]; then
    docker exec -it "$CONTAINER"1 sh -c "pkill pdsagent"
    sleep 2
    start-pdsagent-dut "${DOL_CFG}1"
    client-app-cfg 1
    exit
fi

if [ $DOL_TEST == 1 ]; then
    # Setup already made - just run DOL
    docker exec -it "$CONTAINER"1 sh -c "sudo SKIP_FRU_SETUP=1 ./apollo/tools/rundol.sh --pipeline apulu --topo overlay --feature overlay_networking --sub underlay_trig $CMDARGS" || ret=$?
    exit
fi

# Pull the pensando/nic container image from the registry
# and create a container
# Use custom target instead of docker/shell to skip pull-assets on warmd VM
make docker/build-runtime-ctr

#Cleanup any stale test containers
echo "Cleanup pre-existing test containers"
ids=$(docker ps --format {{.Names}}| grep $CONTAINER)
for id in $ids
do
    echo "Kill container: $id"
    docker kill $id > /dev/null
done

INTF_NAME=`ip link show | grep -v @ | grep en.*UP | head -n1 | cut -d ":" -f2 | xargs`
echo "Using interface $INTF_NAME"

#Cleanup mac-vlan interface, if exists
ip link del ${INTF_NAME}.3006 > /dev/null
ip link add link ${INTF_NAME} name ${INTF_NAME}.3006 type vlan id 3006 >/dev/null
ip link set ${INTF_NAME} up > /dev/null
ip link set ${INTF_NAME}.3006 up > /dev/null
ip link del ${INTF_NAME}.3007 > /dev/null
ip link add link ${INTF_NAME} name ${INTF_NAME}.3007 type vlan id 3007 >/dev/null
ip link set ${INTF_NAME} up > /dev/null
ip link set ${INTF_NAME}.3006 up > /dev/null

# delete all existing networks
echo "Cleanup pre-existing test networks"
ids=$(docker network ls --format {{.Name}} | grep $SUBNET)
for id in $ids
do
    echo "Delete network: $id"
    docker network rm $id > /dev/null
done

#create networks with test subnets as per topology
echo "Create network: "$SUBNET"1 $SUBNET1"
docker network create -d bridge "$SUBNET"1 --subnet $SUBNET1 --gateway 10.1.1.254 -o parent=${INTF_NAME}.3007 > /dev/null
echo "Create network: "$SUBNET"2 $SUBNET2"
docker network create -d macvlan --subnet $SUBNET2 --gateway 11.1.1.254 -o parent=${INTF_NAME}.3006 "$SUBNET"2 > /dev/null

if [ "$ORF_TEST" -eq "1" ]; then
    MAX_NODES=4
else
    MAX_NODES=3
fi
for ((i=1 ; i<=$MAX_NODES; i++)); do
    if [ "$i" -ge "3" ] && [ $UNDERLAY == 1 ]; then
        echo "Skip CTR3 in Underlay mode"
        continue
    fi
    docker run -dit --rm -e PYTHONPATH=/sw/nic/third-party/metaswitch/code/comn/tools/mibapi --sysctl net.ipv6.conf.all.disable_ipv6=1 --net="$SUBNET"1 --privileged --name "$CONTAINER"$i -v $SW_DIR:/sw  -w /sw/nic pensando/nic > /dev/null
    id=$(docker ps --format {{.Names}}| grep "$CONTAINER"$i)
    ip=$(docker exec -it "$CONTAINER"$i ip -o -4 addr list eth0 | awk '{print $4}' | cut -d "/" -f 1)
    printf "Container: $id is started with eth0 $ip "
    if [ $i -lt  3 ]; then
        docker network connect "$SUBNET"2 "$CONTAINER"$i > /dev/null
        ip2=$(docker exec -it "$CONTAINER"$i ip -o -4 addr list eth1 | awk '{print $4}' | cut -d "/" -f 1)
        echo "and eth1 $ip2"
    fi
    docker exec -dit "$CONTAINER"$i cp "$DOL_CFG"$i/fru.json /tmp/fru.json
done
echo ""
if [ $RR -eq 1 ]; then
   # RR has no Linux route programming - so configure explicit route to TEP IPs
   docker exec -dit "$CONTAINER"3 ip route add 100.100.1.1/32 via 10.1.1.1
   docker exec -dit "$CONTAINER"3 ip route add 200.200.2.2/32 via 10.1.1.2
   docker exec -dit "$CONTAINER"3 ip route add 4.4.4.4/32 via 10.1.1.4
   docker exec -dit "$CONTAINER"4 ip route add 3.3.3.3/32 via 10.1.1.3
fi

# Remove interface addresses - controlplane should install this
docker exec -dit "$CONTAINER"1 ip addr del 10.1.1.1/24 dev eth0
docker exec -dit "$CONTAINER"1 ip addr del 11.1.1.1/24 dev eth1
docker exec -dit "$CONTAINER"2 ip addr del 10.1.1.2/24 dev eth0
docker exec -dit "$CONTAINER"2 ip addr del 11.1.1.2/24 dev eth1

# First start PDS-Agent on nodes 2 and 3
for ((i=2 ; i<=$MAX_NODES; i++)); do
    ret=0
    if [ "$i" = "3" ] && [ $UNDERLAY == 1 ]; then
        echo "Skip CTR3 in Underlay mode"
        continue
    fi
    if [ "$i" -ge "3" ] && [ $RR == 1 ]; then
        echo "Starting Pegasus in "$CONTAINER"$i"
        docker exec -dit -w "$DOL_CFG"$i -e LD_LIBRARY_PATH=/sw/nic/third-party/metaswitch/output/x86_64/ "$CONTAINER"$i sh -c '/sw/nic/build/x86_64/apulu/capri/bin/pegasus' || ret=$?
    else
        echo "start pdsagent in "$CONTAINER"$i in PDS_MOCK_MODE"
        docker exec -dit -w "$DOL_CFG"$i "$CONTAINER"$i sh -c "PDS_MOCK_MODE=1 $PDSAGENT --log-dir ${DOL_CFG}$i" || ret=$?
    fi
    if [ $ret -ne 0 ]; then
        echo "failed to start pdsagent in "$CONTAINER"$i: $ret"
    fi
    echo "sleep 10 seconds to let n-base to start in containers"
    t_s=10
    while [ $t_s -gt 0 ]; do
       echo -ne "$t_s\033[0K\r"
       sleep 1
       : $((t_s--))
    done
done

if [ $UNDERLAY == 0 ] && [ $RR == 0 ]; then
    # 3 TEP node topology - configure EVPN TEP IP for node 3 even though it doesn't have lo
    amx-open 3
    docker exec -it "$CONTAINER"3 python $MIB_PY set localhost evpnEntTable evpnEntEntityIndex=2 evpnEntLocalRouterAddressType=inetwkAddrTypeIpv4 evpnEntLocalRouterAddress='0xd2 0xd2 0x3 0x3'
    amx-close 3
fi

# Configure underlay and overlay on nodes 2 and 3 using gRPC Client App
for ((i=2 ; i<=$MAX_NODES; i++)); do
    client-app-cfg $i
done

# Finally start Pds-Agent on the DUT of this is not the DOL run
# and configure underlay and overlay using gRPC Client App
if [ $DOL_RUN == 0 ]; then
    start-pdsagent-dut "${DOL_CFG}1"
    client-app-cfg 1
fi

if [ $UNDERLAY == 1 ]; then
    grep ++++ $DUTDIR/pds-agent.log
    if grep -q "++++ Underlay Pathset.* Num NH 2.*Async reply Success" $DUTDIR/pds-agent.log; then
       echo Underlay ECMP found PASS
    else
       echo Underlay ECMP not found FAIL
       ret=1
    fi
    exit
fi

echo "Originate EVPN Type 5 route from "$CONTAINER"2"

amx-open 2
docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmRedistTable rtmRedistFteIndex=2 rtmRedistEntryId=10 rtmRedistRowStatus=createAndGo rtmRedistAdminStat=adminStatusUp  rtmRedistInfoSrc=atgQcProtStatic  rtmRedistInfoDest=atgQcProtBgp rtmRedistDestInstFlt=true rtmRedistDestInst=2  rtmRedistRedistFlag=true"
docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost limL3InterfaceAddressTable \
                                     limEntEntityIndex=1 \
                                     limL3InterfaceAddressIfIndex=1073823745 \
                                     limL3InterfaceAddressIPAddrType=inetwkAddrTypeIpv4 \
                                     limL3InterfaceAddressIPAddress=12.0.0.254 \
                                     limL3InterfaceAddressRowStatus=rowCreateAndGo \
                                     limL3InterfaceAddressAdminStatus=adminStatusUp \
                                     limL3InterfaceAddressPrefixLen=24"

docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=0x80100000 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c00000f rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=createAndGo rtmStaticRtAdminStat=adminStatusUp rtmStaticRtOverride=true rtmStaticRtAdminDist=150"
amx-close 2

if [ $RR == 0 ]; then
    echo "Originate EVPN Type 5 route from "$CONTAINER"3"

    amx-open 3
    docker exec -it "$CONTAINER"3 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmRedistTable rtmRedistFteIndex=2 rtmRedistEntryId=10 rtmRedistRowStatus=createAndGo rtmRedistAdminStat=adminStatusUp  rtmRedistInfoSrc=atgQcProtStatic  rtmRedistInfoDest=atgQcProtBgp rtmRedistDestInstFlt=true rtmRedistDestInst=2  rtmRedistRedistFlag=true"
    docker exec -it "$CONTAINER"3 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost limL3InterfaceAddressTable \
                                     limEntEntityIndex=1 \
                                     limL3InterfaceAddressIfIndex=1073823745 \
                                     limL3InterfaceAddressIPAddrType=inetwkAddrTypeIpv4 \
                                     limL3InterfaceAddressIPAddress=12.0.0.254 \
                                     limL3InterfaceAddressRowStatus=rowCreateAndGo \
                                     limL3InterfaceAddressAdminStatus=adminStatusUp \
                                     limL3InterfaceAddressPrefixLen=24"

    docker exec -it "$CONTAINER"3 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=0x80100000 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c00000e rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=createAndGo rtmStaticRtAdminStat=adminStatusUp rtmStaticRtOverride=true rtmStaticRtAdminDist=150"
    amx-close 3
fi

if [ $DOL_RUN == 1 ]; then
    docker exec -it "$CONTAINER"1 sh -c "sudo SKIP_FRU_SETUP=1 ./apollo/tools/rundol.sh --pipeline apulu --topo overlay --feature overlay_networking --sub underlay_trig $CMDARGS" || ret=$?
    if [ $ret -ne 0 ]; then
        echo "DOL run failed: $ret"
    else
        echo "DOL run passed: $ret"
    fi

    cd $NICDIR && tar -cvzf controlplane_dol_logs.tgz core* *.log metaswitch/config/dol_ctr*/*.log metaswitch/config/dol_ctr*/core*

    grep ++++ $NICDIR/pds-agent.log
    if grep -q "++++ Underlay Pathset.* Num NH 2.*Async reply Success" $NICDIR/pds-agent.log; then
       echo Underlay ECMP found PASS
    else
       echo Underlay ECMP not found FAIL
       ret=1
    fi
    if grep -q "++++ Overlay Pathset.* Num NH 2.*Async reply Success" $NICDIR/pds-agent.log; then
       echo Overlay ECMP found PASS
    else
       echo Overlay ECMP not found FAIL
       ret=1
    fi
    if grep -q "Route Add IPS VRF 2 Prefix 128.16.0.0/16" $DUTDIR/pds-agent.log; then
       echo Type 5 route pending batch PASS
    else
       echo Type 5 route not installed FAIL
       ret=1
    fi
    if grep -q "++++ MS Route Batch Commit for Route table:.*Async reply Success" $DUTDIR/pds-agent.log; then
       echo Type 5 route batch commit PASS
    else
       echo Type 5 route batch commit FAIL
       ret=1
    fi
    if grep -q "++++ BD 1 MAC 00:ee:00:00:00:05.*Async reply Success" $NICDIR/pds-agent.log; then
       echo Type 2 route installed PASS
    else
       echo Type 2 route not installed FAIL
       ret=1
    fi
    exit $ret
else
    sleep 5
    ret=0
    grep ++++ $DUTDIR/pds-agent.log
    if grep -q "++++ Underlay Pathset.* Num NH 2.*Async reply Success" $DUTDIR/pds-agent.log; then
       echo Underlay ECMP found PASS
    else
       echo Underlay ECMP not found FAIL
       ret=1
    fi
    if [ $RR == 0 ]; then
        if grep -q "++++ Overlay Pathset.* Num NH 2.*Async reply Success" $DUTDIR/pds-agent.log; then
           echo Overlay ECMP found PASS
        else
           echo Overlay ECMP not found FAIL
           ret=1
        fi
    else
        if grep -q "++++ Overlay Pathset.* Num NH 1.*Async reply Success" $DUTDIR/pds-agent.log; then
           echo Overlay Pathset found PASS
        else
           echo Overlay Pathset not found FAIL
           ret=1
        fi
    fi
    if grep -q "128.16.0.0/16 route.*added to pending batch" $DUTDIR/pds-agent.log; then
       echo Type 5 route pending batch PASS
    else
       echo Type 5 route not installed FAIL
       ret=1
    fi
    if grep -q "++++ MS Route Batch Commit for Route table:.*Async reply Success" $DUTDIR/pds-agent.log; then
       echo Type 5 route batch commit PASS
    else
       echo Type 5 route batch commit FAIL
       ret=1
    fi
    if [ $RR == 1 ]; then
        if grep -q "++++ BD 1 MAC 00:12:23:45:67:08.*Async reply Success" $DUTDIR/pds-agent.log; then
           echo Type 2 route installed PASS
        else
           echo Type 2 route not installed FAIL
           ret=1
        fi
    else
        if grep -q "++++ BD 1 MAC 00:ee:00:00:00:05.*Async reply Success" $DUTDIR/pds-agent.log; then
           echo Type 2 route installed PASS
        else
           echo Type 2 route not installed FAIL
           ret=1
        fi
    fi
    exit $ret
fi
remove_conf_files


# Older way to create Type 5
#Create an connected L3 interface subnet in Tenant VRF to act as nexthop for the prefix so that the Prefix gets advertised to EVPN
#docker exec -it "$CONTAINER"2 python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost limGenIrbInterfaceTable \
#                                     limEntEntityIndex=1 \
#                                     limGenIrbInterfaceBdType=limBridgeDomainEvpn \
#                                     limGenIrbInterfaceBdId=1 \
#                                     limGenIrbInterfaceRowStatus=rowCreateAndGo \
#                                     limGenIrbInterfaceMacAddr="0x00 0x11 0x22 0x33 0x44 0x55" \
#                                     limGenIrbInterfaceAdminStatus=adminStatusUp
#
#docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost limInterfaceCfgTable \
#                                     limEntEntityIndex=1 \
#                                     limInterfaceCfgIfIndex=1073823745 \
#                                     limInterfaceCfgIpv6Enabled=triStateFalse \
#                                     limInterfaceCfgRowStatus=rowCreateAndGo \
#                                     limInterfaceCfgBindVrfName=0x32"
#
#docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost limL3InterfaceAddressTable \
#                                     limEntEntityIndex=1 \
#                                     limL3InterfaceAddressIfIndex=1073823745 \
#                                     limL3InterfaceAddressIPAddrType=inetwkAddrTypeIpv4 \
#                                     limL3InterfaceAddressIPAddress=192.168.1.254 \
#                                     limL3InterfaceAddressRowStatus=rowCreateAndGo \
#                                     limL3InterfaceAddressAdminStatus=adminStatusUp \
#                                     limL3InterfaceAddressPrefixLen=24"

#docker exec -it "$CONTAINER"2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=0x80100000 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0xc0a8010a rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=createAndGo rtmStaticRtAdminStat=adminStatusUp rtmStaticRtOverride=true rtmStaticRtAdminDist=150"

