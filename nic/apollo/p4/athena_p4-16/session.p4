/******************************************************************************/
/* Tx pipeline                                                                */
/******************************************************************************/
#define PKT_IPV4_HDRLEN_0            ((bit<16>) hdr.ipv4_0.ihl << 2)
#define PKT_IPV4_HDRLEN_1            ((bit<16>) hdr.ip_1.ipv4.ihl << 2)
#define PKT_IPV4_TOTAL_LEN_0         hdr.ipv4_0.totalLen
#define PKT_IPV4_TOTAL_LEN_1         hdr.ip_1.ipv4.totalLen
#define PKT_IPV4_HDRLEN_2            ((bit<16>) hdr.ip_2.ipv4.ihl << 2)
#define PKT_IPV4_TOTAL_LEN_2         hdr.ip_2.ipv4.totalLen
//#define PKT_IPV6_PAYLOAD_LEN_0       hdr.ip_0.ipv6.payloadLen
#define PKT_IPV6_PAYLOAD_LEN_1       hdr.ip_1.ipv6.payloadLen
#define PKT_IPV6_PAYLOAD_LEN_2       hdr.ip_2.ipv6.payloadLen

control session_info_lookup(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".session_info")
    action session_info_a(
				        bit<1>  skip_flow_log,
				        bit<22> conntrack_id,
				 @__ref bit<18> timestamp,
				        bit<48> smac,
				        bit<16> h2s_epoch_vnic_value,
					bit<20> h2s_epoch_vnic_id,
					bit<16> h2s_epoch_mapping_value,
					bit<20> h2s_epoch_mapping_id,
					bit<13> h2s_throttle_bw1_id,
					bit<13> h2s_throttle_bw2_id,
				        bit<9>  h2s_vnic_statistics_id,
				        bit<32> h2s_vnic_statistics_mask,
				        bit<9>  h2s_vnic_histogram_packet_len_id,
				        bit<9>  h2s_vnic_histogram_latency_id,
				        bit<8>  h2s_slow_path_tcp_flags_match,
					bit<22> h2s_session_rewrite_id,
					bit<3>  h2s_egress_action,
					bit<10> h2s_allowed_flow_state_bitmap,
							
					bit<16> s2h_epoch_vnic_value,
					bit<20> s2h_epoch_vnic_id,
					bit<16> s2h_epoch_mapping_value,
					bit<20> s2h_epoch_mapping_id,
					bit<13> s2h_throttle_bw1_id,
					bit<13> s2h_throttle_bw2_id,
				        bit<9>  s2h_vnic_statistics_id,
				        bit<32> s2h_vnic_statistics_mask,
				        bit<9>  s2h_vnic_histogram_packet_len_id,
				        bit<9>  s2h_vnic_histogram_latency_id,
				        bit<8>  s2h_slow_path_tcp_flags_match,
					bit<22> s2h_session_rewrite_id,
					bit<3>  s2h_egress_action,
					bit<10> s2h_allowed_flow_state_bitmap,
				        bit<1>  valid_flag
					
			  ) {
      if(valid_flag == TRUE) {
	bit<48> current_time;
	current_time = __current_time();
	timestamp = current_time[40:23];
	__unlock();
	metadata.cntrl.skip_flow_log = skip_flow_log;
	if(conntrack_id != 0) {
	  metadata.cntrl.conntrack_index = conntrack_id;
	  metadata.cntrl.conntrack_index_valid = TRUE;
	  // hdr.p4i_to_p4e_header.index = (bit<24>)conntrack_id;
	  // hdr.p4i_to_p4e_header.conntrack_en = TRUE;
	}
	
	//	if(metadata.cntrl.direction == TX_FROM_HOST) {
	if(hdr.p4i_to_p4e_header.direction == TX_FROM_HOST) {
	  if(metadata.cntrl.l2_vnic == FALSE) {
	    if(h2s_session_rewrite_id != 0) {
	      metadata.cntrl.session_rewrite_id_valid = TRUE;      
	      metadata.cntrl.session_rewrite_id = h2s_session_rewrite_id;      
	      metadata.cntrl.session_encap_id_valid = TRUE;      
	      metadata.cntrl.session_encap_id = h2s_session_rewrite_id;      
					     
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }
	  }
	  if(metadata.cntrl.l2_vnic == TRUE) {
	    if(h2s_session_rewrite_id != 0) {
	      metadata.cntrl.session_rewrite_id_valid = TRUE;      
	      metadata.cntrl.session_rewrite_id = h2s_session_rewrite_id;      					     
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }
	    if(hdr.p4i_to_p4e_header.l2_index != 0) {
	      metadata.cntrl.session_encap_id_valid = TRUE;      
	      metadata.cntrl.session_encap_id = hdr.p4i_to_p4e_header.l2_index[21:0];      
	      
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }

	  }
	  
	  if(hdr.l4_u.tcp.isValid()) {
	    if((hdr.l4_u.tcp.flags & h2s_slow_path_tcp_flags_match) != 0) {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }
	  }

	  if(smac != 0) {
	    if(hdr.ethernet_1.srcAddr != smac) {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }	    
	  }
	  //	  metadata.cntrl.smac = smac;
	    
	  if(h2s_epoch_vnic_id != 0) {
	    metadata.cntrl.epoch1_id = h2s_epoch_vnic_id;
	    metadata.cntrl.epoch1_value = h2s_epoch_vnic_value;
	    metadata.cntrl.epoch1_id_valid = TRUE;
	  }

	  if(h2s_epoch_mapping_id != 0) {
	    metadata.cntrl.epoch2_id = h2s_epoch_mapping_id;
	    metadata.cntrl.epoch2_value = h2s_epoch_mapping_value;
	    metadata.cntrl.epoch2_id_valid = TRUE;
	  }

	  if (h2s_throttle_bw1_id != 0) {
	    metadata.cntrl.throttle_bw1_id = h2s_throttle_bw1_id;
	    metadata.cntrl.throttle_bw1_id_valid = TRUE;
	  }
	  if (h2s_throttle_bw2_id != 0) {
	    metadata.cntrl.throttle_bw2_id = h2s_throttle_bw2_id;
	    metadata.cntrl.throttle_bw2_id_valid = TRUE;
	  }

            /* TODO: Stats ID and histogram IDs are likely shared across both directions, 
                Confirm if this is true and if stats mask needs to be per-direction
            */
	  
	  if (h2s_vnic_statistics_id != 0) {
	    metadata.cntrl.statistics_id_valid = TRUE;
	    metadata.cntrl.vnic_statistics_id = h2s_vnic_statistics_id;
	    metadata.cntrl.vnic_statistics_mask = h2s_vnic_statistics_mask;
	  }

	  if (h2s_vnic_histogram_packet_len_id != 0) {
	    metadata.cntrl.histogram_packet_len_id_valid = TRUE;
	    metadata.cntrl.histogram_packet_len_id = h2s_vnic_histogram_packet_len_id;
	  }
	  if (h2s_vnic_histogram_latency_id != 0) {
	    metadata.cntrl.histogram_latency_id_valid = TRUE;
	    metadata.cntrl.histogram_latency_id = h2s_vnic_histogram_latency_id;
	  }
	  metadata.cntrl.egress_action = h2s_egress_action;
	  metadata.cntrl.allowed_flow_state_bitmap = h2s_allowed_flow_state_bitmap;

          metadata.cntrl.p4e_stats_flag = P4E_STATS_FLAG_TX_TO_SWITCH;
	  metadata.cntrl.redir_type = PACKET_ACTION_REDIR_UPLINK;
	  metadata.cntrl.redir_oport = UPLINK_SWITCH;
    
	}

	//	if(metadata.cntrl.direction == RX_FROM_SWITCH) {
	if(hdr.p4i_to_p4e_header.direction == RX_FROM_SWITCH) {
	  if(metadata.cntrl.l2_vnic == FALSE) {
	    if(s2h_session_rewrite_id != 0) {
	      metadata.cntrl.session_rewrite_id_valid = TRUE;  	           
	      metadata.cntrl.session_rewrite_id = s2h_session_rewrite_id;
	      metadata.cntrl.session_encap_id_valid = TRUE;      
	      metadata.cntrl.session_encap_id = s2h_session_rewrite_id;      
					     
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }
	  }
	  if(metadata.cntrl.l2_vnic == TRUE) {
	    if(s2h_session_rewrite_id != 0) {
	      metadata.cntrl.session_rewrite_id_valid = TRUE;      
	      metadata.cntrl.session_rewrite_id = s2h_session_rewrite_id;      					     
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }

	    if(hdr.p4i_to_p4e_header.l2_index != 0) {
	      metadata.cntrl.session_encap_id_valid = TRUE;      
	      metadata.cntrl.session_encap_id = hdr.p4i_to_p4e_header.l2_index[21:0];      
	      
	    } else {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }

	  }
	  

	  if(hdr.l4_u.tcp.isValid()) {
	    
	    if((hdr.l4_u.tcp.flags & s2h_slow_path_tcp_flags_match) != 0) {
	      metadata.cntrl.flow_miss = TRUE;
	      return;
	    }
	  }

	  //	  metadata.cntrl.smac = smac;
	    
	  if(s2h_epoch_vnic_id != 0) {
	    metadata.cntrl.epoch1_id = s2h_epoch_vnic_id;
	    metadata.cntrl.epoch1_value = s2h_epoch_vnic_value;
	    metadata.cntrl.epoch1_id_valid = TRUE;
	  }

	  if(s2h_epoch_mapping_id != 0) {
	    metadata.cntrl.epoch2_id = s2h_epoch_mapping_id;
	    metadata.cntrl.epoch2_value = s2h_epoch_mapping_value;
	    metadata.cntrl.epoch2_id_valid = TRUE;
	  }

	  if (s2h_throttle_bw1_id != 0) {
	    metadata.cntrl.throttle_bw1_id = s2h_throttle_bw1_id;
	    metadata.cntrl.throttle_bw1_id_valid = TRUE;
	  }
	  if (s2h_throttle_bw2_id != 0) {
	    metadata.cntrl.throttle_bw2_id = s2h_throttle_bw2_id;
	    metadata.cntrl.throttle_bw2_id_valid = TRUE;
	  }

            /* TODO: Stats ID and histogram IDs are likely shared across both directions, 
                Confirm if this is true and if stats mask needs to be per-direction
            */
	  
	  if (s2h_vnic_statistics_id != 0) {
	    metadata.cntrl.statistics_id_valid = TRUE;
	    metadata.cntrl.vnic_statistics_id = s2h_vnic_statistics_id;
	    metadata.cntrl.vnic_statistics_mask = s2h_vnic_statistics_mask;
	  }

	  if (s2h_vnic_histogram_packet_len_id != 0) {
	    metadata.cntrl.histogram_packet_len_id_valid = TRUE;
	    metadata.cntrl.histogram_packet_len_id = s2h_vnic_histogram_packet_len_id;
	  }
	  if (s2h_vnic_histogram_latency_id != 0) {
	    metadata.cntrl.histogram_latency_id_valid = TRUE;
	    metadata.cntrl.histogram_latency_id = s2h_vnic_histogram_latency_id;
	  }
	  metadata.cntrl.egress_action = s2h_egress_action;
	  metadata.cntrl.allowed_flow_state_bitmap = s2h_allowed_flow_state_bitmap;

          metadata.cntrl.p4e_stats_flag = P4E_STATS_FLAG_TX_TO_HOST;
	  metadata.cntrl.redir_type = PACKET_ACTION_REDIR_UPLINK;
	  metadata.cntrl.redir_oport = UPLINK_HOST;

	}

	//	timestamp = timestamp;
	// valid_flag = valid_flag;
      } else {
	metadata.cntrl.flow_miss = TRUE;
      }	
    }


    @name(".session_info_error")
      action session_info_error() {
      intr_global.drop = 1; 
    }

    @hbm_table
    @capi_bitfields_struct
      @name(".session_info")
	table session_info {
        key = {
	  // metadata.cntrl.session_index  : exact;
	  //	hdr.p4i_to_p4e_header.index : exact  @name(".control.metadata.session_index");
	hdr.p4i_to_p4e_header.index : table_index  @name(".control.metadata.session_index");
        } 
        actions  = {
	  session_info_a;
        }
        size  = SESSION_TABLE_SIZE;
	default_action = session_info_a;
	error_action = session_info_error;
        stage = 0;
        placement = HBM;
    }


