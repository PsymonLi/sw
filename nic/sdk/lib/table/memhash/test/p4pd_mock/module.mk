# {C} Copyright 2019 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = libmemhashp4pd_mock.so
MODULE_ARCH     = x86_64
MODULE_FLAGS    = -fno-permissive
MODULE_FLAGS    = -O3
include ${MKDEFS}/post.mk
