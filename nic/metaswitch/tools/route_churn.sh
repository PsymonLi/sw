#! /bin/bash
#set -x

BGP_LOCAL_IP=0
BGP_PEER_IP=0
NAPLES=0
QUERY_BGP=0
QUERY_PEER=0
NUM_ROUTES=1050
NUM_ITER=120
DISABLE_BGP_PEER=0
MONITOR_LOG_PATH=./route-churn/
MONITOR_LOG_FILE=monitor.log
CLEANUP=0
SKIP_INIT=0
TYPE5_GEN=0

argc=$#
argv=("$@")
for (( j=0; j<argc; j++ )); do
    if [ ${argv[j]} == '--naples' ];then
        NAPLES=1
        (( j=j+1 ))
        NAPLES_IP=${argv[j]}
    elif [ ${argv[j]} == '--monitor' ];then
        (( j=j+1 ))
        NAPLES_IP_MONITOR=${argv[j]}
    elif [ ${argv[j]} == '--peer' ];then
        (( j=j+1 ))
        BGP_PEER_IP=${argv[j]}
    elif [ ${argv[j]} == '--routes' ];then
        (( j=j+1 ))
        NUM_ROUTES=${argv[j]}
    elif [ ${argv[j]} == '--local' ];then
        (( j=j+1 ))
        BGP_LOCAL_IP=${argv[j]}
    elif [ ${argv[j]} == '--qpeer' ];then
        QUERY_PEER=1
    elif [ ${argv[j]} == '--qbgp' ];then
        QUERY_BGP=1
    elif [ ${argv[j]} == '--peerdis' ];then
        DISABLE_BGP_PEER=1
    elif [ ${argv[j]} == '--cleanup' ];then
        CLEANUP=1
    elif [ ${argv[j]} == '--skipinit' ];then
        SKIP_INIT=1
    elif [ ${argv[j]} == '--t5gen' ];then
        TYPE5_GEN=1
    fi
done

export PYTHONPATH=../../third-party/metaswitch/code/comn/tools/mibapi/
dckr_cmd="docker exec -it CTR2 sh -c "

if [[ $NAPLES == 1 ]]; then
    amx_cmd="python ../../third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set $NAPLES_IP"
    amx_get_cmd="python ../../third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py get $NAPLES_IP"
else
    amx_cmd="python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost"
fi

function open_amx {
    if [[ $NAPLES == 1 ]]; then
       ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP "/nic/bin/pdsctl debug routing open"
    else
       docker exec -it CTR2 sh -c "/sw/nic/build/x86_64/apulu/bin/pdsctl debug routing open"
    fi
}

function start_top {
    if [[ $NAPLES == 0 ]]; then
      docker exec -dit CTR1 sh -c 'top -b -p `pgrep pdsagent` > /sw/nic/metaswitch/config/dol_ctr1/route_churn_top_out'
   fi
}

function setup_type5 {
    rtredist_cmd="rtmRedistTable rtmRedistFteIndex=2 rtmRedistEntryId=10 rtmRedistRowStatus=createAndGo rtmRedistAdminStat=adminStatusUp  rtmRedistInfoSrc=atgQcProtStatic  rtmRedistInfoDest=atgQcProtBgp rtmRedistDestInstFlt=true rtmRedistDestInst=2  rtmRedistRedistFlag=true"
    limintf_cmd="limL3InterfaceAddressTable \
                                     limEntEntityIndex=1 \
                                     limL3InterfaceAddressIfIndex=1073823745 \
                                     limL3InterfaceAddressIPAddrType=inetwkAddrTypeIpv4 \
                                     limL3InterfaceAddressIPAddress=12.0.0.254 \
                                     limL3InterfaceAddressRowStatus=rowCreateAndGo \
                                     limL3InterfaceAddressAdminStatus=adminStatusUp \
                                     limL3InterfaceAddressPrefixLen=24"

    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $rtredist_cmd
        $amx_cmd $limintf_cmd
    fi
}

