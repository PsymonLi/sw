/*
 *  Stage 4 Table 0
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s4_t0_tcp_rx_k.h"

struct phv_ p;
struct s4_t0_tcp_rx_k_ k;

%%
    .align
    .param          tcp_rx_fc_drop_packet

tcp_rx_s4_t0_bubble_drop_pkt:
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_DIS,
                tcp_rx_fc_drop_packet, k.common_phv_qstate_addr,
                TCP_TCB_FC_OFFSET, TABLE_SIZE_512_BITS)
    nop

    .align
    .param         tcp_rx_s5_t0_launch_dma_serq

tcp_rx_s4_t0_bubble_feedb_rst:
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_s5_t0_launch_dma_serq)
    nop.e
    nop
