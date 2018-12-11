#include "capri.h"
#include "aq_tx.h"
#include "aqcb.h"
#include "common_phv.h"
#include "p4/common/defines.h"
#include "types.h"

struct aq_tx_phv_t p;
struct aqwqe_t d;
struct aq_tx_s2_t1_k k;

#define IN_TO_S_P to_s2_info

#define TO_SQCB2_RQCB0_INFO_P to_s5_info
#define TO_SQCB0_INFO_P to_s6_info    

#define DMA_CMD_BASE r6
    
#define WQE2_TO_RQCB0_P t1_s2s_wqe2_to_rqcb0_info
#define WQE2_TO_SQCB2_P t2_s2s_wqe2_to_sqcb2_info

#define K_COMMON_GLOBAL_QID CAPRI_KEY_RANGE(phv_global_common, qid_sbit0_ebit4, qid_sbit21_ebit23)
#define K_COMMON_GLOBAL_QTYPE CAPRI_KEY_FIELD(phv_global_common, qtype)
#define K_AH_BASE_ADDR_PAGE_ID CAPRI_KEY_RANGE(IN_TO_S_P, ah_base_addr_page_id_sbit0_ebit15, ah_base_addr_page_id_sbit16_ebit21)
#define K_SQCB_BASE_ADDR_HI CAPRI_KEY_RANGE(IN_TO_S_P, sqcb_base_addr_hi_sbit0_ebit1, sqcb_base_addr_hi_sbit18_ebit23)
#define K_RQCB_BASE_ADDR_HI CAPRI_KEY_RANGE(IN_TO_S_P, rqcb_base_addr_hi_sbit0_ebit1, rqcb_base_addr_hi_sbit18_ebit23)

%%

    .param      rdma_aq_tx_rqcb0_process
    .param      rdma_aq_tx_sqcb2_process
.align
rdma_aq_tx_modify_qp_2_process:

    add         r3, r0, d.{id_ver}.wx  //TODO: Need to optimize 
    SQCB_ADDR_GET(r1, r3[23:0], K_SQCB_BASE_ADDR_HI)
    RQCB_ADDR_GET(r2, r3[23:0], K_RQCB_BASE_ADDR_HI)

    CAPRI_RESET_TABLE_1_ARG()
    CAPRI_RESET_TABLE_2_ARG()
    
hdr_update:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_AV], 1, rrq_base
    
    add         r4, d.{mod_qp.ah_id_len}.wx, r0 //BD Slot
    add         r4, r4[23:0], r0
    muli        r5, r4, AT_ENTRY_SIZE_BYTES
    nop
    
    add         r5, r5, K_AH_BASE_ADDR_PAGE_ID, HBM_PAGE_SIZE_SHIFT
    srl         r7, r5, HDR_TEMP_ADDR_SHIFT

    //ah_len
    add         r4, d.{mod_qp.ah_id_len}.wx, r0 
    srl         r4, r4, 24

    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, av_valid), 1
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, ah_len), r4
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, ah_addr), r7

    //Send to Rx pipeline for setting SQCB1 and RQCB1
    phvwr       p.rdma_feedback.modify_qp.av_valid, 1
    phvwr       p.rdma_feedback.modify_qp.ah_len, r4
    phvwr       p.rdma_feedback.modify_qp.ah_addr, r7
    
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, av_valid), 1
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, ah_len), r4
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, ah_addr), r7
    
    //setup DMA in this stage but do CB updates in later stages.
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MOD_QP_AH_SRC)
    DMA_HOST_MEM2MEM_SRC_SETUP(DMA_CMD_BASE, r4, d.{mod_qp.dma_addr}.dx)
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MOD_QP_AH_DST)
    DMA_HBM_MEM2MEM_DST_SETUP(DMA_CMD_BASE, r4, r5)
    
rrq_base:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_MAX_QP_RD_ATOMIC], 1, rsq_base

    PT_BASE_ADDR_GET2(r4)   // BD Slot
    add         r5, r4, d.{mod_qp.rrq_index}.wx, CAPRI_LOG_SIZEOF_U64
    srl         r5, r5, RRQ_BASE_ADDR_SHIFT

    //Send to Rx pipeline for setting SQCB1 and RQCB1
    phvwr       p.rdma_feedback.modify_qp.rrq_valid, 1
    phvwr       p.rdma_feedback.modify_qp.rrq_depth_log2, d.mod_qp.rrq_depth[4: 0]
    phvwr       p.rdma_feedback.modify_qp.rrq_base_addr, r5[31:0]
    
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, rrq_valid), 1
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, rrq_depth_log2), d.mod_qp.rrq_depth[4: 0]
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, rrq_base_addr), r5[31:0]
    
