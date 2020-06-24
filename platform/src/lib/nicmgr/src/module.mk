# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := common_p4plus_rxdma.p4bin common_p4plus_txdma.p4bin hal.memrgns
MODULE_TARGET   := libnicmgr.lib
MODULE_PIPELINE := iris gft
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                   ${MODULE_SRC_DIR}/../../edma \
                   ${BLD_PROTOGEN_DIR}/
MODULE_SRCS     := $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' \
                   ! -name 'ftl*' \
                   ! -name '*athena*' \
                   ! -name '*apollo*')
MODULE_SOLIBS   := edma
ifeq ($(ASIC),elba)
MODULE_FLAGS     := -DELBA
endif
include ${MKDEFS}/post.mk
