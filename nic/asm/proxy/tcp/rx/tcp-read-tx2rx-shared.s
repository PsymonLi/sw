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

    .param          tcp_rx_process_start
    .align
tcp_rx_read_shared_stage0_start:
#ifdef CAPRI_IGNORE_TIMESTAMP
    add             r4, r0, r0
#endif
    tblwr           d.rx_ts, r4

    /* Write all the tx to rx shared state from table data into phv */

    phvwr           p.common_phv_flags, k.tcp_app_header_flags

    /*
     * If we see a pure SYN drop it
     * (Don't drop pure RST)
     */
    and             r2, k.tcp_app_header_flags, (TCPHDR_ACK | TCPHDR_RST)
    seq             c1, r2, 0
    phvwri.c1       p.p4_intr_global_drop, 1
    bcf             [c1], flow_terminate

    phvwrpair       p.s1_s2s_fin_sent, d.fin_sent, \
                        p.s1_s2s_cc_rto_signal, d.rto_event
    tblwr           d.fin_sent, 0
    tblwr.f         d.rto_event, 0

    phvwr           p.s1_s2s_rst_sent, d.rst_sent

table_read_RX:
    CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_EN,
                tcp_rx_process_start, k.p4_rxdma_intr_qstate_addr,
                TCP_TCB_RX_OFFSET, TABLE_SIZE_512_BITS)

    /* Setup the to-stage/stage-to-stage variables based
     * on the p42p4+ app header info
     */
    phvwrpair       p.s1_s2s_rcv_tsval[31:8], k.tcp_app_header_ts_s0_e23, \
                        p.s1_s2s_rcv_tsval[7:0], k.tcp_app_header_ts_s24_e31
    phvwr           p.to_s3_rcv_tsecr, k.tcp_app_header_prev_echo_ts

    add             r1, r0, d.debug_dol
    smeqh           c1, r1, TCP_DDOL_TSOPT_SUPPORT, TCP_DDOL_TSOPT_SUPPORT
    phvwr.c1        p.common_phv_tsopt_enabled, 1
    phvwr.c1        p.common_phv_tsopt_available, 1    /* Until P4 is updated to support this in TCP app hdr */

    phvwrpair       p.common_phv_fid, k.p4_rxdma_intr_qid, \
                        p.common_phv_qstate_addr, k.p4_rxdma_intr_qstate_addr
    phvwrpair       p.common_phv_debug_dol, d.debug_dol[7:0], \
                        p.common_phv_ip_tos_ecn, k.tcp_app_header_ecn

    phvwr           p.to_s1_serq_cidx, d.serq_cidx
    phvwr           p.to_s5_serq_cidx, d.serq_cidx

    phvwr.f.e       p.to_s6_payload_len, k.tcp_app_header_payload_len
    nop

flow_terminate:
    CAPRI_CLEAR_TABLE_VALID(0)
    CAPRI_CLEAR_TABLE_VALID(1)
    CAPRI_CLEAR_TABLE_VALID(2)
    CAPRI_CLEAR_TABLE_VALID(3)
    nop.e
    nop
