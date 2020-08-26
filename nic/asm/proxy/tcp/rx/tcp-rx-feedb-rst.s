/*
 *  Implements the RX stage of the RxDMA P4+ pipeline
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s2_t0_tcp_rx_k.h"

struct phv_ p;
struct s2_t0_tcp_rx_k_ k;
struct s2_t0_tcp_rx_d d;
%%
    .param         tcp_rx_s3_t0_bubble_feedb_rst
    .align
tcp_rx_feedb_rst_pkt:
    CAPRI_CLEAR_TABLE0_VALID  
    /*
     * If we are already in RST state, nothing to do,
     * we would have already posted it on SERQ
     */
    seq             c1, d.u.tcp_rx_d.state, TCP_RST
    b.c1            flow_rx_drop 
    nop

    // check for serq full
    add             r2, d.u.tcp_rx_d.serq_pidx, r0
    mincr           r2, d.u.tcp_rx_d.consumer_ring_shift, 1
    seq             c7, r2, k.to_s2_serq_cidx
    tbladd.c7       d.u.tcp_rx_d.serq_full_cnt, 1
    // TODO: Handle this case. RST should never be dropped
    b.c7            flow_rx_drop
    
    // Change state to RST 
    tblwr           d.u.tcp_rx_d.state, TCP_RST
    phvwri          p.common_phv_write_serq, 1
    // get serq slot
    phvwr           p.to_s6_serq_pidx, d.u.tcp_rx_d.serq_pidx
    tblmincri       d.u.tcp_rx_d.serq_pidx, d.u.tcp_rx_d.consumer_ring_shift, 1
    CAPRI_NEXT_TABLE0_READ_NO_TABLE_LKUP(tcp_rx_s3_t0_bubble_feedb_rst)
    nop.e
    nop
flow_rx_drop:
    phvwri.e         p.p4_intr_global_drop, 1
    nop
