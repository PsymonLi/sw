# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET = libhost_mem.so
MODULE_SRCS   = ${MODULE_SRC_DIR}/c_if.cc \
                ${MODULE_SRC_DIR}/host_mem.cc \
                ${TOPDIR}/nic/sdk/lib/bm_allocator/bm_allocator.cc
include ${MKDEFS}/post.mk
