rxlpm2_res_handler:
    /* Disable further stages for LPM2 */
    phvwr            p.p4_to_rxdma_lpm2_enable, FALSE
    /* Check root count and branch to process the right root */
    seq              c1, k.lpm_metadata_root_count, 0
    bcf              [c1], root0
    seq              c1, k.lpm_metadata_root_count, 1
    bcf              [c1], root1
    seq              c1, k.lpm_metadata_root_count, 2
    bcf              [c1], root2
    seq              c1, k.lpm_metadata_root_count, 3
    bcf              [c1], root3
    seq              c1, k.lpm_metadata_root_count, 4
    bcf              [c1], root4
    seq              c1, k.lpm_metadata_root_count, 5
    bcf              [c1], root5
    nop
    nop.e
    nop

root0:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid0, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid0, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid0, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid0, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid0, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid0, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid0, res_reg
    nop.e
    nop

root1:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid1, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid1, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid1, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid1, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid1, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid1, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid1, res_reg
    nop.e
    nop

root2:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid2, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid2, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid2, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid2, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid2, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid2, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid2, res_reg
    nop.e
    nop

root3:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid3, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid3, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid3, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid3, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid3, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid3, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid3, res_reg
    nop.e
    nop

root4:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid4, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid4, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid4, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid4, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid4, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid4, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid4, res_reg
    nop.e
    nop

root5:
    /* Check state and write the result in the correct place*/
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT
    phvwr.c1.e       p.rx_to_tx_hdr_dport_classid5, res_reg
    seq              c1, k.lpm_metadata_sacl_proc_state, SACL_PROC_STATE_SIP_DIP
    phvwr.c1.e       p.rx_to_tx_hdr_dip_classid5, res_reg
    /* This is an STAG result. Compact only the non default classids. */
    seq              c1, res_reg, SACL_CLASSID_DEFAULT
    nop.c1.e
    add              r1, k.lpm_metadata_dtag_count, 1
    phvwr            p.lpm_metadata_dtag_count, r1
    seq              c1, k.lpm_metadata_dtag_count, 0
    phvwr.c1.e       p.rx_to_tx_hdr_dtag0_classid5, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 1
    phvwr.c1.e       p.rx_to_tx_hdr_dtag1_classid5, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 2
    phvwr.c1.e       p.rx_to_tx_hdr_dtag2_classid5, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 3
    phvwr.c1.e       p.rx_to_tx_hdr_dtag3_classid5, res_reg
    seq              c1, k.lpm_metadata_dtag_count, 4
    phvwr.c1.e       p.rx_to_tx_hdr_dtag4_classid5, res_reg
    nop.e
    nop
