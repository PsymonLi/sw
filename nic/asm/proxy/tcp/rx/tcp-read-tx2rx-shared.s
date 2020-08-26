/*
 *    Implements the tx2rx shared state read stage of the RxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-phv.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_common_p4plus_stage0_app_header_table_k.h"

struct phv_ p;
struct common_p4plus_stage0_app_header_table_k_ k;
struct common_p4plus_stage0_app_header_table_read_tx2rx_d d;

%%

    .param          tcp_rx_s1_t0_bubble_launch_rx
    .param          tcp_rx_s1_t0_bubble_win_upd
    .param          tcp_rx_s1_t0_bubble_feedb_rst
    .param          tcp_rx_read_rnmdr_start
    .param          TCP_PROXY_STATS 
    .align


tcp_rx_read_shared_stage0_start:
    seq             c1, d.rsvd, 1
    phvwr.c1        p.p4_intr_global_debug_trace, 1
#ifdef CAPRI_IGNORE_TIMESTAMP
    add             r4, r0, r0
#endif
    phvwrpair       p.s1_s2s_payload_len, k.tcp_app_header_payload_len, \
                        p.s1_s2s_seq, k.tcp_app_header_seqNo
    phvwrpair       p.common_phv_fid, k.p4_rxdma_intr_qid, \
                        p.common_phv_qstate_addr, k.p4_rxdma_intr_qstate_addr
    CAPRI_OPERAND_DEBUG(k.tcp_app_header_ecn)
    phvwrpair       p.common_phv_debug_dol, d.debug_dol[7:0], \
                        p.common_phv_ip_tos_ecn, k.tcp_app_header_ecn
    phvwr           p.to_s5_serq_cidx, d.serq_cidx[14:0]
    // phvwr           p.p4_intr_global_debug_trace, 1 

    seq             c1, k.tcp_app_header_from_ooq_txdma, 1
    // HACK, For tx2rx feedback packets 1 byte following tcp_app_header contains
    // pkt type. This falls in the tcp app header pad region of common rxdma phv.
    // Until we can unionize this header correctly in p4, hardcoding the PHV
    // location for now. This is prone to error, but hopefully if something
    // breaks, we have DOL test cases to catch it.  (refer to
    // iris/gen/p4gen/tcp_proxy_rxdma/asm_out/INGRESS_p.h)
    seq             c2, k._tcp_app_header_end_pad_88[15:8], TCP_TX2RX_FEEDBACK_WIN_UPD
    bcf             [c1 & c2], tcp_rx_tx2rx_win_upd

    seq             c2, k._tcp_app_header_end_pad_88[15:8], TCP_TX2RX_FEEDBACK_LAST_OOO_PKT
    bcf             [c1 & c2], tcp_rx_tx2rx_last_ooo_pkt

    seq             c2, k._tcp_app_header_end_pad_88[15:8], TCP_TX2RX_FEEDBACK_RST_PKT
    bcf             [c1 & c2], tcp_rx_tx2rx_feedb_rst_pkt

    tblwr           d.rx_ts, r4

    /* Write all the tx to rx shared state from table data into phv */

    phvwr           p.common_phv_flags, k.tcp_app_header_flags

    /*
     * If we see a pure SYN drop it
     * (Don't drop pure RST)
     */
    //TODO...challenge ACK these
    and             r2, k.tcp_app_header_flags, (TCPHDR_ACK | TCPHDR_RST)
    seq             c1, r2, 0
    phvwri.c1       p.p4_intr_global_drop, 1
    bcf             [c1], flow_terminate

    seq             c1, d.fin_sent, 1
    phvwr.c1        p.s1_s2s_fin_sent, 1
    seq             c1, d.rto_event, 1
    phvwr.c1        p.to_s3_cc_rto_signal, 1
    phvwr.c1        p.to_s5_cc_rto_signal, 1
    tblwr           d.fin_sent, 0
    tblwr.f         d.rto_event, 0

    seq             c1, d.rst_sent, 1
    phvwr.c1        p.s1_s2s_rst_sent, 1

