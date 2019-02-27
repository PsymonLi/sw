
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_rx_table_s3_t0_k.h"

#include "defines.h"

struct phv_ p;
struct rx_table_s3_t0_k_ k;
struct rx_table_s3_t0_eth_rx_packet_d d;

#define   _r_pktlen     r1        // Packet length
#define   _r_addr       r2        // Buffer address
#define   _r_len        r3        // Buffer length
#define   _r_stats      r4        // Stats
#define   _r_ptr        r5        // Current DMA byte offset in PHV
#define   _r_index      r6        // Current DMA command index in PHV

%%

.param eth_rx_completion
.param eth_rx_stats
.param eth_rx_sg_start

.align
eth_rx:
    LOAD_STATS(_r_stats)

    // Setup DMA CMD PTR
    phvwr           p.p4_rxdma_intr_dma_cmd_ptr, ETH_DMA_CMD_START_OFFSET
    addi            _r_index, r0, (ETH_DMA_CMD_START_FLIT << LOG_NUM_DMA_CMDS_PER_FLIT) | ETH_DMA_CMD_START_INDEX

    bcf             [c2 | c3 | c7], eth_rx_desc_addr_error
    nop

    // Packet length check
    //
    // if (buf.len < pkt.len && sg_desc_addr == 0)
    //      goto eth_rx_desc_data_error;
    //
    slt             c1, d.{len}.hx, k.eth_rx_global_pkt_len     // c1 = buf.len < pkt.len
    seq             c2, k.eth_rx_to_s3_sg_desc_addr, 0          // c2 = sg_desc_addr == 0
    bcf             [c1 & c2], eth_rx_desc_data_error
    nop

    //
    // if (buf.len < pkt.len)
    //      dma.len = buf.len
    // else
    //      dma.len = pkt.len
    //
    cmov            _r_len, c1, d.{len}.hx, k.eth_rx_global_pkt_len

    // DMA packet
    DMA_CMD_PTR(_r_ptr, _r_index, r7)
    DMA_PKT(_r_ptr, _r_addr, _r_len)
    DMA_CMD_NEXT(_r_index)

    //
    // if ( !(buf.len < pkt.len) )
    //      goto eth_rx_done
    // else
    //      goto eth_rx_sg
    //
    bcf            [!c1], eth_rx_done

    // rem_bytes = pkt.len - dma.len
    sub            _r_len, k.eth_rx_global_pkt_len, _r_len      // branch-delay slot

eth_rx_sg:
    SAVE_STATS(_r_stats)

    // Launch eth_rx_sg in next stage
    phvwr           p.eth_rx_t1_s2s_sg_desc_addr, k.eth_rx_to_s3_sg_desc_addr
    phvwr           p.eth_rx_t1_s2s_rem_sg_elems, RX_MAX_SG_ELEMS
    phvwr           p.eth_rx_t1_s2s_rem_pkt_bytes, _r_len

    // Save DMA command pointer
    phvwr           p.eth_rx_global_dma_cur_index, _r_index

    phvwri          p.{app_header_table0_valid...app_header_table3_valid}, (1 << 2)
    phvwri          p.common_te1_phv_table_pc, eth_rx_sg_start[38:6]
    phvwr.e         p.common_te1_phv_table_addr, k.eth_rx_to_s3_sg_desc_addr
    phvwr.f         p.common_te1_phv_table_raw_table_size, LG2_RX_SG_MAX_READ_SIZE

eth_rx_desc_addr_error:
    SET_STAT(_r_stats, _C_TRUE, desc_fetch_error)
    phvwr           p.eth_rx_global_drop, 1     // increment pkt drop counters

    DMA_CMD_PTR(_r_ptr, _r_index, r7)
    DMA_SKIP_TO_EOP(_r_ptr, _C_FALSE)
    DMA_CMD_NEXT(_r_index)

    b               eth_rx_done
    phvwri          p.eth_rx_cq_desc_status, ETH_RX_DESC_ADDR_ERROR

eth_rx_desc_data_error:
    SET_STAT(_r_stats, _C_TRUE, desc_data_error)
    phvwr           p.eth_rx_global_drop, 1     // increment pkt drop counters

    DMA_CMD_PTR(_r_ptr, _r_index, r7)
    DMA_SKIP_TO_EOP(_r_ptr, _C_FALSE)
    DMA_CMD_NEXT(_r_index)

    b               eth_rx_done
    phvwri          p.eth_rx_cq_desc_status, ETH_RX_DESC_DATA_ERROR

eth_rx_done:
    SAVE_STATS(_r_stats)

    // Save DMA command pointer
    phvwr           p.eth_rx_global_dma_cur_index, _r_index

    phvwri          p.{app_header_table0_valid...app_header_table3_valid}, ((1 << 3) | 1)

    // Launch eth_rx_stats action
    phvwri          p.common_te3_phv_table_pc, eth_rx_stats[38:6]
    phvwri          p.common_te3_phv_table_raw_table_size, CAPRI_RAW_TABLE_SIZE_MPU_ONLY

    // Launch eth_rx_completion stage
    phvwri.e        p.common_te0_phv_table_pc, eth_rx_completion[38:6]
    phvwri.f        p.common_te0_phv_table_raw_table_size, CAPRI_RAW_TABLE_SIZE_MPU_ONLY