#define SESSION_REWRITE_COMMON_FIELDS       bit<1> strip_outer_encap_flag, \
                                            bit<1> strip_l2_header_flag, \
	                                    bit<1> strip_vlan_tag_flag,	\
                                            bit<1> valid_flag		

   
#define SESSION_REWRITE_COMMON_FIELDS_ARGS    strip_outer_encap_flag, \
                                              strip_l2_header_flag, \
                                              strip_vlan_tag_flag, \
                                              valid_flag
 

   @name(".session_rewrite_common") 
     action session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS) {
     if (valid_flag == TRUE) {
         /* TODO: Confirm the required semantics for these flags */
       
       if (strip_l2_header_flag == TRUE) {
	 hdr.ethernet_1.setInvalid();
	 //	 hdr.p4i_to_p4e_header.packet_len = hdr.p4i_to_p4e_header.packet_len - 14;
       } 
       if ((strip_vlan_tag_flag == TRUE) && (hdr.ctag_1.isValid())) {
	   //        if(hdr.ctag_1.isValid()) {
	   hdr.ctag_1.setInvalid();
	   hdr.ethernet_1.etherType = hdr.ctag_1.etherType;
	   //}
	   // hdr.p4i_to_p4e_header.packet_len = hdr.p4i_to_p4e_header.packet_len - 4;
       }
       
       if (metadata.cntrl.direction == RX_FROM_SWITCH) {
	 if (strip_outer_encap_flag == TRUE) {
	   hdr.ip_1.ipv4.setInvalid();
	   hdr.udp.setInvalid();
	   if(hdr.geneve_1.isValid()) {
	     hdr.geneve_1.setInvalid();
	     hdr.geneve_options_blob.setInvalid();
	   } else {
	     hdr.mpls_src.setInvalid();
	     hdr.mpls_dst.setInvalid();
	     hdr.mpls_label3_1.setInvalid();
	   }
	 }
       }
       
     } else {
       metadata.cntrl.flow_miss = TRUE;
     }
   }


   @name(".session_rewrite") 
   action session_rewrite(SESSION_REWRITE_COMMON_FIELDS) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);
   }

   @name(".session_rewrite_ipv4_snat") 
   action session_rewrite_ipv4_snat(SESSION_REWRITE_COMMON_FIELDS,
				    bit<32> ipv4_addr_snat) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);
     
     if(metadata.cntrl.direction == TX_FROM_HOST) {
       hdr.ip_1.ipv4.srcAddr = ipv4_addr_snat;
     } else {
       hdr.ip_2.ipv4.srcAddr = ipv4_addr_snat;
     } 
     metadata.cntrl.update_checksum = TRUE;
    
   }

   @name(".session_rewrite_ipv4_dnat") 
   action session_rewrite_ipv4_dnat(SESSION_REWRITE_COMMON_FIELDS,
				    bit<32> ipv4_addr_dnat) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);

     if(metadata.cntrl.direction == TX_FROM_HOST) {
       hdr.ip_1.ipv4.dstAddr = ipv4_addr_dnat;
     } else {
       hdr.ip_2.ipv4.dstAddr = ipv4_addr_dnat;
     } 
     metadata.cntrl.update_checksum = TRUE;
    
   }

   @name(".session_rewrite_ipv4_pat") 
   action session_rewrite_ipv4_pat(SESSION_REWRITE_COMMON_FIELDS,
				   bit<32> ipv4_addr_spat,
				   bit<32> ipv4_addr_dpat,
				   bit<16> l4_port_spat,
				   bit<16> l4_port_dpat

) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);

     if(metadata.cntrl.direction == TX_FROM_HOST) {
       if(ipv4_addr_spat != 0) {
	 hdr.ip_1.ipv4.srcAddr = ipv4_addr_spat;
       }
       if(ipv4_addr_dpat != 0) {
	 hdr.ip_1.ipv4.dstAddr = ipv4_addr_dpat;
       }

     } else {
       if(ipv4_addr_spat != 0) {
	 hdr.ip_2.ipv4.srcAddr = ipv4_addr_spat;
       }
       if(ipv4_addr_dpat != 0) {
	 hdr.ip_2.ipv4.dstAddr = ipv4_addr_dpat;
       }

     } 
     
    if (l4_port_spat != 0) {
      hdr.l4_u.tcp.srcPort = l4_port_spat;
    }    
    if (l4_port_dpat != 0) {
      hdr.l4_u.tcp.dstPort = l4_port_dpat;
    }    
     metadata.cntrl.update_checksum = TRUE;

   }

   @name(".session_rewrite_ipv6_snat") 
     action session_rewrite_ipv6_snat(bit<128> ipv6_addr_snat,
				      SESSION_REWRITE_COMMON_FIELDS) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);

     if(metadata.cntrl.direction == TX_FROM_HOST) {
       hdr.ip_1.ipv6.srcAddr = ipv6_addr_snat;
     } else {
       hdr.ip_2.ipv6.srcAddr = ipv6_addr_snat;
     } 
     metadata.cntrl.update_checksum = TRUE;
    
   }

   @name(".session_rewrite_ipv6_dnat") 
     action session_rewrite_ipv6_dnat(bit<128> ipv6_addr_dnat,
				      SESSION_REWRITE_COMMON_FIELDS) {
     session_rewrite_common(SESSION_REWRITE_COMMON_FIELDS_ARGS);

     if(metadata.cntrl.direction == TX_FROM_HOST) {
       hdr.ip_1.ipv6.dstAddr = ipv6_addr_dnat;
     } else {
       hdr.ip_2.ipv6.dstAddr = ipv6_addr_dnat;
     } 
     metadata.cntrl.update_checksum = TRUE;
    
   }


    @name(".session_rewrite_error")
      action session_rewrite_error() {
      intr_global.drop = 1; 
    }

      @hbm_table
    @capi_bitfields_struct
   @name(".session_rewrite")
      table table_session_rewrite {
        key = {
           metadata.cntrl.session_rewrite_id  : exact;
        } 
        actions  = {
	  session_rewrite;
	  session_rewrite_ipv4_snat;
	  session_rewrite_ipv4_dnat;
	  session_rewrite_ipv4_pat;
	  session_rewrite_ipv6_snat;
	  session_rewrite_ipv6_dnat;
	  
        }
        size  = SESSION_TABLE_SIZE;
	default_action = session_rewrite;
	error_action = session_rewrite_error;
        stage = 3;
        placement = HBM;
    }

