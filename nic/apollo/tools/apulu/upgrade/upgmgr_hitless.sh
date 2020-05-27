#!/bin/bash
# top level script which will be invoked by upgrade manager before
# and after every stage of execution
# the base(hw/sim) environment variable is already set during upgmgr invocation
echo "argument -- $*"

# BUILD_DIR is defined only on sim/mock mode
if [[ ! -z $BUILD_DIR ]];then
     $PDSPKG_TOPDIR/apollo/tools/$PIPELINE/upgrade/upgmgr_hitless_mock.sh $*
     [[ $? -ne 0 ]] && echo "Upgrade hooks, command execution failed for mock/sim!" && exit 1
     exit 0
fi

source $PDSPKG_TOPDIR/tools/upgmgr_base.sh
upgmgr_parse_inputs $*

echo "Starting commands for $STAGE_NAME"

if [[ $STAGE_NAME = "UPG_STAGE_COMPAT_CHECK" && $STAGE_TYPE == "PRE" ]];then
    upgmgr_set_upgrade_status  "in-progress"
    upgmgr_setup
    upgmgr_pkgcheck
    [[ $? -ne 0 ]] && echo "Package check failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_START" && $STAGE_TYPE == "POST" ]]; then
    upgmgr_setup
    # verification of the image is already done
    # $SYS_UPDATE_TOOL -p $FW_PATH check with Stavros, whether we can do sysupdate
    copy_img_to_alt_partition $FW_UPDATE_TOOL $RUNNING_IMAGE
    [[ $? -ne 0 ]] && echo "Firmware store failed!" && exit 1

elif [[ $STAGE_NAME == "UPG_STAGE_BACKUP" && $STAGE_TYPE == "POST" ]]; then
    upgmgr_hitless_backup_files_check

elif [[ $STAGE_NAME == "UPG_STAGE_PREPARE" && $STAGE_TYPE == "PRE" ]]; then
    $PENVISORCTL load

else
    echo "Unknown stage name given"
    exit 1
fi
echo "Commands for $STAGE_NAME processed successfully"
exit 0
