# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ARCH       := aarch64 x86_64
MODULE_TARGET     := dpdk.submake
DPDK              := ${MODULE_DIR}/../../dpdk
MODULE_DEPS       := $(shell find ${DPDK} -name '*.c' -o -name '*.h') \
                     $(shell find ${DPDK}/config -name '*')
MODULE_CLEAN_DIRS := ${DPDK}/build
#For simulator
ifeq ($(ARCH), x86_64)
MODULE_SOLIBS     := dpdksim sdkpal logger
MODULE_ARLIBS	  := dpdksim
endif
include ${MKDEFS}/post.mk
