# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = tcp_plugin.lib
MODULE_PIPELINE = apollo
MODULE_ARCH     = aarch64
MODULE_PREREQS  = vpp_pkg.export
MODULE_SOLIBS   = pdsvpp_impl pdsvpp_api pdsvpp_cfg indexer logger catalog \
				  sdkplatformutils lif_mgr sdkring tcpcb_pd
MODULE_LDLIBS   = ${SDK_THIRD_PARTY_VPP_LIBS}
MODULE_INCS     = ${SDK_THIRD_PARTY_VPP_INCLUDES}
MODULE_DEFS      = ${VPP_DEFINES_${ARCH}}
MODULE_FLAGS     = ${VPP_FLAGS_${ARCH}}
include ${MKDEFS}/post.mk
