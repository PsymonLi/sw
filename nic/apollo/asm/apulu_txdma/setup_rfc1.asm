#include "../../p4/include/apulu_sacl_defines.h"

#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_setup_rfc1_k.h"

struct phv_            p;
struct setup_rfc1_k_   k;

%%

setup_rfc1:
    // Is the current combination done?
    seq              c1, k.txdma_control_recirc_count[0:0], 0x01
    // No. Stop.
    nop.c1.e
    nop

    // Done for combination. Initialize for the next combination.
    seq              c1, k.txdma_control_stag_count, 0
    add.c1           r1, r0, k.rx_to_tx_hdr_stag1_classid0
    seq              c1, k.txdma_control_stag_count, 1
    add.c1           r1, r0, k.rx_to_tx_hdr_stag2_classid0
    seq              c1, k.txdma_control_stag_count, 2
    add.c1           r1, r0, k.rx_to_tx_hdr_stag3_classid0
    seq              c1, k.txdma_control_stag_count, 3
    add.c1           r1, r0, k.rx_to_tx_hdr_stag4_classid0
    seq              c1, k.txdma_control_stag_count, 4
    add.c1           r1, r0, SACL_CLASSID_DEFAULT

    // Is the next STAG is valid.
    sne              c1, r1, SACL_CLASSID_DEFAULT
    // Yes. Increment STAG count, and copy the new STAG to PHV and Stop.
    add.c1           r3, k.txdma_control_stag_count, 1
    phvwr.c1.e       p.txdma_control_stag_count, r3
    phvwr.c1         p.txdma_control_stag_classid, r1

    // Else (STAG is invalid): Reinitialize STAG and Find the next DTAG
    phvwr            p.txdma_control_stag_classid, k.rx_to_tx_hdr_stag0_classid0
    phvwr            p.txdma_control_stag_count, r0
    seq              c1, k.txdma_control_dtag_count, 0
    add.c1           r2, r0, k.rx_to_tx_hdr_dtag1_classid0
    seq              c1, k.txdma_control_dtag_count, 1
    add.c1           r2, r0, k.rx_to_tx_hdr_dtag2_classid0
    seq              c1, k.txdma_control_dtag_count, 2
    add.c1           r2, r0, k.rx_to_tx_hdr_dtag3_classid0
    seq              c1, k.txdma_control_dtag_count, 3
    add.c1           r2, r0, k.rx_to_tx_hdr_dtag4_classid0
    seq              c1, k.txdma_control_dtag_count, 4
    add.c1           r2, r0, SACL_CLASSID_DEFAULT

    // Is the next DTAG is valid.
    sne              c1, r2, SACL_CLASSID_DEFAULT
    // Yes. Increment DTAG count, and copy the new DTAG to PHV and Stop.
    add.c1           r3, k.txdma_control_dtag_count, 1
    phvwr.c1.e       p.txdma_control_dtag_count, r3
    phvwr.c1         p.txdma_control_dtag_classid, r2

    // Else (DTAG is invalid): Reinitialize DTAG
    phvwr            p.txdma_control_dtag_classid, k.rx_to_tx_hdr_dtag0_classid0
    phvwr            p.txdma_control_dtag_count, r0
    // Done for policy root. Handle policy root change
    phvwr            p.txdma_control_root_change, TRUE
    seq              c1, k.txdma_control_root_count, 0
    bcf              [c1], load1
    phvwr.c1         p.rx_to_tx_hdr_sacl_base_addr0, k.rx_to_tx_hdr_sacl_base_addr1
    seq              c1, k.txdma_control_root_count, 1
    bcf              [c1], load2
    phvwr.c1         p.rx_to_tx_hdr_sacl_base_addr0, k.rx_to_tx_hdr_sacl_base_addr2
    seq              c1, k.txdma_control_root_count, 2
    bcf              [c1], load3
    phvwr.c1         p.rx_to_tx_hdr_sacl_base_addr0, k.rx_to_tx_hdr_sacl_base_addr3
    seq              c1, k.txdma_control_root_count, 3
    bcf              [c1], load4
    phvwr.c1         p.rx_to_tx_hdr_sacl_base_addr0, k.rx_to_tx_hdr_sacl_base_addr4
    seq              c1, k.txdma_control_root_count, 4
    bcf              [c1], load5
    phvwr.c1         p.rx_to_tx_hdr_sacl_base_addr0, k.rx_to_tx_hdr_sacl_base_addr5
    b                clearall
    phvwr            p.rx_to_tx_hdr_sacl_base_addr0, 0

load1:
    // Copy the new sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, k.rx_to_tx_hdr_sip_classid1
    add              r3, k.rx_to_tx_hdr_dip_classid1_s6_e9, \
                         k.rx_to_tx_hdr_dip_classid1_s0_e5, 4
    phvwr            p.rx_to_tx_hdr_dip_classid0, r3
    phvwr            p.rx_to_tx_hdr_sport_classid0, k.rx_to_tx_hdr_sport_classid1
    phvwr            p.rx_to_tx_hdr_dport_classid0, k.rx_to_tx_hdr_dport_classid1
    // Load the correct information into regs and go to finish
    add              r1, r0, k.rx_to_tx_hdr_sacl_base_addr1
    add              r2, r0, k.rx_to_tx_hdr_sip_classid1
    b                finish
    add              r3, r0, k.rx_to_tx_hdr_sport_classid1

