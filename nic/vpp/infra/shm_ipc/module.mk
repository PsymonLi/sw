# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET    = libshmipc.lib
MODULE_PIPELINE  = apulu apollo artemis
MODULE_SRCS      = $(wildcard ${MODULE_DIR}/*.cc)
MODULE_SOLIBS	 = shmmgr
MODULE_LDLIBS    = rt
include ${MKDEFS}/post.mk