function conf_type5 {
    conf_t5_cmd="rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=$1 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c0000ee rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=createAndGo rtmStaticRtAdminStat=adminStatusUp rtmStaticRtOverride=true rtmStaticRtAdminDist=150"
    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $conf_t5_cmd
    else    
        $dckr_cmd "$amx_cmd ${conf_t5_cmd}"
    fi
}

function en_bgp_peer {
    adv_t5_cmd="bgpPeerTable   bgpRmEntIndex=1 bgpPeerLocalAddrType=inetwkAddrTypeIpv4 bgpPeerLocalAddr=$BGP_LOCAL_IP \
              bgpPeerLocalPort=0 bgpPeerRemoteAddrType=inetwkAddrTypeIpv4 bgpPeerRemoteAddr=$BGP_PEER_IP \
              bgpPeerRemotePort=0 bgpPeerLocalAddrScopeId=0  bgpPeerAdminStatus=bgpAdminStatusUp"
    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $adv_t5_cmd
    else    
        $dckr_cmd "$amx_cmd ${adv_t5_cmd}"
    fi
   #docker exec -it CTR2 sh -c "CONFIG_PATH=metaswitch/config/dol_ctr2/ build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test bgp-opeer-creat2e"
}

function get_bgp_peers {
    get_bgp_peer_cmd="bgpPeerTable"
    if [[ $NAPLES == 1 ]]; then
        $amx_get_cmd $get_bgp_peer_cmd
    else    
        $dckr_cmd "$amx_get_cmd ${get_bgp_peer_cmd}"
    fi
   #docker exec -it CTR2 sh -c "CONFIG_PATH=metaswitch/config/dol_ctr2/ build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test bgp-opeer-creat2e"
}

function disable_bgp_peer {
    wdrw_t5_cmd="bgpPeerTable   bgpRmEntIndex=1 bgpPeerLocalAddrType=inetwkAddrTypeIpv4 bgpPeerLocalAddr=$BGP_LOCAL_IP \
              bgpPeerLocalPort=0 bgpPeerRemoteAddrType=inetwkAddrTypeIpv4 bgpPeerRemoteAddr=$BGP_PEER_IP \
              bgpPeerRemotePort=0 bgpPeerLocalAddrScopeId=0  bgpPeerAdminStatus=bgpAdminStatusDown"
    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $wdrw_t5_cmd
    else    
        $dckr_cmd "$amx_cmd ${wdrw_t5_cmd}"
    fi
    #docker exec -it CTR2 sh -c "CONFIG_PATH=metaswitch/config/dol_ctr2/ build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test bgp-opeer-del2"
}

function del_type5 {

    del_t5_cmd="rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=$1 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c0000ee rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=destroy"
    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $del_t5_cmd
    else    
        $dckr_cmd "$amx_cmd ${del_t5_cmd}"
    fi
}

function teardown_type5 {

    del_rtredist_cmd="rtmRedistTable rtmRedistFteIndex=2 rtmRedistEntryId=10 rtmRedistRowStatus=rowDestroy"
    del_limintf_cmd="limL3InterfaceAddressTable \
                                     limEntEntityIndex=1 \
                                     limL3InterfaceAddressIfIndex=1073823745 \
                                     limL3InterfaceAddressIPAddrType=inetwkAddrTypeIpv4 \
                                     limL3InterfaceAddressIPAddress=12.0.0.254 \
                                     limL3InterfaceAddressRowStatus=rowDestroy \
                                     limL3InterfaceAddressPrefixLen=24"
    if [[ $NAPLES == 1 ]]; then
        $amx_cmd $del_rtredist_cmd
        $amx_cmd $del_limintf_cmd
    fi
}

function mark_top_out {
    if [[ $NAPLES == 0 ]]; then
        docker exec -it CTR1 sh -c "pkill top"
        echo "=========> $1" >> ../config/dol_ctr1/route_churn_top_out
        docker exec -dit CTR1 sh -c 'top -b -p `pgrep pdsagent` >> /sw/nic/metaswitch/config/dol_ctr1/route_churn_top_out'
   fi
}

