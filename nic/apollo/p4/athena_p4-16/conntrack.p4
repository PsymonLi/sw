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
     if(valid_flag == FALSE) {
       /* treat it like flow miss */
       metadata.cntrl.flow_miss = TRUE;
     } else {

         bit<4> nxt_flow_state = flow_state;
	 bit<48> current_time;
	 current_time = __current_time();
	 //	 timestamp = current_time[46:23];
	 if(flow_type == CONNTRACK_FLOW_TYPE_TCP) {
	   /*  TCP  */

	   /* flow_state transitions */
	   if(flow_state == CONNTRACK_FLOW_STATE_REMOVED) {

	     REDIR_PACKET_EGRESS(P4E_REDIR_CONNTRACK);
             timestamp = current_time[46:23];

	     if((metadata.cntrl.direction == TX_FROM_HOST) && 
		((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	     } 
	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYN_SENT) {
	     
	     //check for valid transition
	     if((hdr.l4_u.tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST)) != 0 ) {
	       //always update valid transition
	       nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_ACK)) == (TCP_FLAG_SYN|TCP_FLAG_ACK))) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYNACK_RECV;	       
	     }
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYN_RECV) {
	    
	     if((hdr.l4_u.tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST)) != 0) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_ACK)) == (TCP_FLAG_SYN|TCP_FLAG_ACK))) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYNACK_SENT;	       
	     }
	   
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYNACK_SENT) {
	     
	     timestamp = current_time[46:23];
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		     ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_ACK) == TCP_FLAG_ACK)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;	       
	     }
	   
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYNACK_RECV) {
	     
	     
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	       } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_ACK) == TCP_FLAG_ACK)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;	       
	     } 
	     
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_ESTABLISHED) {
	     
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	     } 
	     
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_FIN_SENT) {
	     
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_TIME_WAIT;	       	     
	     }
	     
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_FIN_RECV) {
	     
	     timestamp = current_time[46:23];
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_TIME_WAIT;	       	     
	     }
	     
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_TIME_WAIT) {
	     
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	     }
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_RST_CLOSE) {
	     
	     if((metadata.cntrl.direction == TX_FROM_HOST) && 
		((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	     } 
	     
	   } 

	   if( (metadata.cntrl.flow_miss != TRUE) && 
	       (( ( (bit<16>)metadata.cntrl.allowed_flow_state_bitmap | ((bit<16>)1 << CONNTRACK_FLOW_STATE_REMOVED)) & 
		  ((bit<16>)1 << nxt_flow_state) ) == 0)) {
	     REDIR_PACKET_EGRESS(P4E_REDIR_CONNTRACK);
	   } else {
	     timestamp = current_time[46:23];
	   }

	 } else {
	   /* UDP, ICMP, OTHERS */
	   //TBD do we need this flow_miss here and do we change state if a packet is received in removed state
	   if(flow_state == CONNTRACK_FLOW_STATE_REMOVED) {
	     REDIR_PACKET_EGRESS(P4E_REDIR_CONNTRACK);
	   }
	   
	   if((flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_SENT) && (metadata.cntrl.direction == RX_FROM_SWITCH))  {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	   } else if((flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_RECV) && (metadata.cntrl.direction == TX_FROM_HOST))  {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	   } else if(flow_state == CONNTRACK_FLOW_STATE_REMOVED) {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;	//TBD do we need this?     
	   }

	   timestamp = current_time[46:23];
	     
	 }
	 
	 flow_state = nxt_flow_state;
	 
	 metadata.cntrl.conntrack_flow_state_pre = flow_state;
	 metadata.cntrl.conntrack_flow_state_post = nxt_flow_state;
	 
	 
     }	 
   }
   
   
   
   
    @hbm_table    
    @capi_bitfields_struct
    @name(".conntrack") table conntrack {
        key = {
	metadata.cntrl.conntrack_index : exact;
        }
        actions = {
	  conntrack_a;
        }
        size = CONNTRACK_TABLE_SIZE;
        placement = HBM;
	default_action = conntrack_a;
        stage = 2;
    }


    apply {
      //      if(hdr.p4i_to_p4e_header.conntrack_en == TRUE) {
      if(metadata.cntrl.conntrack_index_valid == TRUE) {
	conntrack.apply();
      }
    }

}

