# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk

MODULE_TARGET   = protoc-gen-delphigo.gobin
MODULE_FLAGS    = -ldflags="-s -w"
MODULE_DEPS = ${TOPDIR}/nic/delphi/compiler/delphic

include ${MKDEFS}/post.mk
