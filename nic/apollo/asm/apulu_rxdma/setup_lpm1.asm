#include "nic/apollo/asm/include/apulu.h"
#include "../../p4/include/apulu_sacl_defines.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_setup_lpm1_k.h"

struct phv_             p;
struct setup_lpm1_k_    k;

%%

setup_lpm1:
    // If the state is idle, SACL processing is not underway. Go to end.
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_IDLE
    bcf              [c1], end
    // If the state is SPORT/DPORT. Setup the LPMs for SPORT/DPORT and stop.
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    bcf              [c1], setup_lpm_sport_dport
    // If the state is SIP/DIP. Setup the LPM2 for DIP and come back for more.
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    bal.c1           r7, setup_lpm_dip
    nop
    // Check the look ahead TAGs.
    seq              c1, k.lpm_metadata_stag, SACL_TAG_RSVD
    seq              c2, k.lpm_metadata_dtag, SACL_TAG_RSVD
    // If one of the tags is real, then go to end.
    bcf              [!c1 | !c2], end
    // Otherwise (both tags are done), preload the right sacl root to process next
    seq              c1, k.lpm_metadata_root_count, 0
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, k.rx_to_tx_hdr_sacl_base_addr1
    phvwr            p.lpm_metadata_root_change, TRUE
    seq              c1, k.lpm_metadata_root_count, 1
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, k.rx_to_tx_hdr_sacl_base_addr2
    seq              c1, k.lpm_metadata_root_count, 2
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, k.rx_to_tx_hdr_sacl_base_addr3
    seq              c1, k.lpm_metadata_root_count, 3
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, k.rx_to_tx_hdr_sacl_base_addr4
    seq              c1, k.lpm_metadata_root_count, 4
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, k.rx_to_tx_hdr_sacl_base_addr5
    seq              c1, k.lpm_metadata_root_count, 5
    phvwr.c1.e       p.lpm_metadata_sacl_base_addr, 0
end:
    nop.e
    nop

setup_lpm_dip:
    // Load sacl_base_address
    add              r1, r0, k.lpm_metadata_sacl_base_addr
    // if sacl_base_address == NULL, disable LPM2 and return!
    seq              c1, r1, r0
    jr.c1            r7
    phvwr.c1         p.p4_to_rxdma_lpm2_enable, FALSE

    // Setup root for DIP lookup on LPM2
    addi             r2, r1, SACL_DIP_TABLE_OFFSET
    phvwr            p.lpm_metadata_lpm2_base_addr, r2

    // Setup key for DIP lookup on LPM2
    phvwr            p.lpm_metadata_lpm2_key[127:64], k.p4_to_rxdma_flow_dst[127:64]
    phvwr            p.lpm_metadata_lpm2_key[63:0],  k.p4_to_rxdma_flow_dst[63:0]

    // Enable LPM2 and return
    jr               r7
    phvwr            p.p4_to_rxdma_lpm2_enable, TRUE

setup_lpm_sport_dport:
    // Load sacl_base_address
    add              r1, r0, k.lpm_metadata_sacl_base_addr
    // if sacl_base_address == NULL, disable both LPMs and stop!
    seq              c1, r1, r0
    phvwr.c1.e       p.p4_to_rxdma_lpm1_enable, FALSE
    phvwr.c1         p.p4_to_rxdma_lpm2_enable, FALSE

    // Setup root for SPORT lookup on LPM1
    addi             r2, r1, SACL_SPORT_TABLE_OFFSET
    phvwr            p.lpm_metadata_lpm1_base_addr, r2

    // Setup key for SPORT lookup on LPM1
    phvwr            p.lpm_metadata_lpm1_key, k.p4_to_rxdma_flow_sport

    // Setup root for DPORT lookup on LPM2
    addi             r2, r1, SACL_PROTO_DPORT_TABLE_OFFSET
    phvwr            p.lpm_metadata_lpm2_base_addr, r2

    // Setup key for DPORT lookup on LPM2
    phvwr            p.lpm_metadata_lpm2_key[127:24], k.p4_to_rxdma_flow_proto
    phvwr            p.lpm_metadata_lpm2_key[23:0], k.p4_to_rxdma_flow_dport

    // Enable both LPMs and stop
    phvwr.e          p.p4_to_rxdma_lpm1_enable, TRUE
    phvwr            p.p4_to_rxdma_lpm2_enable, TRUE

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
setup_lpm1_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