rsq_base:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_MAX_DEST_RD_ATOMIC], 1, state

    // r4 has the PT Base address from the previous BD slot
    add         r5, r4, d.{mod_qp.rsq_index}.wx, CAPRI_LOG_SIZEOF_U64   // BD Slot
    srl         r5, r5, RSQ_BASE_ADDR_SHIFT

    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, rsq_valid), 1
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, rsq_depth_log2), d.mod_qp.rsq_depth[4: 0]
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, rsq_base_addr), r5[31:0]

    //Send to Rx pipeline for setting SQCB1 and RQCB1
    phvwr       p.rdma_feedback.modify_qp.rsq_valid, 1
    phvwr       p.rdma_feedback.modify_qp.rsq_depth_log2, d.mod_qp.rsq_depth[4: 0]
    phvwr       p.rdma_feedback.modify_qp.rsq_base_addr, r5[31:0]
    
state:

    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_STATE], 1, tx_psn
    add         r2, d.type_state[2:0], r0
    
    //Need to translate ibv_state ionic_state
    .brbegin
     br          r2[2:0]    
     nop

    .brcase     IBV_QP_STATE_RESET
        add         r2, r0, QP_STATE_RESET
        b           state_next
        nop
    .brcase IBV_QP_STATE_INIT    
        add         r2, r0, QP_STATE_INIT
        b           state_next
        nop
    .brcase IBV_QP_STATE_RTR       
        add         r2, r0, QP_STATE_RTR
        b           state_next
        nop
    .brcase IBV_QP_STATE_RTS       
        add         r2, r0, QP_STATE_RTS
        b           state_next
        nop
    .brcase IBV_QP_STATE_SQD           
        add         r2, r0, QP_STATE_SQD
        b           state_next
        nop
    .brcase IBV_QP_STATE_SQ_ERR
        add         r2, r0, QP_STATE_SQ_ERR
        b           state_next
        nop
    .brcase IBV_QP_STATE_ERR                                            
        add         r2, r0, QP_STATE_ERR
        b           error_disable_qp
        nop
    .brcase IBV_QP_STATE_SQD_ON_ERR
        add         r2, r0, QP_STATE_SQD_ON_ERR
        b           state_next
        nop
    .brend

state_next:
    
    //State is only in SQCB0 and SQCB1
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, state), r2[2:0]
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, state_valid), 1

    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, state), r2[2:0]
    phvwr       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, state_valid), 1

    phvwr       p.rdma_feedback.modify_qp.state, r2[2:0]
    phvwr       p.rdma_feedback.modify_qp.state_valid, 1

tx_psn:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_SQ_PSN], 1, timeout
    nop

    add         r4, d.{mod_qp.sq_psn}.wx, r0
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, tx_psn), r4[23:0] 
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, tx_psn_valid), 1

    phvwr       p.rdma_feedback.modify_qp.tx_psn, r4[23:0]
    phvwr       p.rdma_feedback.modify_qp.tx_psn_valid, 1

timeout:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_TIMEOUT], 1, retry_cnt

    add         r4, d.mod_qp.retry_timeout, r0
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, local_ack_timeout), r4[4:0]
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, local_ack_timeout_valid), 1
    phvwr       CAPRI_PHV_FIELD(TO_SQCB0_INFO_P, local_ack_timeout), r4[4:0]
    phvwr       CAPRI_PHV_FIELD(TO_SQCB0_INFO_P, local_ack_timeout_valid), 1

retry_cnt:
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_RETRY_CNT], 1, pmtu

    add         r4, d.mod_qp.retry, r0
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, err_retry_count), r4[2:0]
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_RQCB0_INFO_P, err_retry_count_valid), 1

    phvwr       p.rdma_feedback.modify_qp.err_retry_count, r4[2:0]
    phvwr       p.rdma_feedback.modify_qp.err_retry_count_valid, 1
    
pmtu:

    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_PATH_MTU], 1, setup_sqcb_stages
    nop
    
    phvwrpair       CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, pmtu_log2), d.mod_qp.pmtu[4:0], CAPRI_PHV_FIELD(WQE2_TO_RQCB0_P, pmtu_valid), 1
    phvwrpair       CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, pmtu_log2), d.mod_qp.pmtu[4:0], CAPRI_PHV_FIELD(WQE2_TO_SQCB2_P, pmtu_valid), 1

    phvwrpair   p.rdma_feedback.modify_qp.pmtu_log2, d.mod_qp.pmtu[4:0], p.rdma_feedback.modify_qp.pmtu_valid, 1
    
