#include "INGRESS_p.h"
#include "ingress.h"
#include "ipsec_asm_defines.h"

struct tx_table_s0_t0_k k;
struct tx_table_s0_t0_esp_v4_tunnel_n2h_txdma1_initial_table_d d;
struct phv_ p;

%%
        .param esp_v4_tunnel_n2h_get_in_desc_from_cb_cindex
        .param esp_v4_tunnel_n2h_load_part2
        .param IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
        .param IPSEC_DEC_DB_ADDR_NOP
        .align
esp_ipv4_tunnel_n2h_txdma_initial_table:
    //seq c6, d.is_v6, 0xFF
    //bcf [c6], txdma1_freeze2
    //nop

    add r4, d.barco_pindex, 1
    and r4, r4, IPSEC_BARCO_RING_INDEX_MASK 
    seq c1, d.barco_cindex, r4
    bcf [c1], esp_v4_tunnel_n2h_barco_ring_error
    nop
    phvwr p.ipsec_to_stage3_barco_pindex, d.barco_pindex
    phvwr p.ipsec_to_stage1_barco_pindex, d.barco_pindex

    tblmincri d.barco_pindex, IPSEC_BARCO_RING_WIDTH, 1
    and r2, d.{rxdma_ring_cindex}.hx, IPSEC_CB_RING_INDEX_MASK

    tblmincri.f     d.{rxdma_ring_cindex}.hx, IPSEC_PER_CB_RING_WIDTH, 1
    phvwr p.txdma1_global_cb_cindex, d.{rxdma_ring_cindex}.hx

    phvwr p.ipsec_to_stage1_barco_cb_ring_base_addr, d.barco_ring_base_addr
    phvwr p.ipsec_to_stage1_ipsec_cb_index, d.ipsec_cb_index
    phvwr p.txdma1_global_ipsec_cb_addr, k.{p4_txdma_intr_qstate_addr_sbit0_ebit1...p4_txdma_intr_qstate_addr_sbit2_ebit33} 
    phvwri p.barco_req_command, IPSEC_BARCO_DECRYPT_AES_GCM_256_LE
    phvwr p.t0_s2s_iv_size, d.iv_size

    sll r2, r2, IPSEC_CB_RING_ENTRY_SHIFT_SIZE 
    add r2, r2, d.cb_ring_base_addr
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_EN, esp_v4_tunnel_n2h_get_in_desc_from_cb_cindex, r2, TABLE_SIZE_64_BITS)
    add r4, k.{p4_txdma_intr_qstate_addr_sbit0_ebit1...p4_txdma_intr_qstate_addr_sbit2_ebit33}, 64
    CAPRI_NEXT_TABLE_READ(2, TABLE_LOCK_EN, esp_v4_tunnel_n2h_load_part2, r4, TABLE_SIZE_64_BITS)
    //addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
    //CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, N2H_TXDMA1_ENTER_OFFSET, 1)
    seq c1, d.{rxdma_ring_pindex}.hx, d.{rxdma_ring_cindex}.hx
    b.!c1 esp_ipv4_tunnel_n2h_txdma1_initial_table_do_nothing2 
    addi r4, r0, IPSEC_DEC_DB_ADDR_NOP
    CAPRI_RING_DOORBELL_DATA(0, d.ipsec_cb_index, 0, 0)
    memwr.dx  r4, r3
    nop.e
    nop

esp_ipv4_tunnel_n2h_txdma1_initial_table_do_nothing:
    phvwri p.p4_intr_global_drop, 1
esp_ipv4_tunnel_n2h_txdma1_initial_table_do_nothing2:
    nop.e
    nop


esp_ipv4_tunnel_n2h_txdma_initial_table_drop_pkt:
    phvwri p.p4_intr_global_drop, 1
    addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, N2H_TXDMA1_ENTER_DROP_OFFSET, 1)
    nop.e
    nop

esp_v4_tunnel_n2h_barco_ring_error:
    addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, N2H_TXDMA1_BARCO_RING_FULL_OFFSSET, 1)
    phvwri p.p4_intr_global_drop, 1
    nop.e
    nop

txdma1_freeze2:
    addi r7, r0, IPSEC_GLOBAL_BAD_DMA_COUNTER_BASE_N2H
    CAPRI_ATOMIC_STATS_INCR1_NO_CHECK(r7, N2H_TXDMA1_FREEZE_OFFSET, 1)
    phvwri p.p4_intr_global_drop, 1
    nop.e
    nop
    
