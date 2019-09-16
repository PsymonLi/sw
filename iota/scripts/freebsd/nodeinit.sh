#! /usr/local/bin/bash
set -e

while [[ "$#" > 0 ]]; do
    case $1 in
        -c|--cleanup) cleanup=1;;
        *) echo "Unknown parameter passed: $1"; exit 1;;
    esac; shift;
done

rm -f /root/.ssh/known_hosts
chown vm:vm /pensando

if [ -n "$cleanup" ]; then
    rm -rf /pensando
    mkdir /pensando
    driver_dir="/naples/drivers-freebsd-eth"
    if [ ! -d "$driver_dir" ]; then
        echo "Cleanup Failed. No driver dir: $driver_dir"
        exit 0
    fi
    cd $driver_dir
    kldunload sys/modules/ionic/ionic.ko 2> /dev/null || rc=$?
    if [ -n "$rc" ] && [ $rc -ne 0 ]; then
        echo "Failed to unload ionic driver. Ignore"
        exit 0
    fi
    echo "Unloaded ionic driver."
else
    cd /naples
    tar xf drivers-freebsd-eth.tar.xz
    cd drivers-freebsd-eth
    env OS_DIR=/usr/src ./build.sh
    kldunload sys/modules/ionic/ionic.ko 2> /dev/null || rc=$?
    kldload sys/modules/ionic/ionic.ko || (dmesg && exit 1)
    sleep 2
    intmgmt=`pciconf -l | grep chip=0x10041dd8 | cut -d'@' -f1 | sed "s/ion/ionic/g"`
    ifconfig $intmgmt 169.254.0.2/24
    ping -c 5 169.254.0.1
fi