function monitor_naples {
    if [[ $NAPLES == 1 ]]; then
       echo "==============>>>>> ITERATION $1 $2 $(date) <<<<<==============" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
       ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "date; /data/ps_mem.py" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
       echo "------------------------------------------------------------" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
       ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "/nic/bin/pdsctl show system memory" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
       echo "------------------ Route Count --------------------------------" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
       ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "/nic/bin/pdsctl show route | wc -l" >> $MONITOR_LOG_PATH/$MONITOR_LOG_FILE
    fi
}

function copy_logs {
    rem_date=$(ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "date +%Y%m%d_%H%M%S")
    ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "tar -cvzf /data/logs_${rem_date}.tgz /var/log/pensando /obfl" > /dev/null
    scp root@$NAPLES_IP_MONITOR:/data/logs_${rem_date}.tgz $MONITOR_LOG_PATH/
    ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP_MONITOR "rm /data/logs_${rem_date}.tgz" 
}

function cleanup {
    for iter in $(seq 1 $NUM_ROUTES);
    do
       CUR_OFFSET=$((iter * OFFSET))
       printf -v NEWBASE '%#x' "$((BASE + CUR_OFFSET))"
       echo "Del route#$iter prefix $NEWBASE"
       del_type5 $NEWBASE
    done
    teardown_type5
    en_bgp_peer
}

function type5_gen {
    # Setup IRB interafce and RTM redist
    setup_type5
    # Create prefix routes and advertise
    for iter in $(seq 1 $NUM_ROUTES);
    do
       CUR_OFFSET=$((iter * OFFSET))
       printf -v NEWBASE '%#x' "$((BASE + CUR_OFFSET))"
       echo "Add route#$iter prefix $NEWBASE"
       conf_type5 $NEWBASE
    done
}

BASE=2148532224
OFFSET=65536
if [[ $QUERY_BGP == 1 ]]; then
    if [[ $NAPLES == 1 ]]; then
        cat ~/.ssh/id_rsa.pub | ssh root@$NAPLES_IP 'cat >> .ssh/authorized_keys'
        ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP "/nic/bin/pdsctl show bgp"
        exit
    else
        $dckr_cmd "/sw/nic/build/x86_64/apulu/bin/pdsctl show bgp"
        exit
    fi
    exit
fi
open_amx
if [[ $DISABLE_BGP_PEER == 1 ]]; then
    disable_bgp_peer
    exit
fi
if [[ $QUERY_PEER == 1 ]]; then
    if [[ $NAPLES == 1 ]]; then
        ssh -o StrictHostKeyChecking=no -l root $NAPLES_IP "/nic/bin/pdsctl show bgp peers"
    else
        $dckr_cmd "/sw/nic/build/x86_64/apulu/bin/pdsctl show bgp peers"
    fi
    get_bgp_peers
    exit
fi

if [[ $CLEANUP == 1 ]]; then
    cleanup
    exit
fi

if [[ $TYPE5_GEN == 1 ]]; then
    type5_gen
    exit
fi

# Route churn
start_top
monitor_naples 0 "initial"

if [[ $SKIP_INIT == 0 ]]; then
    disable_bgp_peer
    type5_gen
fi

echo "Starting route churn"
echo `date`
monitor_naples 0 "initial"
sleep 5

for i in $(seq 1 $NUM_ITER);
do
    # Churn advertise
    mark_top_out "Starting advertise $i"
    echo "Starting advertise $i `date`"
    en_bgp_peer
    echo "Waiting for advertise $i"
    sleep 75
    monitor_naples $i "advertise"

    # Churn withdraw
    mark_top_out "Starting withdraw $i"
    echo "Starting withdraw $i `date`"
    disable_bgp_peer
    echo "Waiting for withdraw $i"
    sleep 60
    monitor_naples $i "withdraw"
    
    # Copy logs every 5 iterations
    if ((i % 5 == 0 )); then
        copy_logs
    fi
done

echo "Finished route churn"
echo `date`
if [[ $NAPLES == 0 ]]; then
   docker exec -it CTR1 sh -c "pkill top"
fi

echo "Cleaning up"
cleanup
