# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = libpd_iris.so
MODULE_PIPELINE = iris
MODULE_PREREQS  = iris.p4bin \
                  common_p4plus_txdma.p4bin common_p4plus_rxdma.p4bin \
                  proxy.p4bin adminq.p4bin app_redir_p4plus.p4bin \
                  cpu_p4plus.p4bin eth.p4bin gc.p4bin ipfix.p4bin \
                  ipsec_p4plus_esp_v4_tun_hton.p4bin \
                  ipsec_p4plus_esp_v4_tun_ntoh.p4bin \
                  p4pt.p4bin tls.p4bin rdma.p4bin smbdc.p4bin \
                  storage.p4bin
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/aclqos/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/internal/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/l4lb/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/lif/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/mcast/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/nw/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/ipsec/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/tcp_proxy/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/debug/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/firewall/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/dos/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/telemetry/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/event/*.cc) \
		  $(wildcard ${MODULE_SRC_DIR}/../common_p4plus/*.cc)
MODULE_INCS     = ${NIC_CSR_INCS} \
				  ${BLD_PROTOGEN_DIR}
include ${MKDEFS}/post.mk
