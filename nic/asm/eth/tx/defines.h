
#include "../defines.h"

#define LG2_TX_DESC_SIZE            (4)
#define LG2_TX_CMPL_DESC_SIZE       (4)
#define LG2_TX_SG_ELEM_SIZE         (4)
#define LG2_TX_SG_MAX_READ_SIZE     (6)
#define TX_SG_MAX_READ_SIZE         (64)    // 4 sg elements
#define TX_SG_MAX_READ_ELEM         (4)
#define LG2_TX_SG_DESC_SIZE         (8)
#define LG2_TX_QSTATE_SIZE          (6)

// TX limits
#define MAX_DESC_SPEC               (64)
#define MAX_DESC_PER_PHV            (4)
#define MAX_BYTES_PER_PHV           (16384)

// TX Spurious Doorbell config
#define MAX_SPURIOUS_DB             (8)
#define LG2_MAX_SPURIOUS_DB         (3)

// TX Descriptor Opcodes
#define TXQ_DESC_OPCODE_CALC_NO_CSUM        0x0
#define TXQ_DESC_OPCODE_CALC_CSUM           0x1
#define TXQ_DESC_OPCODE_CALC_CSUM_TCPUDP    0x2
#define TXQ_DESC_OPCODE_TSO                 0x3

// DMA Macros
#define ETH_DMA_CMD_START_OFFSET    (CAPRI_PHV_START_OFFSET(dma_dma_cmd_type) / 16)
#define ETH_DMA_CMD_START_FLIT      ((offsetof(p, dma_dma_cmd_type) / 512) + 1)
#define ETH_DMA_CMD_START_INDEX     0

