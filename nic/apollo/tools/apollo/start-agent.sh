#!/bin/sh

# remove logs
rm -f $LOG_DIR/pds-agent.log*

ulimit -c unlimited

export PERSISTENT_LOG_DIR=/obfl/
export LD_LIBRARY_PATH=$LIBRARY_PATH
export IPC_MOCK_MODE=1
exec $NIC_DIR/bin/pdsagent -c hal_hw.json $*
