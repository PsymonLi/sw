# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = port_test.gtest
MODULE_PIPELINE = iris gft
MODULE_SRCS   = ${MODULE_SRC_DIR}/port_test.cc
MODULE_SOLIBS = pal sdkfru linkmgr_test_utils bm_allocator \
                hal_lib agent_api linkmgr_src block_list \
                list shmmgr slab ht timerfd periodic halproto \
                linkmgr_src sdklinkmgr haltrace \
                linkmgr_libsrc sdklinkmgrcsr  \
                haldelphiutils \
                logger thread twheel sdkpal catalog utils trace
MODULE_LDLIBS = ${NIC_HAL_ALL_LDLIBS}
MODULE_FLAGS  = -pthread -rdynamic
MODULE_ARCH   = x86_64
include ${MKDEFS}/post.mk
