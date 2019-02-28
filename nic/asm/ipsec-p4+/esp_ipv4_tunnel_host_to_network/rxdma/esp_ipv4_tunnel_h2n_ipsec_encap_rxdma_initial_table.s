#include "INGRESS_p.h"
#include "ingress.h"
#include "ipsec_asm_defines.h"

struct common_p4plus_stage0_app_header_table_ipsec_encap_rxdma_initial_table_d d;
struct common_p4plus_stage0_app_header_table_k k;
struct phv_ p;

%%
        .param          IPSEC_PAD_BYTES_HBM_TABLE_BASE
        .param          esp_ipv4_tunnel_h2n_rxmda_ring_full_error
        .param          esp_ipv4_tunnel_h2n_allocate_input_desc_semaphore
        .param          IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_H2N
        .align 
esp_ipv4_tunnel_h2n_ipsec_encap_rxdma_initial_table:
    add r1, d.cb_pindex, 1
    and r1, r1, IPSEC_CB_RING_INDEX_MASK 
    seq c5, d.cb_cindex, r1
    bcf [c5], esp_ipv4_tunnel_h2n_ipsec_encap_rxdma_initial_table_cb_ring_full
    phvwr p.ipsec_global_ipsec_cb_pindex, d.cb_pindex 
    tbladd d.esn_lo, 1
    tbladd d.iv, 1
    tblmincri.f d.cb_pindex, IPSEC_PER_CB_RING_WIDTH, 1
    phvwr p.ipsec_int_header_payload_start, k.{p42p4plus_hdr_ipsec_payload_start_sbit0_ebit7, p42p4plus_hdr_ipsec_payload_start_sbit8_ebit15}
    // I understand that I need to take care of 32 bit overflow into esn-hi etc.
    smeqb c1, d.flags, IPSEC_FLAGS_V6_MASK, IPSEC_FLAGS_V6_MASK 
    cmov r1, c1, IPV6_HDR_SIZE+ESP_FIXED_HDR_SIZE, IPV4_HDR_SIZE+ESP_FIXED_HDR_SIZE
    add r2, r1, d.iv_size
    add r2, r2, k.p42p4plus_hdr_ipsec_payload_end
    phvwr p.ipsec_int_header_tailroom_offset, r2
    sub r3, k.p42p4plus_hdr_ipsec_payload_end, k.{p42p4plus_hdr_ipsec_payload_start_sbit0_ebit7, p42p4plus_hdr_ipsec_payload_start_sbit8_ebit15}
    add.c1 r3, r3, IPV6_HDR_SIZE
    phvwr p.t1_s2s_payload_size, r3
    phvwrpair p.t0_s2s_payload_size, r3, p.t0_s2s_payload_start, k.{p42p4plus_hdr_ipsec_payload_start_sbit0_ebit7, p42p4plus_hdr_ipsec_payload_start_sbit8_ebit15}
    sub r6, d.block_size, 1
    and r4, r3, r6 
    sub r5, d.block_size, r4
    slt c2, r5, 2
    add.c2 r5, r5, d.block_size 
    subi r5, r5, 2
    phvwrpair p.ipsec_to_stage3_pad_size, r5, p.ipsec_to_stage3_iv_size, d.iv_size
    addui r6, r0, hiword(IPSEC_PAD_BYTES_HBM_TABLE_BASE)
    addi r6, r6, loword(IPSEC_PAD_BYTES_HBM_TABLE_BASE)
    phvwr p.ipsec_to_stage3_pad_addr, r6
    phvwr p.ipsec_int_header_payload_size, r3
    phvwrpair p.ipsec_int_header_pad_size, r5, p.ipsec_int_header_l4_protocol, k.p42p4plus_hdr_l4_protocol
    add  r1, r0, k.p42p4plus_hdr_ipsec_payload_end
    add.c1 r1, r1, IPV6_HDR_SIZE 
    phvwr p.ipsec_to_stage3_packet_len, r1
    addi r5, r0, INDESC_SEMAPHORE_ADDR
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, esp_ipv4_tunnel_h2n_allocate_input_desc_semaphore, r5, TABLE_SIZE_64_BITS) 
    addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_H2N
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, H2N_RXDMA_ENTER_OFFSET, 1)
    phvwr.e p.t1_s2s_ipsec_cb_addr, k.{p4_rxdma_intr_qstate_addr_sbit0_ebit1...p4_rxdma_intr_qstate_addr_sbit2_ebit33}
    nop

esp_ipv4_tunnel_h2n_ipsec_encap_rxdma_initial_table_cb_ring_full:
    add r5, k.{p4_rxdma_intr_qstate_addr_sbit0_ebit1...p4_rxdma_intr_qstate_addr_sbit2_ebit33}, IPSEC_H2N_STATS_CB_OFFSET 
    CAPRI_NEXT_TABLE_READ(2, TABLE_LOCK_EN, esp_ipv4_tunnel_h2n_rxmda_ring_full_error, r5, TABLE_SIZE_512_BITS) 
    nop.e
    nop
