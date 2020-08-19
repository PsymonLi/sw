#!/bin/sh

counter=300
oob0_up=0
dsc0_up=0
dsc1_up=0
ctrl0_up=0
int_mnic0_up=0

log() {
    d=`date '+%Y-%m-%d %H:%M:%S'`
    echo "[$d] $1"
}

while [ $counter -gt 0 ]
do
    if [ -d "/sys/class/net/oob_mnic0" ] && [ $oob0_up -eq 0 ] ; then
        ethtool -K oob_mnic0 rx off tx off
        ifconfig oob_mnic0 up
        irq_number=`find /proc/irq  -name *oob_mnic0* | awk -F/ '{ print $4 }'`
        if [[ ! -z $irq_number ]]; then
            echo d > /proc/irq/$irq_number/smp_affinity
            oob0_up=1
        fi
        echo "oob interface is up"
        dhclient oob_mnic0 > /dev/null 2>&1 &
    fi

    if [ -d "/sys/class/net/dsc0" ] && [ $dsc0_up -eq 0 ] ; then
        ethtool -K dsc0 rx off tx off
        ifconfig dsc0 up
        irq_number=`find /proc/irq  -name *dsc0* | awk -F/ '{ print $4 }'`
        if [[ ! -z $irq_number ]]; then
            echo 3 > /proc/irq/$irq_number/smp_affinity
            dsc0_up=1
        fi
        echo "dsc0 interface is up"
        iptables -t mangle -A POSTROUTING -o dsc0 -p tcp -j DSCP --set-dscp-class CS6
    fi

    if [ -d "/sys/class/net/dsc1" ] && [ $dsc1_up -eq 0 ] ; then
        ethtool -K dsc1 rx off tx off
        ifconfig dsc1 up && dsc1_up=1
        irq_number=`find /proc/irq  -name *dsc1* | awk -F/ '{ print $4 }'`
        if [[ ! -z $irq_number ]]; then
            echo 3 > /proc/irq/$irq_number/smp_affinity
            dsc1_up=1
        fi
        echo "dsc1 interface is up"
        iptables -t mangle -A POSTROUTING -o dsc1 -p tcp -j DSCP --set-dscp-class CS6
    fi

    if [ -d "/sys/class/net/ctrl0" ] && [ $ctrl0_up -eq 0 ] ; then
        ethtool -K ctrl0 rx off tx off
        ifconfig ctrl0 up && ctrl0_up=1
        irq_number=`find /proc/irq  -name *ctrl0* | awk -F/ '{ print $4 }'`
        if [[ ! -z $irq_number ]]; then
            echo 3 > /proc/irq/$irq_number/smp_affinity
            ctrl0_up=1
        fi
        echo "ctrl0 interface is up"
        ip netns add vcn
        ip link set ctrl0 netns vcn
    fi

    if [ $oob0_up -eq 1 ] && [ $dsc0_up -eq 1 ] && [ $dsc1_up -eq 1 ] && [ $ctrl0_up -eq 1 ] ; then
        break
    else
        echo "Waiting for mgmt interfaces to be created ..."
        sleep 1
        counter=$(( $counter - 1 ))
    fi
done

if [ -d "/sys/class/net/int_mnic0" ] ; then
    sysctl -w net.ipv4.conf.int_mnic0.arp_ignore=1
    counter=100
    while [ $counter -gt 0 ]
    do
        bus=`/nic/bin/pcieutil dev -D 1dd8:1004`
        if [ ! -z "$bus" ]; then
            ipaddr="169.254.$bus.1"
            log "Bringing up int_mnic0 $ipaddr"
            ifconfig int_mnic0 $ipaddr netmask 255.255.255.0 up && int_mnic0_up=1
            echo "int_mnic0 interface is up"
            break
        else
            log "Waiting to configure ipaddr for int_mnic0"
            ifconfig int_mnic0 up && int_mnic0_up=1
        fi
    done
fi

echo ""
if [ $oob0_up -eq 1 ] && [ $dsc0_up -eq 1 ] && [ $dsc1_up -eq 1 ] && [ $ctrl0_up -eq 1 ]; then
    echo "All internal interfaces are brought up"
else
    echo "All internal interfaces didn't show up for 5 minutes !!"
fi
