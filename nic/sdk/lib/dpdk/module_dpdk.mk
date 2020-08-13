# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ARCH       := aarch64 x86_64
MODULE_TARGET     := pen_dpdk.submake
PEN_DPDK          := ${MODULE_DIR}/../../pen-dpdk
MODULE_DEPS       := $(shell find ${PEN_DPDK}/dpdk -name '*.c' -o -name '*.h') \
                     $(shell find ${PEN_DPDK}/dpdk/config -name '*')
MODULE_CLEAN_DIRS := ${PEN_DPDK}/dpdk/build
#For simulator
ifeq ($(ARCH), x86_64)
MODULE_SOLIBS     := dpdksim sdkpal logger
MODULE_ARLIBS	  := dpdksim
endif
include ${MKDEFS}/post.mk
