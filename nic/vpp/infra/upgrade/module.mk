# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET    = libpdsvpp_upgrade.lib
MODULE_PIPELINE  = apollo artemis apulu
MODULE_PREREQS   = vpp_pkg.export
MODULE_SRCS      = $(wildcard ${MODULE_DIR}/*.c)                             \
		   $(wildcard ${MODULE_DIR}/*.cc)
MODULE_SOLIBS    = upgrade_ev pdsvpp_api
MODULE_LDLIBS    = ${SDK_THIRD_PARTY_VPP_LIBS}
MODULE_INCS      = ${VPP_PLUGINS_INCS}
MODULE_DEFS      = ${VPP_DEFINES_${ARCH}}
MODULE_FLAGS     = ${VPP_FLAGS_${ARCH}}
include ${MKDEFS}/post.mk
