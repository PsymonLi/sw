# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = ${PIPELINE}_policy_test.gtest
MODULE_PIPELINE = apollo artemis apulu
MODULE_ARCH     = x86_64
MODULE_PREREQS  = metaswitch.submake
MODULE_INCS     = ${BLD_PROTOGEN_DIR} $(TOPDIR)/nic/metaswitch/stubs/hals \
                  $(addprefix $(MS_ROOT)/,$(MS_INCLPATH))
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi pdsapi_impl \
                  pdstest pdstestapiutils \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils sdk_asicrw_if sdk${ASIC} \
                  sdkplatformutils sdkxcvrdriver sdkasicpd \
                  lpmitree_${PIPELINE} rfc_${PIPELINE} pdsrfc penmetrics \
                  bm_allocator sdklinkmgr sdklinkmgrcsr kvstore_lmdb \
                  memhash sltcam ${NIC_${PIPELINE}_NICMGR_LIBS} slhash \
                  shmmgr pdsmsmgmt pdsmscommon pdsmshals pdsmsstubs pdsmsmgmtsvc
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_LDLIBS   = ${NIC_COMMON_LDLIBS} ${NIC_CAPSIM_LDLIBS} \
                  ${${PIPELINE}_GTEST_COMMON_LDLIBS} \
                  AAPL edit ncurses lmdb rt $(MS_LD_LIBS)
MODULE_FLAGS    = ${NIC_CSR_FLAGS} $(addprefix -D,$(MS_COMPILATION_SWITCH))
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
ifdef AGENT_MODE
MODULE_SOLIBS  += agentclient
endif
include ${MKDEFS}/post.mk
