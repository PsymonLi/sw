#include "../../common-p4+/common_txdma_dummy.p4"

#define tx_table_s0_t0_action esp_v4_tunnel_n2h_txdma2_initial_table 

#define tx_table_s1_t0_action esp_v4_tunnel_n2h_txdma2_load_barco_req 
#define tx_table_s1_t1_action esp_v4_tunnel_n2h_txdma2_increment_global_ci

#define tx_table_s2_t0_action esp_v4_tunnel_n2h_txdma2_load_in_desc 
#define tx_table_s2_t1_action esp_v4_tunnel_n2h_txdma2_load_out_desc 
#define tx_table_s2_t2_action esp_v4_tunnel_n2h_txdma2_load_ipsec_int

#define tx_table_s3_t0_action esp_v4_tunnel_n2h_txdma2_load_pad_size_and_l4_proto

#define tx_table_s4_t0_action esp_v4_tunnel_n2h_txdma2_build_decap_packet
#define tx_table_s4_t1_action ipsec_txdma_stats_update 

#define tx_table_s5_t0_action ipsec_release_resources

#include "../../common-p4+/common_txdma.p4"

#include "esp_v4_tunnel_n2h_headers.p4"

#include "../ipsec_defines.h"
#include "../ipsec_dummy_defines.h"
#include "../ipsec_txdma_common.p4"

header_type ipsec_txdma2_global_t {
    fields {
        in_desc_addr   : ADDRESS_WIDTH;
        ipsec_cb_index : 16;
        iv_size : 8;
        flags : 8;
        pad_size : 8;
        l4_protocol : 8;
        payload_size : 16;
    }
}

header_type ipsec_table0_s2s {
    fields {
        out_page_addr : ADDRESS_WIDTH;
        pad : 8;
        cb_addr : 40;
        barco_sw_cindex : 16;
        headroom_offset : 16;
        tailroom_offset : 16;
    }
}

#define IPSEC_DECRYPT_TXDMA2_T0_S2S_SCRATCH \
    modify_field(t0_s2s_scratch.pad, t0_s2s.pad); \
    modify_field(t0_s2s_scratch.cb_addr, t0_s2s.cb_addr); \
    modify_field(t0_s2s_scratch.barco_sw_cindex, t0_s2s.barco_sw_cindex); \
    modify_field(t0_s2s_scratch.out_page_addr, t0_s2s.out_page_addr); \
    modify_field(t0_s2s_scratch.headroom_offset, t0_s2s.headroom_offset); \
    modify_field(t0_s2s_scratch.tailroom_offset, t0_s2s.tailroom_offset); \


header_type ipsec_table1_s2s {
    fields {
        out_desc_addr : ADDRESS_WIDTH;
        out_page_addr : ADDRESS_WIDTH;
    }
}

header_type ipsec_table2_s2s {
    fields {
        in_desc_addr : ADDRESS_WIDTH;
        in_page_addr : ADDRESS_WIDTH;
    }
}

header_type ipsec_table3_s2s {
    fields {
        out_desc_addr : ADDRESS_WIDTH;
        out_page_addr : ADDRESS_WIDTH;
    }
}

header_type ipsec_to_stage1_t {
    fields {
        head_desc_addr : ADDRESS_WIDTH;
        stage1_pad     : ADDRESS_WIDTH;
    }
}

header_type ipsec_to_stage2_t {
    fields {
        barco_req_addr   : ADDRESS_WIDTH;
        out_desc_addr : ADDRESS_WIDTH;
    }
}

header_type ipsec_to_stage3_t {
    fields {
        ipsec_cb_addr   : ADDRESS_WIDTH;
        block_size      : 8;
        sem_cindex      : 32;
        stage3_pad      : 24;
    }
}

header_type ipsec_to_stage4_t {
    fields {
        in_page        : ADDRESS_WIDTH;
        headroom       : 16;
        dot1q_etype    : 16;
        vrf_vlan       : 16;
        ip_etype       : 16;
    }
}

header_type ipsec_to_stage5_t {
    fields {
        in_desc_addr   : ADDRESS_WIDTH;
        out_desc_addr : ADDRESS_WIDTH;
    }
}


@pragma pa_header_union ingress app_header
metadata ipsec_p4plus_to_p4_header_t p4plus2p4_hdr;

//unionize
@pragma pa_header_union ingress common_global 
metadata ipsec_txdma2_global_t txdma2_global;

@pragma pa_header_union ingress to_stage_1
metadata ipsec_to_stage1_t ipsec_to_stage1;

@pragma pa_header_union ingress to_stage_2
metadata ipsec_to_stage2_t ipsec_to_stage2;

@pragma pa_header_union ingress to_stage_3
metadata ipsec_to_stage3_t ipsec_to_stage3;

