#include "INGRESS_p.h"
#include "ingress.h"
#include "ipsec_asm_defines.h"
#include "capri-macros.h"

struct tx_table_s3_t0_k k;
struct tx_table_s3_t0_esp_v4_tunnel_n2h_txdma2_load_pad_size_and_l4_proto_d d;
struct phv_ p;

%%
        .align
        .param esp_v4_tunnel_n2h_txdma2_build_decap_packet
        .param esp_ipv4_tunnel_n2h_txdma2_ipsec_update_tx_stats

esp_ipv4_tunnel_n2h_txdma2_load_pad_size_l4_proto:
    sub r2, k.ipsec_to_stage3_block_size, 1
    and r1, d.pad_size, r2 
    phvwr p.txdma2_global_pad_size, r1 

    phvwri p.p4_intr_global_tm_oport, TM_OPORT_P4INGRESS
    phvwri p.p4_intr_global_tm_iport, TM_OPORT_DMA
    phvwri p.p4_intr_global_lif, ARM_CPU_LIF
    phvwri p.ipsec_to_stage4_dot1q_etype, DOT1Q_ETYPE

    phvwri p.{app_header_table0_valid...app_header_table1_valid}, 3
    phvwri p.{common_te0_phv_table_lock_en...common_te0_phv_table_raw_table_size}, 14 
    phvwri p.common_te0_phv_table_pc, esp_v4_tunnel_n2h_txdma2_build_decap_packet[33:6]
    phvwr  p.common_te0_phv_table_addr, k.ipsec_to_stage3_ipsec_cb_addr

    add r3, k.ipsec_to_stage3_ipsec_cb_addr, IPSEC_N2H_STATS_CB_OFFSET
    phvwri p.{common_te1_phv_table_lock_en...common_te1_phv_table_raw_table_size}, 14 
    phvwri p.common_te1_phv_table_pc, esp_ipv4_tunnel_n2h_txdma2_ipsec_update_tx_stats[33:6]
    phvwr  p.common_te1_phv_table_addr, r3 
    
    nop.e
    nop

