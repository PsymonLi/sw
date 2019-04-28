# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := common_p4plus_rxdma.p4bin common_p4plus_txdma.p4bin hal.memrgns
MODULE_TARGET   := libnicmgr.so
MODULE_PIPELINE := iris gft
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                   ${BLD_PROTOGEN_DIR}/ \
                   ${BLD_P4GEN_DIR}/common_rxdma_actions/include  \
                   ${BLD_P4GEN_DIR}/common_txdma_actions/include \
                   ${TOPDIR}/nic/sdk/platform/devapi
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' \
                   ! -name '*apollo*')
include ${MKDEFS}/post.mk
