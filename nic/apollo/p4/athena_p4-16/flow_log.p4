/******************************************************************************/
/* TX pipeline                                                                */
/******************************************************************************/
control flow_log_lookup(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }


    @name(".flow_log_select")
      action flow_log_select_a(bit<8> flow_log_tbl_sel, bit<504> dummy) {
      metadata.cntrl.flow_log_select = flow_log_tbl_sel[0:0];
    }
           
    @name(".flow_log_crc_0")
      action flow_log_crc_0_a(bit<8> dummy) {
      metadata.cntrl.flow_log_hash =  __hash_value()[20:0];
      metadata.scratch.dummy = dummy;
    }

    /*
    @name(".flow_log_crc_1")
      action flow_log_crc_1_a(bit<8> dummy) {
      metadata.cntrl.flow_log_hash =  __hash_value()[20:0];
      metadata.scratch.dummy = dummy;
    }
    */    
    

   @hbm_table
   @name(".flow_log_select") table flow_log_select {
        key = {
	   metadata.cntrl.flow_log_select_ptr  :  table_index;
        }
        actions = {
	  flow_log_select_a;
        }
        size = 1;
        placement = HBM;
	default_action = flow_log_select_a;
        stage = 3;
    }
   

   
    @name(".flow_log_crc_0") table flow_log_crc_0 {
        key = {
	    metadata.flow_log_key.src           : exact;
	    metadata.flow_log_key.dst           : exact;
            metadata.flow_log_key.proto         : exact;
            metadata.flow_log_key.sport         : exact;
            metadata.flow_log_key.dport         : exact;
            metadata.flow_log_key.vnic_id       : exact;
            metadata.flow_log_key.ktype         : exact;
            metadata.flow_log_key.disposition   : exact;
            metadata.flow_log_key.salt          : exact;
	    
        }
        actions = {
	  flow_log_crc_0_a;
        }
	stage = 3;
	size = 4;
        default_action = flow_log_crc_0_a;
    }
	    
    /*    
    @name(".flow_log_crc_1") table flow_log_crc_1 {
        key = {
            metadata.flow_log_key.src           : exact;
            metadata.flow_log_key.dst           : exact;
            metadata.flow_log_key.proto         : exact;
            metadata.flow_log_key.sport         : exact;
            metadata.flow_log_key.dport         : exact;
            metadata.flow_log_key.vnic_id       : exact;
            metadata.flow_log_key.ktype         : exact;
            metadata.flow_log_key.disposition   : exact;

        }
        actions = {
	  flow_log_crc_1_a;
        }
	hash_type = CRC32C;
        stage = 3;
	size = 4;
        default_action = flow_log_crc_1_a;

    }
    */

   @name(".flow_log")
     action flow_log_a ( @__ref bit<1> vld,
			   @__ref bit<1> disposition,
			   @__ref bit<2> ktype,
			   @__ref bit<9> vnic_id,
			   @__ref bit<128> src_ip,
			   @__ref bit<128> dst_ip,
			   @__ref bit<8> proto,
			   @__ref bit<16> sport,
			   @__ref bit<16> dport,
			   @__ref bit<8> security_state,
			   @__ref bit<32> pkt_from_host,
			   @__ref bit<32> pkt_from_switch,
			   @__ref bit<40> bytes_from_host,
			   @__ref bit<40> bytes_from_switch,
			   @__ref bit<18> start_timestamp,			    
			   @__ref bit<18> last_timestamp) {
     
       bit<48> current_time;
       current_time = __current_time();

       if(vld == FALSE) {
	 metadata.cntrl.flow_log_done = TRUE;
	 vld = 1;
	 /*  write key */
	 disposition = metadata.flow_log_key.disposition;
	 ktype = metadata.flow_log_key.ktype;
	 vnic_id = metadata.flow_log_key.vnic_id;
	 src_ip = metadata.flow_log_key.src;
	 dst_ip = metadata.flow_log_key.dst;
	 proto= metadata.flow_log_key.proto;
	 sport = metadata.flow_log_key.sport;
	 dport = metadata.flow_log_key.dport;
	 /*  write data */
	 //	 security_state = 0x3; //TBD
	 start_timestamp = current_time[40:23];	 
	 last_timestamp = current_time[40:23];	 
	 if(metadata.cntrl.direction == TX_FROM_HOST) {
	   pkt_from_host = (bit<32>)1;
	   bytes_from_host = (bit<40>)intr_p4.packet_len + 4;
	 } else {
	   pkt_from_switch = (bit<32>)1;
	   bytes_from_switch = (bit<40>)intr_p4.packet_len + 4;
	 }
       } else {
	 if((disposition == metadata.flow_log_key.disposition) &&
	    (ktype == metadata.flow_log_key.ktype) &&
	    (vnic_id == metadata.flow_log_key.vnic_id) &&
	    (src_ip == metadata.flow_log_key.src) &&
	    (dst_ip == metadata.flow_log_key.dst) &&
	    (proto == metadata.flow_log_key.proto) &&
	    (sport == metadata.flow_log_key.sport) &&
	    (dport == metadata.flow_log_key.dport)) {
	   /*  update data */
	   // security_state = 0x3; //TBD
	   metadata.cntrl.flow_log_done = TRUE;
	   last_timestamp = current_time[40:23];	        
	   if(metadata.cntrl.direction == TX_FROM_HOST) {
	     pkt_from_host = pkt_from_host + (bit<32>)1;
	     bytes_from_host = bytes_from_host + (bit<40>)intr_p4.packet_len + 4;
	   } else {
	     pkt_from_switch = pkt_from_switch + (bit<32>)1;
	     bytes_from_switch = bytes_from_switch + (bit<40>)intr_p4.packet_len + 4;
	   }
	      
	 } else { 
	   //FLOW MISS
	   metadata.cntrl.flow_log_collision = TRUE;
	 }
       }
    }
   

    @hbm_table
    @capi_bitfields_struct
    @name(".flow_log_0") table flow_log_0 {
        key = {
           metadata.cntrl.flow_log_hash : exact;
        }
        actions = {
	  flow_log_a;
        }
	  
	size = FLOW_LOG_TABLE_SIZE;
	default_action = flow_log_a;
        stage = 4;
        placement = HBM;
    }

    @hbm_table
    @capi_bitfields_struct
    @name(".flow_log_1") table flow_log_1 {
        key = {
           metadata.cntrl.flow_log_hash : exact;
        }
        actions = {
	  flow_log_a;
        }
	  
	size = FLOW_LOG_TABLE_SIZE;
	default_action = flow_log_a;
        stage = 4;
        placement = HBM;
    }

 
    apply {
      
      if(metadata.cntrl.skip_flow_log == FALSE) { //TO BE DONE drop by security list should be applied
	
	if(hdr.egress_recirc_header.isValid() == false) {
	  flow_log_select.apply();
	}
	
	flow_log_crc_0.apply();
	
	if(metadata.cntrl.flow_log_select == 0) {
	  flow_log_0.apply();
	} 
	
	if(metadata.cntrl.flow_log_select == 1) {
	  flow_log_1.apply();
	}
      }
      
    }
}