@pragma pa_header_union ingress to_stage_4
metadata ipsec_to_stage4_t ipsec_to_stage4;

@pragma pa_header_union ingress to_stage_5
metadata ipsec_to_stage5_t ipsec_to_stage5;

@pragma pa_header_union ingress common_t0_s2s
metadata ipsec_table0_s2s t0_s2s;
@pragma pa_header_union ingress common_t1_s2s
metadata ipsec_table1_s2s t1_s2s;
@pragma pa_header_union ingress common_t2_s2s
metadata ipsec_table2_s2s t2_s2s;
@pragma pa_header_union ingress common_t3_s2s
metadata ipsec_table3_s2s t3_s2s;

//TXDMA - IPsec feature specific scratch
@pragma dont_trim
metadata ipsec_int_header_t ipsec_int_header;

@pragma dont_trim
metadata dma_cmd_phv2pkt_t intrinsic_app_hdr;
@pragma dont_trim
metadata dma_cmd_phv2pkt_t ipsec_app_hdr;
@pragma dont_trim
metadata dma_cmd_mem2pkt_t eth_hdr;
@pragma dont_trim
metadata dma_cmd_phv2pkt_t vrf_vlan_hdr;
@pragma dont_trim
metadata dma_cmd_mem2pkt_t dec_pay_load;

@pragma dont_trim
metadata dma_cmd_phv2mem_t rnmdr;
@pragma dont_trim
metadata dma_cmd_phv2mem_t tnmdr;
@pragma dont_trim
metadata dma_cmd_phv2mem_t sem_cindex;


@pragma scratch_metadata
metadata ipsec_cb_metadata_t ipsec_cb_scratch;
@pragma scratch_metadata
metadata ipsec_txdma2_global_t txdma2_global_scratch;

@pragma scratch_metadata
metadata ipsec_to_stage2_t ipsec_to_stage2_scratch;

@pragma scratch_metadata
metadata ipsec_to_stage3_t ipsec_to_stage3_scratch;

@pragma scratch_metadata
metadata ipsec_to_stage4_t ipsec_to_stage4_scratch;

@pragma scratch_metadata
metadata ipsec_to_stage5_t ipsec_to_stage5_scratch;

@pragma scratch_metadata
metadata ipsec_table0_s2s t0_s2s_scratch;

@pragma scratch_metadata
metadata barco_request_t barco_req_scratch;

@pragma scratch_metadata
metadata barco_descriptor_t barco_desc_scratch;

@pragma scratch_metadata
metadata ipsec_int_header_t ipsec_int_hdr_scratch;
@pragma scratch_metadata
metadata ipsec_int_pad_t ipsec_int_pad_scratch;

@pragma scratch_metadata
metadata barco_shadow_params_d_t barco_shadow_params_d;

@pragma scratch_metadata
metadata n2h_stats_header_t ipsec_stats_scratch;


#define BARCO_REQ_SCRTATCH_SET \
    modify_field(barco_req_scratch.input_list_address,input_list_address); \
    modify_field(barco_req_scratch.output_list_address,output_list_address); \
    modify_field(barco_req_scratch.command,command); \
    modify_field(barco_req_scratch.key_desc_index,key_desc_index); \
    modify_field(barco_req_scratch.iv_address,iv_address); \
    modify_field(barco_req_scratch.auth_tag_addr,auth_tag_addr); \
    modify_field(barco_req_scratch.header_size,header_size); \
    modify_field(barco_req_scratch.status_address,status_address); \
    modify_field(barco_req_scratch.opaque_tag_value,opaque_tag_value); \
    modify_field(barco_req_scratch.opaque_tag_write_en,opaque_tag_write_en); \
    modify_field(barco_req_scratch.rsvd1,rsvd1); \
    modify_field(barco_req_scratch.sector_size,sector_size); \
    modify_field(barco_req_scratch.application_tag,application_tag); \

#define IPSEC_DECRYPT_GLOBAL_SCRATCH \
    modify_field(txdma2_global_scratch.in_desc_addr, txdma2_global.in_desc_addr); \
    modify_field(txdma2_global_scratch.ipsec_cb_index, txdma2_global.ipsec_cb_index); \
    modify_field(txdma2_global_scratch.iv_size, txdma2_global.iv_size); \
    modify_field(txdma2_global_scratch.flags, txdma2_global.flags); \
    modify_field(txdma2_global_scratch.pad_size, txdma2_global.pad_size); \
    modify_field(txdma2_global_scratch.l4_protocol, txdma2_global.l4_protocol); \
    modify_field(txdma2_global_scratch.payload_size, txdma2_global.payload_size); \

