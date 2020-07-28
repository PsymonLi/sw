# Copyright (C) 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_ARCH     := aarch64
MODULE_TARGET   := mctputil.bin
MODULE_PIPELINE := iris
MODULE_SRCS     := ${MODULE_SRC_DIR}/mctputil.cc
MODULE_INCS     := ${MODULE_SRC_DIR} ${TOPDIR}/platform/src/lib/mctp
MODULE_SOLIBS   := mctp
MODULE_LDLIBS   :=
include ${MKDEFS}/post.mk
