# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := apulu_rxdma.p4bin apulu_txdma.p4bin hal.memrgns
MODULE_TARGET   := libnicmgr_apulu.lib
MODULE_PIPELINE := apulu
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                   ${TOPDIR}/nic/sdk/lib/edma
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' \
                   ! -name 'ftl*' \
                   ! -name 'accel*' \
                   ! -name '*athena*' \
                   ! -name '*iris*')
MODULE_SOLIBS   := edma
include ${MKDEFS}/post.mk
