# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = agent_pdsupgproto.submake
MODULE_PIPELINE = apollo artemis apulu athena
MODULE_DIR      := ${GOPATH}/src/github.com/pensando/sw/nic/${MODULE_DIR}
MODULE_DEPS     := $(wildcard ${MODULE_DIR}/*.proto)
include ${MKDEFS}/post.mk
