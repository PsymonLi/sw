# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

ifeq "${P4VER}" "P4_16"
include ${MKDEFS}/pre.mk
MODULE_TARGET   = athena_l2_flow_cache_scale_test.gtest
MODULE_PIPELINE = athena
MODULE_ARCH     = x86_64
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi_athena pdsapi_athena_impl \
                  pdstest_athena pdstestapiutils_athena \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils sdk_asicrw_if sdk${ASIC} penmetrics \
                  sdkplatformutils sdkxcvrdriver sdkasicpd kvstore_lmdb \
                  bm_allocator sdklinkmgr sdklinkmgrcsr memhash sltcam \
                  slhash ${NIC_${PIPELINE}_NICMGR_LIBS} ${NIC_FTL_LIBS}
MODULE_LDLIBS   = ${NIC_COMMON_LDLIBS} ${NIC_CAPSIM_LDLIBS} \
                  ${${PIPELINE}_GTEST_COMMON_LDLIBS} \
                  AAPL edit ncurses lmdb
MODULE_FLAGS    = ${NIC_CSR_FLAGS}
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
MODULE_INCS     = ${MODULE_SRC_DIR}/../ftl_p4pd_mock
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  ${MODULE_SRC_DIR}/../ftl_p4pd_mock/ftl_p4pd_mock.cc)
include ${MKDEFS}/post.mk
endif
