# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET = libtcpcb_pd.lib
MODULE_SRCS = $(wildcard ${MODULE_SRC_DIR}/*.cc)
MODULE_PIPELINE = iris apollo artemis
MODULE_PREREQS = proxy.p4bin
include ${MKDEFS}/post.mk
