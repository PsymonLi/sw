# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ARCH     := x86_64
MODULE_TARGET   := devc_vfio.bin
MODULE_CFLAGS   := -g -O0 -ggdb3
MODULE_INCS     := ${TOPDIR}/platform/drivers/common/
include ${MKDEFS}/post.mk
