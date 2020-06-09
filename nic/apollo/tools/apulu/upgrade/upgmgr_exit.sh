 #!/bin/bash
# top level script which will be invoked by upgrade manager before
# the base(hw/sim) environment variable is already set during upgmgr invocation
echo "argument -- $*"

# BUILD_DIR is defined only on sim/mock mode
if [[ ! -z $BUILD_DIR ]];then
    echo " Not generating tech support on sim/mock mode"
    exit 0
fi

source $PDSPKG_TOPDIR/tools/upgmgr_base.sh
echo "Starting commands for $STAGE_NAME"

valid_response="ok, critical, fail, noresponse"
function usage() {
    echo "Usage: $0 -r <ok|fail|critical|no-response>" 1>&2;
    exit 1;
}

while getopts ":r:s" o; do
    case "${o}" in
        r)
            response=${OPTARG}
            ;;
        s)
            upgmgr_check_and_update_upgrade_status
            exit 0;
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
    if [ -e ${PDSPKG_TOPDIR}/tools/collect_techsupport.sh ]
    then
        echo "Collect techsupport ..."
        ${PDSPKG_TOPDIR}/tools/collect_techsupport.sh
        echo "Techsupport collected ..."
    fi
    upgmgr_set_upgrade_status "failed"
else
    upgmgr_set_upgrade_status "success"
fi

echo "$0 processed successfully"
exit 0
