#!/bin/sh

# sync the disks, and then check if a cpldreset is pending
# if a cpld reboot is pending use cpld reboot else reboot cleanly.
sync
CPLD_UPGRADE=/tmp/cpldreset
if [ -e $CPLD_UPGRADE ]; then
    /nic/bin/cpldapp -reload
fi
watchdog -T 10 /dev/watchdog
reboot -f
