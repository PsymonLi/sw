# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = libsdkcoremgr.so
MODULE_LDLIBS = z
MODULE_SRCS   = $(wildcard ${MODULE_SRC_DIR}/*.cc)
include ${MKDEFS}/post.mk
