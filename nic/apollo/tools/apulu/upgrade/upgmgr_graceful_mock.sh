#!/bin/bash
# top level script which will invoke this if mock/sim mode set

source $PDSPKG_TOPDIR/sdk/upgrade/core/upgmgr_core_base.sh
source $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade/upgmgr_base.sh
upgmgr_parse_inputs $*

echo "In mock/sim, starting commands for $STAGE_NAME"

if [[ $STAGE_NAME = "UPG_STAGE_COMPAT_CHECK" && $STAGE_TYPE == "PRE" ]];then
    upgmgr_clear_upgrade_status $STAGE_NAME

elif [[ $STAGE_NAME == "UPG_STAGE_START" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PREPARE" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PRE_SWITCHOVER" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_SWITCHOVER" && $STAGE_TYPE == "PRE" ]]; then
    upgmgr_set_init_mode "graceful"

elif [[ $STAGE_NAME == "UPG_STAGE_READY" && $STAGE_TYPE == "POST" ]]; then
    upgmgr_clear_init_mode
    upgmgr_set_graceful_status $STAGE_STATUS

else
    echo "Unknown input"
    exit 1
fi

echo "Commands for $STAGE_NAME processed successfully"
upgmgr_update_upgrade_stage $STAGE_NAME
exit 0