#define SESSION_REWRITE_ENCAP_COMMON_FIELDS       bit<1> valid_flag,		\
                                            bit<1> add_vlan_tag_flag, \
                                            bit<48> dmac, \
                                            bit<48> smac, \
                                            bit<12> vlan


#define SESSION_REWRITE_ENCAP_COMMON_FIELDS_ARGS     valid_flag,		\
                                                     add_vlan_tag_flag, \
                                                     dmac, \
                                                     smac, \
                                                     vlan


#define SESSION_REWRITE_ENCAP_IP_FIELDS   bit <32> ipv4_sa,   \
                                            bit <32> ipv4_da, \
	                                    bit<8> ip_proto, \
                                            bit<16> tot_len


#define SESSION_REWRITE_ENCAP_IP_FIELDS_ARGS  ipv4_sa,   \
                                              ipv4_da, \
	                                      ip_proto, \
                                              tot_len

#define SESSION_REWRITE_ENCAP_L2_FIELDS     bit<1> add_vlan_tag_flag, \
                                            bit <48> dmac,   \
                                            bit <48> smac, \
	                                    bit <12> vlan, \
	                                    bit<16> ethertype
                                           

   @name(".session_rewrite_encap_l2_common") 
     action session_rewrite_encap_l2_common(SESSION_REWRITE_ENCAP_L2_FIELDS) {

     hdr.ethernet_0.setValid();
     hdr.ethernet_0.dstAddr     = dmac;
     hdr.ethernet_0.srcAddr     = smac; 
     
     if (add_vlan_tag_flag == TRUE) {
       hdr.ctag_0.setValid();
       hdr.ctag_0.vid   = vlan;
       hdr.ctag_0.etherType   = ethertype;
       hdr.ethernet_0.etherType   = ETHERTYPE_VLAN;
     } else {
       hdr.ethernet_0.etherType   = ethertype;
     }
   }

   @name(".session_rewrite_encap_common") 
     action session_rewrite_encap_common(SESSION_REWRITE_ENCAP_COMMON_FIELDS) {
     if (valid_flag == FALSE) {
       metadata.cntrl.flow_miss = TRUE;
       return;
     }

   }

   @name(".session_rewrite_encap_l2") 
     action session_rewrite_encap_l2(SESSION_REWRITE_ENCAP_COMMON_FIELDS) {
     session_rewrite_encap_common(SESSION_REWRITE_ENCAP_COMMON_FIELDS_ARGS);
     if(hdr.ip_2.ipv4.isValid()) {
       hdr.ethernet_0.etherType = ETHERTYPE_IPV4;
     }
     if(hdr.ip_2.ipv6.isValid()) {
       hdr.ethernet_0.etherType = ETHERTYPE_IPV6;
     }
     
     session_rewrite_encap_l2_common(add_vlan_tag_flag,dmac, smac, vlan, hdr.ethernet_0.etherType);     
   }

   @name(".session_rewrite_insert_ctag") 
     action session_rewrite_insert_ctag( bit<1> valid_flag,
					 bit<12> vlan) {

     if (valid_flag == FALSE) {
       metadata.cntrl.flow_miss = TRUE;
       return;
     }
     hdr.ctag_2.setValid();
     hdr.ctag_2.vid = vlan;
     hdr.ethernet_2.etherType = ETHERTYPE_VLAN;

     if(hdr.ip_2.ipv4.isValid()) {
       hdr.ctag_2.etherType = ETHERTYPE_IPV4;
     }
     if(hdr.ip_2.ipv6.isValid()) {
       hdr.ctag_2.etherType = ETHERTYPE_IPV6;
     }
     
   }

   @name(".session_rewrite_encap_ip") 
     action session_rewrite_encap_ip(SESSION_REWRITE_ENCAP_IP_FIELDS) {

     hdr.ipv4_0.setValid();
     hdr.ipv4_0.version = 4;
     hdr.ipv4_0.ihl = 5;
     hdr.ipv4_0.totalLen = tot_len;
     hdr.ipv4_0.ttl = 64;
     hdr.ipv4_0.protocol = ip_proto;
     hdr.ipv4_0.srcAddr = ipv4_sa;
     hdr.ipv4_0.dstAddr = ipv4_da;

     metadata.csum.ip_hdr_len_0        = PKT_IPV4_HDRLEN_0;
     ipv4HdrCsumDepEg_0.enable_update();
     
   }

   @name(".session_rewrite_encap_mplsoudp") 
     action session_rewrite_encap_mplsoudp(SESSION_REWRITE_ENCAP_COMMON_FIELDS,
					   bit <32> ipv4_sa,
					   bit <32> ipv4_da,
					   bit <16> udp_sport,
					   bit <16> udp_dport,
					   bit<20>    mpls_label1,
					   bit<20>    mpls_label2,
					   bit<20>    mpls_label3
					  
) {
      session_rewrite_encap_common(SESSION_REWRITE_ENCAP_COMMON_FIELDS_ARGS);
      session_rewrite_encap_l2_common(add_vlan_tag_flag, dmac, smac, vlan, ETHERTYPE_IPV4);     
      // metadata.csum.ip_hdr_len_0        = PKT_IPV4_HDRLEN_0;
      //  ipv4HdrCsumDepEg_0.enable_update();

	
	 hdr.mpls_label1_0.setValid();
	 hdr.mpls_label1_0.label = mpls_label1;
	 //hdr.mpls_label1_0.label_b20_b4 = mpls_label1[19:4];
	 //hdr.mpls_label1_0.label_b20_b12 = mpls_label1[19:12];
	 //hdr.mpls_label1_0.label_b11_b4 = mpls_label1[11:4];
	 //hdr.mpls_label1_0.label_b3_b0 = mpls_label1[3:0];
	 hdr.mpls_label1_0.ttl = 64;
	 if(hdr.ctag_1.isValid()) {
	   hdr.udp_0.len = hdr.p4i_to_p4e_header.packet_len -6;// 8 UDP + 4 MPLS0 - 14 L1ETH -4 L1CTAG
         } else {
	   hdr.udp_0.len = hdr.p4i_to_p4e_header.packet_len -2;// 8 UDP + 4 MPLS0 - 14 L1ETH
	 }

	 if(mpls_label2 == 0) {
	   hdr.mpls_label1_0.bos = 1;	   
	 } else {
	   hdr.mpls_label2_0.setValid();
	   hdr.mpls_label2_0.label = mpls_label2;
	   //hdr.mpls_label2_0.label_b20_b4 = mpls_label2[19:4];
	   //hdr.mpls_label2_0.label_b20_b12 = mpls_label2[19:12];
	   //hdr.mpls_label2_0.label_b11_b4 = mpls_label2[11:4];
	   //hdr.mpls_label2_0.label_b3_b0 = mpls_label2[3:0];

	   hdr.mpls_label2_0.ttl = 64;
	   //	   hdr.ipv4_0.totalLen = hdr.ipv4_0.totalLen + 4;
	   hdr.udp_0.len = hdr.udp_0.len + 4;
	 
	   if(mpls_label3 == 0) {
	     hdr.mpls_label2_0.bos = 1;	   
	   } else {
	     hdr.mpls_label3_0.setValid();
	     hdr.mpls_label3_0.label = mpls_label3;
	     // hdr.mpls_label3_0.label_b20_b4 = mpls_label3[19:4];
	     //hdr.mpls_label3_0.label_b20_b12 = mpls_label3[19:12];
	     //hdr.mpls_label3_0.label_b11_b4 = mpls_label3[11:4];
	     //hdr.mpls_label3_0.label_b3_b0 = mpls_label3[3:0];
	     
	     hdr.mpls_label3_0.ttl = 64;
	     //	     hdr.ipv4_0.totalLen = hdr.ipv4_0.totalLen + 4;
	     hdr.udp_0.len = hdr.udp_0.len + 4;
	     hdr.mpls_label3_0.bos = 1;	   
	     
	   }
	 }	
	 hdr.ipv4_0.totalLen = hdr.udp_0.len + 20;
	 
	 //Setup UDP
	 hdr.udp_0.setValid();
	 if(udp_sport == 0) {
	   //	   hdr.udp_0.srcPort = ((bit<16>)(0x3 << 14)) & ((bit<16>) hdr.p4i_to_p4e_header.flow_hash[13:0]);
	   hdr.udp_0.srcPort[15:14] = 0x3;
	   hdr.udp_0.srcPort[13:0] = hdr.p4i_to_p4e_header.flow_hash[13:0];

	 } else {
	   hdr.udp_0.srcPort = udp_sport;
	 }
	 hdr.udp_0.dstPort = 0x19EB;
	 session_rewrite_encap_ip(ipv4_sa, ipv4_da,IP_PROTO_UDP, hdr.ipv4_0.totalLen);
   }

   @name(".session_rewrite_encap_geneve") 
     action session_rewrite_encap_geneve(SESSION_REWRITE_ENCAP_COMMON_FIELDS,
					   bit <32> ipv4_sa,
					   bit <32> ipv4_da,
					   bit <16> udp_sport,
					   bit <16> udp_dport,
					 bit <24> vni, 
					 bit<32> source_slot_id,
					 bit<32> destination_slot_id,
					 bit<16> sg_id1,
					 bit<16> sg_id2,
					 bit<16> sg_id3,
					 bit<16> sg_id4,
					 bit<16> sg_id5,
					 bit<16> sg_id6,
					 bit<32> originator_physical_ip
					  
) {
      session_rewrite_encap_common(SESSION_REWRITE_ENCAP_COMMON_FIELDS_ARGS);
      session_rewrite_encap_l2_common(add_vlan_tag_flag, dmac, smac, vlan, ETHERTYPE_IPV4);     
      //    metadata.csum.ip_hdr_len_0        = PKT_IPV4_HDRLEN_0;
      // ipv4HdrCsumDepEg_0.enable_update();
      bit<6> geneve_optlen = 0;
      hdr.geneve_0.setValid();
      hdr.geneve_0.oam = 0; //TODO
      hdr.geneve_0.protoType = ETHERTYPE_ETHERNET;
      hdr.geneve_0.vni = vni;
      //      hdr.geneve_0.optLen = 0;

      if(source_slot_id != 0) {
	hdr.geneve_option_srcSlotId.setValid();
	hdr.geneve_option_srcSlotId.optionClass = GENEVE_OPTION_OPTION_CLASS;
	hdr.geneve_option_srcSlotId.type = GENEVE_OPTION_SRC_SLOT_ID;
	hdr.geneve_option_srcSlotId.Lenght = 1;
	hdr.geneve_option_srcSlotId.srcSlotId = source_slot_id;
	geneve_optlen = geneve_optlen + 2;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 2;	
      } 

      if(destination_slot_id != 0) {
	hdr.geneve_option_dstSlotId.setValid();
	hdr.geneve_option_dstSlotId.optionClass = GENEVE_OPTION_OPTION_CLASS;
	hdr.geneve_option_dstSlotId.type = GENEVE_OPTION_DST_SLOT_ID;
	hdr.geneve_option_dstSlotId.Lenght = 1;
	hdr.geneve_option_dstSlotId.dstSlotId = destination_slot_id;	
	geneve_optlen = geneve_optlen + 2;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 2;	
      } 

      if(originator_physical_ip != 0) {
	hdr.geneve_option_origPhysicalIp.setValid();
	hdr.geneve_option_dstSlotId.optionClass = GENEVE_OPTION_OPTION_CLASS;
	hdr.geneve_option_origPhysicalIp.type = GENEVE_OPTION_ORIGINATOR_PHYSICAL_IP;
	hdr.geneve_option_origPhysicalIp.Lenght = 1;
	hdr.geneve_option_origPhysicalIp.origPhysicalIp = originator_physical_ip;	
	geneve_optlen = geneve_optlen + 2;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 2;	
      } 

      if(sg_id6 != 0 || sg_id5 != 0) {
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.setValid();
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.optionClass = GENEVE_OPTION_OPTION_CLASS;

	hdr.geneve_0.critical = 1;

	if(sg_id6 != 0) { 
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_EVEN;
	} else {
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_ODD;
	}
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.Lenght = 3;
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp0 = sg_id1;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp1 = sg_id2;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp2 = sg_id3;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp3 = sg_id4;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp4 = sg_id5;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_3.srcSecGrp5 = sg_id6;	
	geneve_optlen = geneve_optlen + 4;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 4;	
      } else if (sg_id4 != 0 || sg_id3 != 0) {
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.setValid();
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.optionClass = GENEVE_OPTION_OPTION_CLASS;
	hdr.geneve_0.critical = 1;

	if(sg_id4 != 0) { 
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_EVEN;
	} else {
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_ODD;
	}
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.Lenght = 2;
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.srcSecGrp0 = sg_id1;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.srcSecGrp1 = sg_id2;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.srcSecGrp2 = sg_id3;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_2.srcSecGrp3 = sg_id4;	
	geneve_optlen = geneve_optlen + 3;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 3;	
      } else if (sg_id2 != 0 || sg_id1 != 0) {
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.setValid();
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.optionClass = GENEVE_OPTION_OPTION_CLASS;
	hdr.geneve_0.critical = 1;

	if(sg_id2 != 0) { 
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_EVEN;
	} else {
	  hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.type = GENEVE_OPTION_SRC_SECURITY_GRP_LIST_ODD;
	}
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.Lenght = 1;
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.srcSecGrp0 = sg_id1;	
	hdr.geneve_option_srcSecGrpList_u.srcSecGrpList_1.srcSecGrp1 = sg_id2;	
	geneve_optlen = geneve_optlen + 2;	
	//	hdr.geneve_0.optLen = hdr.geneve_0.optLen + 2;	
      } 

      hdr.geneve_0.optLen = geneve_optlen;
      hdr.udp_0.len = hdr.p4i_to_p4e_header.packet_len + 12 + ((bit<16>)(geneve_optlen) << 2); // packet -4 CTAG + 8 UDP + 8 GENEVE + OPTLEN
      //      hdr.udp_0.len = hdr.p4i_to_p4e_header.packet_len + 12 + ((bit<16>)(hdr.geneve_0.optLen) << 2); // packet -4 CTAG + 8 UDP + 8 GENEVE + OPTLEN
      hdr.ipv4_0.totalLen = hdr.udp_0.len + 20;
      //Setup UDP
      hdr.udp_0.setValid();
      if(udp_sport == 0) {
	//	hdr.udp_0.srcPort = ((bit<16>)(0x3 << 14)) & ((bit<16>) hdr.p4i_to_p4e_header.flow_hash[13:0]);
	hdr.udp_0.srcPort[15:14] = 0x3;
	hdr.udp_0.srcPort[13:0] = hdr.p4i_to_p4e_header.flow_hash[13:0];
      } else {
	hdr.udp_0.srcPort = udp_sport;
      }   
      hdr.udp_0.dstPort = 0x17C1;
      session_rewrite_encap_ip(ipv4_sa, ipv4_da,IP_PROTO_UDP, hdr.ipv4_0.totalLen);
   }

   @name(".session_rewrite_encap_error")
      action session_rewrite_encap_error() {
      intr_global.drop = 1; 
    }

      @hbm_table
    @capi_bitfields_struct
    @name(".session_rewrite_encap")
      table table_session_encap {
        key = {
           metadata.cntrl.session_encap_id  : exact;
        } 
        actions  = {
	  session_rewrite_encap_l2;
	  session_rewrite_encap_mplsoudp;
	  session_rewrite_encap_geneve;	  
	  session_rewrite_insert_ctag;	  
        }
	default_action = session_rewrite_encap_l2;
	error_action = session_rewrite_encap_error;
        size  = SESSION_TABLE_SIZE;
        stage = 3;
        placement = HBM;
    }

    apply {
      if(hdr.p4i_to_p4e_header.session_info_en == TRUE) {
        session_info.apply();

	//	if(metadata.cntrl.l2_vnic == TRUE) {
	//  l2_session_info.apply();
	//}

      }
      if(metadata.cntrl.flow_miss == FALSE) {
	if(metadata.cntrl.session_rewrite_id_valid == TRUE) {
	  table_session_rewrite.apply();
	  //	table_session_encap.apply();
	}
	if(metadata.cntrl.session_encap_id_valid == TRUE) {
	  // table_session_rewrite.apply();
	  table_session_encap.apply();
	}
      }
    }
}

