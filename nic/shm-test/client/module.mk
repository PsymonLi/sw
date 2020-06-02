# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET    = shm-test-client.bin
MODULE_PIPELINE  = apulu
MODULE_SRCS      = $(wildcard ${MODULE_DIR}/*.cc)
MODULE_SOLIBS    = pdsproto vppinternalproto shmipc
MODULE_LDLIBS    = rt ${NIC_THIRDPARTY_GOOGLE_LDLIBS}
MODULE_INCS      = ${BLD_PROTOGEN_DIR}
include ${MKDEFS}/post.mk
