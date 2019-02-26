#include "INGRESS_p.h"
#include "ingress.h"
#include "ipsec_asm_defines.h"

struct tx_table_s2_t2_k k;
struct tx_table_s2_t2_esp_v4_tunnel_n2h_txdma2_load_ipsec_int_d d;
struct phv_ p;

%%
        .align
        .param IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
esp_ipv4_tunnel_n2h_txdma2_load_ipsec_int:
    phvwri p.app_header_table2_valid, 0
    seq c1, d.{status_addr}.dx, 0
    bcf [!c1], esp_ipv4_tunnel_n2h_txdma2_load_ipsec_int_phv_drop
    phvwr p.txdma2_global_pad_size, d.pad_size
    phvwr p.txdma2_global_l4_protocol, d.l4_protocol
    phvwr p.txdma2_global_payload_size, d.payload_size
    phvwr p.t0_s2s_headroom_offset, d.headroom_offset
    phvwr p.t0_s2s_tailroom_offset, d.tailroom_offset
    phvwr p.ipsec_to_stage4_in_page, d.in_page
    phvwr.e p.ipsec_to_stage4_headroom, d.headroom
    nop

esp_ipv4_tunnel_n2h_txdma2_load_ipsec_int_phv_drop:
    addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, N2H_LOAD_IPSEC_INT_OFFSET, 1)
    phvwri p.p4_intr_global_drop, 1
    phvwri.e p.{app_header_table0_valid...app_header_table3_valid}, 0
    nop
     
    
