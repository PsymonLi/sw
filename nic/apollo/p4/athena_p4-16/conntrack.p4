/******************************************************************************/
/* Tx pipeline                                                                */
/******************************************************************************/
control conntrack_state_update(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }

          
   
   @name(".conntrack")
     action conntrack_a (@__ref bit<1> valid_flag,
			   @__ref bit<2> flow_type,
			   @__ref bit<4> flow_state,
			   @__ref bit<24> timestamp) {
     
         bit<4> nxt_flow_state = flow_state;
	 bit<48> current_time;
	 current_time = __current_time();
	 timestamp = current_time[46:23];
	 if(flow_type == CONNTRACK_FLOW_TYPE_TCP) {
	   /*  TCP  */
	 } 
	 
	 flow_state = nxt_flow_state;
	 
   }
   
   
    @hbm_table    
    @capi_bitfields_struct
    @name(".conntrack") table conntrack {
        key = {
            metadata.cntrl.conntrack_index  : exact;

        }
        actions = {
	  conntrack_a;
        }
        size = CONNTRACK_TABLE_SIZE;
        placement = HBM;
	default_action = conntrack_a;
        stage = 1;
    }


    apply {
      if(hdr.p4i_to_p4e_header.conntrack_en == TRUE) {
	conntrack.apply();
      }
    }

}

