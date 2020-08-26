/*
 * Implements the TSO stage of the TxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-phv.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "defines.h"
#include "INGRESS_s5_t0_tcp_tx_k.h"

struct phv_ p    ;
struct s5_t0_tcp_tx_k_ k    ;
struct s5_t0_tcp_tx_tso_d d    ;


%%
    .align
    .param          tcp_tx_stats_start

tcp_tso_process_start:
    /* check SESQ for pending data to be transmitted */
    sne             c6, k.common_phv_debug_dol_dont_tx, r0
    bcf             [c6], flow_tso_process_drop
    or              r1, k.to_s5_pending_tso_data, k.common_phv_pending_ack_send
    or              r1, r1, k.common_phv_rst
    sne             c1, r1, r0
    b.!c1           flow_tso_process_drop
    nop
tcp_write_xmit:
    seq             c2, k.to_s5_ts_offset, r0
    tblwr.!c2       d.ts_offset, k.to_s5_ts_offset
    tblwr.!c2       d.ts_time, k.to_s5_ts_time
    seq             c1, k.common_phv_pending_ack_send, 1
    seq             c2, k.common_phv_rst, 1
    
    /* disable keepalives when sending a rst */
    add.c2          r1, k.common_phv_qstate_addr, TCP_TCB_RX2TX_KEEPA_TO_OFFSET
    memwr.h.c2      r1, 0
    /*
     * If RST is being sent, send close indication on ccq close ring
     * Note the branch uses r5 and c3
     */
    bal.c2          r7, ring_db_ccq_close_ind
    nop
    bcf             [c1 | c2], dma_cmd_intrinsic

    seq             c1, k.t0_s2s_addr, r0
    bcf             [c1], flow_tso_process_drop
    nop

dma_cmd_intrinsic:
    // We rang the doorbell with TCP proxy service lif, but the P4
    // pipeline needs the original source_lif of the packet to derive
    // the input properties, as well as for spoofing checks
    phvwr           p.p4_intr_global_lif, d.source_lif

    // app header
#ifdef HW
    /*
     * In real HW, we want to increment ip_id, otherwise linux does not perform
     * GRO
     */
    phvwr           p.tcp_app_header_ip_id_delta, d.ip_id
    tbladd          d.ip_id, 1
#else
    /*
     * In simulation, don't increment ip_id, otherwise a lot of DOL cases need
     * to change
     */
    phvwr           p.tcp_app_header_ip_id_delta, d.ip_id
#endif

dma_cmd_hdr:
    add             r5, k.common_phv_qstate_addr, TCP_TCB_HEADER_TEMPLATE_OFFSET

    CAPRI_OPERAND_DEBUG(d.header_len)
    CAPRI_DMA_CMD_MEM2PKT_SETUP(l2l3_header_dma_dma_cmd, r5, d.header_len[13:0])
dma_cmd_tcp_header:
    phvwr           p.{tcp_header_source_port...tcp_header_dest_port}, \
                        d.{source_port...dest_port}
    phvwrpair       p.tx2rx_rcv_wup, k.t0_s2s_rcv_nxt, \
                        p.tx2rx_rcv_wnd_adv, k.to_s5_rcv_wnd
    seq             c2, k.to_s5_rcv_wnd, r0
    tbladd.c2       d.zero_window_sent, 1

    // r1 = tcp hdr len = 20 + sack_opt_len + ts_opt_len
    // r2 = r1 / 4
    add             r1, r0, 20

    // c1 = sack option
    // c2 = timestamp option
    sne             c1, k.to_s5_sack_opt_len, 0
    add.c1          r1, r1, k.to_s5_sack_opt_len

    smeqb           c2, d.tcp_opt_flags, TCP_OPT_FLAG_TIMESTAMPS, \
                        TCP_OPT_FLAG_TIMESTAMPS

    // optimization for linux
    bcf             [!c1 & c2], ts_option_only
    nop

    bcf             [!c1 & !c2], options_done
    nop

    b.!c2           nop_options

ts_opt:
    add.c2          r1, r1, TCPOLEN_TIMESTAMP

    srl             r2, r4, 13
    setcf           c4, [c0]    
    seq             c4, d.ts_time, r0
    tblwr.c4        d.ts_time, r2
    sub             r2, r2, d.ts_time
    add             r2, d.ts_offset, r2

    phvwr           p.{tcp_ts_opt_kind...tcp_ts_opt_len}, \
                        TCPOPT_TIMESTAMP << 8 | TCPOLEN_TIMESTAMP

    // r2 = ts_val << 32 | ts_ecr
    or              r2, k.to_s5_ts_ecr, r2, 32
    phvwr           p.{tcp_ts_opt_ts_val...tcp_ts_opt_ts_ecr}, r2

nop_options:
    // if both sack and timestamp are present, no need to add 2 byte pad
    bcf             [c1 & c2], options_done
    nop

    // set nop at the end of options
    // r4 = length of options
    // r3 = location in phv to write nop
    add             r1, r1, 2
    sub             r4, r1, 20
    sub             r3, offsetof(struct phv_, tcp_header_urg), r4, 3
    phvwrp          r3, 0, 16, (TCPOPT_NOP << 8) | TCPOPT_NOP

