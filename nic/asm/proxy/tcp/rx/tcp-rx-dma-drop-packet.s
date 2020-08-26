/*
 *  Stage 6 Table 0
 */

#include "tcp-constants.h"
#include "tcp-phv.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tls_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s6_t0_tcp_rx_k.h"

struct phv_ p;
struct s6_t0_tcp_rx_k_ k;
struct s6_t0_tcp_rx_dma_drop_d d;

%%
    .align
    .param          RX_RNMDR_GC_TABLE_BASE
    .param          TCP_PROXY_STATS

tcp_rx_dma_drop_packet:
    CAPRI_CLEAR_TABLE0_VALID
    phvwri          p.p4_rxdma_intr_dma_cmd_ptr, TCP_PHV_RXDMA_COMMANDS_START
    add             r1, d.sw_pi, 1
    and             r1, r1, ASIC_HBM_GC_PER_PRODUCER_RING_MASK
    seq             c1, r1, d.sw_ci
    b.c1            fatal_error
    seq             c1, k.to_s6_descr, 0
    b.c1            drop_packet

    /*
     * r2 = GC PI
     */
    add             r2, d.sw_pi, r0
    tblmincri.f     d.sw_pi, ASIC_HBM_GC_PER_PRODUCER_RING_SHIFT, 1

    /*
     * r1 = address in GC ring to DMA into
     * r2 = gc pi
     */
    addui           r1, r0, hiword(RX_RNMDR_GC_TABLE_BASE)
    addi            r1, r1, loword(RX_RNMDR_GC_TABLE_BASE)
    add             r1, r1, r2, RNMDR_TABLE_ENTRY_SIZE_SHFT

    /*
     * If the pkt to be dropped is a regular/non-ooo-looped-back pkt with
     * payload, set skip dma.
     * Note: If we receive data in rxdma, we are required to do one of the
     *  following else dma engine will be hung
     * 1) Drop the packet (p4_intr_global_drop)
     * 2) Setup dma of the complete packet
     * 3) setup skip dma instruction
     */
    sne             c1, k.s1_s2s_payload_len, 0
    seq.c1          c1, k.common_phv_ooq_tx2rx_pkt, 0
    balcf           r6, [c1], dma_skip_pkt

dma_free_rnmdr:
    phvwr           p.ring_entry_descr_addr, k.to_s6_descr
    CAPRI_DMA_CMD_PHV2MEM_SETUP(gc_dma_dma_cmd, r1, ring_entry_64_dword, \
                        ring_entry_64_dword)

dma_free_rnmdr_doorbell:
    CAPRI_DMA_CMD_RING_DOORBELL2_SET_PI_FENCE(gc_doorbell_dma_cmd, LIF_GC,
                      ASIC_HBM_GC_RNMDR_QTYPE,
                      ASIC_RNMDR_GC_TCP_RX_RING_PRODUCER, 0,
                      d.sw_pi, db_data_pid, db_data_index)
    /*
     * If ACK_SEND is pending, the DB DMA for that would happen in parallel in t1 and set EOP there.
     * Else, no other DMA will happen, so set EOP
     */
    smeqb          c1, k.common_phv_pending_txdma, TCP_PENDING_TXDMA_ACK_SEND, TCP_PENDING_TXDMA_ACK_SEND
    phvwri.!c1       p.gc_doorbell_dma_cmd_eop, 1
    nop.e
    nop

fatal_error:
    addui           r2, r0, hiword(TCP_PROXY_STATS)
    addi            r2, r2, loword(TCP_PROXY_STATS)
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r2, TCP_PROXY_STATS_GC_FULL, 1)
    phvwri          p.p4_intr_global_drop, 1
    phvwri.e        p.{app_header_table0_valid...app_header_table3_valid}, 0
    nop

drop_packet:
    CAPRI_CLEAR_TABLE0_VALID
    phvwr.e         p.p4_intr_global_drop, 1
    nop

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////
dma_skip_pkt:
    CAPRI_DMA_CMD_SKIP_SETUP(pkt_dma_skip_dma_cmd)
    jr              r6
    nop

