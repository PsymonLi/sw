typedef bit<128> ip_addr_t;
typedef bit<48>  mac_addr_t;


struct capri_deparser_len_t {
	bit<16> trunc_pkt_len;
	bit<16> ipv4_0_hdr_len;
	bit<16> ipv4_1_hdr_len;
	bit<16> ipv4_2_hdr_len;
	bit<16> l4_payload_len;
    }


struct key_metadata_t {
	bit<2> ktype;
	bit<7> ktype7;
	bit<9> vnic_id;
	bit<128> src;
	bit<128> dst;
	bit<48> smac;
	bit<48> dmac;
	bit<8> proto;
	bit<16> sport;
	bit<16> dport;
	bit<32> ipv4_src;
	bit<32> ipv4_dst;
	bit<6> tcp_flags;
	bit<1> ingress_port;
    }

struct l2_key_metadata_t {
	bit<2> ktype;
	bit<7> ktype7;
	bit<9> vnic_id;
	bit<16> vnic_id16;
	bit<128> src;
	bit<128> dst;
	bit<48> smac;
	bit<48> dmac;
	bit<8> proto;
	bit<16> sport;
	bit<16> dport;
	bit<32> ipv4_src;
	bit<32> ipv4_dst;
	bit<6> tcp_flags;
	bit<1> ingress_port;
    }

struct flow_log_key_metadata_t {
	bit<2> ktype;
	bit<9> vnic_id;
	bit<128> src;
	bit<128> dst;
	bit<8> proto;
	bit<16> sport;
	bit<16> dport;
	bit<1> disposition;
	bit<8> salt;
    }

struct control_metadata_t {
        bit<1> egress_recirc;
        bit<1> nacl_permit;
        bit<16> geneve_prototype;
        bit<1> conntrack_tcp;
        bit<4> conntrack_flow_state_pre;
        bit<4> conntrack_flow_state_post;
        bit<16> drop_reason;
        bit<16> egress_drop_reason;
	bit<1> parser_encap_error;
	bit<1> forward_to_uplink;
	bit<1> redir_to_rxdma;
	bit<1> skip_l2_flow_lkp;
	bit<1> skip_flow_lkp;
	bit<1> skip_dnat_lkp;
	bit<1> l2_flow_ohash_lkp;
	bit<1> flow_ohash_lkp;
	bit<1> dnat_ohash_lkp;
	bit<1> direction;
	bit<1> from_arm;
	bit<1> parse_tcp_option_error;
	bit<1> l2_flow_miss;
	bit<1> flow_miss;
	bit<1> l2_session_index_valid;
	bit<1> session_index_valid;
	bit<1> conntrack_index_valid;
	bit<1> epoch1_id_valid;
	bit<1> epoch2_id_valid;
	bit<1> l2_epoch1_id_valid;
	bit<1> l2_epoch2_id_valid;
	bit<1> throttle_bw1_id_valid;
	bit<1> throttle_bw2_id_valid;
	bit<1> statistics_id_valid;
	bit<1> histogram_packet_len_id_valid;
	bit<1> histogram_latency_id_valid;
	bit<1> update_checksum;
	bit<1> launch_v4;
	bit<1> vnic_type;
	bit<1> strip_outer_encap_flag;
	bit<1> strip_l2_header_flag;
	bit<1> strip_vlan_tag_flag;
	bit<1> add_vlan_tag_flag;
	bit<1> skip_flow_log;
	bit<1> l2_vnic;
	bit<1> session_rewrite_id_valid;
	bit<1> session_encap_id_valid;
	bit<2> nat_type;
	bit<2> encap_type;
	bit<16> mpls_label_b20_b4;
	bit<8> mpls_label_b20_b12;
	bit<8> mpls_label_b11_b4;
//	bit<8> mpls_label_b3_b0;
	bit<4> mpls_label_b3_b0;
        bit<20> mpls_vnic_label; 
        /* Rewrite info */
	bit<128> nat_address;
	bit<48> dmac;
	bit<48> smac;
	bit<12> vlan;
	bit<32> ipv4_sa;
	bit<32> ipv4_da;
	bit<16> udp_sport;
	bit<16> udp_dport;
	bit<20> mpls_label1;
	bit<20> mpls_label2;
	bit<20> mpls_label3;

        /* Session Info */
	bit<3> egress_action;
	bit<9> vnic_statistics_id;
	bit<9> histogram_packet_len_id;
	bit<9> histogram_latency_id;
	bit<10> allowed_flow_state_bitmap;
	bit<32> vnic_statistics_mask;
	bit<22> l2_index;
	bit<22> index;
	bit<24> l2_session_index;
	bit<24> session_index;
	bit<22> conntrack_index;
	bit<22> session_rewrite_id;
	bit<22> session_encap_id;
	bit<16> l2_epoch1_value;
	bit<16> l2_epoch2_value;
	bit<20> l2_epoch1_id;
	bit<20> l2_epoch2_id;
	bit<16> epoch1_value;
	bit<16> epoch2_value;
	bit<20> epoch1_id;
	bit<20> epoch2_id;
	bit<13> throttle_bw1_id;
	bit<13> throttle_bw2_id;

	bit<14> rx_packet_len;
	bit<14> tx_packet_len;
	bit<32> p4i_drop_reason;
	bit<32> p4e_drop_reason;
	bit<16> p4i_redir_reason;
	bit<16> p4e_redir_reason;

