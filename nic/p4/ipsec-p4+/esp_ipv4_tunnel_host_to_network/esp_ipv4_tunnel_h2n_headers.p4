#include "../ipsec_defines.h"
#include "../ipsec_dummy_defines.h"
#include "../ipsec_common_headers.p4"

#define IPSEC_BLOCK_SIZE 16

header_type ipsec_int_header_t {
    fields {
        in_desc           : ADDRESS_WIDTH;
        out_desc          : ADDRESS_WIDTH;
        in_page           : ADDRESS_WIDTH;
        out_page          : ADDRESS_WIDTH;
        ipsec_cb_index    : 16;
        headroom          : 8;
        tailroom          : 8;
        headroom_offset   : 16;
        tailroom_offset   : 16;
        payload_start     : 16;
        buf_size          : 16;
        payload_size      : 16;
        pad_size          : 8;
        l4_protocol       : 8;
        ip_hdr_size       : 8;
        //ipsec_int_pad     : 56;
    }
}

header_type ipsec_int_pad_t {
    fields {
        ipsec_int_pad : 56;
        status : 64;
    }
}

#define IPSEC_INT_HDR_SCRATCH \
    modify_field(ipsec_int_hdr_scratch.in_desc, in_desc); \
    modify_field(ipsec_int_hdr_scratch.out_desc, out_desc); \ 
    modify_field(ipsec_int_hdr_scratch.in_page, in_page); \ 
    modify_field(ipsec_int_hdr_scratch.out_page, out_page); \ 
    modify_field(ipsec_int_hdr_scratch.ipsec_cb_index, ipsec_cb_index); \ 
    modify_field(ipsec_int_hdr_scratch.headroom, headroom); \ 
    modify_field(ipsec_int_hdr_scratch.tailroom, tailroom); \ 
    modify_field(ipsec_int_hdr_scratch.headroom_offset, headroom_offset); \ 
    modify_field(ipsec_int_hdr_scratch.tailroom_offset, tailroom_offset); \ 
    modify_field(ipsec_int_hdr_scratch.payload_start, payload_start); \ 
    modify_field(ipsec_int_hdr_scratch.buf_size, buf_size); \ 
    modify_field(ipsec_int_hdr_scratch.payload_size, payload_size); \ 
    modify_field(ipsec_int_hdr_scratch.pad_size, pad_size); \ 
    modify_field(ipsec_int_hdr_scratch.l4_protocol, l4_protocol); \ 
    modify_field(ipsec_int_hdr_scratch.ip_hdr_size, ip_hdr_size); \ 
    
header_type esp_header_t {
    fields {
        spi : 32;
        seqno : 32;
        iv    : 64;
        iv2   : 64;
    }
}


#define IPSEC_CB_ENC_NEXTHOP_ID_OFFSET      60
#define IPSEC_CB_ENC_NEXTHOP_TYPE_OFFSET    62

header_type ipsec_cb_metadata_t {
    fields {
        pc                      : 8;
        rsvd                    : 8;
        cosA                    : 4;
        cosB                    : 4;
        cos_sel                 : 8;
        eval_last               : 8;
        host                    : 4;
        total                   : 4;
        pid                     : 16;

        rxdma_ring_pindex       : RING_INDEX_WIDTH;
        rxdma_ring_cindex       : RING_INDEX_WIDTH;
        barco_ring_pindex       : RING_INDEX_WIDTH;
        barco_ring_cindex       : RING_INDEX_WIDTH;

        key_index               : 16;
        iv_size                 : 8;
        icv_size                : 8;
        spi                     : 32;
        esn_lo                  : 32;
        iv                      : 64;
        ipsec_cb_index          : 16;
        cb_pindex               : 16;
        cb_cindex               : 16;
        barco_pindex            : 16;
        barco_cindex            : 16;
        cb_ring_base_addr       : 32;
        barco_ring_base_addr    : 32;
        iv_salt                 : 32;
        barco_full_count        : 8;
        flags                   : 8;
        nexthop_id              : 16; // IPSEC_CB_ENC_NEXTHOP_ID_OFFSET
        nexthop_type            : 8;  // IPSEC_CB_ENC_NEXTHOP_TYPE_OFFSET
        nexthop_pad             : 8;
    }
}

