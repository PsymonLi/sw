# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = athena_client.bin
MODULE_PIPELINE = athena
MODULE_ARCH     = aarch64
MODULE_INCS     = ${MODULE_GEN_DIR} ${BLD_OUT_DIR}/pen_dpdk_submake/include/
MODULE_LDFLAGS  = -L${TOPDIR}/nic/third-party/gflags/${ARCH}/lib
MODULE_PREREQS  = pen_dpdk.submake
MODULE_LDLIBS   = zmq gflags
MODULE_DEFS     = -DRTE_FORCE_INTRINSICS
include ${MKDEFS}/post.mk
