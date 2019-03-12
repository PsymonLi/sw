# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET       = tpc.proto
MODULE_PIPELINE     = apollo
MODULE_PREREQS      = gogo.proto
MODULE_GEN_TYPES    = PY CC GO
MODULE_INCS         = ${MODULE_DIR} \
                      ${TOPDIR}/nic/${MODULE_DIR} \
                      ${TOPDIR}/nic/${MODULE_DIR}/meta \
                      ${TOPDIR}/vendor/github.com/gogo/protobuf/protobuf \
                      ${TOPDIR}/vendor/github.com/gogo/protobuf/gogoproto \
                      ${TOPDIR}/vendor/github.com/pensando/grpc-gateway/third_party \
                      ${TOPDIR}/vendor/github.com/pensando/grpc-gateway/third_party/googleapis \
                      /usr/local/include
MODULE_SRCS         = $(wildcard ${MODULE_DIR}/*.proto) \
                      $(wildcard ${MODULE_DIR}/meta/*.proto)
MODULE_POSTGEN_MK   = module.mk
MODULE_PROTOC_GOFAST_OPTS = --gofast_out=grpc:${MODULE_DIR}
MODULE_GEN_DIR      = ${BLD_PROTOGEN_DIR}
include ${MKDEFS}/post.mk
