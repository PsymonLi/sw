# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET = libsldirectmap.lib
MODULE_PREREQS= pen_dpdk.submake
MODULE_INCS   = ${BLD_OUT_DIR}/pen_dpdk_submake/include
MODULE_FLAGS = -DRTE_ARCH_64 -DRTE_CACHE_LINE_SIZE=64
MODULE_SOLIBS = rte_indexer
include ${MKDEFS}/post.mk
