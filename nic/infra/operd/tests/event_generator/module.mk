# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = event_gen.bin
MODULE_PIPELINE = apulu
MODULE_SOLIBS   = operd operd_event_defs operd_event 
MODULE_LDLIBS   = rt
include ${MKDEFS}/post.mk
