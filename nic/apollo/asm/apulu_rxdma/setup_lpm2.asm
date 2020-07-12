#include "nic/apollo/asm/include/apulu.h"
#include "../../p4/include/apulu_sacl_defines.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_setup_lpm2_k.h"

struct phv_             p;
struct setup_lpm2_k_    k;

%%

setup_lpm2:
    // Check if root_change and branch to handle it
    seq              c1, k.lpm_metadata_root_change, TRUE
    bcf              [c1], handle_root_change
    // Check state and branch to handle it
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    bcf              [c1], handle_sport_dport
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    bcf              [c1], handle_sip_dip
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG0_DTAG0
    bcf              [c1], handle_stag0_dtag0
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG1_DTAG1
    bcf              [c1], handle_stag1_dtag1
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG2_DTAG2
    bcf              [c1], handle_stag2_dtag2
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG3_DTAG3
    bcf              [c1], handle_stag3_dtag3
    nop
    nop.e
    nop

handle_root_change:
    // Root has changed. Increment root count
    add              r1, k.lpm_metadata_root_count, 1
    phvwr            p.lpm_metadata_root_count, r1
    phvwr.e          p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr            p.lpm_metadata_root_change, FALSE

handle_sport_dport:
    bal              r7, setup_lpm_sip
    // Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.e          p.lpm_metadata_stag, k.lpm_metadata_stag0
    phvwr            p.lpm_metadata_dtag, k.lpm_metadata_dtag0

handle_sip_dip:
    // Done with IPs. Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG0_DTAG0
    // Setup LPMs for STAG0/DTAG0.
    add              r3, r0, k.lpm_metadata_stag0
    bal              r7, setup_lpm_stag_dtag
    add              r4, r0, k.lpm_metadata_dtag0
    // Preload STAG1/DTAG1 (look ahead)
    phvwr.e          p.lpm_metadata_stag, k.lpm_metadata_stag1
    phvwr            p.lpm_metadata_dtag, k.lpm_metadata_dtag1

handle_stag0_dtag0:
    // Done with STAG0/DTAG0. Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG1_DTAG1
    // Setup LPMs for STAG1/DTAG1.
    add              r3, r0, k.lpm_metadata_stag1
    bal              r7, setup_lpm_stag_dtag
    add              r4, r0, k.lpm_metadata_dtag1
    // Preload STAG2/DTAG2 (look ahead)
    phvwr.e          p.lpm_metadata_stag, k.lpm_metadata_stag2
    phvwr            p.lpm_metadata_dtag, k.lpm_metadata_dtag2

handle_stag1_dtag1:
    // Done with STAG1/DTAG1. Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG2_DTAG2
    // Setup LPMs for STAG2/DTAG2.
    add              r3, r0, k.lpm_metadata_stag2
    bal              r7, setup_lpm_stag_dtag
    add              r4, r0, k.lpm_metadata_dtag2
    // Preload STAG3/DTAG3 (look ahead)
    phvwr.e          p.lpm_metadata_stag, k.lpm_metadata_stag3
    phvwr            p.lpm_metadata_dtag, k.lpm_metadata_dtag3

handle_stag2_dtag2:
    // Done with STAG2/DTAG2. Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG3_DTAG3
    // Setup LPMs for STAG3/DTAG3.
    add              r3, r0, k.lpm_metadata_stag3
    bal              r7, setup_lpm_stag_dtag
    add              r4, r0, k.lpm_metadata_dtag3
    // Preload STAG4/DTAG4 (look ahead)
    phvwr.e          p.lpm_metadata_stag, k.lpm_metadata_stag4
    phvwr            p.lpm_metadata_dtag, k.lpm_metadata_dtag4

handle_stag3_dtag3:
    // Done with STAG3/DTAG3. Transition to the next state.
    phvwr            p.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_STAG4_DTAG4
    // Setup LPMs for STAG4/DTAG4.
    add              r3, r0, k.lpm_metadata_stag4
    bal              r7, setup_lpm_stag_dtag
    add              r4, r0, k.lpm_metadata_dtag4
    // Preload STAG4/DTAG4 (look ahead)
    phvwr.e          p.lpm_metadata_stag, SACL_TAG_RSVD
    phvwr            p.lpm_metadata_dtag, SACL_TAG_RSVD

setup_lpm_sip:
    // Load sacl_base_address
    add              r1, r0, k.lpm_metadata_sacl_base_addr
    // if sacl_base_address == NULL, disable LPM1 and return!
    seq              c1, r1, r0
    jr.c1            r7
    phvwr.c1         p.p4_to_rxdma_lpm1_enable, FALSE

    // Load SIP table offset into r1 based on IP family
    seq              c1, k.p4_to_rxdma_iptype, IPTYPE_IPV4
    addi.c1          r2, r0, SACL_IPV4_SIP_TABLE_OFFSET
    addi.!c1         r2, r0, SACL_IPV6_SIP_TABLE_OFFSET

    // Setup root for SIP lookup on LPM1
    add              r2, r1, r2
    phvwr            p.lpm_metadata_lpm1_base_addr, r2

    // Setup key for SIP lookup on LPM1
    phvwr            p.lpm_metadata_lpm1_key[127:64], k.p4_to_rxdma_flow_src[127:64]
    phvwr            p.lpm_metadata_lpm1_key[63:0],  k.p4_to_rxdma_flow_src[63:0]

    // Enable LPM1 and return
    jr               r7
    phvwr            p.p4_to_rxdma_lpm1_enable, TRUE

setup_lpm_stag_dtag:
    // Load sacl_base_address
    add              r1, r0, k.lpm_metadata_sacl_base_addr
    // if sacl_base_address == NULL, disable both LPMs and return!
    seq              c1, r1, r0
    phvwr.c1         p.p4_to_rxdma_lpm1_enable, FALSE
    jr.c1            r7
    phvwr.c1         p.p4_to_rxdma_lpm2_enable, FALSE

    // is stag valid ?
    sne              c1, r3, SACL_TAG_RSVD
    // If so, Setup root and Key for STAG lookup on LPM1
    addi.c1          r2, r1, SACL_STAG_TABLE_OFFSET
    phvwr.c1         p.lpm_metadata_lpm1_base_addr, r2
    phvwr.c1         p.lpm_metadata_lpm1_key, r3
    phvwr.c1         p.p4_to_rxdma_lpm1_enable, TRUE
    // Else, disable LPM1!
    phvwr.!c1        p.p4_to_rxdma_lpm1_enable, FALSE

    // is dtag valid ?
    sne              c1, r4, SACL_TAG_RSVD
    // If so, Setup root and Key for DTAG lookup on LPM2
    addi.c1          r2, r1, SACL_DTAG_TABLE_OFFSET
    phvwr.c1         p.lpm_metadata_lpm2_base_addr, r2
    phvwr.c1         p.lpm_metadata_lpm2_key, r4
    phvwr.c1         p.p4_to_rxdma_lpm2_enable, TRUE
    jr               r7
    // Else, disable LPM2!
    phvwr.!c1        p.p4_to_rxdma_lpm2_enable, FALSE

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
setup_lpm2_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
