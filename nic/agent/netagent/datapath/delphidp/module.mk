# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = agent_delphidp.submake
MODULE_PIPELINE = iris
MODULE_DIR      := ${GOPATH}/src/github.com/pensando/sw/nic/${MODULE_DIR}
MODULE_DEPS     := $(wildcard ${NICDIR}/proto/hal/*.proto)
include ${MKDEFS}/post.mk
