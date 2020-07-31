# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := athena_rxdma.p4bin athena_txdma.p4bin hal.memrgns
MODULE_TARGET   := libnicmgr_athena.lib
MODULE_PIPELINE := athena
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                   ${TOPDIR}/nic/sdk/lib/edma \
                   ${TOPDIR}/nic/sdk/platform/devapi
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' \
                   ! -name 'accel*' \
                   ! -name 'nicmgr_init_apollo.cc' \
                   ! -name '*iris*')
MODULE_SOLIBS   := edma
include ${MKDEFS}/post.mk
