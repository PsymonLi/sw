# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = operd_syslog_test.gtest
MODULE_PIPELINE = iris apulu
MODULE_ARCH     = x86_64
MODULE_LDLIBS   = ${NIC_THIRDPARTY_GOOGLE_LDLIBS} rt ev
MODULE_SRCS     = ${MODULE_SRC_DIR}/syslog_test.cc \
	${MODULE_SRC_DIR}/syslog_endpoint.cc
include ${MKDEFS}/post.mk
