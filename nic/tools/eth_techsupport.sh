#!/bin/bash
#
# This script gathers techsupport for ethernet devices
#
LIFS=`/nic/bin/halctl show lif | grep -e 'mnic' -e 'host' -e 'swm' | cut -d " " -f 1`

for LIF in $LIFS
do
    QINFO=`/platform/bin/eth_dbgtool qinfo $LIF`
    if [[ $? -eq 0 ]]; then
        /platform/bin/eth_dbgtool dbginfo $LIF
    fi
done
