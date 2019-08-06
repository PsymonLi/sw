# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = libstorage_rdma.so
MODULE_PIPELINE = iris
MODULE_SRCS     = ${MODULE_SRC_DIR}/rdma.cc
MODULE_INCS     = ${TOPDIR}/nic/third-party/gflags/include \
                  ${TOPDIR}/nic/hal/third-party/grpc/include \
                  ${TOPDIR}/nic/hal/third-party/google/include \
		  ${TOPDIR}/nic/hal/tls/engine \
                  ${TOPDIR}/platform/src/lib/nicmgr/include \
                  ${TOPDIR}/platform/include/common \
                  ${TOPDIR}/platform/src/lib/pciemgr/include \
                  ${TOPDIR}/platform/src/lib/pciemgrutils/include \
                  ${TOPDIR}/platform/src/lib/pciehdevices/include \
                  ${TOPDIR}/platform/src/lib/pcieport/include \
                  ${BLD_GEN_DIR}/proto \
                  ${TOPDIR}/nic/sdk/include \
                  ${TOPDIR}/nic/include

include ${MKDEFS}/post.mk
