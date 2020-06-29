/******************************************************************************/
/* Eg pipeline                                                                */
/******************************************************************************/
/******************************************************************************/
// NAT CONSIDERATIONS
// Key need to be normalized like flow hash
// 1) In case of dnat S2H we need the rewritten DIP. 
//       - We can re-write the packet
//       - carry 128 bit in the recirc over
// 2) In case of dnat H2S we should use the original packet or carry the private SIP
//
// PAT and double NAT are only for iSCSI that is not part of flow-log
//
// RE-WRITE CONSIDERATIONS
// 1) If we do encapsulation/decpasulation in the first pass, the recirculation will
//    always see the reversed.Should be OK if we bring the recirc bit as a tcam entry
//    and the direction. But should not matter if we do header stack info
//                                                          
/******************************************************************************/

control egress_key_init(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }

    @name(".eg_native_ipv4_packet") action eg_native_ipv4_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_0;
        metadata.flow_log_key.vnic_id =  hdr.p4i_to_p4e_header.vnic_id;
        metadata.flow_log_key.disposition =  metadata.cntrl.egress_action[0:0]; //TBD
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV4;
	if(metadata.cntrl.direction == TX_FROM_HOST) {
	  hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  // hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr = (bit<64>)hdr.ip_1.ipv4.srcAddr;
	  metadata.flow_log_key.src = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_1.ipv4.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_1;

	} else  {
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  //	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.p4i_to_p4e_nat_header.flow_log_dstAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	  } else {
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_1;
	  }
	} 
	metadata.flow_log_key.proto = hdr.ip_1.ipv4.protocol;
    }

   @name(".eg_native_ipv6_packet") action eg_native_ipv6_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_0;
        metadata.flow_log_key.vnic_id =  hdr.p4i_to_p4e_header.vnic_id;
        metadata.flow_log_key.disposition =  metadata.cntrl.egress_action[0:0]; //TBD
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV6;

	if(metadata.cntrl.direction == TX_FROM_HOST) {
	  hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr = hdr.ip_1.ipv6.srcAddr;	
	  //	  hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr = hdr.ip_1.ipv6.srcAddr[63:0];	
	  metadata.flow_log_key.src = hdr.ip_1.ipv6.srcAddr;
	  metadata.flow_log_key.dst = hdr.ip_1.ipv6.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	} else {	
	  metadata.flow_log_key.dst = hdr.ip_1.ipv6.srcAddr;
	  //	  metadata.flow_log_key.src = hdr.eg_nat_u.p4i_to_p4e_nat_header.flow_log_dstAddr;
	  // metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.p4i_to_p4e_nat_header.flow_log_dstAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	  } else {
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_1;
	  }
	}
	metadata.flow_log_key.proto = hdr.ip_1.ipv6.nextHdr;

    }

   @name(".eg_native_nonip_packet") action eg_native_nonip_packet() {

        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;

    }
   
    //egress recirculation 
    // Decap Packet already re-written 
    @name(".eg_native_recirc_ipv4_packet") action eg_native_recirc_ipv4_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_1;
        metadata.flow_log_key.vnic_id =  hdr.egress_recirc_header.vnic_id;
        metadata.flow_log_key.disposition = hdr.egress_recirc_header.flow_log_disposition;
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV4;
	if(metadata.cntrl.direction == TX_FROM_HOST) {
	  //	  metadata.flow_log_key.src = hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  //	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.src = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_1.ipv4.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_1;

	} else  {
	  //Real case from switch
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	  metadata.flow_log_key.src = (bit<128>)hdr.ip_1.ipv4.dstAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	  } else {
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_1;
	  }
	} 
	metadata.flow_log_key.proto = hdr.ip_1.ipv4.protocol;
    }

   @name(".eg_native_recirc_ipv6_packet") action eg_native_recirc_ipv6_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_1;
        metadata.flow_log_key.vnic_id =  hdr.egress_recirc_header.vnic_id;
        metadata.flow_log_key.disposition = hdr.egress_recirc_header.flow_log_disposition;
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV6;

	if(metadata.cntrl.direction == TX_FROM_HOST) {	
	  //	  metadata.flow_log_key.src = hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  //	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.src = hdr.ip_1.ipv6.srcAddr;
	  metadata.flow_log_key.dst = hdr.ip_1.ipv6.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	} else {
	  //Real case
	  metadata.flow_log_key.dst = hdr.ip_1.ipv6.srcAddr;
	  metadata.flow_log_key.src = hdr.ip_1.ipv6.srcAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_1;
	  } else {
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_1;
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_1;
	  }
	}
	metadata.flow_log_key.proto = hdr.ip_1.ipv6.nextHdr;

    }

   @name(".eg_native_recirc_nonip_packet") action eg_native_recirc_nonip_packet() {

        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;

    }

   //original from switch
    @name(".eg_tunneled_ipv4_packet") action eg_tunneled_ipv4_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_0;
        metadata.flow_log_key.vnic_id =  hdr.p4i_to_p4e_header.vnic_id;
        metadata.flow_log_key.disposition =  metadata.cntrl.egress_action[0:0]; //TBD
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV4;

	if(metadata.cntrl.direction == TX_FROM_HOST) {	
	  metadata.flow_log_key.src = (bit<128>)hdr.ip_2.ipv4.srcAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_2.ipv4.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_2; 

	} else {
	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.p4i_to_p4e_nat_header.flow_log_dstAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_2.ipv4.srcAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  } else {
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_2;
	  }
	}
	metadata.flow_log_key.proto = hdr.ip_2.ipv4.protocol;

    }

    @name(".eg_tunneled_ipv6_packet") action eg_tunneled_ipv6_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_0;
        metadata.flow_log_key.vnic_id =  hdr.p4i_to_p4e_header.vnic_id;
        metadata.flow_log_key.disposition =  metadata.cntrl.egress_action[0:0]; //TBD
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV6;
	if(metadata.cntrl.direction == TX_FROM_HOST) {	
	  metadata.flow_log_key.src = hdr.ip_2.ipv6.srcAddr;
	  metadata.flow_log_key.dst = hdr.ip_2.ipv6.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  metadata.l2_key.dmac = hdr.ethernet_2.dstAddr;
	
	} else {	
	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.p4i_to_p4e_nat_header.flow_log_dstAddr;
	  metadata.flow_log_key.dst = hdr.ip_2.ipv6.srcAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  } else {
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_2;
	  }	
	}
	metadata.flow_log_key.proto = hdr.ip_2.ipv6.nextHdr;
    }

   @name(".eg_tunneled_nonip_packet") action eg_tunneled_nonip_packet() {

        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;
    }

   //from host re-written 
    @name(".eg_tunneled_recirc_ipv4_packet") action eg_tunneled_recirc_ipv4_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_1;
        metadata.flow_log_key.vnic_id =  hdr.egress_recirc_header.vnic_id;
        metadata.flow_log_key.disposition = hdr.egress_recirc_header.flow_log_disposition;
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV4;

	if(metadata.cntrl.direction == TX_FROM_HOST) {	
	  //	  metadata.flow_log_key.src = hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.ip_2.ipv4.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_2; 

	} else {	
	  metadata.flow_log_key.src = (bit<128>)hdr.ip_2.ipv4.dstAddr;
	  //	  metadata.flow_log_key.dst = hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  } else {
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_2;
	  }
	}
	metadata.flow_log_key.proto = hdr.ip_2.ipv4.protocol;

    }

    @name(".eg_tunneled_recirc_ipv6_packet") action eg_tunneled_recirc_ipv6_packet() {
        metadata.flow_log_key.salt =  FLOW_LOG_SALT_1;
        metadata.flow_log_key.vnic_id =  hdr.egress_recirc_header.vnic_id;
        metadata.flow_log_key.disposition = hdr.egress_recirc_header.flow_log_disposition;
        metadata.flow_log_key.ktype =  P4_KEY_TYPE_IPV6;
	if(metadata.cntrl.direction == TX_FROM_HOST) {	
	  //	  metadata.flow_log_key.src = hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.src = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  metadata.flow_log_key.dst = hdr.ip_2.ipv6.dstAddr;
	  metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	  metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  metadata.l2_key.dmac = hdr.ethernet_2.dstAddr;
	
	} else {	
	  metadata.flow_log_key.src = hdr.ip_2.ipv6.dstAddr;
	  metadata.flow_log_key.dst = (bit<128>)hdr.eg_nat_u.egress_recirc_nat_header.flow_log_srcAddr;
	  if(metadata.l4.icmp_valid == TRUE) {
	    metadata.flow_log_key.sport = metadata.l4.l4_sport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_dport_2;
	  } else {
	    metadata.flow_log_key.sport = metadata.l4.l4_dport_2;
	    metadata.flow_log_key.dport = metadata.l4.l4_sport_2;
	  }	
	}
	metadata.flow_log_key.proto = hdr.ip_2.ipv6.nextHdr;
    }

   @name(".eg_tunneled_recirc_nonip_packet") action eg_tunneled_recirc_nonip_packet() {

        /* Treat it as a flow miss for now */
	metadata.cntrl.flow_miss = TRUE;
    }


   //////////////////////////////////////////////////////////
   // Egress recirculation packet :
   //   direction : from host
   //     packet is re-written and encapsulated
   //     normalized key is at layer 2
   //     ip_src comes from recirculation header because packet could be natted
   //   direction : from switch
   //     packet is re-written and decapsulated
   //     normalized key is at layer 1 all private address, need to be swapped
   //     
   // Ingress to Egress packet :
   //   direction : from host
   //     packet is native
   //     key extarction is before re-write
   //     normalized key is at layer 1
   //     all private address
   //   direction : from switch
   //     packet is encapsulated
   //     normalized key is at layer 2, need to be swapped
   //     ip_dest private comes from p4i_to_p4e because of posible dnat (before swapping)
   //     
   
    @name(".egress_key_native") table egress_key_native {
        key = {
	    hdr.egress_recirc_header.isValid()        : ternary;
	    //    metadata.cntrl.direction                  : ternary;
            hdr.ip_1.ipv4.isValid()                   : ternary;
            hdr.ip_1.ipv6.isValid()                   : ternary;
            hdr.ethernet_2.isValid()                  : ternary;
            hdr.ip_2.ipv4.isValid()                   : ternary;
            hdr.ip_2.ipv6.isValid()                   : ternary;
        }
        actions  = {
            nop;
            eg_native_ipv4_packet;
            eg_native_ipv6_packet;
            eg_native_nonip_packet;
            eg_native_recirc_ipv4_packet;
            eg_native_recirc_ipv6_packet;
            eg_native_recirc_nonip_packet;
        }
        size  = KEY_MAPPING_TABLE_SIZE;
        stage = 1;
        default_action = nop;
    }

    @name(".egress_key_tunneled") table egress_key_tunneled {
        key = {
	    hdr.egress_recirc_header.isValid()        : ternary;
	    //  metadata.cntrl.direction                  : ternary;
            hdr.ip_1.ipv4.isValid()                   : ternary;
            hdr.ip_1.ipv6.isValid()                   : ternary;
            hdr.ethernet_2.isValid()                  : ternary;
            hdr.ip_2.ipv4.isValid()                   : ternary;
            hdr.ip_2.ipv6.isValid()                   : ternary;
        }
        actions  = {
            nop;
            eg_tunneled_ipv4_packet;
            eg_tunneled_ipv6_packet;
            eg_tunneled_nonip_packet;
            eg_tunneled_recirc_ipv4_packet;
            eg_tunneled_recirc_ipv6_packet;
            eg_tunneled_recirc_nonip_packet;
        }
        size  = KEY_MAPPING_TABLE_SIZE;
        stage = 1;
        default_action = nop;
    }
    

 
    apply {
      if(metadata.cntrl.skip_flow_log == FALSE) {
	egress_key_native.apply();
	egress_key_tunneled.apply();
      }
    }
}



