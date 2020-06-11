#! /bin/bash
#set -x

function open_amx {
docker exec -it CTR2 sh -c "/sw/nic/build/x86_64/apulu/bin/pdsctl debug routing open"
}

function conf_type5 {

docker exec -it CTR2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=$1 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c00000f rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=createAndGo rtmStaticRtAdminStat=adminStatusUp rtmStaticRtOverride=true rtmStaticRtAdminDist=150"

}

function adv_type5 {
docker exec -it CTR2 sh -c "CONFIG_PATH=metaswitch/config/dol_ctr2/ build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test bgp-opeer-creat2e"
}

function wdrw_type5 {
docker exec -it CTR2 sh -c "CONFIG_PATH=metaswitch/config/dol_ctr2/ build/x86_64/apulu/capri/bin/pds_ms_uecmp_rr_grpc_test bgp-opeer-del2"
}

function del_type5 {

docker exec -it CTR2 sh -c "python /sw/nic/third-party/metaswitch/code/comn/tools/mibapi/metaswitch/cam/mib.py set localhost rtmStaticRtTable rtmStaticRtFteIndex=2 rtmStaticRtDestAddrType=inetwkAddrTypeIpv4 rtmStaticRtDestAddr=$1 rtmStaticRtDestLen=16 rtmStaticRtNextHopType=inetwkAddrTypeIpv4 rtmStaticRtNextHop=0x0c00000f rtmStaticRtIfIndex=0 rtmStaticRtRowStatus=destroy"

}

function mark_top_out {
docker exec -it CTR1 sh -c "pkill top"
echo "=========> $1" >> ../config/dol_ctr1/route_churn_top_out
docker exec -dit CTR1 sh -c 'top -b -p `pgrep pdsagent` >> /sw/nic/metaswitch/config/dol_ctr1/route_churn_top_out'
}

BASE=2148532224
OFFSET=65536
open_amx
docker exec -dit CTR1 sh -c 'top -b -p `pgrep pdsagent` > /sw/nic/metaswitch/config/dol_ctr1/route_churn_top_out'

for iter in {1..1000}
do
   CUR_OFFSET=$((iter * OFFSET))
   printf -v NEWBASE '%#x' "$((BASE + CUR_OFFSET))"
   echo Add $NEWBASE
   conf_type5 $NEWBASE
done

echo "Starting route churn"
echo `date`
sleep 5

for i in {1..100}
do
mark_top_out "Starting withdraw $i"
echo "Starting withdraw $i `date`"
wdrw_type5
echo "Waiting for withdraw $i"
sleep 25
mark_top_out "Starting advertise $i"
echo "Starting advertise $i `date`"
adv_type5
echo "Waiting for advertise $i"
sleep 35
done

docker exec -it CTR1 sh -c "pkill top"
#for iter in {1..500}
#do
#   CUR_OFFSET=$((iter * OFFSET))
#   printf -v NEWBASE '%#x' "$((BASE + CUR_OFFSET))"
#   echo Del $NEWBASE
#   del_type5 $NEWBASE
#done
