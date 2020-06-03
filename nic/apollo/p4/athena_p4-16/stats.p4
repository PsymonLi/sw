control p4i_statistics(inout cap_phv_intr_global_h capri_intrinsic,
	      inout cap_phv_intr_p4_h intr_p4,
	      inout headers hdr,
	      inout metadata_t metadata) {


    @name(".p4i_stats") action p4i_stats_a(@__ref bit<64>  rx_from_host,
					   @__ref bit<64>  rx_from_switch,
					   @__ref bit<64>  rx_from_arm,
					   @__ref bit<64>  rx_user_csum_err,
					   @__ref bit<64> rx_substrate_csum_err,
					   @__ref bit<64> rx_malformed
					 ) {
      if((capri_intrinsic.error_bits != 0) || (metadata.prs.geneve_option_len_error == 1)) {
	rx_malformed = rx_malformed + 1;	            
	capri_intrinsic.drop = 1;
      }
      
      if(metadata.cntrl.from_arm == TRUE) {
	rx_from_arm = rx_from_arm + 1;
      } else {
	bool csum_err = false;
	if((capri_intrinsic.csum_engine0_error |
	    capri_intrinsic.csum_engine1_error |
	    capri_intrinsic.csum_engine2_error |
	    capri_intrinsic.csum_engine3_error |
	    capri_intrinsic.csum_engine4_error )  
	   != 0) {
	  csum_err = true;
	}

	if(metadata.cntrl.direction == TX_FROM_HOST) {
	  rx_from_host = rx_from_host + 1;
	  /*	  
	  if((capri_intrinsic.csum_engine0_error |
	      capri_intrinsic.csum_engine1_error |
	      capri_intrinsic.csum_engine2_error |
	      capri_intrinsic.csum_engine3_error |
	      capri_intrinsic.csum_engine4_error )  
	     != 0) {
	  */
	  if(csum_err == true) {
	    rx_user_csum_err = rx_user_csum_err + 1;
	    capri_intrinsic.drop = 1;
	  }	  
	  
	} else {
	  rx_from_switch = rx_from_switch + 1;
	  // check if no csum and return otherwise update counters
	  if(csum_err == false) {
	    return;
	  } else {	  

	    capri_intrinsic.drop = 1;
	    if((ipv4HdrCsum_1.get_validate_status() == 1) || (udpCsum_1.get_validate_status() == 1)) { //Checksum engine error
		rx_substrate_csum_err = rx_substrate_csum_err + 1;
	    } else {
	      rx_user_csum_err = rx_user_csum_err + 1;	
	    }
	    
	    /*
	    if(hdr.ip_1.ipv4.isValid()) {
	      if(ipv4HdrCsum_1.get_validate_status() == 0) {
		rx_substrate_csum_err = rx_substrate_csum_err + 1;
		capri_intrinsic.drop = 1;
	      }
	      if(hdr.ip_2.ipv4.isValid()) {
		if(ipv4HdrCsum_2.get_validate_status() == 0) {
		  rx_user_csum_err = rx_user_csum_err + 1;	
		  capri_intrinsic.drop = 1;
		}	      
	      }
	      if(hdr.l4_u.udp.isValid()) {
		if(udpCsum_2.get_validate_status() == 0) {
		  rx_user_csum_err = rx_user_csum_err + 1;
		  capri_intrinsic.drop = 1;
		}
	      } else if(hdr.l4_u.tcp.isValid()) {
		if(tcpCsum_2.get_validate_status() == 0) {
		  rx_user_csum_err = rx_user_csum_err + 1;
		  capri_intrinsic.drop = 1;
		}
	      } else if(hdr.l4_u.icmpv4.isValid()) {
		if(icmpv4Csum_2.get_validate_status() == 0) {
		  rx_user_csum_err = rx_user_csum_err + 1;
		  capri_intrinsic.drop = 1;
		}
	      } else if(hdr.l4_u.icmpv6.isValid()) {
		if(icmpv6Csum_2.get_validate_status() == 0) {
		  rx_user_csum_err = rx_user_csum_err + 1;
		  capri_intrinsic.drop = 1;
		}
	      }
	    
	    }
	    */
	  }
	} 
      }
    }


 @name(".p4i_stats_error")
  action p4i_stats_error() {
    intr_p4.setValid();
    capri_intrinsic.drop = 1;
 //   capri_intrinsic.debug_trace = 1;
    if (capri_intrinsic.tm_oq != TM_P4_RECIRC_QUEUE) {
      capri_intrinsic.tm_iq = capri_intrinsic.tm_oq;
    }
    
  }
 
    @name(".p4i_stats") table p4i_stats {
        key = {
	  metadata.cntrl.stats_id : table_index;
        }
        actions = {
            p4i_stats_a;
        }
        size = 1;
        placement = HBM;
        default_action = p4i_stats_a;
	error_action = p4i_stats_error;
        stage = 5;
    }

    apply {
      p4i_stats.apply();
    }
}

control p4e_statistics(inout cap_phv_intr_global_h capri_intrinsic,
	      inout cap_phv_intr_p4_h intr_p4,
	      inout headers hdr,
	      inout metadata_t metadata) {


    @name(".p4e_stats") action p4e_stats_a(@__ref bit<64>  tx_to_host,
  					 @__ref bit<64>  tx_to_switch,
 					   @__ref bit<64>  tx_to_arm,
					   @__ref bit<64>  nacl_drop
					 ) {
      if((metadata.cntrl.p4e_stats_flag & P4E_STATS_FLAG_TX_TO_HOST) == P4E_STATS_FLAG_TX_TO_HOST) {
	tx_to_host = tx_to_host + 1;
      } 
      if((metadata.cntrl.p4e_stats_flag & P4E_STATS_FLAG_TX_TO_SWITCH) == P4E_STATS_FLAG_TX_TO_SWITCH) {
	tx_to_switch = tx_to_switch + 1;
      } 
      if((metadata.cntrl.p4e_stats_flag & P4E_STATS_FLAG_TX_TO_ARM) == P4E_STATS_FLAG_TX_TO_ARM) {
	tx_to_arm = tx_to_arm + 1;
      } 

      if(capri_intrinsic.drop == 1) {
        nacl_drop = nacl_drop + 1;
      } 

    }

 @name(".p4i_stats_error")
  action p4i_stats_error() {
    intr_p4.setValid();
    capri_intrinsic.drop = 1;
 //   capri_intrinsic.debug_trace = 1;
    if (capri_intrinsic.tm_oq != TM_P4_RECIRC_QUEUE) {
      capri_intrinsic.tm_iq = capri_intrinsic.tm_oq;
    }
    
  }

 @name(".p4e_stats") table p4e_stats {
   key = {
     metadata.cntrl.stats_id : table_index;
   }
   actions = {
     p4e_stats_a;
   }
   size = 1;
   placement = HBM;
   default_action = p4e_stats_a;
   stage = 5;
 }
 
    apply {
      p4e_stats.apply();
    }
}
