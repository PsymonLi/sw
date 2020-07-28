# Copyright (C) 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PIPELINE := iris
MODULE_TARGET   := libmctp.lib
MODULE_DEFS     := -DMCTP_HAVE_FILEIO -DMCTP_DEFAULT_ALLOC
MODULE_INCS     := ${MODULE_SRC_DIR} ${MODULE_SRC_DIR}/tests
include ${MKDEFS}/post.mk

