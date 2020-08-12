# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_ASIC     = elba
MODULE_TARGET   := elbtrace.bin
MODULE_SOLIBS   := sdkpal logger sdk_asicrw_if  pal catalog \
                   sdkplatformutils sdkfru shmmgr
ifeq ($(ARCH),x86_64)
MODULE_SOLIBS   += model_client
endif
MODULE_LDLIBS   := dl sknobs Judy rt ${SDK_THIRDPARTY_ELBA_LDLIBS}
MODULE_FLAGS    = ${NIC_CSR_FLAGS}
MODULE_DEFS     = -DELBA_SW ${NIC_CSR_DEFINES}
include ${MKDEFS}/post.mk
