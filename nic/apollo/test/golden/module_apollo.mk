# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = apollo_test.gtest
MODULE_PIPELINE = apollo
MODULE_ARCH     = x86_64
MODULE_SOLIBS   = ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  pal pack_bytes \
                  sdkcapri_asicrw_if sdkasicpd \
                  ${NIC_SDK_SOLIBS} \
                  bm_allocator bitmap \
                  sdkcapri sdkp4 sdkp4utils sdkxcvrdriver
MODULE_LDLIBS   = ${NIC_CAPSIM_LDLIBS} \
                  ${NIC_COMMON_LDLIBS} \
                  ${${PIPELINE}_GTEST_COMMON_LDLIBS}
MODULE_SRCS     = ${MODULE_SRC_DIR}/apollo.cc
include ${MKDEFS}/post.mk