// stage 5
action ipsec_release_resources(sem_cindex)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    modify_field(ipsec_to_stage3_scratch.sem_cindex, sem_cindex);
    modify_field(ipsec_to_stage5_scratch.in_desc_addr, ipsec_to_stage5.in_desc_addr);
    modify_field(ipsec_to_stage5_scratch.out_desc_addr, ipsec_to_stage5.out_desc_addr);
}

//stage 4 - table1
action ipsec_txdma_stats_update(N2H_STATS_UPDATE_PARAMS)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    N2H_STATS_UPDATE_SET
}


//stage 4
action esp_v4_tunnel_n2h_txdma2_build_decap_packet(pc, rsvd, cosA, cosB,
                                       cos_sel, eval_last, host, total, pid,
                                       rxdma_ring_pindex, rxdma_ring_cindex,
                                       barco_ring_pindex, barco_ring_cindex,
                                       key_index, new_key_index, iv_size, icv_size,
                                       expected_seq_no, last_replay_seq_no,
                                       replay_seq_no_bmp, barco_enc_cmd,
                                       ipsec_cb_index, barco_full_count, is_v6,
                                       cb_pindex, cb_cindex, 
                                       barco_pindex, barco_cindex, 
                                       cb_ring_base_addr, barco_ring_base_addr, 
                                       iv_salt, vrf_vlan)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    IPSEC_DECRYPT_TXDMA2_T0_S2S_SCRATCH
    modify_field(ipsec_to_stage4_scratch.in_page, ipsec_to_stage4.in_page);
    modify_field(ipsec_to_stage4_scratch.headroom, ipsec_to_stage4.headroom);
    modify_field(ipsec_to_stage4_scratch.vrf_vlan, ipsec_to_stage4.vrf_vlan);
    modify_field(ipsec_to_stage4_scratch.dot1q_etype, ipsec_to_stage4.dot1q_etype);
    modify_field(ipsec_to_stage4_scratch.ip_etype, ipsec_to_stage4.ip_etype);
    // Add intrinsic and app header
    DMA_COMMAND_PHV2PKT_FILL(intrinsic_app_hdr, 0, 32, 0)

    // Add ethernet, optional-vlan and outer-ip from input-descriptor
    //DMA_COMMAND_MEM2PKT_FILL(eth_hdr, t0_s2s.in_page_addr, t0_s2s.headroom_offset, 0, 0, 0) 

    // Add decrypted payload from output page size is payload_size-pad
    //DMA_COMMAND_MEM2PKT_FILL(dec_pay_load, t0_s2s.out_page_addr, (txdma2_global.payload_size - txdma2_global.pad_size), 0, 0, 0)

    modify_field(p4_txdma_intr.dma_cmd_ptr, TXDMA2_DECRYPT_DMA_COMMANDS_OFFSET);
}

//stage 3
action esp_v4_tunnel_n2h_txdma2_load_pad_size_and_l4_proto(pad_size, l4_proto)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    IPSEC_DECRYPT_TXDMA2_T0_S2S_SCRATCH
    modify_field(txdma2_global.pad_size, pad_size);
    modify_field(ipsec_to_stage3_scratch.ipsec_cb_addr, ipsec_to_stage3.ipsec_cb_addr);
    modify_field(ipsec_to_stage3_scratch.block_size, ipsec_to_stage3.block_size);
}
 
//stage 2 table 2 
action esp_v4_tunnel_n2h_txdma2_load_ipsec_int(in_desc, out_desc,
                                               in_page, out_page,
                                               ipsec_cb_index, headroom,
                                               tailroom, headroom_offset,
                                               tailroom_offset,
                                               payload_start, buf_size,
                                               payload_size, l4_protocol, pad_size, 
                                               spi, ipsec_int_pad, status_addr)
{
    IPSEC_INT_HDR_SCRATCH
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    modify_field(txdma2_global.pad_size, pad_size);
    modify_field(txdma2_global.l4_protocol, l4_protocol);
    modify_field(txdma2_global.payload_size, payload_size);
    modify_field(t0_s2s.tailroom_offset, tailroom_offset); 
    modify_field(t0_s2s.headroom_offset, headroom_offset);

    modify_field(p4plus2p4_hdr.table2_valid, 0);
}

//stage 2 table 1 
action esp_v4_tunnel_n2h_txdma2_load_out_desc(addr0, offset0, length0,
                                        addr1, offset1, length1,
                                        addr2, offset2, length2,
                                        nextptr, rsvd)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    modify_field(t0_s2s.out_page_addr, addr0);
    //modify_field(txdma2_global.payload_size, length0-ESP_FIXED_HDR_SIZE);
    modify_field(barco_desc_scratch.L0, length0);
    modify_field(barco_desc_scratch.O0, offset0);
    modify_field(p4plus2p4_hdr.table1_valid, 0);
}

