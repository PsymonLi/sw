#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Multiqueue: Using pktgen threads for sending on multiple CPUs
#  * adding devices to kernel threads
#  * notice the naming scheme for keeping device names unique
#  * nameing scheme: dev@thread_number
#  * flow variation via random UDP source port
#
basedir=`dirname $0`
source ${basedir}/functions.sh
root_check_run_with_sudo "$@"
#
# Required param: -i dev in $DEV
source ${basedir}/parameters.sh

[ -z "$COUNT" ] && COUNT="100000" # Zero means indefinitely

# Base Config
DELAY="0"        # Zero means max speed
[ -z "$CLONE_SKB" ] && CLONE_SKB="0"

# Flow variation random source port between min and max
[ -z "$UDP_SP_MIN" ] && UDP_SP_MIN="1000"
[ -z "$UDP_SP_MAX" ] && UDP_SP_MAX="65000"
[ -z "$UDP_DP_MIN" ] && UDP_DP_MIN="1000"
[ -z "$UDP_DP_MAX" ] && UDP_DP_MAX="65000"

# (example of setting default params in your script)
if [ -z "$DEST_IP" ]; then
    [ -z "$IP6" ] && DEST_IP="2.0.0.33" || DEST_IP="FD00::1"
fi

# General cleanup everything since last run
pg_ctrl "reset"

# Threads are specified with parameter -t value in $THREADS
for ((thread = $F_THREAD; thread <= $L_THREAD; thread++)); do
    # The device name is extended with @name, using thread number to
    # make then unique, but any name will do.
    dev=${DEV}@${thread}

    # Add remove all other devices and add_device $dev to thread
    pg_thread $thread "rem_device_all"
    pg_thread $thread "add_device" $dev

    # Notice config queue to map to cpu (mirrors smp_processor_id())
    # It is beneficial to map IRQ /proc/irq/*/smp_affinity 1:1 to CPU number
    pg_set $dev "flag QUEUE_MAP_CPU"

    # Base config of dev
    pg_set $dev "count $COUNT"
    pg_set $dev "clone_skb $CLONE_SKB"
    pg_set $dev "pkt_size $PKT_SIZE"
    pg_set $dev "delay $DELAY"

    # Flag example disabling timestamping
    pg_set $dev "flag NO_TIMESTAMP"

    # Destination
    pg_set $dev "dst_mac $DST_MAC"
#    pg_set $dev "vlan_id 100" 
    pg_set $dev "dst_min $IP6 $DEST_IP"
    pg_set $dev "dst_max $DEST_IP"

    # Source
    if [ -z "$IP6" ]; then
        echo "Set ipv4 address range"
        pg_set $dev "src_min $SRC_IP"
        pg_set $dev "src_max $SRC_IP"
        pg_set $dev "flag IPSRC_RND"
    else
        echo "Setting ipv6 address 2019::200:1"
        pg_set $dev "src6 2019::200:1"
    fi

    # Setup random UDP port src range
    pg_set $dev "flag UDPSRC_RND"
    pg_set $dev "flag UDPDST_RND"
    pg_set $dev "udp_src_min $UDP_SP_MIN"
    pg_set $dev "udp_src_max $UDP_SP_MAX"
    pg_set $dev "udp_dst_min $UDP_DP_MIN"
    pg_set $dev "udp_dst_max $UDP_DP_MAX"

    if [ -z "$PPS" ]; then
        echo "PPS not set"
    else
        pg_set $dev "ratep $PPS"
    fi
done

# start_run
echo "Running... ctrl^C to stop" >&2
pg_ctrl "start"
echo "Done" >&2

# Print results
for ((thread = $F_THREAD; thread <= $L_THREAD; thread++)); do
    dev=${DEV}@${thread}
    echo "Device: $dev"
    cat /proc/net/pktgen/$dev | grep -A2 "Result:"
done
