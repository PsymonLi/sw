# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = cond_var_test.gtest
MODULE_SOLIBS = utils logger condvar
MODULE_ARCH   = x86_64
include ${MKDEFS}/post.mk
