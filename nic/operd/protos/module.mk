# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET        = liboperdproto.lib
MODULE_PIPELINE      = apulu
MODULE_INCS          = /usr/local/include \
                       ${TOPDIR}/nic/hal/third-party/google/include \
                       ${BLD_PROTOGEN_DIR}
MODULE_LDLIBS        = pthread
MODULE_FLAGS         = -O3
MODULE_EXCLUDE_FLAGS = -O2
MODULE_SRCS          = $(wildcard ${BLD_PROTOGEN_DIR}/gogo.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/types.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/metrics.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/flow.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/alerts.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/oper.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/techsupport.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/syslog.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/meta/*.cc)
include ${MKDEFS}/post.mk
