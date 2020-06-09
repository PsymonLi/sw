# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ASIC     := elba
MODULE_TARGET   := axitrace.bin
MODULE_SOLIBS   := pal sdkpal logger
MODULE_LDLIBS   := dl
#MODULE_ARCH     := aarch64
include ${MKDEFS}/post.mk
