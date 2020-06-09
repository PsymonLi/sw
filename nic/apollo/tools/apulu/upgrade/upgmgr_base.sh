#!/bin/bash

unset FW_PATH RUNNING_IMAGE FW_UPDATE_TOOL SYS_UPDATE_TOOL

source $PDSPKG_TOPDIR/tools/upgmgr_core_base.sh

UPGRADE_STATUS_FILE='/update/pds_upg_status.txt'
RESPAWN_IN_PROGRESS_FILE='/tmp/.respawn_in_progress'
PENVISORCTL='penvisorctl'

function upgmgr_set_upgrade_status() {
    time_stamp="$(date +"%Y-%m-%d %H:%M:%S")"
    status=$1
    echo "$time_stamp::$status" > $UPGRADE_STATUS_FILE
}

function upgmgr_check_and_update_upgrade_status() {
    grep -ie "in-progress\|failed\|success" $UPGRADE_STATUS_FILE
    [[ $? -ne 0 ]] && upgmgr_set_upgrade_status "failed"
}

function upgmgr_clear_upgrade_status() {
    rm -f $UPGRADE_STATUS_FILE
}

function upgmgr_setup() {
    FW_UPDATE_TOOL=$PDSPKG_TOPDIR/tools/fwupdate
    SYS_UPDATE_TOOL=$PDSPKG_TOPDIR/tools/sysupdate.sh

    if [[ -f "$FW_PKG_NAME" ]];then
        FW_PATH=$FW_PKG_NAME
    elif [[ -f "/update/$FW_PKG_NAME" ]];then
        FW_PATH="/update/$FW_PKG_NAME"
    else
        echo "$FW_PKG_NAME could not locate"
        exit 1
    fi

    if [[ ! -x "$FW_UPDATE_TOOL" ]];then
        echo "$FW_UPDATE_TOOL could not locate"
        exit 1
    fi

    RUNNING_IMAGE=`$FW_UPDATE_TOOL -r`
}

function upgmgr_pkgcheck() {
    local CC_CHECK_TOOL=${PDSPKG_TOPDIR}/tools/upgmgr_cc_meta.py

    # verify the image
    $FW_UPDATE_TOOL -p $FW_PATH -v
    [[ $? -ne 0 ]] && echo "FW verfication failed!" && exit 1

    # extract meta files
    local META_NEW='/tmp/meta_cc_new.json'
    local META_RUNNING='/tmp/meta_cc_running.json'
    rm -rf $META_NEW $META_RUNNING

    # extract meta file from new firmware
    tar -xf $FW_PATH MANIFEST -O >  $META_NEW
    [[ $? -ne 0 ]] && echo "Meta extraction from new fw failed!" && exit 1

    # extract meta file from running firmware
    $FW_UPDATE_TOOL -l > $META_RUNNING
    [[ $? -ne 0 ]] && echo "Meta extraction from running fw failed!" && exit 1

    # compare the extracted files
    python $CC_CHECK_TOOL $META_RUNNING $RUNNING_IMAGE $META_NEW
    if [[ $? -ne 0 ]];then
        rm -rf $META_NEW $META_RUNNING
        echo "Meta comparison failed!"
        exit 1
    fi
    rm -rf $META_NEW $META_RUNNING
    return 0
}

function upgmgr_backup() {
    local files_must="/update/pcieport_upgdata /update/pciemgr_upgdata "
    files_must+="/update/pds_nicmgr_upgdata "
    local files_optional="/update/pciemgr_upgrollback "
    for e in $files_must; do
        if [[ ! -f $e ]]; then
            echo "File $e not present"
            return 1
        fi
    done
    return 0
}

function upgmgr_restore() {
    echo "Restore, nothing to do, skipping"
    return 0
}

function upgmgr_hitless_backup_files_check() {
    return 0 # TODO need to add domain here. dom_b to dom_a will have extensions

    local files_must="/update/pcieport_upgdata /update/pciemgr_upgdata "
    files_must+="/update/pds_nicmgr_upgdata /update/pds_agent_upgdata "
    local files_optional="/update/pciemgr_upgrollback "
    # first make sure all the mandatory files are present
    for e in $files_must; do
        if [[ ! -f $e ]]; then
            echo "File $e not present"
            return 1
        fi
    done
    return 0
}

function upgmgr_hitless_restore_files_check() {
    ehco "Nothing to to"
    return 0
}

function reload_drivers() {
    echo "Reloading mnic drivers"
    rmmod mnet mnet_uio_pdrv_genirq ionic_mnic

    # load kernel modules for mnics
    insmod $PDSPKG_TOPDIR/bin/ionic_mnic.ko &> $NON_PERSISTENT_LOG_DIR/ionic_mnic_load.log
    [[ $? -ne 0 ]] && echo "Aborting reload, failed to load mnic driver!" && exit 1

    insmod $PDSPKG_TOPDIR/bin/mnet_uio_pdrv_genirq.ko &> $NON_PERSISTENT_LOG_DIR/mnet_uio_pdrv_genirq_load.log
    [[ $? -ne 0 ]] && echo "Aborting reload, failed to load mnet_uio_pdrv_genirq driver!" && exit 1

    insmod $PDSPKG_TOPDIR/bin/mnet.ko &> $NON_PERSISTENT_LOG_DIR/mnet_load.log
    [[ $? -ne 0 ]] && echo "Aborting reload, failed to load mnet driver!" && exit 1
    return 0
}

function copy_img_to_alt_partition() {
    tool=$1
    running_image=$2
    if [[ $running_image == "mainfwa" ]];then
        img="mainfwb"
    else
        img="mainfwa"
    fi
    $tool -i $img -p /update/naples_fw.tar
    return $?
}

function upgmgr_clear_respawn_status() {
    rm -f $RESPAWN_IN_PROGRESS_FILE
}

function upgmgr_set_respawn_status() {
    touch $RESPAWN_IN_PROGRESS_FILE
}

function upgmgr_graceful_success() {
    if [ -e  $RESPAWN_IN_PROGRESS_FILE ];then
        upgmgr_set_upgrade_status "failed"
        $PDSPKG_TOPDIR/tools/bringup_mgmt_ifs.sh &> $NON_PERSISTENT_LOG_DIR/mgmt_if.log
    else
        upgmgr_set_upgrade_status "success"
    fi
    return 0
}
