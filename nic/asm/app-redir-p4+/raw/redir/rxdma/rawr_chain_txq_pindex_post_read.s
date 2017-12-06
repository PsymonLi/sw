#include "app_redir_common.h"

struct phv_                                                 p;
struct rawr_chain_txq_pindex_k                              k;
struct rawr_chain_txq_pindex_chain_txq_pindex_post_read_d   d;

/*
 * Registers usage
 */
#define r_chain_pindex              r1  // must match rawr_chain_xfer.s
#define r_pi                        r2
#define r_ci                        r3


%%
    .param      rawr_s6_cleanup_discard
    .param      rawr_s6_chain_xfer
    
    .align

/*
 * Next service chain TxQ ring pindex/cindex post read handling.
 * We check for queue full and launch cleanup if so.
 * Otherwise, branch to common code to set up DMA transfers.
 */
rawr_s6_chain_txq_pindex_post_read:

    CAPRI_CLEAR_TABLE1_VALID
        
    /*
     * If next service chain TxQ ring full, launch clean up.
     * Ring is full when PI+1 == CI
     */    
    add         r_pi, r0, d.{pi_curr}.hx
    add         r_chain_pindex, r0, r_pi
    add         r_ci, r0, d.{ci_curr}.hx
    mincr       r_pi, k.{common_phv_chain_ring_size_shift_sbit0_ebit1...\
                         common_phv_chain_ring_size_shift_sbit2_ebit4}, 1
    mincr       r_ci, k.{common_phv_chain_ring_size_shift_sbit0_ebit1...\
                         common_phv_chain_ring_size_shift_sbit2_ebit4}, r0
    beq         r_pi, r_ci, _txq_ring_full_discard

    /*
     * Pass the obtained pindex to a common DMA transfer function via r1
     */
    mincr       r_chain_pindex, k.{common_phv_chain_ring_size_shift_sbit0_ebit1...\
                                   common_phv_chain_ring_size_shift_sbit2_ebit4}, r0 // delay slot
    j           rawr_s6_chain_xfer
    nop


/*
 * TxQ ring full discard
 */                       
_txq_ring_full_discard:

    /*
     * TODO: add stats here
     */
    j           rawr_s6_cleanup_discard
    nop
