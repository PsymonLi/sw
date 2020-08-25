# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libsdkcapri.lib
MODULE_ASIC     = capri
MODULE_LDLIBS	= ssl crypto
MODULE_DEFS     = ${NIC_CSR_DEFINES}
MODULE_PREREQS  = dpdk.submake
MODULE_INCS     = ${BLD_OUT_DIR}/dpdk_submake/include
MODULE_FLAGS    = -O3 -DRTE_ARCH_64 -DRTE_CACHE_LINE_SIZE=64
include ${MKDEFS}/post.mk
