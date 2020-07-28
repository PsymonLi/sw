/******************************************************************************/
/* Ingr pipeline                                                                */
/******************************************************************************/

control ingress_policers(inout cap_phv_intr_global_h intr_global,
            inout cap_phv_intr_p4_h intr_p4,
            inout headers hdr,
            inout metadata_t metadata) {

    @name(".policer_pps")
    action policer_pps_a (   bit<1>     entry_valid,
                             bit<1>     pkt_rate,
                             bit<1>     rlimit_en,
                             bit<2>     rlimit_prof,
                             bit<1>     color_aware,
                             bit<1>     rsvd,
                             bit<1>     axi_wr_pend,
                             bit<40>    burst,
                             bit<40>    rate,
                             bit<40>    tbkt) {
    if ((entry_valid == TRUE) && ((tbkt >> 39) == 1)) {
      DROP_PACKET_INGRESS(P4I_DROP_POLICER);
    }
   }

    @name(".policer_pps")
      table policer_pps {
        key = {
            metadata.key.vnic_id : exact;
        }
        actions  = {
            policer_pps_a;
        }
        size =  POLICER_PPS_SIZE;
        policer = two_color;
        placement = SRAM;
        default_action = policer_pps_a;
        stage = 2;
    }

    apply {
      if(metadata.cntrl.skip_pps_policer == FALSE) {
	policer_pps.apply();
      }
    }
}

/******************************************************************************/
/* Egr pipeline                                                                */
/******************************************************************************/

control policers(inout cap_phv_intr_global_h intr_global,
            inout cap_phv_intr_p4_h intr_p4,
            inout headers hdr,
            inout metadata_t metadata) {

    @name(".policer_bw1")
    action policer_bw1_a (   bit<1>     entry_valid,
                             bit<1>     pkt_rate,
                             bit<1>     rlimit_en,
                             bit<2>     rlimit_prof,
                             bit<1>     color_aware,
                             bit<1>     rsvd,
                             bit<1>     axi_wr_pend,
                             bit<40>    burst,
                             bit<40>    rate,
                             bit<40>    tbkt) {
    if ((entry_valid == TRUE) && ((tbkt >> 39) == 1)) {
      DROP_PACKET_EGRESS(P4E_DROP_POLICER);
      metadata.cntrl.skip_flow_log = TRUE;		   
    }
   }

    @name(".policer_bw1")
    table policer_bw1 {
        key = {
            metadata.cntrl.throttle_bw1_id : table_index;
        }
        actions  = {
            policer_bw1_a;
        }
        size =  POLICER_BW1_SIZE;
        policer = two_color;
        placement = SRAM;
        default_action = policer_bw1_a;
        stage = 2;
    }



    @name(".policer_bw2")
      action policer_bw2_a(bit<1>     entry_valid,
                             bit<1>    pkt_rate,
                             bit<1>     rlimit_en,
                             bit<2>    rlimit_prof,
                             bit<1>     color_aware,
                             bit<1>     rsvd,
                             bit<1>     axi_wr_pend,
                             bit<40>    burst,
                             bit<40>    rate,
                             bit<40>    tbkt) {
    if ((entry_valid == TRUE) && ((tbkt >> 39) == 1)) {
      DROP_PACKET_EGRESS(P4E_DROP_POLICER);
      metadata.cntrl.skip_flow_log = TRUE;		   

    }
   }

    @name(".policer_bw2")
    table policer_bw2 {
        key = {
            metadata.cntrl.throttle_bw2_id : table_index;
        }
        actions  = {
            policer_bw2_a;
        }
        size =  POLICER_BW2_SIZE;
        policer = two_color;
        placement = SRAM;
        default_action = policer_bw2_a;
        stage = 4;
    }

    apply {
      if(metadata.cntrl.throttle_bw1_id_valid == TRUE) {
        policer_bw1.apply();
      }
      if(metadata.cntrl.throttle_bw2_id_valid == TRUE) {
        policer_bw2.apply();
      }
    }
}


