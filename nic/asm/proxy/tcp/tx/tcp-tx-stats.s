/*
 *  Implements the stats stage of the TCP TxDMA P4+ pipeline
 */

#include "defines.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp-constants.h"
#include "tcp-phv.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s6_t0_tcp_tx_k.h"

struct phv_ p;
struct s6_t0_tcp_tx_k_ k;

%%
    .param          TCP_PROXY_STATS
    .align
tcp_tx_stats_start:
    CAPRI_CLEAR_TABLE_VALID(0)

// Load-balance phvwr from previous stage
dma_cmd_intrinsic:
    phvwri          p.p4_txdma_intr_dma_cmd_ptr, TCP_PHV_TXDMA_COMMANDS_START
    phvwri          p.{p4_intr_global_tm_iport...p4_intr_global_tm_oport}, \
                        (TM_PORT_DMA << 4) | TM_PORT_INGRESS
    CAPRI_DMA_CMD_PHV2PKT_SETUP2(intrinsic_dma_dma_cmd, p4_intr_global_tm_iport,
                                p4_intr_global_tm_instance_type,
                                p4_txdma_intr_qid, tcp_app_header_vlan_tag)
#ifdef HW
    /*
     * In real HW, we want to increment ip_id, otherwise linux does not perform
     * GRO
     */
    phvwrpair       p.tcp_app_header_p4plus_app_id, P4PLUS_APPTYPE_TCPTLS, \
                        p.tcp_app_header_flags, \
                            (P4PLUS_TO_P4_FLAGS_LKP_INST | \
                            P4PLUS_TO_P4_FLAGS_UPDATE_IP_LEN | \
                            P4PLUS_TO_P4_FLAGS_COMPUTE_L4_CSUM | \
                            P4PLUS_TO_P4_FLAGS_UPDATE_IP_ID)
#else
    /*
     * In simulation, don't increment ip_id, otherwise a lot of DOL cases need
     * to change
     */
    phvwrpair       p.tcp_app_header_p4plus_app_id, P4PLUS_APPTYPE_TCPTLS, \
                        p.tcp_app_header_flags, \
                        (P4PLUS_TO_P4_FLAGS_LKP_INST | \
                            P4PLUS_TO_P4_FLAGS_UPDATE_IP_LEN | \
                            P4PLUS_TO_P4_FLAGS_COMPUTE_L4_CSUM)
#endif

dma_cmd_write_tx2rx_shared:
    /* Set the DMA_WRITE CMD for copying tx2rx shared data from phv to mem */
    add             r5, k.common_phv_qstate_addr, TCP_TCB_TX2RX_SHARED_WRITE_OFFSET

    CAPRI_DMA_CMD_PHV2MEM_SETUP_STOP(tx2rx_dma_dma_cmd, r5, tx2rx_snd_nxt, tx2rx_pad1_tx2rx)

    phvwr           p.tcp_header_seq_no, k.t0_s2s_snd_nxt
    phvwr           p.tcp_header_ack_no, k.t0_s2s_rcv_nxt

// TODO: Move to multi stats update

// **NOTE: Offsets need to match definition in __tcp_tx_stats_t in 
// p4pd_tcp_proxy_api.h
bytes_sent_atomic_stats_update_start:
    CAPRI_ATOMIC_STATS_INCR1(bytes_sent, k.t0_s6_s2s_tx_stats_base,
                0 * 8, k.to_s6_bytes_sent)
bytes_sent_atomic_stats_update_done:

pkts_sent_atomic_stats_update_start:
    CAPRI_ATOMIC_STATS_INCR1(pkts_sent, k.t0_s6_s2s_tx_stats_base,
                1 * 8, k.to_s6_pkts_sent)
pkts_sent_atomic_stats_update_done:

pure_acks_sent_atomic_stats_update_start:
    CAPRI_ATOMIC_STATS_INCR1(pure_acks_sent, k.t0_s6_s2s_tx_stats_base,
                2 * 8, k.to_s6_pure_acks_sent)
pure_acks_sent_atomic_stats_update_done:

    seq             c1, k.common_phv_fin, 1
    balcf           r7, [c1], tcp_tx_update_fin_stats
    nop
    
    seq             c1, k.common_phv_rst, 1
    balcf           r7, [c1], tcp_tx_update_rst_stats
    nop

tcp_rx_stats_done:
    nop.e
    nop

tcp_tx_update_fin_stats:
    addui           r2, r0, hiword(TCP_PROXY_STATS)
    addi            r2, r2, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_FIN_SENT, 1)
    jr              r7
    nop 

tcp_tx_update_rst_stats:
    addui           r2, r0, hiword(TCP_PROXY_STATS)
    addi            r2, r2, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_RST_SENT, 1)
    jr              r7
    nop 
