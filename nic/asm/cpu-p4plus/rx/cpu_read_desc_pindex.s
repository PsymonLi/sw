#include "INGRESS_p.h"
#include "ingress.h"
#include "capri-macros.h"
#include "cpu-table.h"

struct phv_ p;
struct cpu_rx_read_cpu_desc_k k;
struct cpu_rx_read_cpu_desc_d d;

%%
    .param cpu_rx_desc_alloc_start
    .param RNMDPR_BIG_TABLE_BASE
    .align

cpu_rx_read_desc_pindex_start:
    CAPRI_CLEAR_TABLE1_VALID
    add     r4, r0, d.{u.read_cpu_desc_d.desc_pindex}.wx
    andi    r4, r4, ((1 << CAPRI_RNMDPR_BIG_RING_SHIFT) - 1)
    phvwr   p.s2_t1_s2s_desc_pindex, r4 

table_read_desc_alloc:
    addui   r3, r0, hiword(RNMDPR_BIG_TABLE_BASE)
    addi    r3, r3, loword(RNMDPR_BIG_TABLE_BASE)
    sll     r1, r4, RNMDPR_BIG_TABLE_ENTRY_SIZE_SHFT
    add     r1, r1, r3

    phvwri  p.common_te1_phv_table_lock_en, 0

    // Read 512 bits if the pindex is byte aligned
    smeqb   c1, r4, 0x7, 0x0
    phvwri.c1  p.common_te1_phv_table_raw_table_size, TABLE_SIZE_512_BITS 
    phvwri.!c1 p.common_te1_phv_table_raw_table_size, TABLE_SIZE_64_BITS 
    
    phvwri  p.common_te1_phv_table_pc, cpu_rx_desc_alloc_start[33:6]
    phvwr   p.common_te1_phv_table_addr, r1 
    phvwri  p.app_header_table1_valid, 1
	nop.e
	nop 

