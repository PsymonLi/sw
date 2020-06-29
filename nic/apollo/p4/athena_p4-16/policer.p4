/******************************************************************************/
/* Egr pipeline                                                                */
/******************************************************************************/

control policers(inout cap_phv_intr_global_h capri_intrinsic,
            inout cap_phv_intr_p4_h intr_p4,
            inout headers hdr,
            inout metadata_t metadata) {

    @name(".policer_bw1")
    action policer_bw1_a (   bit<1>     entry_valid,
                             bit<16>    pkt_rate,
                             bit<1>     rlimit_en,
                             bit<16>    rlimit_prof,
                             bit<1>     color_aware,
                             bit<1>     rsvd,
                             bit<1>     axi_wr_pend,
                             bit<16>    burst,
                             bit<16>    rate,
                             bit<40>    tbkt) {
    if ((entry_valid == TRUE) && ((tbkt >> 39) == 1)) {
      DROP_PACKET_EGRESS(P4E_DROP_POLICER);
      metadata.cntrl.skip_flow_log = TRUE;		   
    }
   }

    @name(".policer_bw1")
    table policer_bw1 {
        key = {
            metadata.cntrl.throttle_bw1_id : exact;
        }
        actions  = {
            policer_bw1_a;
        }
        size =  POLICER_BW1_SIZE;
        policer = two_color;
        placement = SRAM;
        stage = 2;
    }



    @name(".policer_bw2")
      action policer_bw2_a(bit<1>     entry_valid,
                             bit<16>    pkt_rate,
                             bit<1>     rlimit_en,
                             bit<16>    rlimit_prof,
                             bit<1>     color_aware,
                             bit<1>     rsvd,
                             bit<1>     axi_wr_pend,
                             bit<16>    burst,
                             bit<16>    rate,
                             bit<40>    tbkt) {
    if ((entry_valid == TRUE) && ((tbkt >> 39) == 1)) {
      DROP_PACKET_EGRESS(P4E_DROP_POLICER);
      metadata.cntrl.skip_flow_log = TRUE;		   

    }
   }

    @name(".policer_bw2")
    table policer_bw2 {
        key = {
            metadata.cntrl.throttle_bw2_id : exact;
        }
        actions  = {
            policer_bw2_a;
        }
        size =  POLICER_BW2_SIZE;
        policer = two_color;
        placement = SRAM;
        stage = 3;
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