//stage 2 table 0 
action esp_v4_tunnel_n2h_txdma2_load_in_desc(BARCO_SHADOW_PARAMS)
{
    IPSEC_DECRYPT_GLOBAL_SCRATCH
    GENERATE_BARCO_SHADOW_PARAMS_D
}

//stage 2 table0
action esp_v4_tunnel_n2h_txdma2_load_barco_req(input_list_address, output_list_address,
                                         command, key_desc_index,
                                         iv_address, auth_tag_addr,
                                         header_size, status_address,
                                         opaque_tag_value, opaque_tag_write_en,
                                         rsvd1, sector_size, application_tag)

{
    BARCO_REQ_SCRTATCH_SET
    IPSEC_DECRYPT_TXDMA2_T0_S2S_SCRATCH
    modify_field(txdma2_global.in_desc_addr, input_list_address);

    modify_field(p4plus2p4_hdr.table0_valid, 1);
    modify_field(common_te0_phv.table_pc, 0);
    modify_field(common_te0_phv.table_raw_table_size, 7);
    modify_field(common_te0_phv.table_lock_en, 0);
    modify_field(common_te0_phv.table_addr, input_list_address+64);

    modify_field(p4plus2p4_hdr.table1_valid, 1);
    modify_field(common_te1_phv.table_pc, 0);
    modify_field(common_te1_phv.table_raw_table_size, 7);
    modify_field(common_te1_phv.table_lock_en, 0);
    modify_field(common_te1_phv.table_addr, output_list_address+64);

    modify_field(p4plus2p4_hdr.table2_valid, 1);
    modify_field(common_te2_phv.table_pc, 0);
    modify_field(common_te2_phv.table_raw_table_size, 7);
    modify_field(common_te2_phv.table_lock_en, 0);
    modify_field(common_te2_phv.table_addr, input_list_address);
    modify_field(ipsec_to_stage2.out_desc_addr, output_list_address);
}

//stage 1
action esp_v4_tunnel_n2h_txdma2_load_barco_req_ptr(barco_req_address)
{
    modify_field(p4plus2p4_hdr.table0_valid, 1);
    modify_field(common_te0_phv.table_pc, 0);
    modify_field(common_te0_phv.table_raw_table_size, 6);
    modify_field(common_te0_phv.table_lock_en, 0);
    modify_field(common_te0_phv.table_addr, barco_req_address);
}

//stage 1
action esp_v4_tunnel_n2h_txdma2_increment_global_ci(BARCO_SHADOW_PARAMS)
{
    GENERATE_BARCO_SHADOW_PARAMS_D
}
 

//stage 0
action esp_v4_tunnel_n2h_txdma2_initial_table(rsvd, cosA, cosB,
                                       cos_sel, eval_last, host, total, pid,
                                       rxdma_ring_pindex, rxdma_ring_cindex,
                                       barco_ring_pindex, barco_ring_cindex,
                                       key_index, new_key_index, iv_size, icv_size,
                                       expected_seq_no, last_replay_seq_no,
                                       replay_seq_no_bmp, barco_enc_cmd,
                                       ipsec_cb_index, barco_full_count, is_v6,
                                       cb_pindex, cb_cindex, 
                                       barco_pindex, barco_cindex, 
                                       cb_ring_base_addr, barco_ring_base_addr, 
                                       iv_salt, vrf_vlan)
{
    IPSEC_CB_SCRATCH

    modify_field(p4_intr_global_scratch.lif, p4_intr_global.lif);
    modify_field(p4_intr_global_scratch.tm_iq, p4_intr_global.tm_iq);
    modify_field(p4_txdma_intr_scratch.qid, p4_txdma_intr.qid);
    modify_field(p4_txdma_intr_scratch.qtype, p4_txdma_intr.qtype);
    modify_field(p4_txdma_intr_scratch.qstate_addr, p4_txdma_intr.qstate_addr);

    modify_field(txdma2_global.ipsec_cb_index, ipsec_cb_index);
    modify_field(txdma2_global.iv_size, iv_size);
 
    modify_field(p4plus2p4_hdr.table0_valid, 1);
    modify_field(common_te0_phv.table_pc, 0);
    modify_field(common_te0_phv.table_raw_table_size, 7);
    modify_field(common_te0_phv.table_lock_en, 0);
    //modify_field(common_te0_phv.table_addr, BRQ_REQ_RING_BASE_ADDR+(BRQ_REQ_RING_ENTRY_SIZE * barco_ring_cindex));
    modify_field(ipsec_to_stage3_scratch.ipsec_cb_addr, ipsec_to_stage3.ipsec_cb_addr);
}
