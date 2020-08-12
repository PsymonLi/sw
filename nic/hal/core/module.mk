# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libcore.lib
MODULE_PIPELINE = iris gft
MODULE_SOLIBS   = device
include ${MKDEFS}/post.mk
