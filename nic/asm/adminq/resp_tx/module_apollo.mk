# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET       = adminq_resp_apollo.asmbin
MODULE_PREREQS      = adminq.p4bin
MODULE_PIPELINE     = apollo
MODULE_INCS         = ${BLD_P4GEN_DIR}/nicmgr_txdma_actions/asm_out \
                      ${BLD_P4GEN_DIR}/nicmgr_txdma_actions/alt_asm_out \
                      ${TOPDIR}/nic/asm/common-p4+/include \
                      ${TOPDIR}/nic/p4/include \
                      ${TOPDIR}/nic/include
MODULE_DEPS         = $(shell find ${MODULE_DIR} -name '*.h')
MODULE_BIN_DIR      = ${BLD_BIN_DIR}/p4pasm_txdma
include ${MKDEFS}/post.mk
