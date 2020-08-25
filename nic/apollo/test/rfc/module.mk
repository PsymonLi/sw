# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = ${PIPELINE}_rte_bitmap_test.gtest
MODULE_PIPELINE = apollo apulu
MODULE_ARCH     = x86_64
MODULE_PREREQS  = dpdk.submake
MODULE_INCS     = ${BLD_OUT_DIR}/dpdk_submake/include
MODULE_FLAGS    = -DRTE_ARCH_64 -DRTE_CACHE_LINE_SIZE=64
include ${MKDEFS}/post.mk
