# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := common_p4plus_rxdma.p4bin common_p4plus_txdma.p4bin
MODULE_TARGET   := libedma.lib
MODULE_PIPELINE := iris gft
MODULE_INCS     := ${MODULE_SRC_DIR}
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc')
include ${MKDEFS}/post.mk
