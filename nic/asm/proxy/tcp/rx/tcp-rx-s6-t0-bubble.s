/*
 *  Stage 6 Table 0 Bubble stage for launching stats stage
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct phv_ p;
struct s6_t0_tcp_rx_dma_d d;

%%
    .align
    .param          tcp_rx_stats_stage_start
tcp_rx_s6_t0_bubble_launch_stats:
    phvwr       p.s7_s2s_rx_stats_base, d.rx_stats_base
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_stats_stage_start)
    nop.e
    nop
