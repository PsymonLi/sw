# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = flowlog_gen.bin
MODULE_PIPELINE = apulu
MODULE_SOLIBS   = operd
MODULE_LDLIBS   = rt
include ${MKDEFS}/post.mk
