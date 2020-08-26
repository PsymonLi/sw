/*
 *  Stage 1 Table 0 Bubble stage for all cases
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s1_t0_tcp_rx_k.h"

struct phv_ p;
struct s1_t0_tcp_rx_k_ k;

%%
    .align
    .param          tcp_rx_process_start
tcp_rx_s1_t0_bubble_launch_rx:
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_EN,
                tcp_rx_process_start, k.common_phv_qstate_addr,
                TCP_TCB_RX_OFFSET, TABLE_SIZE_512_BITS)
    nop

    .align
    .param          tcp_rx_win_upd_process_start
tcp_rx_s1_t0_bubble_win_upd:
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_EN,
                tcp_rx_win_upd_process_start, k.common_phv_qstate_addr,
                TCP_TCB_RX_OFFSET, TABLE_SIZE_512_BITS)
    nop

    .align
    .param          tcp_rx_feedb_rst_pkt
tcp_rx_s1_t0_bubble_feedb_rst:
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_EN,
                tcp_rx_feedb_rst_pkt, k.common_phv_qstate_addr,
                TCP_TCB_RX_OFFSET, TABLE_SIZE_512_BITS)
    nop
