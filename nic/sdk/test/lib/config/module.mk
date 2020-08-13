# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = config_test.gtest
MODULE_SOLIBS =  config logger utils
MODULE_ARCH   = x86_64
include ${MKDEFS}/post.mk
