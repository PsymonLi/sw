header_type scratch_metadata_t {
    fields {
        flag            :   1;
        field4          :   4;
        field7          :   7;
        field8          :   8;
        field10         :  10;
        field12         :  12;
        field16         :  16;
        field20         :  20;
        field23         :  23;
        field32         :  32;
        field40         :  40;
        field64         :  64;
        field128        : 128;
        stag            :  32;
        dtag            :  32;
        qid             :  24;
        dma_size        :  16;
        hint_valid      :   1;
        mapping_hash    :  10;
        mapping_hint    :  19;
    }
}

header_type lpm_metadata_t {
    fields {
        lpm1_key        : 128;
        lpm1_base_addr  : 40;
        lpm1_next_addr  : 40;

        lpm2_key        : 128;
        lpm2_base_addr  : 40;
        lpm2_next_addr  : 40;

        sacl_base_addr  : 40;
        root_change     : 1;
        root_count      : 3;
        stag_count      : 4;
        dtag_count      : 4;
        sacl_proc_state : 4;

        mapping_ohash   : 32;
        mapping_tag_idx : 24;

        stag            : 32;
        dtag            : 32;

        stag0           : 32;
        stag1           : 32;
        stag2           : 32;
        stag3           : 32;
        stag4           : 32;
        dtag0           : 32;
        dtag1           : 32;
        dtag2           : 32;
        dtag3           : 32;
        dtag4           : 32;
    }
}

@pragma scratch_metadata
metadata scratch_metadata_t     scratch_metadata;

@pragma scratch_metadata
metadata qstate_hdr_t           scratch_qstate_hdr;

@pragma scratch_metadata
metadata qstate_info_t          scratch_qstate_info;

@pragma dont_trim
@pragma pa_header_union ingress app_header
metadata apulu_p4i_to_rxdma_header_t p4_to_rxdma;

@pragma dont_trim
@pragma pa_header_union ingress to_stage_1
metadata doorbell_data_t        doorbell_data;

@pragma dont_trim
metadata apulu_rx_to_tx_header_t rx_to_tx_hdr;

// DMA commands
@pragma pa_align 128
@pragma dont_trim
metadata dma_cmd_phv2mem_t      pktdesc_phv2mem;

@pragma dont_trim
metadata dma_cmd_phv2mem_t      pktdesc2_part1_phv2mem;

@pragma dont_trim
metadata dma_cmd_phv2mem_t      pktdesc2_part2_phv2mem;

@pragma dont_trim
metadata dma_cmd_pkt2mem_t      pktbuf_pkt2mem;

@pragma dont_trim
metadata dma_cmd_phv2mem_t      doorbell_phv2mem;

@pragma dont_trim
metadata lpm_metadata_t         lpm_metadata;