table_read_RX:
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_s1_t0_bubble_launch_rx)

    /* Setup the to-stage/stage-to-stage variables based
     * on the p42p4+ app header info
     */
    phvwr           p.to_s2_data_ofs_rsvd[7:4], k.tcp_app_header_dataOffset
    phvwrpair       p.to_s2_rcv_wup, d.rcv_wup, \
                        p.to_s2_rcv_wnd_adv, d.rcv_wnd_adv
    //phvwr           p.cpu_hdr3_tcp_window, k.{tcp_app_header_window}.hx
    phvwrpair       p.s1_s2s_ack_seq, k.tcp_app_header_ackNo, \
                        p.s1_s2s_snd_nxt, d.snd_nxt
    phvwr           p.to_s3_window, k.tcp_app_header_window
    phvwr           p.s1_s2s_sack_blks, k.tcp_app_header_num_sack_blocks
    phvwrpair       p.s1_s2s_rcv_tsval[31:8], k.tcp_app_header_ts_s0_e23, \
                        p.s1_s2s_rcv_tsval[7:0], k.tcp_app_header_ts_s24_e31
    phvwr           p.to_s4_rcv_tsecr, k.tcp_app_header_prev_echo_ts
    phvwrpair       p.to_s4_rtt_time, d.rtt_time, \
                    p.to_s4_rtt_seq, d.rtt_seq
    add             r1, r0, d.debug_dol
    smeqh           c1, r1, TCP_DDOL_TSOPT_SUPPORT, TCP_DDOL_TSOPT_SUPPORT
    phvwr.c1        p.common_phv_tsopt_enabled, 1
    seq.c1          c2, k.tcp_app_header_prev_echo_ts, r0
    setcf           c1, [c1 & !c2]
    phvwr.c1        p.common_phv_tsopt_available, 1

    phvwr           p.to_s2_serq_cidx, d.serq_cidx
    phvwr           p.to_s6_payload_len, k.tcp_app_header_payload_len

    seq             c1, k.tcp_app_header_from_ooq_txdma, 1
    b.!c1           launch_rnmdr_alloc_if_need
    phvwr.c1        p.common_phv_ooq_tx2rx_pkt, 1
    // HACK, 1+8 bytes following tcp_app_header is ooq_header which contains the
    // descriptor address. This falls in app_data1 region of common rxdma phv.
    // Until we can unionize this header correctly in p4, hardcoding the PHV
    // location for now. This is prone to error, but hopefully if something
    // breaks, we have DOL test cases to catch it.  (refer to
    // iris/gen/p4gen/tcp_proxy_rxdma/asm_out/INGRESS_p.h)
    seq             c1, k._tcp_app_header_end_pad_88[15:8], TCP_TX2RX_FEEDBACK_OOO_PKT
    b.!c1           invalid_ooo_feedb_pkt 

    // OOO pkt feedback
    add.c1          r1, r0, k.app_header_app_data1[87:32]
    phvwr           p.to_s6_descr, r1
    add             r1, r1, ASIC_NMDPR_PAGE_OFFSET
    phvwr           p.to_s6_page, r1
    nop.e
    nop
launch_rnmdr_alloc_if_need:
    /*
     * Launch rnmdr descr alloc if data pkt of fin or rst (or syn in some cases for iris)
     */
    sne             c1, k.tcp_app_header_payload_len, 0
    smneb.!c1       c1, k.tcp_app_header_flags, (TCPHDR_FIN | TCPHDR_RST | TCPHDR_SYN), 0
    b.!c1           tcp_rx_stage0_end
launch_rnmdr_alloc:
    CAPRI_NEXT_TABLE_READ_i(1, TABLE_LOCK_DIS, tcp_rx_read_rnmdr_start,
                        RNMDPR_ALLOC_IDX, TABLE_SIZE_64_BITS)

tcp_rx_stage0_end:
    nop.e
    nop

invalid_ooo_feedb_pkt:
    phvwri p.p4_intr_global_drop, 1
flow_terminate:
    CAPRI_CLEAR_TABLE_VALID(0)
    CAPRI_CLEAR_TABLE_VALID(1)
    CAPRI_CLEAR_TABLE_VALID(2)
    CAPRI_CLEAR_TABLE_VALID(3)
    nop.e
    nop

tcp_rx_tx2rx_feedb_rst_pkt:
/**** TEMPORARY *****************************************************/
    addui           r3, r0, hiword(TCP_PROXY_STATS)
    addi            r3, r3, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r3,TCP_PROXY_STATS_DEBUG1, 1)
/**** TEMPORARY END *************************************************/
    phvwr           p.to_s2_serq_cidx, d.serq_cidx[14:0]
    /*
     * Feedb rst pkt does not hve app header populated. But we DMA flags to SERQ from phv app hdr.
     * Write tcp_add_header_flags for RST pkt
     */
    phvwri          p.tcp_app_header_flags, (TCPHDR_ACK |  TCPHDR_RST)
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_s1_t0_bubble_feedb_rst)
    // Need to alloc buffer for emulation of RST
    b               launch_rnmdr_alloc
    phvwr           p.common_phv_feedb_rst_pkt, 1


tcp_rx_tx2rx_win_upd:
    phvwr           p.common_phv_ooq_tx2rx_win_upd, 1
    b               launch_bubble_rx_win_upd
    phvwr           p.common_phv_ooq_tx2rx_pkt, 1

tcp_rx_tx2rx_last_ooo_pkt:
    phvwr           p.common_phv_ooq_tx2rx_pkt, 1
    phvwr           p.common_phv_ooq_tx2rx_last_ooo_pkt, 1

launch_bubble_rx_win_upd:
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_s1_t0_bubble_win_upd)
    nop.e
    nop

