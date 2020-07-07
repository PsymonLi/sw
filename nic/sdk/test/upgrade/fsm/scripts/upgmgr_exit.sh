#!/bin/bash
# no need to generate techsupport for fsm test
echo "argument -- $*"

function usage() {
    echo "Usage: $0 -r <ok|fail|critical|no-response>" 1>&2;
    exit 1;
}

while getopts ":r:s" o; do
    case "${o}" in
        r)
            response=${OPTARG}
            echo $response > /tmp/upgmgr_fsm_test/upg_status.txt
            ;;
        s)
            exit 0;
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

echo "$0 processed successfully"
exit 0
