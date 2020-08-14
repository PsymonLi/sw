# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = operdctl.bin
MODULE_PIPELINE = iris apollo apulu artemis gft
MODULE_SOLIBS   = operd
ifeq (${PIPELINE},apulu)
MODULE_SOLIBS   += operdproto operdsvc
endif
MODULE_LDLIBS   = ${NIC_THIRDPARTY_GOOGLE_LDLIBS} z rt pthread dl
MODULE_INCS     = ${MODULE_GEN_DIR}
MODULE_ARLIBS   =
ALL_CC_FILES    = $(wildcard ${MODULE_SRC_DIR}/*.cc)
ifeq (${PIPELINE},apulu)
ALL_CC_FILES    += $(wildcard ${MODULE_SRC_DIR}/apulu/*.cc)
else
ALL_CC_FILES    += $(wildcard ${MODULE_SRC_DIR}/stub/*.cc)
endif
ALL_TEST_FILES  = $(wildcard ${MODULE_SRC_DIR}/*_test.cc)
MODULE_SRCS     = $(filter-out $(ALL_TEST_FILES), $(ALL_CC_FILES))
include ${MKDEFS}/post.mk
