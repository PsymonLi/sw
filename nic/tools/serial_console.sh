#!/bin/sh

CONFIG=/sysconfig/config0
SYSTEM_CONFIG_FILE=${CONFIG}/system-config.json

set_config() {
    if [ ! -f $SYSTEM_CONFIG_FILE ]; then
        jq -n "{ \"console\": \"$1\" }" > $SYSTEM_CONFIG_FILE
    else
        # jq can't edit file in place, write to temp file and move it over. 
        jq ". + { \"console\": \"$1\" }" $SYSTEM_CONFIG_FILE > \
            ${SYSTEM_CONFIG_FILE}_console
        mv ${SYSTEM_CONFIG_FILE}_console $SYSTEM_CONFIG_FILE
    fi

    CONSOLE_PID=$(pidof console)
    if [ ! -z "${CONSOLE_PID}" ]; then
        kill -SIGTERM ${CONSOLE_PID}
    fi

    sync
}


case "$1" in
    enable)
        set_config enabled
        ;;
    disable)
        set_config disabled
        echo "Please log out from the console"
        ;;
    *)
        echo "Usage: $0 {enable|disable}"
        exit 1
esac

exit $?
