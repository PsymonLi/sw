# Copyright (C) 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   := mctpd.bin
MODULE_ARCH     := aarch64
MODULE_PIPELINE := iris
MODULE_SRCS     := ${MODULE_SRC_DIR}/mctpd.cc
MODULE_INCS     := ${MODULE_SRC_DIR} ${TOPDIR}/platform/src/lib/mctp
MODULE_SOLIBS   := mctp operd catalog utils pal sdkfru
MODULE_LDLIBS   := rt ev pthread dl 
include ${MKDEFS}/post.mk