setup_sqcb_stages:

    phvwr       p.rdma_feedback.aq_completion.op, AQ_OP_TYPE_MODIFY_QP
    phvwr       p.p4_to_p4plus.modify_qp_ext.rq_id, r3[23:0]
    add         r1, r1, (2*CB_UNIT_SIZE_BYTES)
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, rdma_aq_tx_sqcb2_process, r1)
    RQCB_ADDR_GET(r2, r3[23:0], K_RQCB_BASE_ADDR_HI)
    CAPRI_NEXT_TABLE1_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, rdma_aq_tx_rqcb0_process, r2)

done:
    nop.e
    nop

error_disable_qp:

    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_REQ_ERR_FEEDBACK)     
    DMA_PHV2PKT_SETUP(DMA_CMD_BASE, req_feedback.p4_intr_global, req_feedback.rdma_feedback)
    phvwrpair      p.req_feedback.p4_intr_global.tm_iport, TM_PORT_INGRESS, p.req_feedback.p4_intr_global.tm_oport, TM_PORT_DMA
    phvwrpair      p.req_feedback.p4_intr_global.tm_iq, 0, p.req_feedback.p4_intr_global.lif, K_GLOBAL_LIF
    phvwr          p.req_feedback.p4_intr_rxdma.intr_qid, r3[23:0]
    phvwri         p.req_feedback.p4_intr_rxdma.intr_rx_splitter_offset, RDMA_FEEDBACK_SPLITTER_OFFSET

    phvwrpair      p.req_feedback.p4_intr_rxdma.intr_qtype, Q_TYPE_RDMA_SQ, p.req_feedback.p4_to_p4plus.p4plus_app_id, P4PLUS_APPTYPE_RDMA
    phvwri         p.req_feedback.p4_to_p4plus.raw_flags, REQ_RX_FLAG_RDMA_FEEDBACK
    phvwri         p.req_feedback.p4_to_p4plus.table0_valid, 1

    phvwrpair      p.req_feedback.rdma_feedback.feedback_type, RDMA_COMPLETION_FEEDBACK, \
                   p.req_feedback.rdma_feedback.completion.status, CQ_STATUS_WQE_FLUSHED_ERR
    phvwrpair      p.req_feedback.rdma_feedback.completion.wrid, 0, \
                   p.req_feedback.rdma_feedback.completion.error, 1

    DMA_SET_END_OF_PKT(DMA_CMD_PHV2PKT_T, DMA_CMD_BASE)

    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, AQ_TX_DMA_CMD_START_FLIT_ID,
                            AQ_TX_DMA_CMD_RESP_ERR_FEEDBACK)
    DMA_PHV2PKT_SETUP(DMA_CMD_BASE, resp_feedback.p4_intr_global, resp_feedback.rdma_feedback)
    phvwrpair      p.resp_feedback.p4_intr_global.tm_iport, TM_PORT_INGRESS, p.resp_feedback.p4_intr_global.tm_oport, TM_PORT_DMA
    phvwrpair      p.resp_feedback.p4_intr_global.tm_iq, 0, p.resp_feedback.p4_intr_global.lif, K_GLOBAL_LIF
    phvwr          p.resp_feedback.p4_intr_rxdma.intr_qid, r3[23:0]
    phvwri         p.resp_feedback.p4_intr_rxdma.intr_rx_splitter_offset, RDMA_FEEDBACK_SPLITTER_OFFSET

    phvwrpair      p.resp_feedback.p4_intr_rxdma.intr_qtype, Q_TYPE_RDMA_RQ, p.resp_feedback.p4_to_p4plus.p4plus_app_id, P4PLUS_APPTYPE_RDMA
    phvwri         p.resp_feedback.p4_to_p4plus.raw_flags, RESP_RX_FLAG_ERR_DIS_QP
    phvwri         p.resp_feedback.p4_to_p4plus.table0_valid, 1

    phvwrpair      p.resp_feedback.rdma_feedback.feedback_type, RDMA_COMPLETION_FEEDBACK, \
                   p.resp_feedback.rdma_feedback.completion.status, CQ_STATUS_WQE_FLUSHED_ERR
    phvwrpair      p.resp_feedback.rdma_feedback.completion.wrid, 0, \
                   p.resp_feedback.rdma_feedback.completion.error, 1

    DMA_SET_END_OF_PKT(DMA_CMD_PHV2PKT_T, DMA_CMD_BASE)

    b done
    CAPRI_SET_TABLE_1_VALID(0)  // BD Slot
