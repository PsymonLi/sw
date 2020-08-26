/*
 *  Stage 5 Table 0
 */


#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp-constants.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s5_t0_tcp_rx_k.h"

struct phv_ p;
struct s5_t0_tcp_rx_k_ k;
struct s5_t0_tcp_rx_tcp_fc_d d;

%%
    .param          tcp_rx_dma_drop_packet
    .align

tcp_rx_fc_drop_packet:
    seq             c1, d.rnmdr_gc_base, 0
    b.c1            tcp_rx_fc_drop_packet_illegal

    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_EN, tcp_rx_dma_drop_packet,
                        d.rnmdr_gc_base, TABLE_SIZE_32_BITS)
    nop.e
    nop

tcp_rx_fc_drop_packet_illegal:
    phvwr           p.p4_intr_global_drop, 1
    CAPRI_CLEAR_TABLE0_VALID
    nop.e
    nop
