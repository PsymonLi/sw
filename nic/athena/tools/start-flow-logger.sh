#!/bin/sh

CUR_DIR=$( readlink -f $( dirname $0 ) )
source $CUR_DIR/setup_env_hw.sh
#export IPC_MOCK_MODE=1

# remove logs
rm -f $NON_PERSISTENT_LOG_DIR/pds-flow-logger.log*

ulimit -c unlimited

export PERSISTENT_LOG_DIR=/obfl/

exec taskset 1 $PDSPKG_TOPDIR/bin/athena_flow_logger -d /data -t 60 $* >$NON_PERSISTENT_LOG_DIR/flow_logger_console.log &
