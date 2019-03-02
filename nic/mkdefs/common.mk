# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

export GRPC_CPP_PLUGIN  := ${TOPDIR}/nic/hal/third-party/grpc/x86_64/bin/grpc_cpp_plugin
export GRPC_PY_PLUGIN   := ${TOPDIR}/nic/hal/third-party/grpc/x86_64/bin/grpc_python_plugin

NCPUS=$(shell grep processor /proc/cpuinfo | wc -l)
MAKEFLAGS += -j${NCPUS}
ifdef JOB_ID
include ${MKDEFS}/jobd_targets.mk
endif

include ${MKDEFS}/pkg.mk
include ${MKDEFS}/docker.mk
include ${MKDEFS}/image.mk
include ${SDKDIR}/mkdefs/common.mk

export THIRD_PARTY_INCLUDES := \
       ${TOPDIR}/nic/hal/third-party/spdlog/include \
       ${TOPDIR}/nic/third-party/gflags/include \
       ${TOPDIR}/nic/hal/third-party/grpc/include \
       ${TOPDIR}/nic/hal/third-party/google/include \
       ${TOPDIR}/nic/hal/third-party/openssl/include \
       ${TOPDIR}/nic/third-party/libz/include \
       ${TOPDIR}/nic/third-party/liblmdb/include \
       ${SDK_THIRD_PARTY_INCLUDES}

export NIC_COMMON_LDLIBS_x86_64     := zmq
export NIC_COMMON_LDLIBS_aarch64    :=
export NIC_COMMON_LDLIBS            := pthread z m rt Judy dl ev ${NIC_COMMON_LDLIBS_${ARCH}}

export NIC_COMMON_FLAGS := -pthread -rdynamic

export NIC_SDK_SOLIBS   := list slab shmmgr mmgr sdkpal sdkfru \
    ht indexer logger thread utils periodic twheel directmap \
    hash hbmhash tcam timerfd catalog sdkplatformutils sdkcapri \
    sdkp4loader sdkasicrw lif_mgr

export NIC_HAL_DLOPEN_SOLIBS := cfg_plugin_nw \
                                cfg_plugin_rdma \
                                cfg_plugin_aclqos \
                                cfg_plugin_nat \
                                cfg_plugin_gft \
                                cfg_plugin_ipsec \
                                cfg_plugin_mcast \
                                cfg_plugin_telemetry \
                                cfg_plugin_l4lb \
                                cfg_plugin_lif \
                                cfg_plugin_accel \
                                plugin_network \
                                plugin_alg_utils \
                                plugin_sfw \
                                plugin_alg_sip \
                                plugin_alg_tftp \
                                plugin_alg_dns \
                                plugin_alg_rpc \
                                plugin_alg_rtsp \
                                plugin_alg_ftp \
                                plugin_ep_learn \
                                plugin_lb \
                                plugin_nat \
                                plugin_proxy \
                                plugin_telemetry \
                                plugin_app_redir

export NIC_HAL_PROTO_SOLIBS := halproto hal_svc_gen hal_svc
export NIC_HAL_CFG_PLUGIN_SOLIBS := cfg_plugin_tcp_proxy
export NIC_HAL_PLUGIN_SOLIBS := plugin_classic \
                                plugin_ep_learn_common \
                                plugin_sfw_pkt_utils \
                                isc_dhcp


export NIC_HAL_UTILS_SOLIBS := bitmap block_list nat eventmgr \
    trace mtrack fsm bm_allocator

export NIC_HAL_PD_SOLIBS_x86_64 := model_client
export NIC_HAL_PD_SOLIBS_aarch64 :=
export NIC_HAL_PD_SOLIBS := sdkcapri_csrint \
       sdkcapri_asicrw_if pdcapri pdcommon sdkp4 \
       pd_${PIPELINE} sdkasicpd asicpd pd_acl_tcam pd_met pdaccel \
       ${NIC_HAL_PD_SOLIBS_${ARCH}}
export NIC_CAPSIM_LDLIBS := mpuobj capisa isa sknobs

export NIC_HAL_CORE_SOLIBS := hal_src heartbeat core hal_lib

export NIC_FTE_SOLIBS := fte fte_iris

export NIC_HAL_TLS_SOLIBS := tls_pse tls_api

export NIC_LINKMGR_SOLIBS := linkmgr_libsrc sdklinkmgrcsr sdklinkmgr linkmgrdelphi
export NIC_LINKMGR_LDLIBS := AAPL

export NIC_LKL_SOLIBS := lklshim_tls lkl_api

export NIC_P4_NCC_DEPS  := $(shell find ${TOPDIR}/nic/tools/ncc/ -name '*.py') \
                           $(shell find ${TOPDIR}/nic/tools/ncc/ -name '*.cc') \
                           $(shell find ${TOPDIR}/nic/tools/ncc/ -name '*.h')

