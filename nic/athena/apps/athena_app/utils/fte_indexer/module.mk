# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET = libfte_indexer.lib
MODULE_PREREQS= dpdk.submake
MODULE_INCS   = ${BLD_OUT_DIR}/dpdk_submake/include
MODULE_FLAGS  = -O3 -DRTE_CACHE_LINE_SIZE=64 -DRTE_ARCH_64
MODULE_SOLIBS = utils
include ${MKDEFS}/post.mk
