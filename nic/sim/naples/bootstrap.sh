#!/bin/bash 

#NAT Gateway IP and nat subnet 
NAT_GWIP=10.101.0.1      # IP address of the NAT gateway 
NAT_SUBNET=10.100.0.0/16 # This should match the nat IP pool
LOCAL_SUBNET_VLAN=100

#HOST_SUBNET
HOST_SUBNET=10.30.10.0/24 #ip address of l2seg network
#naples uplink interfaces
INTF1=pen-intf1
INTF2=pen-intf2

#naples host interface
HOST0=ep-4

LOGDIR=/var/run/naples/logs

########################################################################
# Setup intf1 (redirect all the traffic detined to NAT_SUBNET via intf1)
########################################################################
function setup_intf3()
{
    set +x
    while true; do
        sleep 5
        pid=$(docker inspect --format '{{.State.Pid}}' naples-sim)
        nsenter -t $pid -n ip link set $HOST0 netns 1 >& /dev/null
        if [ $? -eq 0 ]; then
            set -x
            start=10
            local_host=`expr $start + $1`
            ip link set up dev $HOST0
            subif=$HOST0.$LOCAL_SUBNET_VLAN

            ip link add link $HOST0 name $subif type vlan id $LOCAL_SUBNET_VLAN
            ip link set up dev $subif

            #set ip/mac address
            ip link set dev $subif address 00:22:0a:00:03:$local_host
            ip addr add dev $subif 10.30.10.$local_host/32

            #static ARP for remote ip
            if [ $1 -eq 1 ]; then
                remote_node=`expr $local_host + 1`
            else 
                remote_node=`expr $local_host - 1`
            fi
            ip neigh add 10.30.10.$remote_node lladdr 00:22:0a:00:03:$remote_node dev $subif
            
            #static route to send to naples
            ip route add $HOST_SUBNET dev $subif
            set +x
        fi
    done
}

#######################################################################
#Setup intf2 (connect it to the other node via linux bridge)
#######################################################################
function setup_intf2()
{
    set +x
    while true; do
        sleep 5
        pid=$(docker inspect --format '{{.State.Pid}}' naples-sim)
        nsenter -t $pid -n ip link set $INTF2 netns 1  >& /dev/null
        if [ $? -eq 0 ]; then
            set -x
            brctl addif br0 $INTF2
            ip link set up dev $INTF2
            set +x
        fi
    done
}

function setup_intf1()
{
    set +x
    while true; do
        sleep 5
        pid=$(docker inspect --format '{{.State.Pid}}' naples-sim)
        nsenter -t $pid -n ip link set $INTF1 netns 1 >& /dev/null
        if [ $? -eq 0 ]; then
            set -x
            ip link set up dev $INTF1
            subif=$INTF1.$LOCAL_SUBNET_VLAN

            ip link add link $INTF1 name $subif type vlan id $LOCAL_SUBNET_VLAN
            ip link set up dev $subif

            #set ip/mac address
            ip link set dev $subif address 00:22:0a:00:02:01
            ip addr add dev $subif 10.0.2.1/32

            #interface route for NAT gateway IP
            ip route add $NAT_GWIP/32 dev $subif
            
            #static ARP for NAT gateway IP
            ip neigh add 10.101.0.1 lladdr 00:22:0a:65:00:01 dev $subif
            
            #static route for NAT subnet
            ip route add $NAT_SUBNET via $NAT_GWIP
            set +x
        fi
    done
}

# redirect output to bootstrap.log
mkdir -p $LOGDIR
exec > $LOGDIR/bootstrap.log
exec 2>&1
set -x

#setup the bridge
brctl addbr br0
brctl stp br0 off
brctl addif br0 eth1
ip link set up dev br0
ip link set up dev eth1
echo "$1"
# setup the uplinks in background
$(setup_intf1 $1 >& $LOGDIR/bootstrap-intf1.log) &
$(setup_intf2 $1 >& $LOGDIR/bootstrap-intf2.log) &
$(setup_intf3 $1 >& $LOGDIR/bootstrap-intf3.log) &

