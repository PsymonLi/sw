# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = rdma_common.asmbin
MODULE_PREREQS  = rdma.p4bin common_p4plus_rxdma.p4bin common_p4plus_txdma.p4bin
MODULE_PIPELINE = iris gft
MODULE_INCS     = ${BLD_P4GEN_DIR}/rdma_rxdma/asm_out \
                  ${BLD_P4GEN_DIR}/ \
                  ${MODULE_DIR}/../common/include \
                  ${MODULE_DIR}/include \
                  ${MODULE_DIR}/../req_rx/include \
                  ${MODULE_DIR}/../resp_rx/include \
                  ${MODULE_DIR}/../req_tx/include \
                  ${MODULE_DIR}/../resp_tx/include \
                  ${MODULE_DIR}/../aq_tx/include \
                  ${MODULE_DIR}/../aq_rx/include \
                  ${TOPDIR}/nic/asm/common-p4+/include \
                  ${TOPDIR}/nic/p4/include \
                  ${TOPDIR}/nic/include \
                  ${TOPDIR}/nic
MODULE_DEPS     = $(shell find ${MODULE_DIR} -name '*.h')
MODULE_BIN_DIR  = ${BLD_BIN_DIR}/p4pasm
include ${MKDEFS}/post.mk
