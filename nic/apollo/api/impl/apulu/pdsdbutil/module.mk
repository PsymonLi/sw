# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET        = pdsdbutil.bin
MODULE_PIPELINE      = apulu
MODULE_SOLIBS        = kvstore_lmdb logger
MODULE_SRCS          = $(wildcard ${MODULE_SRC_DIR}/*.cc)
MODULE_LDLIBS        = lmdb
include ${MKDEFS}/post.mk

