# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libupgrade.so
MODULE_PIPELINE = iris
MODULE_SOLIBS   = delphisdk upgradeutils sysmgr utils trace
ALL_CC_FILES    = $(wildcard ${MODULE_SRC_DIR}/*.cc ${MODULE_SRC_DIR}/../include/c/*.cc)
ALL_TEST_FILES  = $(wildcard ${MODULE_SRC_DIR}/*_test.cc)
MODULE_SRCS     = $(filter-out $(ALL_TEST_FILES), $(ALL_CC_FILES))
include ${MKDEFS}/post.mk
