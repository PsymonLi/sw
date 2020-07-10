
/*****************************************************************************/
/* Inter pipe : ingress pipeline                                             */
/*****************************************************************************/
control ingress_inter_pipe(inout cap_phv_intr_global_h intr_global,
			    inout cap_phv_intr_p4_h intr_p4,
			    inout headers hdr,
			    inout metadata_t metadata) {



   @name(".p4i_to_p4e") action p4i_to_p4e_a() {
     if((metadata.cntrl.from_arm == FALSE) && (
	hdr.ingress_recirc_header.flow_done != TRUE ||
	hdr.ingress_recirc_header.dnat_done != TRUE || 
	hdr.ingress_recirc_header.l2_flow_done != TRUE) 
	) {
         /* Recirc back to P4I */
       hdr.ingress_recirc_header.setValid();
       hdr.ingress_recirc_header.direction = metadata.cntrl.direction;
       intr_global.tm_oport = TM_PORT_INGRESS;
     } else {
        /* To P4E */
       hdr.ingress_recirc_header.setInvalid();
       hdr.p4i_to_p4e_header.setValid();
       hdr.p4i_to_p4e_header.vnic_id = metadata.key.vnic_id;
       hdr.p4i_to_p4e_header.flow_miss = metadata.cntrl.flow_miss;
       hdr.p4i_to_p4e_header.direction = metadata.cntrl.direction;
       hdr.p4i_to_p4e_header.skip_flow_log = 0; // TO BE DONE based on P4PLUS 
       hdr.p4i_to_p4e_nat_header.setValid();
       hdr.p4i_to_p4e_nat_header.flow_log_dstAddr = metadata.key.src;
       intr_global.tm_oport = TM_PORT_EGRESS;
     }
   }


    @name(".p4i_to_p4e") table p4i_to_p4e {
        actions  = {
            p4i_to_p4e_a;
        }
        default_action = p4i_to_p4e_a;
	stage = 5;
    }

    apply{
      if(intr_global.drop == 0) {
	p4i_to_p4e.apply();
      }
    }

}

/*****************************************************************************/
/* Inter pipe : egress pipeline                                              */
/*****************************************************************************/

