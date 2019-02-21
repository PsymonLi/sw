/*****************************************************************************
 *  seq_comp_status_desc1_handler:
 *****************************************************************************/

#include "storage_asm_defines.h"
#include "ingress.h"
#include "INGRESS_p.h"


struct s1_tbl1_k k;
struct s1_tbl1_seq_comp_status_desc1_handler_d d;
struct phv_ p;

/*
 * Registers usage:
 */
#define r_src_qaddr                 r7  // for SEQ_METRICS_TABLE_COMMIT

%%
    SEQ_METRICS_PARAMS()

storage_seq_comp_status_desc1_handler_start:

    // Relaunch metrics commit for table 1
    SEQ_METRICS0_TABLE1_COMMIT(SEQ_KIVEC5_SRC_QADDR)

    phvwrpair	p.seq_kivec5_data_len, d.data_len[16:0], \
                p.{seq_kivec5_stop_chain_on_error...seq_kivec5_padding_en}, \
    	        d.{stop_chain_on_error...padding_en}
    phvwrpair	p.seq_kivec3_comp_buf_addr, d.comp_buf_addr, \
                p.seq_kivec3_pad_boundary_shift, d.pad_boundary_shift
    phvwr       p.comp_hdr_version, d.hdr_version

if0:
    bbne        d.aol_update_en, 1, else0
    phvwr       p.seq_kivec4_pad_boundary_shift, d.pad_boundary_shift // delay slot
then0:

    // AOL update is enabled
    phvwrpair	p.seq_kivec6_aol_src_vec_addr, d.aol_src_vec_addr, \
                p.seq_kivec6_aol_dst_vec_addr, d.aol_dst_vec_addr
                
    // If enabled, SGL update and PDMA would be expected to have been
    // configured as follows:
    //
    // sgl_update_en=1 && sgl_pdma_en=1 && sgl_pdma_pad_only=1: 
    //         d.sgl_vec_addr points to barco_sgl_t and only pad data
    //         would be PDMA'ed after pad calculation.
    // sgl_update_en=0 && sgl_pdma_en=1 && sgl_pdma_pad_only=0: 
    //         d.sgl_vec_addr points to chain_sgl_pdma_t
    //         only both compressed data and pad data would be PDMA'ed

    b           endif0                
    phvwrpair 	p.seq_kivec2_sgl_pdma_dst_addr, d.sgl_vec_addr, \
                p.seq_kivec2_sgl_vec_addr, d.sgl_vec_addr       // delay slot
else0:

    // AOL update is disabled:
    // SGL update and/or SGL PDMA might be enabled, in which case,
    //         d.aol_dst_vec_addr would point to chain_sgl_pdma_t, and
    //         d.sgl_vec_addr would point to barco_sgl_t
    
    phvwrpair	p.seq_kivec2_sgl_pdma_dst_addr, d.aol_dst_vec_addr, \
                p.seq_kivec2_sgl_vec_addr, d.sgl_vec_addr
endif0:                
    
    phvwrpair   p.seq_kivec5_pad_buf_addr, d.pad_buf_addr[33:0], \
                p.seq_kivec5_alt_data_len, d.alt_data_len[16:0]
    seq         c3, d.padding_en, 1
    phvwr.e     p.seq_kivec8_alt_buf_addr, d.alt_buf_addr
    phvwr.!c3   p.seq_kivec5_pad_buf_addr, r0                   // delay slot

