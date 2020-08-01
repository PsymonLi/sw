# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libsdkcapri.lib
MODULE_ASIC     = capri
MODULE_LDLIBS	= ssl crypto
MODULE_FLAGS    = -O3
MODULE_DEFS     = ${NIC_CSR_DEFINES}
ifeq ($(ARCH),aarch64)
MODULE_INCS     = ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include/config \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include/arch/arm \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_compat
else
MODULE_INCS     = ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include/config \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_eal/common/include/arch/x86 \
                  ${SDKDIR}/third-party/dpdk/v18.11/lib/librte_compat
endif
MODULE_FLAGS    = -DRTE_ARCH_64 -DRTE_CACHE_LINE_SIZE=64
include ${MKDEFS}/post.mk
