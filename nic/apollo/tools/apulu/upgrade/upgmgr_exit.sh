#!/bin/bash
# top level script which will be invoked by upgrade manager before
# the base(hw/sim) environment variable is already set during upgmgr invocation

# BUILD_DIR is defined only on sim/mock mode
if [[ ! -z $BUILD_DIR ]];then
    source $PDSPKG_TOPDIR/sdk/upgrade/core/upgmgr_core_base.sh
    source $PDSPKG_TOPDIR/apollo/tools/apulu/upgrade/upgmgr_base.sh
    export TECH_SUPPORT_DISABLED="1"
else
    source $PDSPKG_TOPDIR/tools/upgmgr_base.sh
fi

valid_response="ok, critical, fail, noresponse"
function usage() {
    echo "Usage: $0 -r <ok|fail|critical|no-response>" 1>&2;
    exit 1;
}

interactive=0

while getopts ":r:i:m:s" o; do
    case "${o}" in
        r)
            response=${OPTARG}
            ;;
        s)
            upgmgr_check_and_update_exit_status
            exit 0;
            ;;
        i)
            interactive=1
            response=${OPTARG}
            ;;
        m)
            mode=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

# disable tech support in respawn case
if [ -e  $RESPAWN_IN_PROGRESS_FILE ];then
    export TECH_SUPPORT_DISABLED="1"
fi

if [ "${response}" != "ok" ];then
    upgmgr_exit $interactive "$response" "$mode"
    if [ -e ${PDSPKG_TOPDIR}/tools/collect_techsupport.sh ]
    then
        echo "Collect techsupport ..."
        ${PDSPKG_TOPDIR}/tools/collect_techsupport.sh
        echo "Techsupport collected ..."
    fi
else
    upgmgr_exit $interactive "$response" "$mode"
fi

echo "$0 processed successfully"
exit 0
