/******************************************************************************/
/* Rx pipeline                                                                */
/******************************************************************************/
control flow_lookup(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }
    
    @appdatafields ("idx", "idx_type")
    @hwfields_access_api  
    @name(".flow_hash")
    action flow_hash(
		           bit<22>     idx,
                            bit<1>     idx_type,
			   //    bit<8>     pad,
		            bit<11>    hash1,
		            bit<20>    hint1,
		            bit<11>    hash2,
		            bit<20>    hint2,
		            bit<11>    hash3,
		            bit<20>    hint3,
		            bit<11>    hash4,
		            bit<20>    hint4,
		            bit<11>    hash5,
		            bit<20>    hint5,
                            bit<1>      more_hashes,
			   bit<20>     more_hints,
			   bit<1>      entry_valid ) {
       bit<32>  hardware_hash = __hash_value();
       hdr.p4i_to_p4e_header.flow_hash = hardware_hash;

        if (entry_valid == FALSE) {
	    hdr.ingress_recirc_header.flow_done = TRUE;

	    // hdr.p4i_to_p4e_header.index = 0;
	    // hdr.p4i_to_p4e_header.index_type = 0;
            metadata.cntrl.flow_miss = TRUE;
 	    // metadata.cntrl.index = 0;
          return;
        }
        if (__table_hit()) {
	    hdr.ingress_recirc_header.flow_done = TRUE;
	    hdr.p4i_to_p4e_header.index = (bit<24>)idx;
	    hdr.p4i_to_p4e_header.index_type = idx_type;
	    hdr.p4i_to_p4e_header.direction = metadata.cntrl.direction;
	    if(idx_type == FLOW_CACHE_INDEX_TYPE_CONNTRACK_INFO) {
	      hdr.p4i_to_p4e_header.conntrack_en = TRUE;
	      metadata.cntrl.flow_miss = TRUE;

	    } else {
	      hdr.p4i_to_p4e_header.session_info_en = TRUE;
	    }
	    hdr.p4i_to_p4e_header.direction = metadata.cntrl.direction;
	    hdr.ingress_recirc_header.flow_ohash_lkp = metadata.cntrl.flow_ohash_lkp;

	    //    metadata.cntrl.index = idx;
	    //metadata.scratch.hint_valid = TRUE;

	} else {
	  bit<1> hint_valid = FALSE; 
	  if ((hint1 != 0 ) && (hash1 == hardware_hash[31:21])) {
	  hint_valid = TRUE;
	  metadata.scratch.flow_hint = hint1;
	  } else if ((hint2 != 0 ) && (hash2 == hardware_hash[31:21])) {
	    hint_valid = TRUE;
	    metadata.scratch.flow_hint = hint2;
	  } else if ((hint3 != 0 ) && (hash3 == hardware_hash[31:21])) {
	    hint_valid = TRUE;
	    metadata.scratch.flow_hint = hint3;
	  } else if ((hint4 != 0 ) &&  (hash4 == hardware_hash[31:21])) {
	    hint_valid = TRUE;
	    metadata.scratch.flow_hint = hint4;
	  } else if ((hint5 != 0 ) && (hash5 == hardware_hash[31:21])) {
	    hint_valid = TRUE;
	    metadata.scratch.flow_hint = hint5;
	  } else if ((more_hashes == 1)) {
	    hint_valid = TRUE;
	    metadata.scratch.flow_hint = more_hints;
	  }
	  
	  if( hint_valid == TRUE ) {
	    metadata.cntrl.flow_ohash_lkp = TRUE;
	    hdr.ingress_recirc_header.flow_ohash_lkp = TRUE;
	    hdr.ingress_recirc_header.flow_ohash = 
	      POS_OVERFLOW_HASH_BIT | (bit<32>)metadata.scratch.flow_hint;
	  } else {
	    hdr.ingress_recirc_header.flow_done = TRUE;
	    // hdr.p4i_to_p4e_header.index = 0;
	    // hdr.p4i_to_p4e_header.index_type = 0;
            metadata.cntrl.flow_miss = TRUE;
	  }
	}
        
    }

   @name(".flow_ohash_error")
     action flow_ohash_error() {
     intr_global.drop = 1;
    
   }

   @name(".flow_error")
     action flow_error() {
     intr_global.drop = 1;
    
   }

    @hbm_table    
    @name(".flow_ohash") 
    table flow_ohash {
        key = {
            hdr.ingress_recirc_header.flow_ohash : exact;
        }
        actions = {
            flow_hash;
        }
	size = FLOW_OHASH_TABLE_SIZE;
        default_action = flow_hash;
	error_action = flow_ohash_error;
        stage = 4;
        placement = HBM;
    }

    @capi_bitfields_struct
    @name(".flow") table flow {
        key = {
            metadata.key.vnic_id       : exact;
            metadata.key.src           : exact;
            metadata.key.dst           : exact;
            metadata.key.proto         : exact;
            metadata.key.sport         : exact;
            metadata.key.dport         : exact;
            metadata.key.ktype         : exact;

        }
        actions = {
            flow_hash;
        }
        size = FLOW_TABLE_SIZE;
        placement = HBM;
        default_action = flow_hash;
	error_action = flow_error;
        stage = 3;
        hash_collision_table = flow_ohash;
    }




    apply {
      if(metadata.cntrl.skip_flow_lkp == FALSE) {
	flow.apply();	  
      }
      
      if (metadata.cntrl.flow_ohash_lkp == TRUE) {
	 flow_ohash.apply();
       } 
    }

}

