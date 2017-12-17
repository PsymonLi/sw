#include "app_redir_common.h"
#include "../../../../cpu-p4plus/include/cpu-macros.h"

struct phv_                                             p;
struct rawr_chain_qidxr_k                               k;
struct rawr_chain_qidxr_chain_qidxr_pindex_post_read_d  d;

/*
 * Registers usage
 */
#define r_chain_pindex              r1  // must match rawr_chain_xfer.s
#define r_ring_select               r2  // ring index select

 
%%
    .param      rawr_s6_chain_xfer
    
    .align

/*
 * Next service chain queue index table post read handling.
 * Upon entry in this stage, the table has been locked
 * allowing for atomic read-update.
 */
rawr_s6_chain_qidxr_pindex_post_read:

    CAPRI_CLEAR_TABLE2_VALID
        
    /*
     * TODO: check for queue full
     */    
    /*
     * Evaluate which per-core queue applies
     */
    add         r_ring_select, r0, k.common_phv_chain_ring_index_select
    CPU_ARQ_PIDX_READ_INC(r_chain_pindex, r_ring_select, d, pi_0, r3, r4)
    j           rawr_s6_chain_xfer
    mincr       r_chain_pindex, k.{common_phv_chain_ring_size_shift_sbit0_ebit1...\
                                   common_phv_chain_ring_size_shift_sbit2_ebit4}, r0 // delay slot

