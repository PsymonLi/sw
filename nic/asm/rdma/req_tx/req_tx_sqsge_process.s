# include "capri.h"
#include "req_tx.h"
#include "sqcb.h"

struct req_tx_phv_t p;
struct req_tx_s3_t0_k k;

#define SGE_TO_LKEY_T struct req_tx_sge_to_lkey_info_t
#define SGE_TO_LKEY_P t0_s2s_sge_to_lkey_info
#define SQCB_WRITE_BACK_P t2_s2s_sqcb_write_back_info
#define SQCB_WRITE_BACK_SEND_WR_P t2_s2s_sqcb_write_back_info_send_wr
#define WQE_TO_SGE_P t2_s2s_wqe_to_sge_info

#define IN_P t0_s2s_wqe_to_sge_info
#define IN_TO_S_P to_s3_sqsge_info

#define TO_S0_SQSGE_P to_s0_sqsge_info
#define TO_S3_SQSGE_P to_s3_sqsge_info
#define TO_S4_DCQCN_BIND_MW_P to_s4_dcqcn_bind_mw_info
#define TO_S6_ADD_HDR2_P to_s6_add_hdr2_info

#define K_CURRENT_SGE_ID CAPRI_KEY_RANGE(IN_P, current_sge_id_sbit0_ebit1, current_sge_id_sbit2_ebit7)
#define K_CURRENT_SGE_OFFSET CAPRI_KEY_RANGE(IN_P, current_sge_offset_sbit0_ebit1, current_sge_offset_sbit26_ebit31)
#define K_REMAINING_PAYLOAD_BYTES CAPRI_KEY_RANGE(IN_P, remaining_payload_bytes_sbit0_ebit1, remaining_payload_bytes_sbit10_ebit15)
#define K_DMA_CMD_START_INDEX CAPRI_KEY_FIELD(IN_P, dma_cmd_start_index)
#define K_NUM_VALID_SGES CAPRI_KEY_RANGE(IN_P, num_valid_sges_sbit0_ebit1, num_valid_sges_sbit2_ebit7)
#define K_HEADER_TEMPLATE_ADDR CAPRI_KEY_FIELD(IN_TO_S_P, header_template_addr)
#define K_PACKET_LEN CAPRI_KEY_RANGE(IN_TO_S_P, packet_len_sbit0_ebit7, packet_len_sbit8_ebit13)
#define K_PRIV_OPER_ENABLE CAPRI_KEY_FIELD(IN_TO_S_P, priv_oper_enable)

%%
    .param    req_tx_sqlkey_process
    .param    req_tx_sqlkey_rsvd_lkey_process
    .param    req_tx_dcqcn_enforce_process
    .param    req_tx_sqsge_iterate_process

.align
req_tx_sqsge_process:
    // Use conditional flag to select between sge_index 0 and 1
    // sge_index = 0
    setcf          c7, [c0]

    // sge_p[0]
    //add            r1, HBM_CACHE_LINE_SIZE_BITS, r0
    // Data structures are accessed from bottom to top in big-endian, hence go to
    // the bottom of the SGE_T
    // big-endian
    add            r1, r0, (HBM_NUM_SGES_PER_CACHELINE - 1), LOG_SIZEOF_SGE_T_BITS

    // r2 = k.args.current_sge_offset
    add            r2, r0, K_CURRENT_SGE_OFFSET

    // r3 = k.args.remaining_payload_bytes
    add            r3, r0, K_REMAINING_PAYLOAD_BYTES

    // r5 = packet_len
    add            r5, r0, K_PACKET_LEN

