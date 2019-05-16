# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = fsm_test.gtest
MODULE_PIPELINE = gft iris
MODULE_SOLIBS   = fsm trace logger thread hal_mock \
                  list twheel slab \
                  shmmgr haltestutils haltrace
MODULE_LDLIBS   = rt
MODULE_ARCH     = x86_64
include ${MKDEFS}/post.mk
