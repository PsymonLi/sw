#!/bin/bash
# top level script which will invoke this if mock/sim mode set
set -x
source $PDSPKG_TOPDIR/sdk/upgrade/core/upgmgr_core_base.sh
source $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade/upgmgr_base.sh
upgmgr_parse_inputs $*

echo "In mock/sim, starting commands for $STAGE_NAME"

if [[ $STAGE_NAME = "UPG_STAGE_COMPAT_CHECK" && $STAGE_TYPE == "PRE" ]];then
    upgmgr_clear_upgrade_status $STAGE_NAME
    [[ $? -ne 0 ]] && echo "Package check failed!" && exit 1
    # copy the upgrade image version meta to update. for sim tests, both current
    # and new are always same till we support different images during upgrade
    upgmgr_cc_version_copy $CONFIG_PATH/$PIPELINE "to"

elif [[ $STAGE_NAME == "UPG_STAGE_START" && $STAGE_TYPE == "POST" ]]; then
    # copy the running image version meta to update. this would come as previous
    # image version meta to the new
    upgmgr_cc_version_copy $CONFIG_PATH/$PIPELINE "from"

elif [[ $STAGE_NAME == "UPG_STAGE_BACKUP" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PREPARE" && $STAGE_TYPE == "PRE" ]]; then
    # get the next domain to be booted up
    dom=$( upgmgr_get_alt_domain  )
    echo $dom > /tmp/upgrade_start_new_instance.txt

elif [[ $STAGE_NAME == "UPG_STAGE_READY" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PRE_SWITCHOVER" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_CONFIG_REPLAY" && $STAGE_TYPE == "PRE" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_FINISH" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

else
    echo "Unknown input"
    exit 1
fi

echo "Commands for $STAGE_NAME processed successfully"
upgmgr_update_upgrade_stage $STAGE_NAME
exit 0
