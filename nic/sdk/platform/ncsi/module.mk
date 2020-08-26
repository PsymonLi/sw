# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ARCH     := aarch64
MODULE_TARGET   = libncsi.lib
MODULE_INCS     := ${MODULE_SRC_DIR}
MODULE_FLAGS    = ${NIC_CSR_FLAGS} -O2
include ${MKDEFS}/post.mk
