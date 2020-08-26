/*
 *  Stage 3 Table 0
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct phv_ p;

%%
    .align
    .param          tcp_rx_s4_t0_bubble_drop_pkt
tcp_rx_s3_t0_bubble_drop_pkt:
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_rx_s4_t0_bubble_drop_pkt)
    nop.e
    nop

    .align
    .param          tcp_rx_s4_t0_bubble_feedb_rst
tcp_rx_s3_t0_bubble_feedb_rst:
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tcp_rx_s4_t0_bubble_feedb_rst)
    nop.e
    nop
