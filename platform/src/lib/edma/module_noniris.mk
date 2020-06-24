# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := ${PIPELINE}_rxdma.p4bin ${PIPELINE}_txdma.p4bin
MODULE_TARGET   := libedma_${PIPELINE}.lib
MODULE_PIPELINE := apollo apulu artemis athena
MODULE_INCS     := ${MODULE_SRC_DIR}
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc')
include ${MKDEFS}/post.mk
