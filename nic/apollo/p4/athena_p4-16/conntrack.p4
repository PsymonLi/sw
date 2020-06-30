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

	     metadata.cntrl.flow_miss = TRUE;
             timestamp = current_time[46:23];

	     // TBD does it make sense to send to CPU and update the state?
	     if((metadata.cntrl.direction == TX_FROM_HOST) && 
		((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	     } 
	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYN_SENT) {

	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_SYN_SENT:CONNTRACK_FLOW_STATE_SYN_SENT] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {

	       timestamp = current_time[46:23];
	       if((hdr.l4_u.tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST)) != 0 ) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;	       	     
	       } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
			 ((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_ACK)) == (TCP_FLAG_SYN|TCP_FLAG_ACK))) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_SYNACK_RECV;	       
	       }
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYN_RECV) {
	       
	       if((metadata.cntrl.flow_miss != TRUE) && 
		  (metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_SYN_RECV:CONNTRACK_FLOW_STATE_SYN_RECV] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	       } else {

		 timestamp = current_time[46:23];	     
		 if((hdr.l4_u.tcp.flags & (TCP_FLAG_FIN|TCP_FLAG_RST)) != 0) {
		   nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;	       	     
		 } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
			   ((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_ACK)) == (TCP_FLAG_SYN|TCP_FLAG_ACK))) {
		   nxt_flow_state = CONNTRACK_FLOW_STATE_SYNACK_SENT;	       
		 }
	       }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYNACK_SENT) {
	     
	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_SYNACK_SENT:CONNTRACK_FLOW_STATE_SYNACK_SENT] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {

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
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_SYNACK_RECV) {

	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_SYNACK_RECV:CONNTRACK_FLOW_STATE_SYNACK_RECV] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {

	       timestamp = current_time[46:23];
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
	       
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_ESTABLISHED) {
	     
	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_ESTABLISHED:CONNTRACK_FLOW_STATE_ESTABLISHED] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {
	       timestamp = current_time[46:23];
	       if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	       } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	       } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	       } 
	       
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_FIN_SENT) {
	       
	       if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_FIN_SENT:CONNTRACK_FLOW_STATE_FIN_SENT] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	       } else {
		 
		 timestamp = current_time[46:23];
		 if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
		   nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
		 } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
			   ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
		   nxt_flow_state = CONNTRACK_FLOW_STATE_TIME_WAIT;	       	     
		 }
	       }	     

	   } else if(flow_state == CONNTRACK_FLOW_STATE_FIN_RECV) {
	     
	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_FIN_RECV:CONNTRACK_FLOW_STATE_FIN_RECV] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {

	       timestamp = current_time[46:23];
	       if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	       } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
			   ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
		   nxt_flow_state = CONNTRACK_FLOW_STATE_TIME_WAIT;	       	     
	       }
	       
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_TIME_WAIT) {
	     
	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_TIME_WAIT:CONNTRACK_FLOW_STATE_TIME_WAIT] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {
	     
	       timestamp = current_time[46:23];
	       if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_REMOVED;	       	     
	       } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	       } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	       }
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_RST_CLOSE) {
	     
	     if((metadata.cntrl.flow_miss != TRUE) && 
		(metadata.cntrl.allowed_flow_state_bitmap[CONNTRACK_FLOW_STATE_RST_CLOSE:CONNTRACK_FLOW_STATE_RST_CLOSE] == 0)) {
	       DROP_PACKET_EGRESS(P4E_DROP_CONNTRACK);
	     } else {
	       timestamp = current_time[46:23];
	       if((metadata.cntrl.direction == TX_FROM_HOST) && 
		  ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_SENT;
	       } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
			 ((hdr.l4_u.tcp.flags & TCP_FLAG_SYN) == TCP_FLAG_SYN)) {
		 nxt_flow_state = CONNTRACK_FLOW_STATE_SYN_RECV;
	       } 
	     }

	   } else if(flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_SENT) {
	     //TBD what about RST and REMOVED state
	     timestamp = current_time[46:23];
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	     } else if ((metadata.cntrl.direction == RX_FROM_SWITCH ) && 
			((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_FIN|TCP_FLAG_ACK)) == TCP_FLAG_ACK))  {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	     }
	     
	   } else if(flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_RECV) {
	     timestamp = current_time[46:23];
	     //TBD what about RST and REMOVED state
	     if((hdr.l4_u.tcp.flags & TCP_FLAG_RST) == TCP_FLAG_RST) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_RST_CLOSE;	       	     
	     } else if((metadata.cntrl.direction == TX_FROM_HOST) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_SENT;	       	     
	     } else if((metadata.cntrl.direction == RX_FROM_SWITCH) && 
		       ((hdr.l4_u.tcp.flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)) {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_FIN_RECV;	       	     
	     } else if ((metadata.cntrl.direction == TX_FROM_HOST) && 
		 ((hdr.l4_u.tcp.flags & (TCP_FLAG_SYN|TCP_FLAG_FIN|TCP_FLAG_ACK)) == TCP_FLAG_ACK))  {
	       nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	     }
	   }

	 } else {
	   /* UDP, ICMP, OTHERS */
	   //TBD do we need this flow_miss here and do we change state if a packet is received in removed state
	   timestamp = current_time[46:23];
	   if(flow_state == CONNTRACK_FLOW_STATE_REMOVED) {

	     metadata.cntrl.flow_miss = TRUE;
	   }

	   if((flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_SENT) && (metadata.cntrl.direction == RX_FROM_SWITCH))  {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	   } else if((flow_state == CONNTRACK_FLOW_STATE_OPEN_CONN_RECV) && (metadata.cntrl.direction == TX_FROM_HOST))  {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;
	   } else if(flow_state == CONNTRACK_FLOW_STATE_REMOVED) {
	     nxt_flow_state = CONNTRACK_FLOW_STATE_ESTABLISHED;	//TBD do we need this?     
	   }
	 }

	 if(nxt_flow_state != flow_state) {
	   metadata.cntrl.conntrack_flow_state_pre = flow_state;
	   metadata.cntrl.conntrack_flow_state_post = nxt_flow_state;

	 }

	 flow_state = nxt_flow_state;
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

