# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = libipseccb.lib
MODULE_SRCS = $(wildcard ${MODULE_SRC_DIR}/*.cc)
MODULE_PIPELINE = apulu
MODULE_PREREQS = ipsec_p4plus_esp_v4_tun_hton.p4bin \
				 ipsec_p4plus_esp_v4_tun_ntoh.p4bin
include ${MKDEFS}/post.mk