load2:
    // Copy the new sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, k.rx_to_tx_hdr_sip_classid2
    add              r3, k.rx_to_tx_hdr_dip_classid2_s6_e9, \
                         k.rx_to_tx_hdr_dip_classid2_s0_e5, 4
    phvwr            p.rx_to_tx_hdr_dip_classid0, r3
    phvwr            p.rx_to_tx_hdr_sport_classid0, k.rx_to_tx_hdr_sport_classid2
    phvwr            p.rx_to_tx_hdr_dport_classid0, k.rx_to_tx_hdr_dport_classid2
    // Load the correct information into regs and go to finish
    add              r1, r0, k.rx_to_tx_hdr_sacl_base_addr2
    add              r2, r0, k.rx_to_tx_hdr_sip_classid2
    b                finish
    add              r3, r0, k.rx_to_tx_hdr_sport_classid2

load3:
    // Copy the new sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, k.rx_to_tx_hdr_sip_classid3
    add              r3, k.rx_to_tx_hdr_dip_classid3_s9_e9, \
                         k.rx_to_tx_hdr_dip_classid3_s6_e8, 1
    add              r3, r3, k.rx_to_tx_hdr_dip_classid3_s0_e5, 4
    phvwr            p.rx_to_tx_hdr_dip_classid0, r3
    phvwr            p.rx_to_tx_hdr_sport_classid0, k.rx_to_tx_hdr_sport_classid3
    phvwr            p.rx_to_tx_hdr_dport_classid0, k.rx_to_tx_hdr_dport_classid3
    // Load the correct information into regs and go to finish
    add              r1, r0, k.rx_to_tx_hdr_sacl_base_addr3
    add              r2, r0, k.rx_to_tx_hdr_sip_classid3
    b                finish
    add              r3, r0, k.rx_to_tx_hdr_sport_classid3

load4:
    // Copy the new sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, k.rx_to_tx_hdr_sip_classid4
    add              r3, k.rx_to_tx_hdr_dip_classid4_s6_e9, \
                         k.rx_to_tx_hdr_dip_classid4_s0_e5, 4
    phvwr            p.rx_to_tx_hdr_dip_classid0, r3
    phvwr            p.rx_to_tx_hdr_sport_classid0, k.rx_to_tx_hdr_sport_classid4
    phvwr            p.rx_to_tx_hdr_dport_classid0, k.rx_to_tx_hdr_dport_classid4
    // Load the correct information into regs and go to finish
    add              r1, r0, k.rx_to_tx_hdr_sacl_base_addr4
    add              r2, r0, k.rx_to_tx_hdr_sip_classid4
    b                finish
    add              r3, r0, k.rx_to_tx_hdr_sport_classid4

load5:
    // Copy the new sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, k.rx_to_tx_hdr_sip_classid5
    add              r3, k.rx_to_tx_hdr_dip_classid5_s6_e9, \
                         k.rx_to_tx_hdr_dip_classid5_s0_e5, 4
    phvwr            p.rx_to_tx_hdr_dip_classid0, r3
    phvwr            p.rx_to_tx_hdr_sport_classid0, k.rx_to_tx_hdr_sport_classid5
    phvwr            p.rx_to_tx_hdr_dport_classid0, k.rx_to_tx_hdr_dport_classid5
    // Load the correct information into regs and go to finish
    add              r1, r0, k.rx_to_tx_hdr_sacl_base_addr5
    add              r2, r0, k.rx_to_tx_hdr_sip_classid5
    b                finish
    add              r3, r0, k.rx_to_tx_hdr_sport_classid5

clearall:
    // Clear sacl base addr, sip, dip, sport and dport class ids.
    phvwr            p.rx_to_tx_hdr_sip_classid0, r0
    phvwr            p.rx_to_tx_hdr_dip_classid0, r0
    phvwr            p.rx_to_tx_hdr_sport_classid0, r0
    phvwr            p.rx_to_tx_hdr_dport_classid0, r0
    // Load the correct information into regs and go to finish
    add              r1, r0, r0
    add              r2, r0, r0
    b                finish
    add              r3, r0, r0

finish:
    // r1 : sacl base addr
    // r2 : sip class id
    // r3 : sport class id

    // If the new root is NULL, set root count to signal all roots done, and stop.
    seq              c1, r0, r1
    phvwr.c1.e       p.txdma_control_root_count, SACL_ROOT_COUNT_MAX
    // Else, increment root count, setup p1 table lookup
    add              r4, k.txdma_control_root_count, 1
    phvwr            p.txdma_control_root_count, r4

    // P1 table index = SIP:SPORT.
    add              r7, r3, r2, SACL_SPORT_CLASSID_WIDTH
    // Write P1 table index to PHV
    phvwr            p.txdma_control_rfc_index, r7

    // Load r1 = sacl base addr + SACL_P1_TABLE_OFFSET
    addi             r1, r1, SACL_P1_TABLE_OFFSET
    // Compute the byte offset for P1 table index
    div              r2, r7, SACL_P1_ENTRIES_PER_CACHE_LINE
    mul              r2, r2, SACL_CACHE_LINE_SIZE
    // Add the byte offset to table base
    add.e            r1, r1, r2
    // Write the address back to phv for P1 lookup
    phvwr            p.txdma_control_rfc_table_addr, r1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
setup_rfc1_error:
    phvwr.e          p.capri_intr_drop, 1
    nop