#define GET_BUF_ADDR(n, _r, hdr) \
    or          _r, k.eth_tx_##hdr##_addr_lo##n, k.eth_tx_##hdr##_addr_hi##n, sizeof(k.eth_tx_##hdr##_addr_lo##n); \
    add         _r, r0, _r.dx; \
    or          _r, _r[63:16], _r[11:8], sizeof(k.eth_tx_##hdr##_addr_lo##n);

#define GET_FRAG_ADDR(n, _r) \
    or          _r, d.addr_lo##n, d.addr_hi##n, sizeof(d.addr_lo##n); \
    add         _r, r0, _r.dx; \
    or          _r, _r[63:16], _r[11:8], sizeof(d.addr_lo##n);

#define DMA_INTRINSIC(n, _r) \
    DMA_PHV2PKT_3(_r, CAPRI_PHV_START_OFFSET(p4_intr_global_tm_iport), CAPRI_PHV_END_OFFSET(p4_intr_global_tm_instance_type), CAPRI_PHV_START_OFFSET(p4_txdma_intr_qid), CAPRI_PHV_END_OFFSET(p4_txdma_intr_txdma_rsv), CAPRI_PHV_START_OFFSET(eth_tx_app_hdr##n##_p4plus_app_id), CAPRI_PHV_END_OFFSET(eth_tx_app_hdr##n##_vlan_tag), r7);

#define DMA_PKT(n, _r_addr, _r_ptr, hdr) \
    GET_BUF_ADDR(n, _r_addr, hdr); \
    DMA_MEM2PKT(_r_ptr, c0, k.eth_tx_global_host_queue, _r_addr, k.{eth_tx_##hdr##_len##n}.hx);

#define DMA_HDR(n, _r_addr, _r_ptr, hdr) \
    GET_BUF_ADDR(n, _r_addr, hdr); \
    seq          c7, k.eth_tx_global_num_sg_elems, 0; \
    DMA_MEM2PKT(_r_ptr, c7, k.eth_tx_global_host_queue, _r_addr, k.{eth_tx_##hdr##_len##n}.hx);

#define DMA_TSO_HDR(n, _r_addr, _r_ptr, hdr) \
    phvwrpair   p.eth_tx_app_hdr##n##_ip_id_delta, k.eth_tx_t2_s2s_tso_ipid_delta, p.eth_tx_app_hdr##n##_tcp_seq_delta, k.eth_tx_t2_s2s_tso_seq_delta;\
    DMA_MEM2PKT(_r_ptr, !c0, k.eth_tx_global_host_queue, k.eth_tx_t2_s2s_tso_hdr_addr, k.eth_tx_t2_s2s_tso_hdr_len);

#define DMA_FRAG(n, _c, _r_addr, _r_ptr) \
    GET_FRAG_ADDR(n, _r_addr); \
    DMA_MEM2PKT(_r_ptr, _c, k.eth_tx_global_host_queue, _r_addr, d.{len##n}.hx);

#define BUILD_APP_HEADER(n, _r_t1, _r_t2) \
    phvwri      p.eth_tx_app_hdr##n##_p4plus_app_id, P4PLUS_APPTYPE_CLASSIC_NIC; \
    phvwr       p.eth_tx_app_hdr##n##_insert_vlan_tag, d.vlan_insert##n;\
    phvwr       p.eth_tx_app_hdr##n##_vlan_tag, d.{vlan_tci##n}.hx; \
    add         r7, r0, d.opcode##n##; \
.brbegin; \
    br          r7[1:0]; \
    nop; \
    .brcase     TXQ_DESC_OPCODE_CALC_NO_CSUM; \
        b       eth_tx_opcode_done##n; \
        nop; \
    .brcase     TXQ_DESC_OPCODE_CALC_CSUM; \
        b       eth_tx_calc_csum##n; \
        nop; \
    .brcase     TXQ_DESC_OPCODE_CALC_CSUM_TCPUDP; \
        b       eth_tx_calc_csum_tcpudp##n; \
        nop; \
    .brcase     TXQ_DESC_OPCODE_TSO; \
        b       eth_tx_opcode_tso##n; \
        nop; \
.brend; \
eth_tx_calc_csum##n:; \
    or              _r_t1, d.hdr_len_lo##n, d.hdr_len_hi##n, sizeof(d.hdr_len_lo##n); \
    add             _r_t1, r0, _r_t1.dx; \
    or              _r_t1, _r_t1[63:56], _r_t1[49:48], sizeof(d.hdr_len_lo##n); \
    or              _r_t2, d.mss_or_csumoff_lo##n, d.mss_or_csumoff_hi##n, sizeof(d.mss_or_csumoff_lo##n); \
    add             _r_t2, r0, _r_t2.dx; \
    or              _r_t2, _r_t2[63:56], _r_t2[53:48], sizeof(d.mss_or_csumoff_lo##n); \
    addi            _r_t1, _r_t1, 46; \
    add             _r_t2, _r_t2, _r_t1; \
    phvwri          p.eth_tx_app_hdr##n##_gso_valid, 1; \
    b               eth_tx_opcode_done##n; \
    phvwrpair       p.eth_tx_app_hdr##n##_gso_start, _r_t1, p.eth_tx_app_hdr##n##_gso_offset, _r_t2; \
eth_tx_calc_csum_tcpudp##n:; \
    seq             c7, d.encap##n, 1; \
    phvwrpair.c7    p.eth_tx_app_hdr##n##_compute_l4_csum, 1, p.eth_tx_app_hdr##n##_compute_ip_csum, 1; \
    phvwrpair.c7    p.eth_tx_app_hdr##n##_compute_inner_l4_csum, d.csum_l4_or_eot##n, p.eth_tx_app_hdr##n##_compute_inner_ip_csum, d.csum_l3_or_sot##n; \
    b               eth_tx_opcode_done##n; \
    phvwrpair.!c7   p.eth_tx_app_hdr##n##_compute_l4_csum, d.csum_l4_or_eot##n, p.eth_tx_app_hdr##n##_compute_ip_csum, d.csum_l3_or_sot##n; \
eth_tx_opcode_tso##n:; \
    phvwri          p.eth_tx_t0_s2s_do_tso, 1; \
    phvwri          p.eth_tx_app_hdr##n##_tso_valid, 1; \
    phvwri          p.eth_tx_app_hdr##n##_update_ip_len, 1; \
    phvwrpair       p.eth_tx_app_hdr##n##_update_tcp_seq_no, 1, p.eth_tx_app_hdr##n##_update_ip_id, 1; \
    bbeq            d.csum_l3_or_sot##n, 1, eth_tx_opcode_tso_sot##n; \
    nop; \
    b               eth_tx_opcode_tso_cont##n; \
    nop; \
eth_tx_opcode_tso_sot##n:;\
    or              _r_t1, d.addr_lo##n, d.addr_hi##n, sizeof(d.addr_lo##n); \
    add             _r_t1, r0, _r_t1.dx; \
    or              _r_t1, _r_t1[63:16], _r_t1[11:8], sizeof(d.addr_lo##n); \
    or              _r_t2, d.hdr_len_lo##n, d.hdr_len_hi##n, sizeof(d.hdr_len_lo##n); \
    add             _r_t2, r0, _r_t2.dx; \
    or              _r_t2, _r_t2[63:56], _r_t2[49:48], sizeof(d.hdr_len_lo##n); \
    phvwri          p.eth_tx_global_tso_sot, 1; \
    phvwri          p.eth_tx_app_hdr##n##_tso_first_segment, 1;\
    b               eth_tx_opcode_tso_done##n; \
    phvwrpair       p.eth_tx_to_s2_tso_hdr_addr, _r_t1, p.eth_tx_to_s2_tso_hdr_len, _r_t2; \
eth_tx_opcode_tso_cont##n:; \
    or              _r_t1, d.mss_or_csumoff_lo##n, d.mss_or_csumoff_hi##n, sizeof(d.mss_or_csumoff_lo##n); \
    add             _r_t1, r0, _r_t1.dx; \
    or              _r_t1, _r_t1[63:56], _r_t1[53:48], sizeof(d.mss_or_csumoff_lo##n); \
    bbeq            d.csum_l4_or_eot##n, 1, eth_tx_opcode_tso_eot##n; \
    phvwr           p.eth_tx_to_s2_tso_hdr_addr[13:0], _r_t1; \
    b               eth_tx_opcode_tso_done##n; \
    nop; \
eth_tx_opcode_tso_eot##n:; \
    phvwri          p.eth_tx_app_hdr##n##_tso_last_segment, 1;\
eth_tx_opcode_tso_done##n:;\
    seq             c7, d.encap##n, 1; \
    phvwrpair       p.eth_tx_app_hdr##n##_compute_l4_csum, 1, p.eth_tx_app_hdr##n##_compute_ip_csum, 1; \
    b               eth_tx_opcode_done##n; \
    phvwrpair.c7    p.eth_tx_app_hdr##n##_compute_inner_l4_csum, 1, p.eth_tx_app_hdr##n##_compute_inner_ip_csum, 1; \
eth_tx_opcode_done##n:;

#define DEBUG_DESCR_FLD(name) \
    add         r7, r0, d.##name

#define DEBUG_DESCR(n) \
    DEBUG_DESCR_FLD(addr_lo##n); \
    DEBUG_DESCR_FLD(addr_hi##n); \
    DEBUG_DESCR_FLD(rsvd##n); \
    DEBUG_DESCR_FLD(num_sg_elems##n); \
    DEBUG_DESCR_FLD(opcode##n); \
    DEBUG_DESCR_FLD(len##n); \
    DEBUG_DESCR_FLD(vlan_tci##n); \
    DEBUG_DESCR_FLD(hdr_len_lo##n); \
    DEBUG_DESCR_FLD(hdr_len_hi##n); \
    DEBUG_DESCR_FLD(rsvd2##n); \
    DEBUG_DESCR_FLD(vlan_insert##n); \
    DEBUG_DESCR_FLD(cq_entry##n); \
    DEBUG_DESCR_FLD(csum##n); \
    DEBUG_DESCR_FLD(mss_or_csumoff_lo##n); \
    DEBUG_DESCR_FLD(mss_or_csumoff_hi##n); \
    DEBUG_DESCR_FLD(rsvd3_or_rsvd4##n)
