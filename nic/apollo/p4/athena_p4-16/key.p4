/******************************************************************************/
/* Rx pipeline                                                                */
/******************************************************************************/
control key_init(inout cap_phv_intr_global_h intr_global,
             inout cap_phv_intr_p4_h intr_p4,
             inout headers hdr,
             inout metadata_t metadata) {

    @name(".nop") action nop() {
    }

    @name(".native_ipv4_packet") action native_ipv4_packet() {
        metadata.key.ktype =  KEY_TYPE_IPV4;
	metadata.key.src = (bit<128>)hdr.ip_1.ipv4.srcAddr;
	metadata.key.dst = (bit<128>)hdr.ip_1.ipv4.dstAddr;
	metadata.key.proto = hdr.ip_1.ipv4.protocol;
	metadata.key.sport = metadata.l4.l4_sport_1;
	metadata.key.dport = metadata.l4.l4_dport_1;
    }

   @name(".native_ipv6_packet") action native_ipv6_packet() {
        metadata.key.ktype =  KEY_TYPE_IPV6;
	metadata.key.src = hdr.ip_1.ipv6.srcAddr;
	metadata.key.dst = hdr.ip_1.ipv6.dstAddr;
	metadata.key.proto = hdr.ip_1.ipv6.nextHdr;
	metadata.key.sport = metadata.l4.l4_sport_1;
	metadata.key.dport = metadata.l4.l4_dport_1;
    }

   @name(".native_nonip_packet") action native_nonip_packet() {
        metadata.cntrl.skip_flow_lkp = TRUE;
    }

    @name(".tunneled_ipv4_packet") action tunneled_ipv4_packet() {
        metadata.key.ktype =  KEY_TYPE_IPV4;
	metadata.key.src = (bit<128>)hdr.ip_2.ipv4.dstAddr;
	metadata.key.dst = (bit<128>)hdr.ip_2.ipv4.srcAddr;
	metadata.key.proto = hdr.ip_2.ipv4.protocol;
	metadata.key.sport = metadata.l4.l4_dport_2;
	metadata.key.dport = metadata.l4.l4_sport_2;
	
    }

    @name(".tunneled_ipv6_packet") action tunneled_ipv6_packet() {
        metadata.key.ktype =  KEY_TYPE_IPV6;
	metadata.key.src = hdr.ip_2.ipv6.dstAddr;
	metadata.key.dst = hdr.ip_2.ipv6.srcAddr;
	metadata.key.proto = hdr.ip_2.ipv6.nextHdr;
	metadata.key.sport = metadata.l4.l4_dport_2;
	metadata.key.dport = metadata.l4.l4_sport_2;	
    }

   @name(".tunneled_nonip_packet") action tunneled_nonip_packet() {
        metadata.cntrl.skip_flow_lkp = TRUE;
    }

    
    @name(".key_native") table key_native {
        key = {
            hdr.ip_1.ipv4.isValid()                   : ternary;
            hdr.ip_1.ipv6.isValid()                   : ternary;
            hdr.ethernet_2.isValid()                  : ternary;
            hdr.ip_2.ipv4.isValid()                   : ternary;
            hdr.ip_2.ipv6.isValid()                   : ternary;
        }
        actions  = {
            nop;
            native_ipv4_packet;
            native_ipv6_packet;
            native_nonip_packet;
        }
        size  = KEY_MAPPING_TABLE_SIZE;
        stage = 0;
        default_action = nop;
    }

    @name(".key_tunneled") table key_tunneled {
        key = {
            hdr.ip_1.ipv4.isValid()                   : ternary;
            hdr.ip_1.ipv6.isValid()                   : ternary;
            hdr.ethernet_2.isValid()         : ternary;
            hdr.ip_2.ipv4.isValid()                   : ternary;
            hdr.ip_2.ipv6.isValid()                   : ternary;
        }
        actions  = {
            nop;
            tunneled_ipv4_packet;
            tunneled_ipv6_packet;
            tunneled_nonip_packet;
        }
        size  = KEY_MAPPING_TABLE_SIZE;
        stage = 0;
        default_action = nop;
    }


  action ingress_recirc_header_info() {
    if (hdr.ingress_recirc_header.isValid()) {

        metadata.cntrl.flow_ohash_lkp = ~hdr.ingress_recirc_header.flow_done;
        metadata.cntrl.dnat_ohash_lkp = ~hdr.ingress_recirc_header.dnat_done;
    }
  }

  @name(".init_config")
  action init_config_a() {
    ingress_recirc_header_info();
    intr_p4.setValid();
    intr_p4.packet_len = intr_p4.frame_size - (bit<14>)metadata.offset.l2_1;
    hdr.p4i_to_p4e_header.packet_len = (bit<16>)intr_p4.frame_size - (bit<16>)metadata.offset.l2_1;
    if (intr_global.tm_oq != TM_P4_RECIRC_QUEUE) {
        intr_global.tm_iq = intr_global.tm_oq;
    }
    if (metadata.cntrl.skip_flow_lkp == TRUE) {
      metadata.cntrl.flow_miss = TRUE;
    }
    //   if (!hdr.ingress_recirc_header.isValid() &&
    //	(metadata.cntrl.direction == RX_FROM_SWITCH) ) {
    //   metadata.cntrl.dnat_enable = 1;
    // }
      
  }

    
 @name(".init_config") table init_config {
   actions  = {
     init_config_a;
   }
   default_action = init_config_a;
   stage = 0;
   
 }
 
    apply {
      key_native.apply();
      key_tunneled.apply();
      init_config.apply();
    }
}


