#include "ingress.h"
#include "INGRESS_p.h"
#include "ipsec_asm_defines.h"

struct rx_table_s3_t1_k k;
struct rx_table_s3_t1_rx_table_s3_t1_cfg_action_d d;
struct phv_ p;

%%
        .param esp_ipv4_tunnel_n2h_ipsec_cb_tail_enqueue_input_desc
        .param esp_ipv4_tunnel_n2h_rxdma_ipsec_update_rx_stats
        .align

esp_ipv4_tunnel_n2h_update_output_desc_aol:
    phvwri p.{app_header_table0_valid...app_header_table1_valid}, 3
    phvwr p.barco_desc_out_A0_addr, k.{t1_s2s_out_page_addr}.dx 
    add r2, k.ipsec_to_stage3_payload_size, ESP_FIXED_HDR_SIZE
    phvwr p.barco_desc_out_L0, r2.wx 
    phvwri p.common_te0_phv_table_pc, esp_ipv4_tunnel_n2h_ipsec_cb_tail_enqueue_input_desc[33:6]
    phvwri p.{common_te0_phv_table_lock_en...common_te0_phv_table_raw_table_size}, 14
    phvwr p.common_te0_phv_table_addr, k.ipsec_to_stage3_ipsec_cb_addr

    phvwri p.common_te1_phv_table_pc, esp_ipv4_tunnel_n2h_rxdma_ipsec_update_rx_stats[33:6]
    phvwri p.{common_te1_phv_table_lock_en...common_te1_phv_table_raw_table_size}, 14
    add r1, k.ipsec_to_stage3_ipsec_cb_addr, IPSEC_N2H_STATS_CB_OFFSET
    phvwr.e p.common_te1_phv_table_addr, r1 
    nop
