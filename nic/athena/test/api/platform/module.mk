# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = athena_pfmapp.bin
MODULE_PIPELINE = athena
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_SOLIBS   = sdkpal logger sensor
MODULE_FLAGS    = -O3
ifeq ($(ARCH), x86_64)
MODULE_FLAGS  += -march=native
endif
MODULE_DEFS     = -DRTE_FORCE_INTRINSICS
MODULE_LDLIBS   =  dl sknobs Judy ${SDK_THIRDPARTY_CAPRI_LDLIBS}
include ${MKDEFS}/post.mk
