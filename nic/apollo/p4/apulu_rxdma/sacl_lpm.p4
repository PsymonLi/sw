#include "rxlpm1.p4"
#include "rxlpm2.p4"

action rxlpm1_res_handler()
{
    if (lpm_metadata.root_count == 0) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid0, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid0, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid0, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid0, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 1) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid1, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid1, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid1, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid1, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 2) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid2, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid2, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid2, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid2, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 3) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid3, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid3, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid3, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid3, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 4) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid4, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid4, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid4, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid4, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 5) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.sport_classid5, scratch_metadata.field8);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.sip_classid5, scratch_metadata.field10);
        } else {
        // This is an STAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.stag_count == 0) {
                modify_field(rx_to_tx_hdr.stag0_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 1) {
                modify_field(rx_to_tx_hdr.stag1_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 2) {
                modify_field(rx_to_tx_hdr.stag2_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.stag_count == 3) {
                modify_field(rx_to_tx_hdr.stag3_classid5, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.stag4_classid5, scratch_metadata.field10);
            }}}}
            // Just added a valid STAG classid. Increment count.
            modify_field(lpm_metadata.stag_count, lpm_metadata.stag_count + 1);
        }}}
    }}}}}}

    // Disable further lookups for this LPM
    modify_field(p4_to_rxdma.lpm1_enable, FALSE);
}

action rxlpm2_res_handler()
{
    if (lpm_metadata.root_count == 0) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid0, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid0, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid0, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid0, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid0, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 1) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid1, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid1, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid1, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid1, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid1, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 2) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid2, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid2, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid2, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid2, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid2, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 3) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid3, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid3, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid3, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid3, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid3, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 4) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid4, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid4, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid4, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid4, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid4, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    } else {
    if (lpm_metadata.root_count == 5) {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
            modify_field(rx_to_tx_hdr.dport_classid5, scratch_metadata.field10);
        } else {
        if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
            modify_field(rx_to_tx_hdr.dip_classid5, scratch_metadata.field10);
        } else {
        // This is an DTAG result. Compact only the non default classids.
        if (scratch_metadata.field10 != SACL_CLASSID_DEFAULT) {
            if (lpm_metadata.dtag_count == 0) {
                modify_field(rx_to_tx_hdr.dtag0_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 1) {
                modify_field(rx_to_tx_hdr.dtag1_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 2) {
                modify_field(rx_to_tx_hdr.dtag2_classid5, scratch_metadata.field10);
            } else {
            if (lpm_metadata.dtag_count == 3) {
                modify_field(rx_to_tx_hdr.dtag3_classid5, scratch_metadata.field10);
            } else {
                modify_field(rx_to_tx_hdr.dtag4_classid5, scratch_metadata.field10);
            }}}}
            // Just added a valid DTAG classid. Increment count.
            modify_field(lpm_metadata.dtag_count, lpm_metadata.dtag_count + 1);
        }}}
    }}}}}}

    // Disable further lookups for this LPM
    modify_field(p4_to_rxdma.lpm2_enable, FALSE);
}

action setup_lpm_stag_dtag()
{
    if (lpm_metadata.sacl_base_addr != 0) {
        if (scratch_metadata.stag != SACL_TAG_RSVD) {
            // Setup LPM1 for STAG lookup
            modify_field(lpm_metadata.lpm1_base_addr, lpm_metadata.sacl_base_addr +
                         SACL_STAG_TABLE_OFFSET);
            modify_field(lpm_metadata.lpm1_key, scratch_metadata.stag);
            modify_field(p4_to_rxdma.lpm1_enable, TRUE);
        } else {
            modify_field(p4_to_rxdma.lpm1_enable, FALSE);
        }

        if (scratch_metadata.dtag != SACL_TAG_RSVD) {
            // Setup LPM2 for DTAG lookup
            modify_field(lpm_metadata.lpm2_base_addr, lpm_metadata.sacl_base_addr +
                         SACL_DTAG_TABLE_OFFSET);
            modify_field(lpm_metadata.lpm2_key, scratch_metadata.dtag);
            modify_field(p4_to_rxdma.lpm2_enable, TRUE);
        } else {
            modify_field(p4_to_rxdma.lpm2_enable, FALSE);
        }
    } else {
        modify_field(p4_to_rxdma.lpm1_enable, FALSE);
        modify_field(p4_to_rxdma.lpm2_enable, FALSE);
    }
}

