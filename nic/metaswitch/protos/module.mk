# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET        = libpdsgenproto.lib
MODULE_PIPELINE      = apollo artemis apulu athena
MODULE_INCS          = /usr/local/include \
                       ${TOPDIR}/nic/hal/third-party/google/include \
                       ${BLD_PROTOGEN_DIR}
MODULE_LDLIBS        = pthread
MODULE_FLAGS         = -O3
MODULE_EXCLUDE_FLAGS = -O2
MODULE_SRCS          = $(wildcard ${BLD_PROTOGEN_DIR}/lim.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/types.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/bgp.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/evpn.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/cp_route.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/interface.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/internal.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/internal_evpn.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/internal_cp_route.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/internal_bgp.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/epoch.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/debugpdsms.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/cp_test.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/internal_lim.*.cc)
MODULE_PREREQS       = pdsa.proto
include ${MKDEFS}/post.mk
