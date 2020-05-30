# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = librunenv.lib
MODULE_FLAGS    = ${NIC_CSR_FLAGS} -O2
include ${MKDEFS}/post.mk
