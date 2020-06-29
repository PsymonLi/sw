/******************************************************************************/
/* Rx pipeline                                                                */
/******************************************************************************/
control nacl_lookup(inout cap_phv_intr_global_h capri_intrinsic,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

   
   @name(".nop") action nop() {
   }
    
 
    @name(".nacl_drop")
      action nacl_drop() {
        DROP_PACKET_EGRESS(P4E_DROP_NACL);
	metadata.cntrl.skip_flow_log = TRUE;		   
    }
    
    @name(".nacl_permit")
      action nacl_permit() {
      if(!__table_hit()) {
      	DROP_PACKET_EGRESS(P4E_DROP_NACL);
        metadata.cntrl.skip_flow_log = TRUE;		   
      } 
    }

    
    @name(".nacl_redirect")
      action nacl_redirect( bit <1> redir_type,
			    bit <4> app_id,
			    bit <4> oport,
			    bit <11> lif,
			    bit <3>  qtype,
 			    bit <24> qid) {

	metadata.cntrl.redir_type = redir_type;
	metadata.cntrl.redir_oport = oport;
	metadata.cntrl.redir_lif = lif;
	metadata.cntrl.redir_qtype = qtype;
	metadata.cntrl.redir_qid = qid;
	metadata.cntrl.redir_app_id = app_id;
	capri_intrinsic.drop = 0;
	metadata.cntrl.p4i_drop_reason = 0;
        metadata.cntrl.skip_flow_log = TRUE;		   
	metadata.cntrl.p4e_stats_flag = P4E_STATS_FLAG_TX_TO_ARM; //TO BE COMPLETED 
    }
    
    @name(".nacl_error")
      action nacl_error() {
      capri_intrinsic.drop = 1;
      
    }
    
    
    @name(".nacl") table nacl {
        key = {
            metadata.cntrl.direction        : ternary;
            metadata.cntrl.flow_miss        : ternary;
 	    capri_intrinsic.lif : ternary;
        }
        actions = {
	  nacl_drop;
	  nacl_permit;
          nacl_redirect;
        }
        size = NACL_TABLE_SIZE;
        default_action = nacl_drop;
	error_action = nacl_error;
        stage = 2;
    }


    @name(".mirroring_nacl")
      action mirroring_nacl_a(bit<6> session_number) {
      metadata.cntrl.mirroring_valid = TRUE;
      metadata.cntrl.mirroring_session = session_number;
      //TBD setting intrinsic
    }

    @name(".mirroring_nacl") table mirroring_nacl {
        key = {
            metadata.flow_log_key.src           : ternary;
            metadata.flow_log_key.dst           : ternary;
            metadata.flow_log_key.proto         : ternary;
            metadata.flow_log_key.sport         : ternary;
            metadata.flow_log_key.dport         : ternary;
            metadata.flow_log_key.vnic_id       : ternary;
            metadata.flow_log_key.ktype         : ternary;
            metadata.flow_log_key.disposition   : ternary;
            metadata.cntrl.flow_miss            : ternary;
            metadata.cntrl.redir_oport  : ternary;
        }
        actions = {
	  nop;
	  mirroring_nacl_a;
        }
        size = MIRRORING_NACL_TABLE_SIZE;
        default_action = nop;
        stage = 2;
    }


    apply {
        nacl.apply();
	//TBD proper skip on span packets
	if(metadata.cntrl.mirroring_en == TRUE) {
	  mirroring_nacl.apply();
	}
    }

}

