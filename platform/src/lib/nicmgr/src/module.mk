# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_PREREQS  := common_p4plus_rxdma.p4bin common_p4plus_txdma.p4bin \
                   hal.memrgns
MODULE_TARGET   := libnicmgr.so
SW_VERSION      := ${shell git describe --dirty --always}
MODULE_FLAGS    := -DSW_VERSION="\"${SW_VERSION}\""
MODULE_PIPELINE := iris gft
MODULE_INCS     := ${MODULE_SRC_DIR}/../include \
                    ${BLD_PROTOGEN_DIR}/ \
                    ${BLD_P4GEN_DIR}/common_rxdma_actions/include/ \
                    ${BLD_P4GEN_DIR}/common_txdma_actions/include/ \
                    ${TOPDIR}/nic/asic/capri/model/cap_top \
                    ${TOPDIR}/nic/asic/capri/model/cap_top/csr_defines
include ${MKDEFS}/post.mk
