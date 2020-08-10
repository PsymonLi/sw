# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libpdsapi_athena_impl.lib
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/api/impl/*.cc)
MODULE_PIPELINE = athena
MODULE_INCS     = ${BLD_OUT_DIR}/pen_dpdk_submake/include/ ${SDK_THIRD_PARTY_OMAPI_INCLUDES} ${MODULE_GEN_DIR}
MODULE_SOLIBS   = ${NIC_${PIPELINE}_PDSAPI_IMPL_SOLIBS}
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES} -DRTE_FORCE_INTRINSICS
MODULE_FLAGS    = ${NIC_CSR_FLAGS}
MODULE_PREREQS  = pen_dpdk.submake hal.memrgns
MODULE_LDLIBS   = ${SDK_THIRD_PARTY_OMAPI_LIBS}

include ${MKDEFS}/post.mk
