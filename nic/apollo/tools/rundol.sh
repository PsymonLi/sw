#! /bin/bash

CUR_DIR=$( readlink -f $( dirname $0 ) )
source $CUR_DIR/setup_dol.sh $*

if [ $DRYRUN == 0 ]; then
    start_model
    start_processes
    start_upgrade_manager
    check_health
fi

# start DOL now
$DOLDIR/main.py $CMDARGS 2>&1 | tee dol.log
status=${PIPESTATUS[0]}

# end of script
exit $status