options_done:
    srl             r2, r1, 2
    phvwrpair       p.tcp_header_data_ofs, r2, \
                        p.tcp_header_window, k.to_s5_rcv_wnd

    CAPRI_DMA_CMD_PHV2PKT_SETUP_WITH_LEN(r7, tcp_header_dma_dma_cmd, tcp_header_source_port, r1)
    //CAPRI_DMA_CMD_PHV2PKT_SETUP(tcp_header_dma_dma_cmd, tcp_header_source_port, tcp_ts_opts_ts_ecr)

dma_cmd_data:
    /*
     * Check for pure ack
     */
    seq             c1, k.t0_s2s_addr, 0
    // Increment Pure ACK stats here based on c1
    bcf             [!c1], flow_tso_process
pure_acks_sent_stats_update_start:
    CAPRI_STATS_INC(pure_acks_sent, 1, d.pure_acks_sent, p.to_s6_pure_acks_sent)
pure_acks_sent_stats_update_end:
    b               flow_tso_process_done
    phvwri          p.tcp_header_dma_dma_pkt_eop, 1

flow_tso_process:
    /*
     * This catches FIN/RST etc.
     */
    seq             c1, k.t0_s2s_len, 0
    b.c1            pkts_sent_stats_update_start
    phvwri.c1       p.tcp_header_dma_dma_pkt_eop, 1

    /*
     * No TSO supported. Drop packet if greater than mss
     */
    slt             c1, d.smss, k.t0_s2s_len
    b.c1            flow_tso_process_drop
    add             r6, k.t0_s2s_len, r0
    phvwrpair       p.data_dma_dma_cmd_size, r6, \
                        p.data_dma_dma_cmd_addr[33:0], k.t0_s2s_addr
    phvwri          p.{data_dma_dma_cmd_pkt_eop...data_dma_dma_cmd_type}, \
                            1 << 4 | CAPRI_DMA_COMMAND_MEM_TO_PKT
        
    seq             c2, k.common_phv_pending_rto, 1
    seq             c3, k.common_phv_pending_fast_retx, 1
    bcf             [c2 | c3], pkts_sent_stats_update_end
bytes_sent_stats_update_start:
    CAPRI_STATS_INC(bytes_sent, r6, d.bytes_sent, p.to_s6_bytes_sent)
bytes_sent_stats_update_end:

pkts_sent_stats_update_start:
    CAPRI_STATS_INC(pkts_sent, 1, d.pkts_sent, p.to_s6_pkts_sent)
pkts_sent_stats_update_end:


tcp_retx:
tcp_retx_done:

flow_tso_process_done:
    phvwr           p.t0_s6_s2s_tx_stats_base, d.tx_stats_base
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_tx_stats_start)
    nop.e
    nop

flow_tso_process_drop:
    // TODO: Stats
    phvwri          p.p4_intr_global_drop, 1
    phvwr           p.t0_s6_s2s_tx_stats_base, d.tx_stats_base
    CAPRI_CLEAR_TABLE_VALID(0)
    nop.e
    nop


// Linux header prediction expects nop + nop + timestamp otherwise
// enters slow path. This is to keep Linux happy
ts_option_only:
    // NOP + NOP + timestamp
    add             r1, r1, TCPOLEN_TIMESTAMP + 2

    // write nop + nop + tsopt_kind + tsopt_len
    sub             r3, offsetof(struct phv_, tcp_header_urg), 32
    phvwrpi         r3, 0, 32, TCPOPT_NOP << 24 | TCPOPT_NOP << 16 | \
                        TCPOPT_TIMESTAMP << 8 | TCPOLEN_TIMESTAMP

    // calc ts_val
    srl             r2, r4, 13
    setcf           c4, [c0]    
    seq             c4, d.ts_time, r0
    tblwr.c4        d.ts_time, r2
    sub             r2, r2, d.ts_time
    add             r2, d.ts_offset, r2

    // write ts_val + ts_ecr
    // r2 = ts_val << 32 | ts_ecr
    or              r2, k.to_s5_ts_ecr, r2, 32
    sub             r3, offsetof(struct phv_, tcp_header_urg), 96
    phvwrp          r3, 0, 64, r2

    b               options_done
    nop


ring_db_ccq_close_ind:
    // Prep DB addr in r2 for qtype 1, TCP LIF
    addi            r2, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_SET,
                        DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)

    // Load RST close reason in r5
    seq             c3, k.to_s5_int_rst_rsn, 0
    add.!c3         r5, r0, k.to_s5_int_rst_rsn
    add.c3          r5, r0, LOCAL_RST

    /*
     * Prep DB data for cleanup cond 2, Data will be in r3
     * Note: As long as the prod value is non zero, the actual value otherwise has
     * no significance for indicating a close on the ccq ring.
     * With that, here we are using the prod to pass reason code
     */
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid, TCP_CCQ_CONN_CLOSED_IND_RING, r5)
    // DB write
    memwr.dx        r2, r3
    /*
     * c3: Is int_rst_rsn 0
     * Ring db to generate feedback rst pkt if the rst being sent is triggered from within tcp
     */
    b.c3            ring_db_ccq_close_ind_done
    nop
    // Prep DB addr in r2 for qtype 1, TCP LIF
    addi            r2, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_INC,
                        DB_SCHED_UPD_EVAL, TCP_OOO_RX2TX_QTYPE, LIF_TCP)
    // Prep DB data for generating feedb rst, Data will be in r3
    CAPRI_RING_DOORBELL_DATA(0, k.common_phv_fid, TCP_GEN_FEEDB_RST_RING, 0)
    // DB write
    memwr.dx        r2, r3
ring_db_ccq_close_ind_done:
    jr              r7
    nop
