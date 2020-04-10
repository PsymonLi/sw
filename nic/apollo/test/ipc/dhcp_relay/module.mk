# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = ${PIPELINE}_ipc_dhcp_relay_test.gtest
MODULE_PIPELINE = apulu
MODULE_ARCH     = x86_64
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi pdsapi_impl \
                  pdstest pdstestapiutils penipc pdsproto \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils \
                  sdkplatformutils sdkasicpd kvstore_lmdb \
                  lpmitree_${PIPELINE} rfc_${PIPELINE} pdsrfc penmetrics \
                  bm_allocator sdklinkmgr sdklinkmgrcsr \
                  memhash sltcam slhash ${NIC_${PIPELINE}_NICMGR_LIBS}
MODULE_LDLIBS   = ${NIC_COMMON_LDLIBS} ${NIC_CAPSIM_LDLIBS} \
                  ${${PIPELINE}_GTEST_COMMON_LDLIBS} \
                  AAPL edit ncurses lmdb
MODULE_FLAGS    = ${NIC_CSR_FLAGS}
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
ifdef AGENT_MODE
MODULE_SOLIBS  += agentclient
endif
include ${MKDEFS}/post.mk