#define IPSEC_CB_SCRATCH \
    modify_field(ipsec_cb_scratch.rsvd, rsvd); \
    modify_field(ipsec_cb_scratch.cosA, cosA); \
    modify_field(ipsec_cb_scratch.cosB, cosB); \
    modify_field(ipsec_cb_scratch.cos_sel, cos_sel); \
    modify_field(ipsec_cb_scratch.eval_last, eval_last); \
    modify_field(ipsec_cb_scratch.host, host); \
    modify_field(ipsec_cb_scratch.total, total); \
    modify_field(ipsec_cb_scratch.pid, pid); \
    modify_field(ipsec_cb_scratch.rxdma_ring_pindex, rxdma_ring_pindex); \
    modify_field(ipsec_cb_scratch.rxdma_ring_cindex, rxdma_ring_cindex); \
    modify_field(ipsec_cb_scratch.barco_ring_pindex, barco_ring_pindex); \
    modify_field(ipsec_cb_scratch.barco_ring_cindex, barco_ring_cindex); \
    modify_field(ipsec_cb_scratch.key_index, key_index); \
    modify_field(ipsec_cb_scratch.iv_size, iv_size); \
    modify_field(ipsec_cb_scratch.icv_size, icv_size); \
    modify_field(ipsec_cb_scratch.spi, spi); \
    modify_field(ipsec_cb_scratch.esn_lo, esn_lo); \
    modify_field(ipsec_cb_scratch.iv, iv); \
    modify_field(ipsec_cb_scratch.ipsec_cb_index, ipsec_cb_index); \
    modify_field(ipsec_cb_scratch.barco_full_count, barco_full_count); \
    modify_field(ipsec_cb_scratch.cb_pindex, cb_pindex); \
    modify_field(ipsec_cb_scratch.cb_cindex, cb_cindex); \
    modify_field(ipsec_cb_scratch.barco_pindex, barco_pindex); \
    modify_field(ipsec_cb_scratch.barco_cindex, barco_cindex); \
    modify_field(ipsec_cb_scratch.cb_ring_base_addr, cb_ring_base_addr); \
    modify_field(ipsec_cb_scratch.barco_ring_base_addr, barco_ring_base_addr); \
    modify_field(ipsec_cb_scratch.iv_salt, iv_salt); \
    modify_field(ipsec_cb_scratch.flags, flags); \          
    modify_field(ipsec_cb_scratch.nexthop_id, nexthop_id); \
    modify_field(ipsec_cb_scratch.nexthop_type, nexthop_type); \
    modify_field(ipsec_cb_scratch.nexthop_pad, nexthop_pad);

#define IPSEC_CB_SCRATCH_WITH_PC \
    modify_field(ipsec_cb_scratch.pc, pc); \
    IPSEC_CB_SCRATCH 

header_type h2n_stats_header_t {
    fields {
        h2n_rx_pkts : 64;
        h2n_rx_bytes : 64;
        h2n_rx_drops : 64;
        h2n_tx_pkts : 64;
        h2n_tx_bytes : 64; 
        h2n_tx_drops : 64;
    }
}

#define H2N_STATS_UPDATE_PARAMS h2n_rx_pkts,h2n_rx_bytes,h2n_rx_drops,h2n_tx_pkts,h2n_tx_bytes,h2n_tx_drops
#define H2N_STATS_UPDATE_SET \
    modify_field(ipsec_stats_scratch.h2n_rx_pkts, h2n_rx_pkts); \
    modify_field(ipsec_stats_scratch.h2n_rx_bytes, h2n_rx_bytes); \
    modify_field(ipsec_stats_scratch.h2n_rx_drops, h2n_rx_drops); \
    modify_field(ipsec_stats_scratch.h2n_tx_pkts, h2n_tx_pkts); \
    modify_field(ipsec_stats_scratch.h2n_tx_bytes, h2n_tx_bytes); \
    modify_field(ipsec_stats_scratch.h2n_tx_drops, h2n_tx_drops); \
 