control egress_inter_pipe(inout cap_phv_intr_global_h intr_global,
			    inout cap_phv_intr_p4_h intr_p4,
			    inout headers hdr,
			    inout metadata_t metadata) {

  @name(".p4i_to_p4e_state") action p4i_to_p4e_state_a() {

     if(hdr.egress_recirc_header.isValid()) {
       metadata.cntrl.direction = hdr.egress_recirc_header.direction;
       metadata.cntrl.redir_type = PACKET_ACTION_REDIR_UPLINK;
     } else if(hdr.p4i_to_p4e_header.isValid()) {
       metadata.cntrl.direction = hdr.p4i_to_p4e_header.direction;
       metadata.cntrl.update_checksum = hdr.p4i_to_p4e_header.update_checksum;
       if(hdr.p4i_to_p4e_header.index_type == FLOW_CACHE_INDEX_TYPE_CONNTRACK_INFO) {
	 metadata.cntrl.conntrack_index = hdr.p4i_to_p4e_header.index[21:0];
	 metadata.cntrl.conntrack_index_valid = TRUE;
       }
       
     }
  }

  @name(".p4i_to_p4e_state_error")
  action p4i_to_p4e_state_error() {
    intr_global.drop = 1;
//    capri_intrinsic.debug_trace = 1;
    if (intr_global.tm_oq != TM_P4_RECIRC_QUEUE) {
      intr_global.tm_iq = intr_global.tm_oq;
    }

    
  }


  @name(".p4i_to_p4e_state") table p4i_to_p4e_state {
        actions  = {
            p4i_to_p4e_state_a;
        }
	  default_action = p4i_to_p4e_state_a;
	  error_action = p4i_to_p4e_state_error;
	stage = 0;
    }


  @name(".p4e_to_uplink") action p4e_to_uplink_a() {
    if(!hdr.egress_recirc_header.isValid() && (intr_global.drop == 0)) {
      hdr.egress_recirc_header.setValid();
      hdr.eg_nat_u.egress_recirc_nat_header.setValid();
      intr_global.tm_oport = TM_PORT_EGRESS; 
      hdr.egress_recirc_header.flow_log_disposition = metadata.flow_log_key.disposition; // This is also drop_by_securuty_list.
      hdr.egress_recirc_header.flow_log_select = metadata.cntrl.flow_log_select;
      hdr.egress_recirc_header.direction = metadata.cntrl.direction;
      hdr.egress_recirc_header.vnic_id = hdr.p4i_to_p4e_header.vnic_id;
      hdr.egress_recirc_header.oport = metadata.cntrl.redir_oport;
      hdr.egress_recirc_header.skip_flow_log = metadata.cntrl.skip_flow_log | metadata.cntrl.flow_log_done; // TO BE DONE
      //hdr.egress_recirc_header.skip_flow_log = 0; // DUMMY TO TEST RECIRC PATH
    } else {
      hdr.egress_recirc_header.setInvalid();
      hdr.eg_nat_u.egress_recirc_nat_header.setInvalid();
      intr_global.tm_oport = hdr.egress_recirc_header.oport;
    }

   }

   @name(".p4e_to_rxdma") action p4e_to_rxdma_a() {
        intr_global.tm_oport = TM_PORT_DMA;
        intr_global.lif = metadata.cntrl.redir_lif;
	hdr.capri_rxdma_intrinsic.setValid();
	hdr.capri_rxdma_intrinsic.qid = metadata.cntrl.redir_qid;
	hdr.capri_rxdma_intrinsic.qtype = metadata.cntrl.redir_qtype;
	hdr.capri_rxdma_intrinsic.rx_splitter_offset = CAPRI_GLOBAL_INTRINSIC_HDR_SZ +
	  CAPRI_RXDMA_INTRINSIC_HDR_SZ + P4PLUS_CLASSIC_NIC_HDR_SZ;

	hdr.p4_to_p4plus_classic_nic.setValid();
	hdr.p4_to_p4plus_classic_nic.p4plus_app_id = metadata.cntrl.redir_app_id;
	hdr.p4_to_p4plus_classic_nic_ip.setValid();
	hdr.p4_to_p4plus_classic_nic.packet_len = (bit<16>)intr_p4.packet_len;
	if(hdr.ip_1.ipv4.isValid()) {
	  hdr.p4_to_p4plus_classic_nic_ip.ip_sa = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  hdr.p4_to_p4plus_classic_nic_ip.ip_da = (bit<128>)hdr.ip_1.ipv4.dstAddr;
	  if(hdr.l4_u.tcp.isValid()) {
	    hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV4_TCP;
	    hdr.p4_to_p4plus_classic_nic.l4_sport = hdr.l4_u.tcp.srcPort;
	    hdr.p4_to_p4plus_classic_nic.l4_dport = hdr.l4_u.tcp.dstPort;
	  } else {
	    if(hdr.udp.isValid()) {
	      hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV4_UDP;		    hdr.p4_to_p4plus_classic_nic.l4_sport = hdr.udp.srcPort;
	      hdr.p4_to_p4plus_classic_nic.l4_dport = hdr.udp.dstPort;

	    } else {
	      hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV4;
	    }
	  }
	}
	if(hdr.ip_1.ipv6.isValid()) {
	  hdr.p4_to_p4plus_classic_nic_ip.ip_sa = hdr.ip_1.ipv6.srcAddr;
	  hdr.p4_to_p4plus_classic_nic_ip.ip_da = hdr.ip_1.ipv6.dstAddr;
	  if(hdr.l4_u.tcp.isValid()) {
	    hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV6_TCP;
	    hdr.p4_to_p4plus_classic_nic.l4_sport = hdr.l4_u.tcp.srcPort;
	    hdr.p4_to_p4plus_classic_nic.l4_dport = hdr.l4_u.tcp.dstPort;

	  } else {
	    if(hdr.udp.isValid()) {
	      hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV6_UDP;		    hdr.p4_to_p4plus_classic_nic.l4_sport = hdr.udp.srcPort;
	      hdr.p4_to_p4plus_classic_nic.l4_dport = hdr.udp.dstPort;

	    } else {
	      hdr.p4_to_p4plus_classic_nic.pkt_type = CLASSIC_NIC_PKT_TYPE_IPV6;
	    }
	  }
	}
   }



    @name(".p4e_redir") table p4e_redir {
      key = {
	metadata.cntrl.redir_type : exact;
      }
        actions  = {
            p4e_to_rxdma_a;
	    p4e_to_uplink_a;
        }
        default_action = p4e_to_rxdma_a;
        placement = SRAM;
        size = P4E_REDIR_TABLE_SIZE;
	stage = 5;
    }


    apply{
      p4i_to_p4e_state.apply();
      p4e_redir.apply();


    }
}
