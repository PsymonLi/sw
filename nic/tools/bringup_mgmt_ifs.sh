#!/bin/bash

int_mnic0_up=0

oob_mnic0_up=0

inb_mnic0_up=0
inb_mnic0_enslaved=0

inb_mnic1_up=0
inb_mnic1_enslaved=0

#setup the bonding device for inband management
#ip link s dev bond0 type bond mode active-backup miimon 100
ip link s dev bond0 type bond mode balance-rr
mkfifo /tmp/bond0_fifo

log() {
    d=`date '+%Y-%m-%d %H:%M:%S'`
    echo "[$d] $1"
}

debug() {
    log "int_mnic0_up=$int_mnic0_up"
    log "oob_mnic0_up=$oob_mnic0_up"
    log "inb_mnic0_up=$inb_mnic0_up"
    log "inb_mnic0_enslaved=$inb_mnic0_enslaved"
    log "inb_mnic1_up=$inb_mnic1_up"
    log "inb_mnic1_enslaved=$inb_mnic1_enslaved"
}

get_inb_mnic0_mac_addr() {
    if [ -d "/sys/class/net/inb_mnic0/bonding_slave/" ]; then
        inb_mnic0_mac_addr=`cat /sys/class/net/inb_mnic0/bonding_slave/perm_hwaddr`
    else
        inb_mnic0_mac_addr=`cat /sys/class/net/inb_mnic0/address`
    fi
}

trap debug 0 1 2 3 6

if [ ! -z $1 ]; then
    affinity_val=$1
    affinity_mask=`printf "%x" $affinity_val`
else

    #spray across all CPUs if affinity_mask is not specified by user
    affinity_val=$(((1<<`grep "processor" /proc/cpuinfo | wc -l`)-1))
    echo "affinity_val: $affinity_val"
    affinity_mask=`printf "%x" $affinity_val`
fi

log "affinity_mask: $affinity_mask"
interfaces=(int_mnic0, oob_mnic0 inb_mnic0 inb_mnic1)

#two interrupts per interface (one for adminq and one for rxq)
if_intr_cnt=$(( 2*${#interfaces[@]} ))

log "Waiting for mgmt interfaces to show up"

#Wait for mnic interfaces to show up
while true
do
    if [ $int_mnic0_up -eq 0 ] && [ -d "/sys/class/net/int_mnic0" ] ; then
        bus=`/nic/bin/pcieutil dev -D 1dd8:1004`
        if [ ! -z "$bus" ]; then
            ipaddr="169.254.$bus.1"
            log "bringing up int_mnic0 $ipaddr"
            ifconfig int_mnic0 $ipaddr netmask 255.255.255.0 up && int_mnic0_up=1
        else
            log "Waiting for ipaddr for int_mnic0"
            ifconfig int_mnic0 up && int_mnic0_up=1
        fi

        sysctl -w net.ipv4.conf.int_mnic0.arp_ignore=1
    fi

    if [ $oob_mnic0_up -eq 0 ] && [ -d "/sys/class/net/oob_mnic0" ] ; then
            log "bringing up oob_mnic0"
            ifconfig oob_mnic0 up && oob_mnic0_up=1
            sysctl -w net.ipv4.conf.oob_mnic0.arp_ignore=1
    fi

    if [ $inb_mnic0_up -eq 0 ] && [ -d "/sys/class/net/inb_mnic0" ] ; then
        if [ $inb_mnic0_enslaved -eq 0 ]; then
            ifconfig inb_mnic0 up
            log "adding inb_mnic0 (inband mgmt0) interface to bond0"
            ifconfig bond0 up && ifenslave bond0 inb_mnic0 && inb_mnic0_enslaved=1 && inb_mnic0_up=1
        fi
    fi

    # We need to set inb_mnic0 mac address as bond0 mac address for consistent mac address for venice
    if [ $inb_mnic0_enslaved -eq 1 ] && [ $inb_mnic1_up -eq 0 ] && [ -d "/sys/class/net/inb_mnic1" ] ; then
        if [ $inb_mnic1_enslaved -eq 0 ]; then
            ifconfig inb_mnic1 up
            log "adding inb_mnic1 (inband mgmt1) interface to bond0"
            get_inb_mnic0_mac_addr
            ifconfig bond0 up && ifenslave bond0 inb_mnic1 && ifconfig bond0 hw ether $inb_mnic0_mac_addr && inb_mnic1_enslaved=1 && inb_mnic1_up=1
        fi
    fi

    sleep 1

    if [ $int_mnic0_up -eq 1 ] && [ $oob_mnic0_up -eq 1 ] && [ $inb_mnic0_up -eq 1 ] && [ $inb_mnic1_up -eq 1 ]; then
        # Create an array for to store irq handlers numbers
        irq_numbers=(`find /proc/irq  -name "*ionic*" | awk -F/ '{ print $4 }'`)
        log "Waiting until $if_intr_cnt irq handlers to show up"
        log "Number of irq handlers: ${#irq_numbers[@]}"

        # Wait until all irq handlers are available before setting affinity
        if [ ${#irq_numbers[@]} -eq "$if_intr_cnt" ]; then
            for irq_num in ${irq_numbers[@]}
            do
                log "Setting irq affinity for ionic irq: $irq_num"
                echo $affinity_mask > /proc/irq/$irq_num/smp_affinity
            done

            get_inb_mnic0_mac_addr
            log "setting inb_mnic0 mac address as bond0 mac address"
            ifconfig bond0 hw ether $inb_mnic0_mac_addr
            ifconfig bond0

            echo "done" > /tmp/bond0_fifo
            break
        fi
    fi

done
