#!/bin/bash

# parameter extractor which should be sourceed by pipeline specific scripts.
unset HELP FW_PKG_NAME STAGE_NAME STAGE_TYPE STAGE_STATUS

# file to identify the bootup is regular or upgrade
# this should be set by pipeline specific implemenation in apropriate place
UPGRADE_INIT_MODE_FILE='/update/upgrade_init_mode.txt'

# file to identify the hitless bootup domain
UPGRADE_INIT_DOM_FILE='/update/upgrade_init_domain.txt'
UPGRADE_DOMAIN_A="dom_a"
UPGRADE_DOMAIN_B="dom_b"

function upgmgr_parse_inputs() {
    while getopts 'hf:s:t:r:' c
    do
        case $c in
            h) HELP=1 ;;
            f) FW_PKG_NAME=$OPTARG ;;
            s) STAGE_NAME=$OPTARG ;;
            t) STAGE_TYPE=$OPTARG ;;
            r) STAGE_STATUS=$OPTARG ;;
        esac
    done

    if [[ -z "$FW_PKG_NAME" || -z "$STAGE_NAME" || -z "$STAGE_TYPE" ]];then
        echo "Usage <command> -f <pkg-name> -s <stage-name> -t <PRE>"
        exit 1
    fi

    if [[ "$STAGE_TYPE" == "post" && -z "$STAGE_STATUS" ]];then
        echo "Usage <command> -f <pkg-name> -s <stage-name> -t <POST> -r <status>"
        exit 1
    fi
}

function upgmgr_set_init_mode() {
    mkdir -p "$(dirname "$UPGRADE_INIT_MODE_FILE")"
    echo $1 > $UPGRADE_INIT_MODE_FILE
    sync
}

function upgmgr_clear_init_mode() {
    rm -rf $UPGRADE_INIT_MODE_FILE
}

function upgmgr_init_domain() {
    if [ -f $UPGRADE_INIT_MODE_FILE ]; then
        mode=`cat $UPGRADE_INIT_MODE_FILE`
        if [[ $mode == "hitless" ]]; then
            dom=`cat "${UPGRADE_INIT_DOM_FILE}"`
            if [[ $dom != $UPGRADE_DOMAIN_A && $dom != $UPGRADE_DOMAIN_B ]];then
                echo "Invalid domain id"
                exit 1
            fi
            export UPGRADE_DOMAIN_ID=$dom # TODO for ipc now. remove it later
            echo "$dom"
            return
        fi
    fi
    echo "none"
    return
}

function upgmgr_get_alt_domain() {
    dom=$( upgmgr_init_domain )
    if [[ $dom == $UPGRADE_DOMAIN_B ]];then
        dom=$UPGRADE_DOMAIN_A
    else
        dom=$UPGRADE_DOMAIN_B
    fi
    echo $dom
}

# set the new domain
function upgmgr_set_init_domain() {
    if [ "$#" -ne 1 ];then
        echo "Invalid arguments, pass domain"
        exit 1
    fi
    dom=$1
    if [[ $dom != $UPGRADE_DOMAIN_A  && $dom != $UPGRADE_DOMAIN_B ]];then
        echo "Invalid domain id"
        return 1
    fi
    mkdir -p "$(dirname "$UPGRADE_INIT_DOM_FILE")"
    echo $dom > $UPGRADE_INIT_DOM_FILE
    upgmgr_set_init_mode "hitless"
    export UPGRADE_DOMAIN_ID=$dom # TODO for ipc now. remove it later
    return 0
}

function upgmgr_clear_init_domain() {
    rm -rf $UPGRADE_INIT_DOM_FILE
}
