
control vnic(inout cap_phv_intr_global_h intr_global,
            inout cap_phv_intr_p4_h intr_p4,
            inout headers hdr,
            inout metadata_t metadata) {

    @name(".mpls_label_to_vnic")
    action mpls_label_to_vnic_a( bit<1>    vnic_type,
		                 bit<9>   vnic_id) {
      if (vnic_id == 0) {
        /* Skip DNAT and Flow lookup */
	metadata.cntrl.skip_dnat_lkp = TRUE;
	metadata.cntrl.skip_flow_lkp = TRUE;
	metadata.cntrl.skip_l2_flow_lkp = TRUE;
	hdr.ingress_recirc_header.dnat_done = TRUE;
	hdr.ingress_recirc_header.flow_done = TRUE;
	hdr.ingress_recirc_header.l2_flow_done = TRUE;
        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;
	
      } else {		
	metadata.cntrl.vnic_type = vnic_type;
	hdr.p4i_to_p4e_header.l2_vnic = vnic_type;
	if(vnic_type == P4_VNIC_TYPE_L3) {
	  metadata.cntrl.skip_l2_flow_lkp = TRUE;
	  hdr.ingress_recirc_header.l2_flow_done = TRUE;
	}
	metadata.key.vnic_id = vnic_id;
	//	metadata.l2_key.vnic_id16 = (bit<16>)vnic_id;
      }
    }

    @name(".mpls_label_to_vnic_error")
      action mpls_label_to_vnic_error() {
      intr_global.drop = 1;
 //     capri_intrinsic.debug_trace = 1;
      
    }
    

    @hbm_table
    @name(".mpls_label_to_vnic")
    @capi_bitfields_struct
    table  mpls_label_to_vnic{
        key = {
	  //	  hdr.mpls_dst.label : exact;
	   metadata.cntrl.mpls_label_b20_b12 : table_index;
	   metadata.cntrl.mpls_label_b11_b4 : table_index;
	   metadata.cntrl.mpls_label_b3_b0 : table_index;
	  //metadata.cntrl.mpls_vnic_label : table_index;
        }
        actions  = {
          mpls_label_to_vnic_a ;
        }
        size =  MPLS_LABEL_VNIC_MAP_TABLE_SIZE;
        placement = HBM;
        default_action = mpls_label_to_vnic_a;
	error_action = mpls_label_to_vnic_error;
        stage = 0;
    }

    @name(".vlan_to_vnic")
    action vlan_to_vnic_a( bit<1>    vnic_type,
		           bit<9>   vnic_id) {
      if (vnic_id == 0) {
        /* Skip DNAT and Flow lookup */
	metadata.cntrl.skip_dnat_lkp = TRUE;
	metadata.cntrl.skip_flow_lkp = TRUE;
	metadata.cntrl.skip_l2_flow_lkp = TRUE;
	hdr.ingress_recirc_header.dnat_done = TRUE;
	hdr.ingress_recirc_header.flow_done = TRUE;
	hdr.ingress_recirc_header.l2_flow_done = TRUE;

        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;
	
      } else {
		
	metadata.cntrl.vnic_type = vnic_type;
	if(vnic_type == P4_VNIC_TYPE_L3) {
	  metadata.cntrl.skip_l2_flow_lkp = TRUE;
	  hdr.ingress_recirc_header.l2_flow_done = TRUE;
	}
	hdr.p4i_to_p4e_header.l2_vnic = vnic_type;
	metadata.key.vnic_id = vnic_id;
	//	metadata.l2_key.vnic_id16 = (bit<16>)vnic_id;
      }
    }

   @name(".vlan_to_vnic_error")
      action vlan_to_vnic_error() {
      intr_global.drop = 1;
//      capri_intrinsic.debug_trace = 1;
      
    }
 
    @hbm_table
    @capi_bitfields_struct
    @name(".vlan_to_vnic")
    table vlan_to_vnic {
        key = {
            hdr.ctag_1.vid : exact;
	}
        actions  = {
           vlan_to_vnic_a ;
        }
        size =  VLAN_VNIC_MAP_TABLE_SIZE;
        placement = HBM;
        default_action = vlan_to_vnic_a;
	error_action = vlan_to_vnic_error;
        stage = 0;
    }



    apply {
      
      if(metadata.cntrl.direction == TX_FROM_HOST) {
	vlan_to_vnic.apply();
      }

      if(metadata.cntrl.direction == RX_FROM_SWITCH) {
	mpls_label_to_vnic.apply();
      }

    }

}


