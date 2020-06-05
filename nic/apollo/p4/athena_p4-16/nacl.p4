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
        DROP_PACKET_INGRESS(P4I_DROP_NACL);
    }
    
    @name(".nacl_permit")
      action nacl_permit() {
      if(!__table_hit()) {
      	DROP_PACKET_INGRESS(P4I_DROP_NACL);
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
        stage = 3;
    }


    apply {
        nacl.apply();
    }

}