action setup_lpm_sport_dport()
{
    if (lpm_metadata.sacl_base_addr != 0) {
        // Setup LPM1 for SPORT lookup
        modify_field(lpm_metadata.lpm1_base_addr, lpm_metadata.sacl_base_addr +
                     SACL_SPORT_TABLE_OFFSET);
        modify_field(lpm_metadata.lpm1_key, p4_to_rxdma.flow_sport);
        modify_field(p4_to_rxdma.lpm1_enable, TRUE);

        // Setup LPM2 for DPORT lookup
        modify_field(lpm_metadata.lpm2_base_addr, lpm_metadata.sacl_base_addr +
                     SACL_PROTO_DPORT_TABLE_OFFSET);
        modify_field(lpm_metadata.lpm2_key, (p4_to_rxdma.flow_dport |
                                            (p4_to_rxdma.flow_proto << 24)));
        modify_field(p4_to_rxdma.lpm2_enable, TRUE);
    } else {
        modify_field(p4_to_rxdma.lpm1_enable, FALSE);
        modify_field(p4_to_rxdma.lpm2_enable, FALSE);
    }
}

action setup_lpm_sip()
{
    if (lpm_metadata.sacl_base_addr != 0) {
        // Setup LPM1 for SIP lookup
        if (p4_to_rxdma.iptype == IPTYPE_IPV4) {
            modify_field(lpm_metadata.lpm1_base_addr, lpm_metadata.sacl_base_addr +
                         SACL_IPV4_SIP_TABLE_OFFSET);
        } else {
            modify_field(lpm_metadata.lpm1_base_addr, lpm_metadata.sacl_base_addr +
                         SACL_IPV6_SIP_TABLE_OFFSET);
        }

        modify_field(lpm_metadata.lpm1_key, p4_to_rxdma.flow_src);
        modify_field(p4_to_rxdma.lpm1_enable, TRUE);
    } else {
        modify_field(p4_to_rxdma.lpm1_enable, FALSE);
    }
}

action setup_lpm_dip()
{
    if (lpm_metadata.sacl_base_addr != 0) {
        // Setup LPM2 for DIP lookup
        modify_field(lpm_metadata.lpm2_base_addr, lpm_metadata.sacl_base_addr +
                     SACL_DIP_TABLE_OFFSET);
        modify_field(lpm_metadata.lpm2_key, p4_to_rxdma.flow_dst);
        modify_field(p4_to_rxdma.lpm2_enable, TRUE);
    } else {
        modify_field(p4_to_rxdma.lpm2_enable, FALSE);
    }
}

action setup_lpm1()
{
    // Check the state and setup LPMs as necessary.
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_IDLE) {
        // State machine is idle. SACL processing not underway.
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
        setup_lpm_sport_dport();
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
        setup_lpm_dip();
    }

    // Check the look ahead TAGs and preload the next sacl roots if ready.
    if (lpm_metadata.stag == SACL_TAG_RSVD and lpm_metadata.dtag == SACL_TAG_RSVD) {

        // Signal that the root will change.
        modify_field(lpm_metadata.root_change, TRUE);

        if (lpm_metadata.root_count == 0) {
            modify_field(lpm_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr1);
        } else {
        if (lpm_metadata.root_count == 1) {
            modify_field(lpm_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr2);
        } else {
        if (lpm_metadata.root_count == 2) {
            modify_field(lpm_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr3);
        } else {
        if (lpm_metadata.root_count == 3) {
            modify_field(lpm_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr4);
        } else {
        if (lpm_metadata.root_count == 4) {
            modify_field(lpm_metadata.sacl_base_addr, rx_to_tx_hdr.sacl_base_addr5);
        } else {
            modify_field(lpm_metadata.sacl_base_addr, 0);
        }}}}}
    }}}
}

@pragma stage 0
table setup_lpm1 {
    actions {
        setup_lpm1;
    }
}

