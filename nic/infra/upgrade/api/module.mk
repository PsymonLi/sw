# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libpdsupgapi.lib
MODULE_PIPELINE = apulu athena
ifeq ($(findstring $PIPELINE, apulu athena), "")
MODULE_SRCS += $(wildcard ${MODULE_SRC_DIR}/impl/stub/*.cc)
else
MODULE_SRCS += $(wildcard ${MODULE_SRC_DIR}/impl/${PIPELINE}/*.cc)
endif
include ${MKDEFS}/post.mk
