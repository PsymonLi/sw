# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = hal.bin
MODULE_PIPELINE = iris gft
MODULE_SRCS     = ${MODULE_SRC_DIR}/main.cc
MODULE_SOLIBS   = ${NIC_HAL_ALL_SOLIBS} delphisdk
MODULE_LDLIBS   = ${NIC_HAL_ALL_LDLIBS} ev
include ${MKDEFS}/post.mk
