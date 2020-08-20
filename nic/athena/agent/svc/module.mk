# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libsvc_athena.lib
MODULE_PIPELINE = athena
MODULE_INCS     = ${MODULE_GEN_DIR}
MODULE_SOLIBS   = pal pdsframework pdscore pdsapi_athena pdsapi_athena_impl \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  ${NIC_SDK_SOLIBS} ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                  sdkp4 sdkp4utils sdk_asicrw_if sdk${ASIC} \
                  sdkplatformutils sdkxcvrdriver sdkasicpd \
                  bm_allocator sdklinkmgr sdklinkmgrcsr operd \
                  operd_event
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/agent/svc/port.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/agent/svc/device.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/agent/svc/svc_thread.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/agent/svc/debug.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/agent/svc/interface.cc)
MODULE_LDLIBS   = ${NIC_CAPSIM_LDLIBS} \
                  ${SDK_THIRDPARTY_CAPRI_LDLIBS} \
                  AAPL
include ${MKDEFS}/post.mk
