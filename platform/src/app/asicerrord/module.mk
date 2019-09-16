# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = asicerrord.bin
MODULE_PIPELINE = iris
MODULE_SOLIBS   = sdkpal delphisdk logger halproto shmmgr utils sdkasicpd\
                  pdcapri pdcommon p4pd_${PIPELINE} sdkp4 sdkp4utils \
                  p4pd_common_p4plus_rxdma p4pd_common_p4plus_txdma \
                  asicpd ${NIC_HAL_PD_SOLIBS_${ARCH}} ${NIC_SDK_SOLIBS} \
                  bm_allocator sdkxcvrdriver hal_mock \
                  sdkcapri_asicrw_if events_recorder eventproto eventtypes
MODULE_LDLIBS   = rt dl :libprotobuf.so.14 ev grpc++ crypto \
                  ${NIC_COMMON_LDLIBS} \
                  ${SDK_THIRDPARTY_CAPRI_LDLIBS} \
                  ${NIC_CAPSIM_LDLIBS}
include ${MKDEFS}/post.mk
