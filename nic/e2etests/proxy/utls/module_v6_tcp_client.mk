# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = nic_proxy_e2etest_v6_tcp_client.bin
MODULE_ARCH     = x86_64
MODULE_PIPELINE = iris
MODULE_SRCS     = ${MODULE_SRC_DIR}/tcp-client-v6.cc
MODULE_SOLIBS   = e2e_driver trace host_mem bm_allocator \
                  logger model_client halproto  \
                  sdkplatformutils shmmgr
MODULE_LDLIBS   = zmq pthread z m rt Judy dl ev \
                  ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                  ${NIC_THIRDPARTY_SSL_LDLIBS}
MODULE_INCS     = ${TOPDIR}/nic/hal/third-party/openssl/include
include ${MKDEFS}/post.mk
