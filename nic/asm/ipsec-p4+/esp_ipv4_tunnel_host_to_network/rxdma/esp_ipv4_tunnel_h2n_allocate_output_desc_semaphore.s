#include "ingress.h"
#include "INGRESS_p.h"
#include "ipsec_asm_defines.h"

struct rx_table_s1_t1_k k;
struct rx_table_s1_t1_allocate_output_desc_semaphore_d d;
struct phv_ p;

%%
        .param          esp_ipv4_tunnel_h2n_allocate_output_desc_index 
        .param          esp_ipv4_tunnel_h2n_allocate_output_page_index
        .param          TNMDR_TABLE_BASE 
        .param          TNMPR_TABLE_BASE
        .align

esp_ipv4_tunnel_h2n_allocate_output_desc_semaphore:
    phvwri p.app_header_table1_valid, 1
    phvwri p.common_te1_phv_table_pc, esp_ipv4_tunnel_h2n_allocate_output_desc_index[33:6] 
    phvwri p.{common_te1_phv_table_lock_en...common_te1_phv_table_raw_table_size}, 11
    and r1, d.{out_desc_ring_index}.dx, IPSEC_DESC_RING_INDEX_MASK 
    sll r1, r1, 3 
    addui r1, r1, hiword(TNMDR_TABLE_BASE)
    addi r1, r1, loword(TNMDR_TABLE_BASE)
    phvwr p.common_te1_phv_table_addr, r1

    phvwri p.dma_cmd_pkt2mem_dma_cmd_type, CAPRI_DMA_COMMAND_PKT_TO_MEM
    phvwri p.tail_2_bytes_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwri p.tail_2_bytes_dma_cmd_phv_start_addr, IPSEC_TAIL_2_BYTES_PHV_START
    phvwr p.tail_2_bytes_dma_cmd_phv_end_addr, IPSEC_TAIL_2_BYTES_PHV_END
    phvwri p.dma_cmd_iv_salt_dma_cmd_phv_start_addr, IPSEC_IN_DESC_IV_SALT_START
    phvwri p.dma_cmd_iv_salt_dma_cmd_phv_end_addr, IPSEC_IN_DESC_IV_SALT_END

    phvwri p.app_header_table3_valid, 1
    and r1, d.{out_desc_ring_index}.dx, IPSEC_DESC_RING_INDEX_MASK
    sll r1, r1, 3
    addui r1, r1, hiword(TNMPR_TABLE_BASE)
    addi r1, r1, loword(TNMPR_TABLE_BASE)
    phvwr p.common_te3_phv_table_addr, r1
    phvwri p.{common_te3_phv_table_lock_en...common_te3_phv_table_raw_table_size}, ((1 << 3) | 3)
    phvwri.f p.common_te3_phv_table_pc, esp_ipv4_tunnel_h2n_allocate_output_page_index[33:6]

    nop.e 
    nop
