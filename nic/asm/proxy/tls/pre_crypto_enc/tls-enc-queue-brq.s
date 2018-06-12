/*
 * 	Implements the writing of request to BRQ
 *  Stage 5, Table 0
 */

#include "tls-constants.h"
#include "tls-phv.h"
#include "tls-shared-state.h"
#include "tls-macros.h"
#include "tls-table.h"
#include "ingress.h"
#include "INGRESS_p.h"        
#include "tls_common.h"

struct phv_ p	;
struct tx_table_s5_t0_k k	;
struct tx_table_s5_t0_d d;
	
%%
	.param		BRQ_BASE
    .param      tls_enc_pre_crypto_stats_process
        
tls_enc_queue_brq_process:
    smeqb       c5, k.tls_global_phv_debug_dol, TLS_DDOL_BYPASS_BARCO, TLS_DDOL_BYPASS_BARCO

    CAPRI_SET_DEBUG_STAGE4_7(p.to_s6_debug_stage4_7_thread, CAPRI_MPU_STAGE_5, CAPRI_MPU_TABLE_0)
    CAPRI_CLEAR_TABLE0_VALID
	phvwr		p.p4_txdma_intr_dma_cmd_ptr, (CAPRI_PHV_START_OFFSET(dma_cmd_bsq_slot_dma_cmd_type) / 16)

dma_cmd_enc_desc_entry0:
    /* Skip setting up of odesc when bypassing Barco offload */
    bcf         [c5], dma_cmd_enc_aad
    phvwri.c5    p.dma_cmd_odesc_dma_cmd_type, 0

	add		    r5, r0, k.to_s5_odesc
	addi		r5, r5, PKT_DESC_AOL_OFFSET
	phvwr		p.dma_cmd_odesc_dma_cmd_addr, r5

	addi		r4, r0, NIC_PAGE_HEADROOM
    add         r6, r0, k.{to_s5_opage}
	phvwrpair   p.odesc_A0, r6.dx, \
	            p.odesc_O0, r4.wx

    /* odesc_L0 already setup in Stage 2, Table 3 */

	/* r1 = d.cur_tls_data_len + TLS_HDR_SIZE */

    /* Setup Auth Tag Address */
    add         r1, r0, k.to_s5_opage
    add         r1, r1, r4 /* offset */
    addi        r1, r1, NTLS_AAD_SIZE
    add         r1, r1, k.s2_s5_t0_phv_aad_length
    phvwr       p.barco_desc_auth_tag_addr, r1.dx

    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_odesc_dma_cmd, r5, odesc_A0, odesc_next_pkt)


dma_cmd_enc_aad:

    phvwri.c5    p.dma_cmd_aad_dma_cmd_type, 0

dma_cmd_iv:
    bcf         [c5], dma_cmd_enc_brq_slot
    phvwri.c5    p.dma_cmd_iv_dma_cmd_type, 0

    add         r5, r0, k.to_s5_opage

    phvwr       p.barco_desc_iv_address, r5.dx

    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_iv_dma_cmd, r5, crypto_iv_salt, crypto_iv_explicit_iv)

dma_cmd_enc_brq_slot:
    bcf         [c5], dma_cmd_idesc
    phvwri.c5    p.dma_cmd_brq_slot_dma_cmd_type, 0

    /*
     * We've already obtained the Barco GCM0 PI via a global locked-table read in
     * earlier stage.
     */
    //add         r7, r0, d.{u.tls_queue_brq5_d.pi}.wx
    add           r7, r0, k.to_s5_sw_barco_pi

    sll		    r5, r7, NIC_BRQ_ENTRY_SIZE_SHIFT
	/* Set the DMA_WRITE CMD for BRQ slot */
	addui		r1, r0, hiword(BRQ_BASE)
	addi		r1, r1, loword(BRQ_BASE)
	add		    r1, r1, r5

	/* Fill the barco request */        
    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_brq_slot_dma_cmd, r1, barco_desc_input_list_address,
                                barco_desc_second_key_desc_index)
dma_cmd_idesc:
    bcf         [c5], dma_cmd_output_list_addr
    phvwri.c5    p.dma_cmd_idesc_dma_cmd_type, 0

    /* Already setup in Stage 2, Table 0 */
    add         r1, r0, k.to_s5_idesc
    addi        r1, r1, PKT_DESC_AOL_OFFSET
    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_idesc_dma_cmd, r1, idesc_A0, idesc_next_pkt)

dma_cmd_output_list_addr:
	add		    r5, r0, k.to_s5_idesc
	addi		r5, r5, 4

    /* For Barco use output descriptor */
    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_idesc_meta_dma_cmd, r5, barco_desc_output_list_address,
                                barco_desc_output_list_address)

    bcf         [!c5], dma_cmd_ring_bsq_doorbell_skip
    nop

    /* for Barco bypass - use input descriptor */
    CAPRI_DMA_CMD_PHV2MEM_SETUP(dma_cmd_idesc_meta_dma_cmd, r5, barco_desc_input_list_address,
                                barco_desc_input_list_address)
        
dma_cmd_ring_bsq_doorbell:

    CAPRI_DMA_CMD_RING_DOORBELL(dma_cmd_dbell_dma_cmd, LIF_TLS, 0, k.tls_global_phv_fid, TLS_SCHED_RING_BSQ, 0,
                                crypto_iv_explicit_iv)
    CAPRI_DMA_CMD_STOP_FENCE(dma_cmd_dbell_dma_cmd)
    b           tls_queue_brq_process_done
    nop

dma_cmd_ring_bsq_doorbell_skip: 

dma_cmd_brq_doorbell:

    /* Barco DMA Channel PI in r7 */
    //addi        r7, r7, 1
    //and         r7, r7, CAPRI_BARCO_RING_SLOTS_MASK
    mincr       r7, CAPRI_BARCO_RING_SLOTS_SHIFT, 1
    phvwr       p.barco_dbell_pi, r7.wx


    CAPRI_DMA_CMD_PHV2MEM_SETUP_I(dma_cmd_dbell_dma_cmd, CAPRI_BARCO_MD_HENS_REG_GCM0_PRODUCER_IDX,
                                  barco_dbell_pi, barco_dbell_pi)

    CAPRI_DMA_CMD_STOP_FENCE(dma_cmd_dbell_dma_cmd)


tls_queue_brq_process_done:
	CAPRI_NEXT_TABLE_READ_OFFSET(0, TABLE_LOCK_DIS, tls_enc_pre_crypto_stats_process,
	                    k.tls_global_phv_qstate_addr, TLS_TCB_PRE_CRYPTO_STATS_OFFSET,
                        TABLE_SIZE_512_BITS)
	nop.e
	nop
