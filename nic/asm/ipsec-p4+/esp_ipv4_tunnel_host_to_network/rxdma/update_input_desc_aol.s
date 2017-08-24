#include "ingress.h"
#include "INGRESS_p.h"
#include "ipsec_defines.h"

struct rx_table_s3_t0_k k;
struct rx_table_s3_t0_rx_table_s3_t0_cfg_action_d d;
struct phv_ p;

%%
        .param ipsec_cb_tail_enqueue_input_desc
        .align

update_input_desc_aol:
    phvwri p.p42p4plus_hdr_table0_valid, 1
    addi r2, r0, ipsec_cb_tail_enqueue_input_desc
    srl r2, r2, 6
    phvwr p.common_te0_phv_table_pc, r2 
    phvwri p.common_te0_phv_table_raw_table_size, 7
    phvwri p.common_te0_phv_table_lock_en, 0
    phvwr p.common_te0_phv_table_addr, k.ipsec_to_stage3_tail_desc_addr 

    phvwr p.barco_desc_in_A0_addr, k.t0_s2s_in_page_addr 
    // get the correct way of giving it as a single 14 bit field
    //phvwr p.barco_desc_in_L0, k.ipsec_global_frame_size
    phvwri p.barco_desc_in_O0, 0

    phvwri p.barco_desc_in_A1_addr, 0
    phvwri p.barco_desc_in_L1, 0
    phvwri p.barco_desc_in_O1, 0

    phvwri p.barco_desc_in_A2_addr, 0
    phvwri p.barco_desc_in_L2, 0
    phvwri p.barco_desc_in_O2, 0

    phvwri p.barco_desc_in_NextAddr, 0
    phvwri p.barco_desc_in_Reserved, 0
