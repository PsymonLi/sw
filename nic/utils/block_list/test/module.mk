# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = block_list_test.gtest
MODULE_PIPELINE = iris gft
MODULE_SOLIBS   = block_list trace logger hal_mock list shmmgr haltrace
MODULE_LDLIBS   = rt
MODULE_ARCH     = x86_64
include ${MKDEFS}/post.mk
