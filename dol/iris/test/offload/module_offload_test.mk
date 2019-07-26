# {C} Copyright 2019 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = offload_test.gtest
MODULE_ARCH     = x86_64
MODULE_PIPELINE = iris
MODULE_SRCS     = ${MODULE_SRC_DIR}/testvec_parser.cc \
                  ${MODULE_SRC_DIR}/testvec_output.cc \
                  ${MODULE_SRC_DIR}/crypto_rsa_testvec.cc \
                  ${MODULE_SRC_DIR}/crypto_rsa.cc \
                  ${MODULE_SRC_DIR}/crypto_ecdsa_testvec.cc \
                  ${MODULE_SRC_DIR}/crypto_ecdsa.cc \
                  ${MODULE_SRC_DIR}/crypto_asym.cc \
                  ${MODULE_SRC_DIR}/dole_if.cc \
                  ${MODULE_SRC_DIR}/dole_if_rsa.cc \
                  ${MODULE_SRC_DIR}/main.cc
MODULE_LIBS     = -lssl -lcrypto
MODULE_SOLIBS   = storage_hal_if storage_nicmgr_if storage_qstate_if \
                  storage_tests storage_rdma storage_acc_ring \
                  storage_utils storage_r2n storage_ssd_core \
                  storage_nvme_dp host_mem storage_qstate_if model_client \
                  p4pd_pack_storage_seq \
                  sdkplatformutils shmmgr logger \
                  nicmgr mnet \
                  pciemgr_if pciemgr pciemgrutils pciehdevices pcietlp cfgspace \
                  intrutils pal misc evutils \
                  ${NIC_SDK_SOLIBS} trace bm_allocator \
                  commonproto nicmgrproto halproto \
                  delphisdk \
                  upgrade upgrade_app upgradeutils \
                  sysmgr dol_engine \
                  sdkcapri_asicrw_if \
                  sdkasicpd pdcapri pdcommon hal_mock \
                  sdkp4 sdkp4utils sdkxcvrdriver sdkfru p4pd_${PIPELINE} \
                  p4pd_common_p4plus_rxdma p4pd_common_p4plus_txdma \
                  asicpd ${NIC_HAL_PD_SOLIBS_${ARCH}} devapi_iris rdmamgr_iris
MODULE_LDLIBS   := crypto ${NIC_COMMON_LDLIBS} \
                   ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                   ${SDK_THIRDPARTY_CAPRI_LDLIBS} \
                   ${NIC_CAPSIM_LDLIBS} gflags
MODULE_INCS     = ${TOPDIR}/nic/third-party/gflags/include \
                  ${TOPDIR}/dol/iris/test/storage \
                  ${TOPDIR}/nic/hal/third-party/grpc/include \
                  ${TOPDIR}/nic/hal/third-party/google/include \
                  ${TOPDIR}/nic/hal/third-party/openssl/include \
                  ${TOPDIR}/nic/hal/pd/capri \
                  ${TOPDIR}/nic/hal/test \
                  ${TOPDIR}/platform/src/lib/nicmgr/include \
                  ${TOPDIR}/platform/include/common \
                  ${TOPDIR}/platform/src/lib/pciemgr/include \
                  ${TOPDIR}/platform/src/lib/pciemgrutils/include \
                  ${TOPDIR}/platform/src/lib/pciehdevices/include \
                  ${TOPDIR}/platform/src/lib/pcieport/include \
                  ${BLD_GEN_DIR}/proto \
                  ${MODULE_SRC_DIR}/engine \
                  ${TOPDIR}/nic/utils/host_mem \
                  ${TOPDIR}/nic/sdk/include \
                  ${TOPDIR}/nic/sdk/model_sim/include \
                  ${TOPDIR}/nic/include \
                  ${TOPDIR}
include ${MKDEFS}/post.mk
