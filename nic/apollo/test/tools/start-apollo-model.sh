#!/bin/bash -e
NIC_DIR=/sw/nic
export ZMQ_SOC_DIR=${NIC_DIR}
${NIC_DIR}/tools/merge_model_debug.py --p4 apollo --rxdma apollo_rxdma --txdma apollo_txdma
$GDB $NIC_DIR/build/x86_64/apollo/bin/cap_model \
    +PLOG_MAX_QUIT_COUNT=0 +plog=info \
    +model_debug=$NIC_DIR/build/x86_64/apollo/gen/p4gen//apollo/dbg_out/combined_model_debug.json 2>&1 > $NIC_DIR/model.log
