#include "../../../p4/common-p4+/capri_dma_cmd.p4"
#include "../../../p4/common-p4+/capri_doorbell.p4"

header_type scratch_metadata_t {
    fields {
        qid             : 24;
        dma_size        : 16;
    }
}

header_type flow_key_t {
    fields {
        pad             : 12;
        flow_ktype      : 4;
        flow_src        : 128;
        flow_dst        : 128;
        flow_proto      : 8;
        flow_dport      : 16;
        flow_sport      : 16;
    }
}

// PHV instantiation
@pragma dont_trim
metadata cap_phv_intr_global_t capri_intr;
@pragma dont_trim
metadata cap_phv_intr_p4_t capri_p4_intr;
@pragma dont_trim
metadata cap_phv_intr_rxdma_t capri_rxdma_intr;

@pragma dont_trim
metadata p4_2_p4plus_app_header_t app_header;
@pragma dont_trim
metadata p4_2_p4plus_ext_app_header_t ext_app_header;

@pragma dont_trim
@pragma pa_header_union ingress app_header
metadata artemis_p4_to_rxdma_header_t p4_to_rxdma;
@pragma dont_trim
@pragma pa_header_union ingress ext_app_header
metadata predicate_header_t predicate_header;

@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_0;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_1;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_2;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_3;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_4;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_5;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_6;
@pragma dont_trim
metadata p4plus_common_to_stage_t to_stage_7;

@pragma dont_trim
metadata p4plus_common_global_t common_global;
@pragma dont_trim
metadata p4plus_common_raw_table_engine_phv_t common_te0_phv;
@pragma dont_trim
metadata p4plus_common_s2s_t common_t0_s2s;
@pragma dont_trim
metadata p4plus_common_raw_table_engine_phv_t common_te1_phv;
@pragma dont_trim
metadata p4plus_common_s2s_t common_t1_s2s;
@pragma dont_trim
metadata p4plus_common_raw_table_engine_phv_t common_te2_phv;
@pragma dont_trim
metadata p4plus_common_s2s_t common_t2_s2s;
@pragma dont_trim
metadata p4plus_common_raw_table_engine_phv_t common_te3_phv;
@pragma dont_trim
metadata p4plus_common_s2s_t common_t3_s2s;

header_type rxdma_common_pad_t {
    fields {
        rxdma_common_pad : 96;
    }
}

@pragma dont_trim
metadata rxdma_common_pad_t     rxdma_common_pad;

@pragma scratch_metadata
metadata scratch_metadata_t     scratch_metadata;

@pragma dont_trim
@pragma scratch_metadata
metadata qstate_hdr_t           scratch_qstate_hdr;

@pragma dont_trim
@pragma scratch_metadata
metadata qstate_info_t          scratch_qstate_info;

@pragma scratch_metadata
metadata flow_key_t             scratch_flow_key;

@pragma dont_trim
metadata doorbell_addr_t        doorbell_addr;
@pragma dont_trim
@pragma pa_header_union ingress to_stage_1
metadata doorbell_data_t        doorbell_data;

// DMA commands
@pragma dont_trim
@pragma pa_header_union ingress to_stage_2
metadata dma_cmd_pkt2mem_t      pktdesc_pkt2mem;

@pragma dont_trim
@pragma pa_header_union ingress to_stage_3
metadata dma_cmd_pkt2mem_t      pktbuf_pkt2mem;

@pragma dont_trim
@pragma pa_header_union ingress to_stage_4
metadata dma_cmd_phv2mem_t      predicate_phv2mem;
@pragma dont_trim
@pragma pa_header_union ingress to_stage_4
metadata dma_cmd_phv2mem_t      doorbell_phv2mem;

@pragma dont_trim
@pragma pa_header_union ingress to_stage_5
metadata dma_cmd_phv2mem_t      doorbell2_phv2mem;