sge_loop:
    // sge_remaining_bytes = sge_p->len - current_sge_offset
    CAPRI_TABLE_GET_FIELD(r4, r1, SGE_T, len)
    sub            r4, r4, r2

    // transfer_bytes = min(sge_remaining_bytes, remaining_payload_bytes)
    slt            c1, r4, r3
    cmov           r4, c1, r4, r3

    // transfer_va = sge_p->va + current_sge_offset
    CAPRI_TABLE_GET_FIELD(r6, r1, SGE_T, va)
    add            r6, r6, r2

    // Get common.common_t[0]_s2s or common.common_t[1]_s2s... args based on sge_index
    // to invoke programs in multiple MPUs
    CAPRI_GET_TABLE_0_OR_1_ARG(req_tx_phv_t, r7, c7)

    // Fill stage 2 stage data in req_tx_sge_lkey_info_t for next stage
    CAPRI_SET_FIELD(r7, SGE_TO_LKEY_T, sge_va, r6)
    CAPRI_SET_FIELD(r7, SGE_TO_LKEY_T, sge_bytes, r4)

    // current_sge_offset += transfer_bytes
    add            r2, r2, r4

    // remaining_payload_bytes -= transfer_bytes
    sub            r3, r3, r4

    sle            c2, r3, r0
    slt            c3, 1, K_NUM_VALID_SGES
   
    setcf          c4, [!c2 & c3 & c7]

    // packet_len += transfer_bytes
    add            r5, r5, r4

    cmov           r6, c7, 0, 1
    CAPRI_SET_FIELD(r7, SGE_TO_LKEY_T, sge_index, r6)

    add            r4, r0, K_DMA_CMD_START_INDEX
    add.!c7        r4, r4, MAX_PYLD_DMA_CMDS_PER_SGE
    CAPRI_SET_FIELD(r7, SGE_TO_LKEY_T, dma_cmd_start_index, r4)

    // r6 = hbm_addr_get(PHV_GLOBA_KT_BASE_ADDR_GET())
    KT_BASE_ADDR_GET2(r6, r4)

    // r4 = sge_p->lkey
    CAPRI_TABLE_GET_FIELD(r4, r1, SGE_T, l_key)

    seq            c6, r4, RDMA_RESERVED_LKEY_ID
    CAPRI_SET_FIELD_C(r7, SGE_TO_LKEY_T, rsvd_key_err, 1, c6)
    crestore.c6    [c6], K_PRIV_OPER_ENABLE, 0x1

    // key_addr = hbm_addr_get(PHV_GLOBAL_KT_BASE_ADDR_GET())+
    //                     ((sge_p->lkey >> KEY_INDEX_MASK) * sizeof(key_entry_t));
    KEY_ENTRY_ADDR_GET(r6, r6, r4)

    // r4 = sge_p->len
    CAPRI_TABLE_GET_FIELD(r4, r1, SGE_T, len)

    // if (current_sge_offset == sge_p->len)
    seq            c1, r2, r4

    // Get common.common_te[0]_phv_table_addr or common.common_te[1]_phv_table_Addr ... based on
    // sge_index to invoke program in multiple MPUs
    CAPRI_GET_TABLE_0_OR_1_K(req_tx_phv_t, r7, c7)
    // aligned_key_addr and key_id sent to next stage to load lkey
    CAPRI_NEXT_TABLE_I_READ_PC_C(r7, CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, req_tx_sqlkey_rsvd_lkey_process, req_tx_sqlkey_process, r6, c6)

    // big-endian - subtract sizeof(sge_t) as sges are read from bottom to top in big-endian format
    // sge_p[1]
    sub.c1         r1, r1, 1, LOG_SIZEOF_SGE_T_BITS

    // current_sge_offset = 0;
    add.c1         r2, r0, r0

    // while((remaining_payload_bytes > 0) &&
    //       (num_valid_sges > 1) &&
    //       (sge_index == 0)
    bcf            [c4], sge_loop
    // sge_index = 1, if looping
    setcf.c4       c7, [!c0] // branch delay slot

    // Pass packet_len to dcqcn_enforce and to add_headers_2 for padding
    phvwr          CAPRI_PHV_FIELD(TO_S4_DCQCN_BIND_MW_P, packet_len), r5
    phvwr          CAPRI_PHV_FIELD(TO_S6_ADD_HDR2_P, packet_len), r5

    // if (index == num_valid_sges)
    srl            r1, r1, LOG_SIZEOF_SGE_T_BITS
    sub            r1, (HBM_NUM_SGES_PER_CACHELINE-1), r1
    seq            c1, r1, K_NUM_VALID_SGES

    // current_sge_id = 0
    add.c1         r1, r0, r0

    // current_sge_offset = 0
    add.c1         r2, r0, r0

    // else

    // current_sge_id = k.args.current_sge_id + sge_index
    add.!c1        r1, r1, K_CURRENT_SGE_ID

    // in_progress = TRUE
    cmov           r4, c1, 0, 1

    bcf            [!c2 & !c1], iterate_sges
    // num_sges = k.args.current_sge_id + k.args.num_valid_sges
    add            r6, K_CURRENT_SGE_ID, K_NUM_VALID_SGES // Branch Delay Slot

    // Get Table 0/1 arg base pointer as it was modified to 0/1 K base
    CAPRI_GET_TABLE_0_OR_1_ARG_NO_RESET(req_tx_phv_t, r7, c7)

    // if (index == num_valid_sges) last = TRUE else last = FALSE;
    cmov           r3, c1, 1, 0

    CAPRI_RESET_TABLE_2_ARG()
    phvwrpair CAPRI_PHV_FIELD(SQCB_WRITE_BACK_P, in_progress), r4, \
              CAPRI_PHV_RANGE(SQCB_WRITE_BACK_P, op_type, first), CAPRI_KEY_RANGE(IN_P, op_type, first)
    phvwrpair CAPRI_PHV_FIELD(SQCB_WRITE_BACK_P, last_pkt), r3, \
              CAPRI_PHV_FIELD(SQCB_WRITE_BACK_P, num_sges), r6
    phvwrpair CAPRI_PHV_FIELD(SQCB_WRITE_BACK_P, current_sge_offset), r2, \
              CAPRI_PHV_FIELD(SQCB_WRITE_BACK_P, current_sge_id), r1
    phvwr     CAPRI_PHV_RANGE(SQCB_WRITE_BACK_SEND_WR_P, op_send_wr_imm_data, op_send_wr_inv_key_or_ah_handle), \
              CAPRI_KEY_RANGE(IN_P, imm_data_sbit0_ebit7, inv_key_or_ah_handle_sbit24_ebit31)
    // rest of the fields are initialized to default

    mfspr          r1, spr_mpuid
    seq            c1, r1[4:2], STAGE_3

    seq           c2, CAPRI_KEY_FIELD(IN_TO_S_P, congestion_mgmt_enable), 1  
    bcf           [c1 & c2], write_back
    add           r1, AH_ENTRY_T_SIZE_BYTES, K_HEADER_TEMPLATE_ADDR, HDR_TEMP_ADDR_SHIFT // Branch Delay Slot

