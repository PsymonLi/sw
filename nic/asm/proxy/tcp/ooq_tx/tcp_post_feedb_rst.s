#include "nic/p4/common/defines.h"
#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp-phv.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s1_t0_ooq_tcp_tx_k.h"

struct phv_ p;
struct s1_t0_ooq_tcp_tx_k_ k;

%%
    .align
tcp_post_feedb_rst_pkt:
    CAPRI_CLEAR_TABLE_VALID(0)
    phvwr           p.tcp_app_hdr_from_ooq_txdma, 1
    phvwr           p.tcp_app_hdr_table0_valid, 1
    CAPRI_DMA_CMD_PHV2PKT_SETUP2(intrinsic2_dma_cmd, p4_intr_global_tm_iport,
                                p4_intr_packet_len,
                                intr_rxdma2_qid, intr_rxdma2_rxdma_rsv)
    phvwr           p.intr_rxdma2_qid, k.common_phv_fid
    phvwr           p.intr_rxdma2_rx_splitter_offset, \
                    (ASICPD_GLOBAL_INTRINSIC_HDR_SZ + ASICPD_RXDMA_INTRINSIC_HDR_SZ + \
                    P4PLUS_TCP_PROXY_BASE_HDR_SZ + 1)
    CAPRI_DMA_CMD_PHV2PKT_SETUP(tcp_app_hdr1_dma_cmd, tcp_app_hdr_p4plus_app_id, tcp_app_hdr_prev_echo_ts)
    phvwr           p.feedback_type_entry, TCP_TX2RX_FEEDBACK_RST_PKT
    // r1 has feedback type
    CAPRI_DMA_CMD_PHV2PKT_SETUP_STOP(feedback_dma_cmd, feedback_type_entry, feedback_type_entry)
    nop.e
    nop
