/*
 *      Construct the barco request in this stage for decrypt
 * Stage 5, Table 0
 */

#include "tls-constants.h"
#include "tls-phv.h"
#include "tls-shared-state.h"
#include "tls-macros.h"
#include "tls-table.h"
#include "tls_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
        
struct tx_table_s5_t0_k     k;
struct phv_                 p;
struct tx_table_s5_t0_d     d;
        
%%
            .param      tls_dec_queue_brq_process
            .param      tls_dec_queue_brq_mpp_process
            .param      tls_dec_write_arq
#           .param      BRQ_QPCB_BASE
            .param      ARQRX_BASE
        
tls_dec_bld_barco_req_process:
    seq         c1, k.tls_global_phv_write_arq, r0
    bcf         [!c1], tls_cpu_rx
    nop
table_read_QUEUE_BRQ:
    /* Fill the barco request in the phv to be DMAed later into BRQ slot */
    addi        r2, r0, PKT_DESC_AOL_OFFSET
    add         r1, r2, k.{to_s5_idesc}

    add         r3, r2, k.{to_s5_odesc}
    phvwrpair   p.barco_desc_input_list_address, r1.dx, \
                p.barco_desc_output_list_address, r3.dx

    add         r1, r0, k.{to_s5_odesc}
    phvwr       p.odesc_dma_src_odesc, r1.dx

    /* address will be in r4 */
    addi        r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_PIDX_SET, DB_SCHED_UPD_SET, 0, LIF_TLS)

    /*
     * data will be in r3
     *
     * We maintain a shadow-copy of the BSQ PI in qstate CB which we'll increment
     * and use as 'PIDX_SET' instead of using the 'PIDX_INC' auto-increment feature
     * of the doorbell, for better performance.
     */
    tbladd.f    d.{u.tls_bld_brq5_d.sw_bsq_pi}.hx, 1
    add         r6, r0, d.{u.tls_bld_brq5_d.sw_bsq_pi}.hx
    CAPRI_RING_DOORBELL_DATA(0, k.tls_global_phv_fid, TLS_SCHED_RING_BSQ, r6)
    phvwrpair   p.barco_desc_doorbell_address, r4.dx,   \
                p.barco_desc_doorbell_data, r3.dx

    /*
     * Check if this is AES-CCM decrypt case, which has some differences in the barco
     * request encoding as compared to GCM (barco-command[31:24] value 0x05,
     * endian-swapped).
     */
    bbeq        k.tls_global_phv_do_pre_ccm_dec, 1, tls_dec_bld_barco_req_ccm_process
    nop
        
    phvwr       p.s4_s6_t0_phv_aad_seq_num, d.u.tls_bld_brq5_d.sequence_no
    tbladd      d.u.tls_bld_brq5_d.sequence_no, 1

    add         r2, r0, k.{to_s5_idesc}
    addi        r1, r0, NTLS_AAD_SIZE
    phvwrpair   p.barco_desc_header_size, r1.wx,    \
                p.barco_desc_status_address, r2.dx

tls_dec_bld_barco_req_process_done:

    /*
     * NOTE: The next stage program tls_dec_queue_brq_process does NOT need a table-read anymore,
     *	     as we have already read a shadow-copy of Barco PI to be written to.
     */	
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tls_dec_queue_brq_process);
        nop.e
        nop

tls_cpu_rx:

    addui       r5, r0, hiword(ARQRX_BASE)
    addi        r5, r5, loword(ARQRX_BASE)
    phvwr       p.s5_s6_t1_s2s_arq_base, r5

    CPU_ARQ_SEM_INF_ADDR(0, r4)

    CAPRI_NEXT_TABLE_READ(1, 
                          TABLE_LOCK_DIS,
                          tls_dec_write_arq,
                          r4,
                          TABLE_SIZE_64_BITS)
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tls_dec_queue_brq_process)
    nop.e
    nop

    b        tls_dec_bld_barco_req_process_done
    nop

tls_dec_bld_barco_req_ccm_process:
    phvwri      p.ccm_header_with_aad_B_0_flags, TLS_AES_CCM_HDR_B0_FLAGS
        
    add         r1, r0, NTLS_AAD_SIZE
    /* FIXME: Misnomer, this is actually the sequence number */
    phvwrpair   p.ccm_header_with_aad_B_1_aad_size, r1, \
                p.ccm_header_with_aad_B_1_aad_seq_num, d.u.tls_bld_brq5_d.sequence_no
    tbladd      d.u.tls_bld_brq5_d.sequence_no, 1


    // PHV is already zeroed out
    //phvwri      p.ccm_header_with_aad_B_1_zero_pad, 0
        
    addi        r1, r0, TLS_AES_CCM_HEADER_SIZE
    phvwr       p.barco_desc_header_size, r1.wx

tls_dec_queue_to_brq_mpp_ring:

    /*
     * NOTE: The next stage program tls_dec_queue_brq_mpp_process does NOT need a table-read anymore,
     *	     as we have already read a shadow-copy of Barco PI to be written to.
     */
    CAPRI_NEXT_TABLE_READ_NO_TABLE_LKUP(0, tls_dec_queue_brq_mpp_process);
        nop.e
        nop
