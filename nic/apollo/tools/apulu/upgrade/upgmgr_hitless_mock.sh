#!/bin/bash
# top level script which will invoke this if mock/sim mode set
set -x
source $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade/upgmgr_base.sh
upgmgr_parse_inputs $*

echo "In mock/sim, starting commands for $STAGE_NAME"

if [[ $STAGE_NAME = "UPG_STAGE_COMPAT_CHECK" && $STAGE_TYPE == "PRE" ]];then
    upgmgr_set_upgrade_status  "in-progress"
    [[ $? -ne 0 ]] && echo "Package check failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_START" && $STAGE_TYPE == "POST" ]]; then
    echo "start, skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_BACKUP" && $STAGE_TYPE == "POST" ]]; then
    echo "start, skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PREPARE" && $STAGE_TYPE == "PRE" ]]; then
    # get the next domain to be booted up
    dom=$( upgmgr_get_alt_domain  )
    echo $dom > /tmp/start_new
elif [[ $STAGE_NAME == "UPG_STAGE_PRE_SWITCHOVER" && $STAGE_TYPE == "POST" ]]; then
    echo "Nothing to do"
elif [[ $STAGE_NAME == "UPG_STAGE_CONFIG_REPLAY" && $STAGE_TYPE == "PRE" ]]; then
    touch /tmp/config_replay_ready
    while [ ! -e /tmp/config_replay_done ]; do
        sleep 1
    done
else
    echo "unknown input"
    exit 1
fi

echo "Commands for $STAGE_NAME processed successfully"
exit 0
