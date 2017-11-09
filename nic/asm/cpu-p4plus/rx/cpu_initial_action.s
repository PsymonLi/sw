#include "INGRESS_p.h"
#include "ingress.h"
#include "cpu-table.h"

struct phv_ p;
struct common_p4plus_stage0_app_header_table_k k;
struct common_p4plus_stage0_app_header_table_d d;

%%
    .param          cpu_rx_read_desc_pindex_start
    .param          cpu_rx_read_page_pindex_start
    .param          cpu_rx_read_arqrx_pindex_start
    .param          ARQRX_BASE
    .param          ARQRX_QIDXR_BASE
    .align
cpu_rx_read_shared_stage0_start:
    CAPRI_CLEAR_TABLE0_VALID
    // Store timestamp in the quisce pkt trailer
    CAPRI_OPERAND_DEBUG(r6)
    phvwr   p.quiesce_pkt_trlr_timestamp, r6.wx
    
    phvwr   p.common_phv_qstate_addr, k.{p4_rxdma_intr_qstate_addr_sbit0_ebit1...p4_rxdma_intr_qstate_addr_sbit2_ebit33}
    phvwr   p.to_s3_payload_len, k.cpu_app_header_packet_len
    phvwr   p.common_phv_debug_dol, d.u.cpu_rxdma_initial_action_d.debug_dol
    phvwr   p.common_phv_flags, d.u.cpu_rxdma_initial_action_d.flags

table_read_DESC_SEMAPHORE:
    addi    r3, r0, RNMDR_ALLOC_IDX 
    CAPRI_NEXT_TABLE_READ(1, TABLE_LOCK_DIS, 
                         cpu_rx_read_desc_pindex_start,
                         r3,
                         TABLE_SIZE_64_BITS)
table_read_PAGE_SEMAPHORE:
    addi    r3, r0, RNMPR_ALLOC_IDX
    CAPRI_NEXT_TABLE_READ(2, TABLE_LOCK_DIS, 
                         cpu_rx_read_page_pindex_start,
                         r3,
                         TABLE_SIZE_64_BITS)
table_read_ARQRX_PINDEX:
    phvwri  p.to_s3_arqrx_base, ARQRX_BASE
    CPU_ARQRX_QIDX_ADDR(0, r3, ARQRX_QIDXR_BASE)
    CAPRI_NEXT_TABLE_READ(3, TABLE_LOCK_EN,
                          cpu_rx_read_arqrx_pindex_start,
                          r3,
                          TABLE_SIZE_512_BITS)
    nop.e
    nop
