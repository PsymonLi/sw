# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET       = apollo_p4.asmbin
MODULE_PREREQS      = apollo_p4.p4bin
MODULE_PIPELINE     = apollo
MODULE_INCS         = ${BLD_P4GEN_DIR}/apollo/asm_out
MODULE_INCS        += ${BLD_P4GEN_DIR}/apollo/alt_asm_out
MODULE_DEPS         = $(shell find ${MODULE_DIR} -name '*.h')
MODULE_BIN_DIR      = ${BLD_BIN_DIR}/p4asm
include ${MKDEFS}/post.mk
