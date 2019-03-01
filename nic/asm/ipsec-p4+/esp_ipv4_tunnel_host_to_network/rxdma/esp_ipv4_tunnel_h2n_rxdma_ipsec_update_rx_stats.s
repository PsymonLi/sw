#include "INGRESS_p.h"
#include "ingress.h"
#include "ipsec_asm_defines.h"
#include "capri-macros.h"

struct rx_table_s4_t1_k k;
struct rx_table_s4_t1_ipsec_rxdma_stats_update_d d;
struct phv_ p;

%%
        .align
esp_ipv4_tunnel_h2n_rxdma_ipsec_update_rx_stats:
    tbladd.f d.h2n_rx_pkts, 1
    and r6, k.ipsec_to_stage4_flags, IPSEC_N2H_GLOBAL_FLAGS
    seq c1, r6, IPSEC_N2H_GLOBAL_FLAGS
    bcf [c1], esp_ipv4_tunnel_h2n_rxdma_disbale_dma_cmds
    nop
    phvwri p.{dma_cmd_phv2mem_ipsec_int_dma_cmd_phv_end_addr...dma_cmd_phv2mem_ipsec_int_dma_cmd_type}, ((IPSEC_INT_END_OFFSET << 18) | (IPSEC_INT_START_OFFSET << 8) | IPSEC_PHV2MEM_CACHE_ENABLE | CAPRI_DMA_COMMAND_PHV_TO_MEM)
    phvwri p.{dma_cmd_pkt2mem_dma_cmd_cache...dma_cmd_pkt2mem_dma_cmd_type}, (IPSEC_MEM2PKT_CACHE_ENABLE | CAPRI_DMA_COMMAND_PKT_TO_MEM)
    phvwri p.{tail_2_bytes_dma_cmd_phv_end_addr...tail_2_bytes_dma_cmd_type}, ((IPSEC_TAIL_2_BYTES_PHV_END << 18) | (IPSEC_TAIL_2_BYTES_PHV_START << 8) | IPSEC_PHV2MEM_CACHE_ENABLE | CAPRI_DMA_COMMAND_PHV_TO_MEM)
    phvwri p.{dma_cmd_iv_salt_dma_cmd_phv_end_addr...dma_cmd_iv_salt_dma_cmd_type}, ((IPSEC_IN_DESC_IV_SALT_END << 18) | (IPSEC_IN_DESC_IV_SALT_START << 8) | IPSEC_PHV2MEM_CACHE_ENABLE | CAPRI_DMA_COMMAND_PHV_TO_MEM)
    phvwri p.{dma_cmd_out_desc_aol_dma_cmd_phv_end_addr...dma_cmd_out_desc_aol_dma_cmd_type}, ((IPSEC_OUT_DESC_AOL_END << 18) | (IPSEC_OUT_DESC_AOL_START << 8) | IPSEC_PHV2MEM_CACHE_ENABLE | CAPRI_DMA_COMMAND_PHV_TO_MEM)
    phvwri p.{dma_cmd_fill_esp_hdr_dma_cmd_phv_end_addr...dma_cmd_fill_esp_hdr_dma_cmd_type}, ((IPSEC_ESP_HDR_PHV_END << 18) | (IPSEC_ESP_HDR_PHV_START << 8) | IPSEC_PHV2MEM_CACHE_ENABLE | CAPRI_DMA_COMMAND_PHV_TO_MEM)
    phvwri p.{dma_cmd_pkt2mem_dma_cmd_cache...dma_cmd_pkt2mem_dma_cmd_type}, (IPSEC_MEM2PKT_CACHE_ENABLE | CAPRI_DMA_COMMAND_PKT_TO_MEM)
    phvwri.e p.app_header_table2_valid, 0
    nop

esp_ipv4_tunnel_h2n_rxdma_disbale_dma_cmds:
    phvwri p.dma_cmd_phv2mem_ipsec_int_dma_cmd_type, 0
    phvwri p.dma_cmd_pkt2mem_dma_cmd_type, 0
    phvwri p.dma_cmd_iv_salt_dma_cmd_type, 0
    phvwri p.dma_cmd_out_desc_aol_dma_cmd_type, 0
    phvwri p.dma_cmd_fill_esp_hdr_dma_cmd_type, 0
    phvwri p.dma_cmd_pad_byte_src_dma_cmd_type, 0
    phvwri p.dma_cmd_pad_byte_dst_dma_cmd_type, 0
    phvwri p.dma_cmd_in_desc_aol_dma_cmd_type, 0
    phvwri p.dma_cmd_iv_dma_cmd_type, 0
    phvwri.e p.tail_2_bytes_dma_cmd_type, 0
    nop 