export NIC_iris_P4PD_SOLIBS := p4pd_iris p4pd_common_p4plus_txdma \
    p4pd_common_p4plus_rxdma pd_cpupkt pd_wring
export NIC_gft_P4PD_SOLIBS := p4pd_gft p4pd_common_p4plus_txdma \
    p4pd_common_p4plus_rxdma
export NIC_apollo_P4PD_SOLIBS := p4pd_apollo p4pd_apollo_rxdma p4pd_apollo_txdma
export NIC_l2switch_P4PD_SOLIBS := p4pd_l2switch
export NIC_elektra_P4PD_SOLIBS := p4pd_elektra
export NIC_hello_P4PD_SOLIBS := p4pd_hello

# ==========================================================================
#                           P4PD CLI Libs
# ==========================================================================
export CLI_P4PD_INCS := ${NIC_DIR}/hal/third-party/google/include \
                        ${NIC_DIR}/hal/third_party/grpc/include   \
                        /usr/include/python3.6m \
                        /usr/include/python3.4m
export CLI_P4PD_FLAGS := -Wl,--allow-multiple-definition -Wno-sign-compare
export CLI_iris_P4PD_LDLIBS := :libprotobuf.so.14 grpc++_reflection grpc++ grpc_unsecure grpc++_unsecure rt ev
export CLI_apollo_P4PD_LDLIBS := ${NIC_COMMON_LDLIBS} ${NIC_CAPSIM_LDLIBS}
export CLI_iris_P4PD_SOLIBS := halproto sdkp4utils delphisdk
export CLI_apollo_P4PD_SOLIBS := ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                                 ${NIC_SDK_SOLIBS} \
                                 ${NIC_HAL_PD_SOLIBS_${ARCH}} \
                                 sdkp4 sdkp4utils sdkcapri_csrint \
                                 sdkcapri_asicrw_if sdkcapri \
                                 sdkplatformutils sdkasicpd memhash \
                                 bm_allocator pal
# ==========================================================================
#                           Third-party Libs
# ==========================================================================
export NIC_THIRDPARTY_LKL_LDLIBS := lkl
export NIC_THIRDPARTY_SSL_LDLIBS := ssl crypto
export NIC_THIRDPARTY_GOOGLE_LDLIBS := :libprotobuf.so.14 grpc++_reflection \
       grpc++ grpc_unsecure grpc++_unsecure
export NIC_THIRDPARTY_PACKET_PARSER_LDLIBS := packet_parser

# ==========================================================================
#                        HAL Binary/Gtest Libs
# ==========================================================================
export NIC_HAL_PLUGIN_SOLIBS:= ${NIC_HAL_PLUGIN_SOLIBS} \
                               ${NIC_HAL_DLOPEN_SOLIBS} \
                               ${NIC_HAL_PROTO_SOLIBS} \
                               ${NIC_HAL_CFG_PLUGIN_SOLIBS}

export NIC_HAL_ALL_SOLIBS   := ${NIC_HAL_CORE_SOLIBS} \
                               ${NIC_HAL_PLUGIN_SOLIBS} \
                               ${NIC_HAL_UTILS_SOLIBS} \
                               ${NIC_FTE_SOLIBS} \
                               ${NIC_HAL_TLS_SOLIBS} \
                               ${NIC_LKL_SOLIBS} \
                               ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                               ${NIC_HAL_PD_SOLIBS} \
                               ${NIC_LINKMGR_SOLIBS} \
                               ${NIC_SDK_SOLIBS} \
                               pal agent_api delphisdk haldelphi halsysmgr \
                               nicmgrproto commonproto haldelphiutils ftestatsproto

export NIC_HAL_ALL_LDLIBS   := ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                               ${NIC_THIRDPARTY_SSL_LDLIBS} \
                               ${NIC_THIRDPARTY_LKL_LDLIBS} \
                               ${NIC_THIRDPARTY_PACKET_PARSER_LDLIBS} \
                               ${NIC_CAPSIM_LDLIBS} \
                               ${NIC_LINKMGR_LDLIBS} \
                               ${NIC_COMMON_LDLIBS}

export NIC_HAL_GTEST_SOLIBS := ${NIC_HAL_ALL_SOLIBS} haltestutils
export NIC_HAL_GTEST_LDLIBS := ${NIC_HAL_ALL_LDLIBS}
export NIC_HAL_GTEST_WO_MAIN_LDLIBS :=

# ==========================================================================
#                        Pipeline Specific Defs
# ==========================================================================
iris_DEFS       := -DIRIS
gft_DEFS        := -DGFT
apollo_DEFS     := -DAPOLLO
l2switch_DEFS   := -DL2SWITCH
elektra_DEFS    := -DELEKTRA
hello_DEFS      := -DHELLO
