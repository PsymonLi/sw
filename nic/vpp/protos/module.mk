# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET        = libsess_syncproto.lib
MODULE_PIPELINE      = apulu apollo artemis
MODULE_INCS          = /usr/local/include \
                       ${TOPDIR}/nic/hal/third-party/google/include \
                       ${BLD_PROTOGEN_DIR}
#MODULE_LDLIBS        = pthread
MODULE_FLAGS         = -O3
MODULE_EXCLUDE_FLAGS = -O2
MODULE_SRCS          = $(wildcard ${BLD_PROTOGEN_DIR}/sess_sync.*.cc) \
                       $(wildcard ${BLD_PROTOGEN_DIR}/nat_sync.*.cc)
include ${MKDEFS}/post.mk
