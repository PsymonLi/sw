# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk

MODULE_TARGET   = delphi_example.bin
MODULE_SOLIBS   = delphisdk
MODULE_LDLIBS   = ${NIC_THIRDPARTY_GOOGLE_LDLIBS} rt ev
MODULE_ARLIBS   = delphiexampleproto
ALL_CC_FILES    = $(wildcard ${MODULE_SRC_DIR}/*.cc)
ALL_TEST_FILES  = $(wildcard ${MODULE_SRC_DIR}/*_test.cc)
MODULE_SRCS     = $(filter-out $(ALL_TEST_FILES), $(ALL_CC_FILES))

include ${MKDEFS}/post.mk
