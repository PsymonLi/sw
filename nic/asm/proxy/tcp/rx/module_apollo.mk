# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = tcp_proxy_rxdma_apollo.asmbin
MODULE_PREREQS  = proxy.p4bin
MODULE_PIPELINE = apollo
MODULE_INCS     = ${BLD_P4GEN_DIR}/tcp_proxy_rxdma/asm_out \
                  ${BLD_P4GEN_DIR}/tcp_proxy_rxdma/alt_asm_out \
                  ${MODULE_DIR}/../include \
                  ${MODULE_DIR}/../../include \
                  ${MODULE_DIR}/../../../common-p4+/include \
                  ${MODULE_DIR}/../../../cpu-p4plus/include \
                  ${TOPDIR}/nic/include

MODULE_DEPS     = $(shell find ${MODULE_DIR} -name '*.h')
MODULE_BIN_DIR  = ${BLD_BIN_DIR}/p4pasm_rxdma
include ${MKDEFS}/post.mk