write_back_mpu_only:
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, req_tx_dcqcn_enforce_process, r1)
 
    nop.e
    nop

write_back:
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, req_tx_dcqcn_enforce_process, r1)

    nop.e
    nop

iterate_sges:
    // increment dma_cmd_start index by num of DMA cmds consumed for 2 SGEs per pass
    add            r4, K_DMA_CMD_START_INDEX, (MAX_PYLD_DMA_CMDS_PER_SGE * 2)
    // Error if there are no dma cmd space to compose
    beqi           r4, REQ_TX_DMA_CMD_PYLD_BASE_END, err_no_dma_cmds
    CAPRI_RESET_TABLE_2_ARG() // Branch Delay Slot
    
    phvwr CAPRI_PHV_RANGE(WQE_TO_SGE_P, in_progress, inv_key_or_ah_handle), \
          CAPRI_KEY_RANGE(IN_P, in_progress, inv_key_or_ah_handle_sbit24_ebit31)
    phvwrpair CAPRI_PHV_FIELD(WQE_TO_SGE_P, current_sge_id), r1, \
              CAPRI_PHV_FIELD(WQE_TO_SGE_P, current_sge_offset), r2
    phvwrpair CAPRI_PHV_FIELD(WQE_TO_SGE_P, remaining_payload_bytes), r3, \
              CAPRI_PHV_FIELD(WQE_TO_SGE_P, dma_cmd_start_index), r4
    sub            r6, K_NUM_VALID_SGES, 2 
    phvwr CAPRI_PHV_FIELD(WQE_TO_SGE_P, num_valid_sges), r6
    // Pass packet_len to stage 0 and stage3 as on recirc sqsge_process
    // can run in either one of those stages
    phvwr          CAPRI_PHV_FIELD(TO_S0_SQSGE_P, packet_len), r5
    phvwr          CAPRI_PHV_FIELD(TO_S3_SQSGE_P, packet_len), r5

    mfspr          r1, spr_tbladdr
    add            r1, r1, 2, LOG_SIZEOF_SGE_T

    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, req_tx_sqsge_iterate_process, r1)

end:
    nop.e
    nop

err_no_dma_cmds:
    nop.e
    nop

