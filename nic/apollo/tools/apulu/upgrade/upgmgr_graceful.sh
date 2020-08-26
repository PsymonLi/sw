#!/bin/bash
# top level script which will be invoked by upgrade manager before
# and after every stage of execution
# the base(hw/sim) environment variable is already set during upgmgr invocation

# BUILD_DIR is defined only on sim/mock mode
if [[ ! -z $BUILD_DIR ]];then
     $PDSPKG_TOPDIR/apollo/tools/$PIPELINE/upgrade/upgmgr_graceful_mock.sh $*
     [[ $? -ne 0 ]] && echo "Upgrade hooks, command execution failed for mock/sim!" && exit 1
     exit 0
fi

source $PDSPKG_TOPDIR/tools/upgmgr_base.sh
upgmgr_parse_inputs $*

echo "Starting commands for $STAGE_NAME"

if [[ $STAGE_NAME = "UPG_STAGE_COMPAT_CHECK" && $STAGE_TYPE == "PRE" ]];then
    upgmgr_clear_upgrade_status $STAGE_NAME
    upgmgr_setup
    upgmgr_pkgcheck "graceful"
    [[ $? -ne 0 ]] && echo "Package check failed!" && exit 1
    upgmgr_clear_respawn_status
    # copy the new image version meta to shared directory.
    upgmgr_cc_version_copy $NEW_IMG_VERSION_PATH "to"
    [[ $? -ne 0 ]] && echo "Version file copy failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_START" && $STAGE_TYPE == "POST" ]]; then
    upgmgr_setup
    # verification of the image is already done
    $SYS_UPDATE_TOOL -p $FW_PATH
    [[ $? -ne 0 ]] && echo "Firmware store failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_PREPARE" && $STAGE_TYPE == "POST" ]]; then
    echo "Skipping"

elif [[ $STAGE_NAME == "UPG_STAGE_PRE_SWITCHOVER" && $STAGE_TYPE == "POST" ]]; then
    upgmgr_backup
    [[ $? -ne 0 ]] && echo "Files backup failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_RESPAWN" && $STAGE_TYPE == "PRE" ]]; then
    reload_drivers
    upgmgr_set_respawn_status
    upgmgr_set_init_mode "graceful"
    # copy the running image version meta to update. this would come as previous
    # image version meta to the respawn(same image upgrade in this case)
    upgmgr_cc_version_copy $RUNNING_IMG_VERSION_PATH "from"

elif [[ $STAGE_NAME == "UPG_STAGE_SWITCHOVER" && $STAGE_TYPE == "PRE" ]]; then
    upgmgr_set_init_mode "graceful"
    # copy the running image version meta to update. this would come as previous
    # image version meta to the new
    upgmgr_cc_version_copy $RUNNING_IMG_VERSION_PATH "from"

elif [[ $STAGE_NAME == "UPG_STAGE_READY" && $STAGE_TYPE == "POST" ]]; then
    # post ready below is no more in use
    upgmgr_clear_init_mode
    upgmgr_set_graceful_status $STAGE_STATUS
    upgmgr_clear_respawn_status

else
    echo "Unknown stage name given"
    exit 1
fi

echo "Commands for $STAGE_NAME processed successfully"
upgmgr_update_upgrade_stage $STAGE_NAME
exit 0
