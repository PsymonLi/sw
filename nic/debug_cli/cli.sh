#!/bin/bash
SW_DIR=../..
NIC_DIR=$SW_DIR/nic

if [ $# -ne 1 ]
then
    echo "Usage: $0 <iris/gft>"
    exit 1
fi

# dependent modules for python
export PYTHONPATH=$PYTHONPATH:$NIC_DIR/gen/proto/hal:$NIC_DIR/gen/$1/cli:$NIC_DIR/gen/common_txdma_actions/cli:$NIC_DIR/gen/common_rxdma_actions/cli:$NIC_DIR/gen/common/cli:.

# dependent shared libs
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$NIC_DIR/gen/x86_64/lib:.

# start the debug CLI prompt
if [ "$1" = "iris" ] || [ "$1" = "" ]; then
    python3 cli/debug_cli_iris.py repl
elif [ "$1" = "gft" ]; then
    python3 cli/debug_cli_gft.py repl
else
    echo "Unknown pipeline $1"
fi
