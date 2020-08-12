# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   := captrace.bin
MODULE_ASIC     = capri
MODULE_SOLIBS   := sdkpal logger sdk_asicrw_if pal catalog \
                   sdkplatformutils sdkfru shmmgr
MODULE_LDLIBS   := dl sknobs Judy rt ${SDK_THIRDPARTY_CAPRI_LDLIBS}
MODULE_FLAGS    = ${NIC_CSR_FLAGS}
MODULE_DEFS     = -DCAPRI_SW ${NIC_CSR_DEFINES}
include ${MKDEFS}/post.mk
