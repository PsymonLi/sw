# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = flow_test.gtest
MODULE_PIPELINE = apollo artemis apulu
MODULE_ARCH     = x86_64
MODULE_PREREQS  = metaswitch.submake
MODULE_INCS     = ${BLD_PROTOGEN_DIR} $(TOPDIR)/nic/metaswitch/stubs/hals \
                  $(addprefix $(MS_ROOT)/,$(MS_INCLPATH))
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi pdsapi_impl pdstest \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils sdk_asicrw_if sdk${ASIC} \
                  sdkplatformutils sdkxcvrdriver sdkasicpd kvstore_lmdb \
                  lpmitree_${PIPELINE}  rfc_${PIPELINE} pdsrfc penmetrics \
                  bm_allocator sdklinkmgr sdklinkmgrcsr ${NIC_FTL_LIBS} utils \
                  sltcam slhash ${NIC_${PIPELINE}_NICMGR_LIBS} shmmgr \
                  pdsmsmgmt pdsmscommon pdsmshals pdsmsstubs pdsmsmgmtsvc
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_LDLIBS   =  ${NIC_COMMON_LDLIBS} \
                   ${NIC_CAPSIM_LDLIBS} \
                   ${${PIPELINE}_GTEST_COMMON_LDLIBS} \
                   AAPL edit ncurses lmdb rt $(MS_LD_LIBS)
MODULE_INCS     = ${MODULE_GEN_DIR}
MODULE_FLAGS    = ${NIC_CSR_FLAGS} $(addprefix -D,$(MS_COMPILATION_SWITCH))
MODULE_SRCS     = $(shell find ${MODULE_SRC_DIR} -type f -name '*.cc' ! -name 'agenthooks*')
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
include ${MKDEFS}/post.mk
