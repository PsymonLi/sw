# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = librfc_apollo.lib
MODULE_PIPELINE = apollo
MODULE_PREREQS  = dpdk.submake
MODULE_INCS     = ${BLD_OUT_DIR}/dpdk_submake/include
MODULE_FLAGS    = -DRTE_ARCH_64 -DRTE_CACHE_LINE_SIZE=64
include ${MKDEFS}/post.mk