        // NACL Results
	bit<1> redir_type;
	bit<4> redir_oport;
	bit<11> redir_lif;
	bit<3> redir_qtype;
	bit<24> redir_qid;
	bit<4> redir_app_id;

        /*  Stats - TMP */
        bit <1> stats_id;
        bit <4> p4e_stats_flag;
 
        /*  Flow Log */
	bit <1>  flow_log_select_ptr;
	bit <1>  flow_log_done;
	bit <1>  flow_log_select;
	bit <21> flow_log_hash;
	bit <1>  flow_log_collision;   

        /*  Mirroring  */
	bit <1> mirroring_en;
	bit <1> mirroring_valid;
	bit <6> mirroring_session;
 
        /* Ingress policers */
        bit <1> skip_pps_policer;
    }


struct scratch_metadata_t {
	bit<1> flag;
	bit<32> ipv4_src;
	bit<18> flow_hash;
	bit<21> flow_hint;
	bit<18> l2_flow_hash;
	bit<18> l2_flow_hint;
	bit<8> class_id;
	bit<32> addr;
	bit<10> local_vnic_tag;
	bit<10> vpc_id;
	bit<1> drop;
	bit<32> tcp_seq_num;
	bit<32> tcp_ack_num;
	bit<16> tcp_win_sz;
	bit<4> tcp_win_scale;
	bit<48> last_seen_timestamp;
	bit<8> tcp_flags;
	bit<34> session_stats_addr;
	bit<1> hint_valid;
	bit<16> cpu_flags;
	bit<12> nexthop_index;
	bit<4> num_nexthops;
	bit<31> pad31;
	bit<6> pad6;
	bit<1> update_ip_chksum;
	bit<1> update_l4_chksum;
	bit<1> index_type;
	bit<22> index;
        bit<8> dummy;


        //common types
	bit<48> mac;
	bit<32> ipv4;

	bit<5> flow_data_pad;

        // Session info
	bit<18> timestamp;
	bit<32> config_epoch;

        // Session info - substrate encap to switch
	bit<3> encap_type;
	bit<48> smac;
	bit<48> dmac;
	bit<12> vlan;
	bit<8> ip_ttl;
	bit<32> ip_saddr;
	bit<32> ip_daddr;
	bit<16> udp_sport;
	bit<16> udp_dport;
	bit<20> mpls_label;

        // Counters
	bit<64> counter_rx;
	bit<64> counter_tx;

        // policer
	bit<1> policer_valid;
	bit<1> policer_pkt_rate;
	bit<1> policer_rlimit_en;
	bit<2> policer_rlimit_prof;
	bit<1> policer_color_aware;
	bit<1> policer_rsvd;
	bit<1> policer_axi_wr_pend;
	bit<40> policer_burst;
	bit<40> policer_rate;
	bit<40> policer_tbkt;
	bit<16> packet_len;

        // Conntrack
	bit<2> flow_type;
	bit<4> flow_state;

        // NACL Results
	bit<1> redir_type;
	bit<4> redir_oport;
	bit<11> redir_lif;
	bit<3> redir_qtype;
	bit<24> redir_qid;
	bit<4> redir_app_id;

        // Geneve encap (TMP: TB deleted) */
	bit<24> vni;
	bit<32> source_slot_id;
	bit<32> destination_slot_id;
	bit<16> sg_id;
	bit<32> originator_physical_ip;
        
    }


struct offset_metadata_t {
	bit<8> l2_1;
	bit<8> l2_2;
	bit<8> l3_1;
	bit<8> l3_2;
	bit<8> l4_1;
	bit<8> l4_2;
	bit<8> user_packet_offset;
	bit<16> payload_offset;
    }


struct capri_gso_csum_value_t {
    bit<16> gso_checksum;
  }
 

struct l4_metadata_t {
     bit<16>   l4_sport_1;
     bit<16>   l4_dport_1;
     bit<16>   l4_sport_2;
     bit<16>   l4_dport_2;
     bit<1>   icmp_valid;	       
}

struct csum_metadata_t {
     bit<16>     ip_hdr_len_0; 
     bit<16>     udp_len_0; 
     bit<16>     tcp_len_0; 
     bit<16>     ip_hdr_len_1; 
     bit<16>     udp_len_1; 
     bit<16>     tcp_len_1; 
     bit<16>     ip_hdr_len_2; 
     bit<16>     udp_len_2; 
     bit<16>     tcp_len_2; 
     bit<16>     icrc_len; 
     bit<16>     icmp_len_0; 
     bit<16>     icmp_len_1; 
     bit<16>     icmp_len_2; 
     bit<16>     icmp_len; 
     bit<16>     udp_len; 
     bit<16>     tcp_len; 
     bit<16>     l2_csum_len; 
}

struct parser_metadata_t {
     bit<16> geneve_prototype;
     bit<1> geneve_option_len_error;
}

struct metadata_t {
    @name(".scratch_metadata")
    scratch_metadata_t  scratch;
    l4_metadata_t         l4;
    @name(".key_metadata")
    key_metadata_t        key;
    @name(".key_metadata")
    l2_key_metadata_t        l2_key;
    @name(".control_metadata")
    control_metadata_t    cntrl;
    @name(".offset_metadata")
    offset_metadata_t     offset;
    csum_metadata_t csum;
    capri_gso_csum_value_t gso;
    parser_metadata_t prs;
    @name(".flow_log_key_metadata")
    flow_log_key_metadata_t        flow_log_key;

}
