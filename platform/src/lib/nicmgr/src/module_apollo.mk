# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := apollo_rxdma.p4bin apollo_txdma.p4bin hal.memrgns
MODULE_TARGET   := libnicmgr_apollo.lib
MODULE_PIPELINE := apollo
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                   ${MODULE_SRC_DIR}/../../edma
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' \
                   ! -name 'ftl*' \
                   ! -name 'accel*' \
                   ! -name '*athena*' \
                   ! -name '*iris*')
MODULE_SOLIBS   := edma_apollo
include ${MKDEFS}/post.mk
