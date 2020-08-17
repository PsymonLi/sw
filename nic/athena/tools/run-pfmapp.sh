#!/bin/sh

CUR_DIR=$( readlink -f $( dirname $0 ) )
source $CUR_DIR/setup_env_hw.sh

# remove logs
rm -f $NON_PERSISTENT_LOG_DIR/pds-pfmapp.log*

ulimit -c unlimited

exec taskset 1 $PDSPKG_TOPDIR/bin/athena_pfmapp $* >$NON_PERSISTENT_LOG_DIR/pfmapp_console.log &