action setup_lpm2()
{
    if (lpm_metadata.root_change == TRUE) {
        // Root has changed. Increment root count
        modify_field(lpm_metadata.root_count, lpm_metadata.root_count + 1);
        // New SACL root. Transition to the first state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT);
        // Initialize tag counts
        modify_field(lpm_metadata.stag_count, 0);
        modify_field(lpm_metadata.dtag_count, 0);
        // Setup LPMs for sport_dport(). Done in setup1().
        modify_field(lpm_metadata.root_change, FALSE);
    } else {
    // Switch on the current state, process, and transition to the next state.
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SPORT_DPORT) {
        // Done with ports. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_SIP_DIP);
        // Setup LPMs for IPs (dip done in setup1()).
        setup_lpm_sip();
        // Preload STAG0/DTAG0 (look ahead)
        modify_field(lpm_metadata.stag, lpm_metadata.stag0);
        modify_field(lpm_metadata.dtag, lpm_metadata.dtag0);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_SIP_DIP) {
        // Done with IPs. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_STAG0_DTAG0);
        // Setup LPMs for STAG0/DTAG0.
        modify_field(scratch_metadata.stag, lpm_metadata.stag0);
        modify_field(scratch_metadata.dtag, lpm_metadata.dtag0);
        setup_lpm_stag_dtag();
        // Preload STAG1/DTAG1 (look ahead)
        modify_field(lpm_metadata.stag, lpm_metadata.stag1);
        modify_field(lpm_metadata.dtag, lpm_metadata.dtag1);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_STAG0_DTAG0) {
        // Done with STAG0/DTAG0. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_STAG1_DTAG1);
        // Setup LPMs for STAG1/DTAG1.
        modify_field(scratch_metadata.stag, lpm_metadata.stag1);
        modify_field(scratch_metadata.dtag, lpm_metadata.dtag1);
        setup_lpm_stag_dtag();
        // Preload STAG2/DTAG2 (look ahead)
        modify_field(lpm_metadata.stag, lpm_metadata.stag2);
        modify_field(lpm_metadata.dtag, lpm_metadata.dtag2);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_STAG1_DTAG1) {
        // Done with STAG1/DTAG1. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_STAG2_DTAG2);
        // Setup LPMs for STAG2/DTAG2.
        modify_field(scratch_metadata.stag, lpm_metadata.stag2);
        modify_field(scratch_metadata.dtag, lpm_metadata.dtag2);
        setup_lpm_stag_dtag();
        // Preload STAG3/DTAG3 (look ahead)
        modify_field(lpm_metadata.stag, lpm_metadata.stag3);
        modify_field(lpm_metadata.dtag, lpm_metadata.dtag3);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_STAG2_DTAG2) {
        // Done with STAG2/DTAG2. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_STAG3_DTAG3);
        // Setup LPMs for STAG3/DTAG3.
        modify_field(scratch_metadata.stag, lpm_metadata.stag3);
        modify_field(scratch_metadata.dtag, lpm_metadata.dtag3);
        setup_lpm_stag_dtag();
        // Preload STAG4/DTAG4 (look ahead)
        modify_field(lpm_metadata.stag, lpm_metadata.stag4);
        modify_field(lpm_metadata.dtag, lpm_metadata.dtag4);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_STAG3_DTAG3) {
        // Done with STAG3/DTAG3. Transition to the next state.
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_STAG4_DTAG4);
        // Setup LPMs for STAG4/DTAG4.
        modify_field(scratch_metadata.stag, lpm_metadata.stag4);
        modify_field(scratch_metadata.dtag, lpm_metadata.dtag4);
        setup_lpm_stag_dtag();
        // Preload end markers
        modify_field(lpm_metadata.stag, SACL_TAG_RSVD);
        modify_field(lpm_metadata.dtag, SACL_TAG_RSVD);
    } else {
    if (lpm_metadata.sacl_proc_state == SACL_PROC_STATE_STAG4_DTAG4) {
        // For completeness. This case will never happen since
        // lpm_metadata.root_change will always be TRUE in this
        // case, and this will be captured at the top if().
        modify_field(lpm_metadata.sacl_proc_state, SACL_PROC_STATE_SPORT_DPORT);
    }}}}}}}}
}

@pragma stage 7
table setup_lpm2 {
    actions {
        setup_lpm2;
    }
}

control sacl_lpm {

    if (p4_to_rxdma.vnic_info_en == FALSE) {
        apply(setup_lpm1);
    }

    if (p4_to_rxdma.lpm1_enable == TRUE) {
        rxlpm1();
    }

    if (p4_to_rxdma.lpm2_enable == TRUE) {
        rxlpm2();
    }

    if (p4_to_rxdma.mapping_done == TRUE) {
        apply(setup_lpm2);
    }
}
