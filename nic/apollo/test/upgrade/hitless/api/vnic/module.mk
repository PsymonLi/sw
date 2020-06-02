# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = ${PIPELINE}_vnic_upg_test.gtest
MODULE_PIPELINE = apulu
MODULE_ARCH     = x86_64
MODULE_PREREQS  = metaswitch.submake
MODULE_INCS     = ${BLD_PROTOGEN_DIR} $(TOPDIR)/nic/metaswitch/stubs/hals \
                  $(addprefix $(MS_ROOT)/,$(MS_INCLPATH))
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi pdsapi_impl \
                  pdstest pdstestapiutils pdsproto \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils sdk_asicrw_if sdk${ASIC} \
                  sdkplatformutils sdkxcvrdriver sdkasicpd penmetrics \
                  lpmitree_${PIPELINE} rfc_${PIPELINE} pdsrfc \
                  bm_allocator sdklinkmgr sdklinkmgrcsr memhash sltcam \
                  slhash ${NIC_${PIPELINE}_NICMGR_LIBS} kvstore_lmdb shmmgr \
                  pdsmsmgmt pdsmscommon pdsmshals pdsmsstubs pdsmsmgmtsvc
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_LDLIBS   = ${NIC_COMMON_LDLIBS} ${NIC_CAPSIM_LDLIBS} \
                  ${${PIPELINE}_GTEST_COMMON_LDLIBS} \
                  AAPL edit ncurses lmdb rt $(MS_LD_LIBS)
MODULE_FLAGS    = ${NIC_CSR_FLAGS} $(addprefix -D,$(MS_COMPILATION_SWITCH))
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
MODULE_SRCS     = ${MODULE_SRC_DIR}/main.cc ${MODULE_SRC_DIR}/../helper.cc
include ${MKDEFS}/post.mk
