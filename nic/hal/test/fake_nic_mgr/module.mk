# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = fake_nic_mgr.bin
MODULE_PIPELINE = iris gft
MODULE_SOLIBS   = halproto logger thread
MODULE_LDLIBS   = ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                  ${NIC_COMMON_LDLIBS} ev utils
include ${MKDEFS}/post.mk

