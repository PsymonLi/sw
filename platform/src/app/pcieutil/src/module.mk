# {C} Copyright 2018-2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   := pcieutil.bin
MODULE_SRCS     := $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                   $(wildcard ${MODULE_SRC_DIR}/${ASIC}/*.cc)
MODULE_INCS     := ${MODULE_SRC_DIR}/${ASIC}
MODULE_SOLIBS   := pciemgr pciemgrutils cfgspace pcieport pcietlp intrutils \
		   pciemgr_if evutils pal misc \
		   ${NULL}
MODULE_LDLIBS   := m ev
ifeq ($(ARCH),aarch64)
MODULE_SOLIBS   += catalog sdkfru
endif
include ${MKDEFS}/post.mk
