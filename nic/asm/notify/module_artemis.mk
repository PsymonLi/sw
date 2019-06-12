# {C} Copyright 2019 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET       = notify_artemis.asmbin
MODULE_PREREQS      = notify.p4bin
MODULE_PIPELINE     = artemis
MODULE_INCS         = ${BLD_P4GEN_DIR}/notify_txdma_actions/asm_out \
                      ${BLD_P4GEN_DIR}/notify_txdma_actions/alt_asm_out \
                      ${TOPDIR}/nic/asm/common-p4+/include \
                      ${TOPDIR}/nic/p4/include \
                      ${TOPDIR}/nic/include
MODULE_DEPS         = $(shell find ${MODULE_DIR} -name '*.h')
MODULE_BIN_DIR      = ${BLD_BIN_DIR}/p4pasm_txdma
include ${MKDEFS}/post.mk
