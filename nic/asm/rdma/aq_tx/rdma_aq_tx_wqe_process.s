#include "capri.h"
#include "aq_tx.h"
#include "aqcb.h"
#include "common_phv.h"
#include "p4/common/defines.h"
#include "types.h"

struct aq_tx_phv_t p;
struct aqwqe_t d;
struct aq_tx_s1_t0_k k;

#define IN_TO_S_P to_s1_info
#define IN_S2S_P  t0_s2s_aqcb_to_wqe_info
#define TO_S_FB_INFO_P to_s7_fb_stats_info
#define PHV_GLOBAL_COMMON_P phv_global_common
    
#define K_CQCB_BASE_ADDR_HI CAPRI_KEY_FIELD(IN_TO_S_P, cqcb_base_addr_hi)
#define K_SQCB_BASE_ADDR_HI CAPRI_KEY_FIELD(IN_TO_S_P, sqcb_base_addr_hi) 
#define K_RQCB_BASE_ADDR_HI CAPRI_KEY_FIELD(IN_TO_S_P, rqcb_base_addr_hi)
#define K_LOG_NUM_CQ_ENTRIES CAPRI_KEY_FIELD(IN_TO_S_P, log_num_cq_entries)    
#define K_BARMAP_BASE CAPRI_KEY_RANGE(IN_TO_S_P, barmap_base_sbit0_ebit5, barmap_base_sbit6_ebit9)
#define K_BARMAP_SIZE CAPRI_KEY_RANGE(IN_TO_S_P, barmap_size_sbit0_ebit3, barmap_size_sbit4_ebit7)
#define K_CB_ADDR CAPRI_KEY_RANGE(IN_S2S_P, cb_addr_sbit0_ebit31, cb_addr_sbit32_ebit33)
#define K_AH_BASE_ADDR_PAGE_ID CAPRI_KEY_RANGE(IN_TO_S_P, ah_base_addr_page_id_sbit0_ebit3, ah_base_addr_page_id_sbit20_ebit21)
#define K_MAP_COUNT_COMPLETED CAPRI_KEY_RANGE(IN_S2S_P, map_count_completed_sbit0_ebit5, map_count_completed_sbit30_ebit31)
    
#define TO_SQCB2_INFO_P      to_s5_info
#define TO_RQCB0_INFO_P      to_s5_info
#define TO_SQCB0_INFO_P      to_s6_info    
#define TO_S7_STATS_P       to_s7_fb_stats_info

%%

    .param      tx_dummy
    .param      lif_stats_base
    .param      rdma_cq_tx_stage0
    .param      rdma_req_tx_stage0    
    .param      rdma_aq_tx_feedback_process
    .param      rdma_aq_tx_modify_qp_2_process
    .param      rdma_aq_tx_query_sqcb2_process

.align
rdma_aq_tx_wqe_process:

    add         r1, r0, d.op

    .brbegin
    br          r1[3:0]
    phvwr       p.rdma_feedback.aq_completion.op, r1[3:0]  // BD Slot

    .brcase     AQ_OP_TYPE_NOP
        b           prepare_feedback
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, nop), 1 //BD Slot
    .brcase     AQ_OP_TYPE_CREATE_CQ
        b           create_cq
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, create_cq), 1 //BD Slot
    .brcase     AQ_OP_TYPE_CREATE_QP
        b           create_qp
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, create_qp), 1 //BD Slot
    .brcase     AQ_OP_TYPE_REG_MR
        b           reg_mr
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, reg_mr), 1 //BD Slot
    .brcase     AQ_OP_TYPE_STATS_HDRS
        b           report_bad_cmd
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, stats_hdrs), 1 //BD Slot
    .brcase     AQ_OP_TYPE_STATS_VALS
        b           report_bad_cmd
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, stats_vals), 1 //BD Slot
    .brcase     AQ_OP_TYPE_DEREG_MR
        b           dereg_mr
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, dereg_mr), 1 //BD Slot
    .brcase     AQ_OP_TYPE_RESIZE_CQ
        b           report_bad_cmd
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, resize_cq), 1 //BD Slot
    .brcase     AQ_OP_TYPE_DESTROY_CQ
        b           report_bad_cmd
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, destroy_cq), 1 //BD Slot
    .brcase     AQ_OP_TYPE_MODIFY_QP
        b           modify_qp
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, modify_qp), 1 //BD Slot
    .brcase     AQ_OP_TYPE_QUERY_QP
        b           query_qp
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, query_qp), 1 //BD Slot
    .brcase     AQ_OP_TYPE_DESTROY_QP
        b           destroy_qp
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, destroy_qp), 1 //BD Slot
    .brcase     AQ_OP_TYPE_STATS_DUMP
        b           stats_dump
        phvwr       CAPRI_PHV_FIELD(TO_S7_STATS_P, stats_dump), 1 //BD Slot
    .brcase     AQ_OP_TYPE_CREATE_AH
//TODO:  need to add stats code
        b           create_ah
        nop
    .brcase     14
        b           exit
        nop
    .brcase     15
        b           exit
        nop

    .brend

reg_mr:

    //TODO: Need to add host_addr bit after Madhav's checkin
    KEY_INDEX_GET(r1, d.{id_ver}.wx)
    KEY_USER_KEY_GET(r2, d.{id_ver}.wx)

    #c4 - inv_en
    #c3 - is_mw
    #c2 - ukey_en
    crestore    [c4, c3, c2], d.{mr_flags.inv_en, mr_flags.is_mw, mr_flags.ukey_en}, 0x7

    bbne        d.mr_flags.is_mw, 1, skip_mw
    phvwr.!c3   p.key.type, MR_TYPE_MR //BD Slot

    #c6 - mw_type2
    setcf       c6, [c3 & c4]
    phvwr.c6    p.key.type, MR_TYPE_MW_TYPE_2

    #c5 - mw_type1
    setcf       c5, [c3 & !c4]
    phvwrpair.c5 p.key.state, KEY_STATE_VALID, p.key.type, MR_TYPE_MW_TYPE_1

skip_mw:

    phvwr       p.key.acc_ctrl, d.acc_ctrl
    add         r3, d.{mr.pd_id}.wx, r0
    phvwrpair   p.key.pd, r3, p.key.mr_flags, d.mr_flags

    KT_BASE_ADDR_GET2(r3, r4)
    // key_entry_p = key_base_addr + (lkey_index * sizeof(struct key_entry_t))
    add         r4, r3, r1, LOG_SIZEOF_KEY_ENTRY_T
    srl         r3, r3, CAPRI_SIZEOF_U64_BYTES
    // r5 = pt_base
    add         r5, d.{mr.tbl_index}.wx, r0
    blt         r3, r5, report_bad_idx
    phvwrpair   p.key.pt_base, r5, p.key.host_addr, 1   // BD Slot
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MR_KT_UPDATE)
    DMA_PHV2MEM_SETUP(r6, c2, key, key, r4)

    add         r6, r0, d.{mr.map_count}.wx, CAPRI_LOG_SIZEOF_U64
    beq         r6, r0, alloc_lkey
    phvwrpair   p.key.mr_l_key, 0, p.key.mr_cookie, 0   // BD slot

create_mr:
    phvwrpair   p.key.user_key, r2, p.key.state, KEY_STATE_VALID
    add         r3, d.{mr.va}.dx, r0
    phvwrpair   p.key.log_page_size, d.mr.page_size_log2, p.key.base_va, r3
    add         r3, d.{mr.length}.dx, r0
    phvwr       p.key.len, r3

    // pt_seg_size = HBM_NUM_PT_ENTRIES_PER_CACHE_LINE * lkey->hostmem_page_size
    sll         r4, HBM_NUM_PT_ENTRIES_PER_CACHE_LINE, d.mr.page_size_log2
    sub         r4, r4, 1
    // pt_seg_offset = lkey->base_va % pt_seg_size
    and         r4, d.{mr.va}.dx, r4
    //start_page_id = pt_seg_offset >> lkey->hostmem_page_size
    srl         r4, r4, d.mr.page_size_log2

    // hbm_add = (start_page_id + lkey->pt_base) * 8 + (pt_base_addr)
    PT_BASE_ADDR_GET2(r2)
    add         r4, r4, d.{mr.tbl_index}.wx
    add         r4, r4, K_MAP_COUNT_COMPLETED
    
    // r6 holds the map_count
    beqi        r6, 1<<CAPRI_LOG_SIZEOF_U64, mr_skip_dma_pt
    add         r5, r2, r4, CAPRI_LOG_SIZEOF_U64    // BD Slot

    add         r2, d.{mr.dma_addr}.dx, K_MAP_COUNT_COMPLETED, CAPRI_LOG_SIZEOF_U64

    sub         r6, r6, K_MAP_COUNT_COMPLETED, CAPRI_LOG_SIZEOF_U64
    sle         c2, r6, DMA_DATA_SIZE
    add.!c2     r6, r0, DMA_DATA_SIZE
    
    DMA_CMD_STATIC_BASE_GET(r4, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MR_PT_SRC1)
    DMA_HOST_MEM2MEM_SRC_SETUP(r4, r6, r2)
    DMA_CMD_STATIC_BASE_GET(r4, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MR_PT_DST1)
    DMA_HBM_MEM2MEM_DST_SETUP(r4, r6, r5)

    add.!c2          r3, K_MAP_COUNT_COMPLETED, DMA_MAX_MAP_COUNT 
    phvwrpair.!c2    p.map_count_completed, r3, p.first_pass, 0
    phvwr.!c2        CAPRI_PHV_FIELD(TO_S_FB_INFO_P, aq_cmd_done), 0

    b           mr_no_skip_dma_pt
    nop

mr_skip_dma_pt:

    //copy      the phy address of a single page directly.
    //TODO: how     do we ensure this memwr is completed by the time we generate CQ for admin cmd.
    b           mr_no_skip_dma_pt
    memwr.d    r5, d.mr.dma_addr // BD slot

alloc_lkey:
    # num_pt_entries_rsvd (max) = kt_base_page_id - pt_base
    sub         r3, r3, r5
    phvwr       p.key.num_pt_entries_rsvd, r3
    phvwrpair   p.key.user_key, r0, p.key.base_va, r0
    phvwr.!c5   p.key.state, KEY_STATE_FREE
    phvwrpair   p.key.log_page_size, r0, p.key.len, r0

mr_no_skip_dma_pt: 

    b           prepare_feedback
    nop

dereg_mr:

    KEY_INDEX_GET(r1, d.{id_ver}.wx)

    // Assumption is p.key structure is already zeroed out
    KT_BASE_ADDR_GET2(r3, r4)
    add         r4, r3, r1, LOG_SIZEOF_KEY_ENTRY_T
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_MR_KT_UPDATE)
    DMA_PHV2MEM_SETUP(r6, c2, key, key, r4)
    
    b           prepare_feedback
    nop
    
create_cq:

    phvwr   p.{cqcb.intrinsic.total_rings, cqcb.intrinsic.host_rings}, (MAX_CQ_RINGS<<4|MAX_CQ_RINGS)

   //TODO: Need to find a way to initiali pt_pa and pt_next_pa

    //populate the PID in CQCB
    phvwr       p.cqcb.intrinsic.pid, d.dbid_flags

    //compute the offset of the label of CQ program
    addi        r4, r0, rdma_cq_tx_stage0[33:CAPRI_RAW_TABLE_PC_SHIFT] ;
    addi        r3, r0, tx_dummy[33:CAPRI_RAW_TABLE_PC_SHIFT] ;
    sub         r4, r4, r3
    phvwr       p.cqcb.intrinsic.pc, r4

    //          setup the DMA for CQCB
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_CQ_CB)        
    add         r2, r0, d.{id_ver}.wx  //TODO: Need to optimize
    phvwr       p.cqcb.cq_id, r2[23:0]
    AQ_TX_CQCB_ADDR_GET(r1, r2[23:0], K_CQCB_BASE_ADDR_HI)

    DMA_PHV2MEM_SETUP(r6, c1, cqcb, cqcb, r1)
    
    // r3 will have the pt_base_address where pt translations
    // should be copied to
    PT_BASE_ADDR_GET2(r4) 
    add         r3, r4, d.{cq.tbl_index}.wx, CAPRI_LOG_SIZEOF_U64
    srl         r5, r3, CAPRI_LOG_SIZEOF_U64
    phvwrpair   p.cqcb.pt_base_addr, r5, p.cqcb.log_cq_page_size, d.cq.page_size_log2[4:0]
    phvwrpair   p.cqcb.log_wqe_size, d.cq.stride_log2[4:0], p.cqcb.log_num_wqes, d.cq.depth_log2[4:0]
    add         r2, r0, d.{cq.eq_id}.wx  //TODO: Need to optimize
    phvwr       p.cqcb.eq_id, r2[23:0]

//TODO:  host_addr should come from driver
    phvwr       p.cqcb.host_addr, 1
    
    add         r4, r0, d.{cq.map_count}.wx, CAPRI_LOG_SIZEOF_U64
    beqi        r4, 1<<CAPRI_LOG_SIZEOF_U64, cq_skip_dma_pt
    nop
    
    //Setup DMA to copy PT translations from host to HBM
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_CQ_PT_SRC)    

    DMA_HOST_MEM2MEM_SRC_SETUP(r6, r4, d.{cq.dma_addr}.dx)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_CQ_PT_DST)        
    DMA_HBM_MEM2MEM_DST_SETUP(r6, r4, r3)

    //TODO: There   is a race condition here. DMA of CQCB and DMA of cqcb->pt_pa
    //Setup     DMA for first two translations in cqcb for optimized lookup
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_CQCB_PT_SRC)    

    DMA_HOST_MEM2MEM_SRC_SETUP(r6, (2*CAPRI_SIZEOF_U64_BYTES), d.{cq.dma_addr}.dx)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_CQCB_PT_DST)        

    add r1, r1, FIELD_OFFSET(cqcb_t, pt_pa)
    DMA_HBM_MEM2MEM_DST_SETUP(r6, (2*CAPRI_SIZEOF_U64_BYTES), r1)

    phvwrpair   p.cqcb.pt_pg_index, 0, p.cqcb.pt_next_pg_index, 1
    
    b           cq_no_skip_dma_pt
    nop

cq_skip_dma_pt:

    //copy      the phy address of a single page directly.
    //TODO: how     do we ensure this memwr is completed by the time we generate CQ for admin cmd.
    memwr.d    r3, d.cq.dma_addr //BD slot
    phvwr       p.cqcb.pt_pa, d.cq.dma_addr
    phvwrpair   p.cqcb.pt_pg_index, 0, p.cqcb.pt_next_pg_index, 0x1ff
    
cq_no_skip_dma_pt: 

    b           prepare_feedback
    nop

create_ah:

    /* calculate the header_template_addr */
    add         r2, r0, K_AH_BASE_ADDR_PAGE_ID, HBM_PAGE_SIZE_SHIFT
    mul         r3, d.{id_ver}.wx, AT_ENTRY_SIZE_BYTES
    add         r2, r2, r3

    add         r3, d.{ah.length}.wx, r0
    add         r3, r3[7:0], r0
    
    //Setup DMA to copy AH header from host to HBM
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_AH_SRC)
    DMA_HOST_MEM2MEM_SRC_SETUP(r6, r3, d.{ah.dma_addr}.dx)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_AH_DST)        
    DMA_HBM_MEM2MEM_DST_SETUP(r6, r3, r2)

    /* write the AH size at the end*/
    phvwr       p.ah_size, r3[7:0]
    add         r2, r2, HDR_TEMPLATE_T_SIZE_BYTES
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_AH_SIZE)
    DMA_HBM_PHV2MEM_SETUP(r6, ah_size, ah_size, r2)

    b           prepare_feedback
    nop

// TODO: This is just a hack for destroy qp.
destroy_qp:

    add         r2, r0, d.{id_ver}.wx
    SQCB_ADDR_GET(r1, r2[23:0], K_SQCB_BASE_ADDR_HI)
    // service is nearest byte aligned field to state
    add         r3, r1, FIELD_OFFSET(sqcb0_t, service)
    memwr.b     r3, 0x0

    add         r1, r1, CB_UNIT_SIZE_BYTES
    add         r3, r1, FIELD_OFFSET(sqcb1_t, state)
    memwr.b     r3, 0x0

    RQCB_ADDR_GET(r1, r2[23:0], K_RQCB_BASE_ADDR_HI)
    add         r3, r1, FIELD_OFFSET(rqcb0_t, state)
    memwr.b     r3, 0x0

    add         r1, r1, CB_UNIT_SIZE_BYTES
    add         r3, r1, FIELD_OFFSET(rqcb1_t, state)
    memwr.b     r3, 0x0

    b           prepare_feedback
    nop

create_qp:

    // SQCB0:
    
    phvwr       p.{sqcb0.intrinsic.total_rings, sqcb0.intrinsic.host_rings}, (MAX_SQ_DOORBELL_RINGS<<4|MAX_SQ_HOST_RINGS)
    phvwr       p.sqcb0.log_num_wqes, d.qp.sq_depth_log2[4:0]

    // c3: RC QP?
    seq         c3, d.type_state, RDMA_SERV_TYPE_RC

    phvwr       p.sqcb0.state, QP_STATE_RESET
    phvwr       p.sqcb0.color, 1

    //TODO: SQ in HBM still need to be implemented

    phvwr       p.sqcb0.log_sq_page_size, d.qp.sq_page_size_log2[4:0]
    phvwr       p.sqcb0.log_wqe_size, d.qp.sq_stride_log2[4:0]
    phvwr       p.sqcb0.pd, d.{qp.pd_id}.wx
    phvwr       p.sqcb0.service, d.type_state
    phvwr.c3    p.sqcb0.local_ack_timeout, 0xE
    phvwr       p.sqcb0.priv_oper_enable, d.qp.privileged
    phvwr.!c3   p.sqcb0.log_pmtu, 12
    phvwri      p.sqcb0.sqd_cindex, 0xFFFF

    // SQCB1:

    add         r2, d.{qp.sq_cq_id}.wx, r0
    phvwr       p.sqcb1.cq_id, r2[23:0]
    phvwr       p.sqcb1.state, QP_STATE_RESET
    phvwr       p.sqcb1.pd, d.{qp.pd_id}.wx

    phvwr       p.sqcb1.service, d.type_state[3:0]
    phvwr       p.sqcb1.ssn, 1
    phvwr       p.sqcb1.max_ssn, 1
    phvwr.!c3   p.sqcb1.log_pmtu, 12

    //infinite  retries                 
    phvwr.c3    p.sqcb1.credits, 0x1F
    phvwr.c3    p.{sqcb1.err_retry_count, sqcb1.rnr_retry_count}, (0x7<<3|0x7) //default timeout
    phvwr       p.sqcb1.sqcb1_priv_oper_enable, d.qp.privileged

    phvwr       p.sqcb1.log_sqwqe_size, d.qp.sq_stride_log2[4:0]
    //TODO: This should be enabled through modify_qp
    phvwr       p.sqcb1.sqd_async_notify_enable, 1
    //SQCB2:

    phvwr       p.sqcb2.log_sq_size, d.qp.sq_depth_log2[4: 0]
    phvwr       p.sqcb2.ssn, 1
    phvwr       p.sqcb2.service, d.type_state
    // Default should enable credits and set as part of connection negotiation
    // Enable credits by default only for RC QPs
#if !(defined (HAPS) || defined (HW))
    phvwr.c3        p.sqcb2.disable_credits, 1
#else
    phvwr.c3        p.sqcb2.disable_credits, 0
#endif
    phvwr.!c3       p.sqcb2.disable_credits, 1
    phvwrpair.c3    p.{sqcb2.err_retry_ctr, sqcb2.rnr_retry_ctr}, (0x7<<4|0x7), p.sqcb2.lsn, 0
    phvwrpair       p.sqcb2.lsn_tx, 0, p.sqcb2.lsn_rx, 0
    phvwr.c3        p.sqcb2.local_ack_timeout, 0xE

    //          TODO: Move RSQ/RRQ allocation to modify_qp frm create_qp
    //          TODO: Move pmtu setup to modify_qp
    
    //populate the PID in SQCB0
    phvwr       p.sqcb0.intrinsic.pid, d.dbid_flags

    //populate the PC in SQCB0, SQCB1
    addi        r4, r0, rdma_req_tx_stage0[33:CAPRI_RAW_TABLE_PC_SHIFT] ;
    addi        r3, r0, tx_dummy[33:CAPRI_RAW_TABLE_PC_SHIFT] ;
    sub         r4, r4, r3
    phvwr       p.sqcb0.intrinsic.pc, r4
    phvwr       p.sqcb1.pc, r4

    PT_BASE_ADDR_GET2(r4) 
    add         r3, r4, d.{qp.sq_tbl_index_xrcd_id}.wx, CAPRI_LOG_SIZEOF_U64
    srl         r5, r3, CAPRI_LOG_SIZEOF_U64
    phvwr       p.sqcb0.pt_base_addr, r5

    add         r4, r0, d.{qp.sq_map_count}.wx, CAPRI_LOG_SIZEOF_U64 //BD Slot
    beqi        r4, 1<<CAPRI_LOG_SIZEOF_U64, qp_skip_dma_pt
    crestore    [c2, c1], d.{qp.rq_cmb...qp.sq_cmb}, 0x3

    //Setup     DMA for SQ PT

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_QP_SQPT_SRC)

    DMA_HOST_MEM2MEM_SRC_SETUP(r6, r4, d.{qp.sq_dma_addr}.dx)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_QP_SQPT_DST)
    DMA_HBM_MEM2MEM_DST_SETUP(r6, r4, r3)

    b           qp_no_skip_dma_pt
    nop

qp_skip_dma_pt:

    bcf        [!c1 & !c2], qp_skip_cmb
    //obtain barmap end in r2
    add        r1, r0, K_BARMAP_SIZE, BARMAP_SIZE_SHIFT
    add        r2, r1, K_BARMAP_BASE, BARMAP_BASE_SHIFT

    bcf        [!c2], qp_skip_rq_cmb

    //obtain tail of the RQ in r5
    add        r7, d.{qp.rq_dma_addr}.dx, K_BARMAP_BASE, BARMAP_BASE_SHIFT
    add        r5, d.qp.rq_depth_log2[4:0], d.qp.rq_stride_log2[4:0]
    sll        r5, 1, r5
    add        r5, r7, r5

    //if barmap_end < tail_of_rq, report failure
    blt        r2, r5, report_bad_attr
    nop

qp_skip_rq_cmb:

    //obtain tail of the SQ in r5
    add        r4, d.{qp.sq_dma_addr}.dx, K_BARMAP_BASE, BARMAP_BASE_SHIFT
    add        r5, d.qp.sq_depth_log2[4:0], d.qp.sq_stride_log2[4:0]
    sll        r5, 1, r5
    add        r5, r4, r5

    //if barmap_end < tail_of_sq, report failure
    blt        r2, r5, report_bad_attr
    nop

    phvwr      p.sqcb0.hbm_sq_base_addr, r4[33:HBM_SQ_BASE_ADDR_SHIFT]
    phvwr      p.sqcb0.sq_in_hbm, 1

qp_skip_cmb:
    //copy      the phy address of a single page directly.
    //TODO: how     do we ensure this memwr is completed by the time we generate CQ for admin cmd.
    memwr.!c1.d    r3, d.qp.sq_dma_addr

qp_no_skip_dma_pt: 
    
    // setup DMA for SQCB
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_CREATE_QP_CB)        

    add         r2, r0, d.{id_ver}.wx  //TODO: Need to optimize
    SQCB_ADDR_GET(r1, r2[23:0], K_SQCB_BASE_ADDR_HI)

    DMA_PHV2MEM_SETUP(r6, c1, sqcb0, sqcb2, r1)

    add         r2, d.{qp.rq_cq_id}.wx, r0
    add         r2, r2[23:0], r0
    phvwr       p.rdma_feedback.create_qp.rq_cq_id, r2
    phvwrpair   p.rdma_feedback.create_qp.rq_depth_log2, d.qp.rq_depth_log2, p.rdma_feedback.create_qp.rq_stride_log2, d.qp.rq_stride_log2
    add         r2, d.{qp.pd_id}.wx, r0
    phvwrpair   p.rdma_feedback.create_qp.rq_page_size_log2, d.qp.rq_page_size_log2, p.rdma_feedback.create_qp.pd, r2
    phvwr.!c2   p.p4_to_p4plus.create_qp_ext.rq_dma_addr, d.{qp.rq_dma_addr}.dx
    phvwr.c2    p.p4_to_p4plus.create_qp_ext.rq_dma_addr, r7
    phvwr.c2    p.p4_to_p4plus.create_qp_ext.rq_cmb, 1
    phvwr       p.p4_to_p4plus.create_qp_ext.qp_privileged, d.qp.privileged
    // Set log_pmtu for UD
    phvwr.!c3    p.p4_to_p4plus.create_qp_ext.log_pmtu, 12
    phvwr       p.rdma_feedback.create_qp.rq_type_state, d.type_state
    phvwr       p.rdma_feedback.create_qp.rq_map_count, d.{qp.rq_map_count}.wx
    phvwr       p.rdma_feedback.create_qp.rq_tbl_index, d.{qp.rq_tbl_index_srq_id}.wx
    phvwr       p.rdma_feedback.create_qp.pid, d.dbid_flags
    add         r2, r0, d.{id_ver}.wx  //TODO: Need to optimize
    phvwr p.p4_to_p4plus.create_qp_ext.rq_id, r2[23:0]

    b           prepare_feedback
    nop
    
stats_dump:
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)

    add         r1, r0, d.type_state
    .brbegin
    br          r1[3:0]
    nop         // BD slot

    .brcase     AQ_STATS_DUMP_TYPE_QP
        b           qp_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_CQ
        b           cq_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_EQ
        b           eq_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_PT
        b           pt_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_KT
        b           kt_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_AQ
        b           aq_dump
        nop
    .brcase     AQ_STATS_DUMP_TYPE_LIF
        b           lif_dump
        nop
    .brcase     AQ_CAPTRACE_ENABLE
        b           aq_captrace_enable
        nop
    .brcase     AQ_CAPTRACE_DISABLE
        b           aq_captrace_disable
        nop
    .brcase     9
    .brcase     10
    .brcase     11
    .brcase     12
    .brcase     13
    .brcase     14
    .brcase     15
        b           report_bad_type
        nop
    .brend

aq_dump:
    add     r1, r0, K_CB_ADDR
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, TOTAL_AQCB_BYTES, r1)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, TOTAL_AQCB_BYTES, d.{stats.dma_addr}.dx)

    b           prepare_feedback
    nop

aq_captrace_enable:

    add         r1, r0, K_CB_ADDR
    add         r1, r1, FIELD_OFFSET(aqcb0_t, debug)
    memwr.b     r1, 1
    
    b           prepare_feedback
    nop
    
aq_captrace_disable:
    
    add         r1, r0, K_CB_ADDR
    add         r1, r1, FIELD_OFFSET(aqcb0_t, debug)
    memwr.b     r1, 0
    
    b           prepare_feedback
    nop
    
lif_dump:

#ifndef GFT

    addi    r2, r0, lif_stats_base[31:0]
    add     r2, r2, K_GLOBAL_LIF, LIF_STATS_SIZE_SHIFT

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, (1 << LIF_STATS_SIZE_SHIFT), r2)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, (1 << LIF_STATS_SIZE_SHIFT), d.{stats.dma_addr}.dx)

#endif

    b           prepare_feedback
    nop


pt_dump:

    PT_BASE_ADDR_GET2(r4)
    add         r4, r4, d.{id_ver}.wx, CAPRI_LOG_SIZEOF_U64 

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, PAGE_SIZE_4K, r4)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, PAGE_SIZE_4K, d.{stats.dma_addr}.dx) 
    
    b           prepare_feedback
    nop

kt_dump:
    KT_BASE_ADDR_GET2(r4, r3)
    add         r3, r0, d.{id_ver}.wx
    sll         r3, r3, 8
    KEY_ENTRY_ADDR_GET(r4, r4, r3)
    
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, KEY_ENTRY_SIZE_BYTES, r4)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, KEY_ENTRY_SIZE_BYTES, d.{stats.dma_addr}.dx) 

    b           prepare_feedback
    nop

qp_dump:

    add         r3, r0, d.{id_ver}.wx  //TODO: Need to optimize 
    SQCB_ADDR_GET(r1, r3[23:0], K_SQCB_BASE_ADDR_HI)
    RQCB_ADDR_GET(r2, r3[23:0], K_RQCB_BASE_ADDR_HI)

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, TOTAL_CB_BYTES, r1)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, TOTAL_CB_BYTES, d.{stats.dma_addr}.dx) 

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_3)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, TOTAL_CB_BYTES, r2)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_4)
    add         r3, d.{stats.dma_addr}.dx, TOTAL_CB_BYTES
    DMA_HOST_MEM2MEM_DST_SETUP(r6, TOTAL_CB_BYTES, r3)

    b           prepare_feedback
    nop

cq_dump:

    add         r3, r0, d.{id_ver}.wx  //TODO: Need to optimize
    CQCB_ADDR_GET(r1, r3[23:0], K_CQCB_BASE_ADDR_HI)

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, CB_UNIT_SIZE_BYTES, r1)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, CB_UNIT_SIZE_BYTES, d.{stats.dma_addr}.dx)
    
    b           prepare_feedback
    nop
    
eq_dump:

    add         r3, r0, d.{id_ver}.wx  //TODO: Need to optimize
    EQCB_ADDR_GET(r1, r2, r3[23:0], K_CQCB_BASE_ADDR_HI, K_LOG_NUM_CQ_ENTRIES)

    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_1)
    DMA_HBM_MEM2MEM_SRC_SETUP(r6, CB_UNIT_SIZE_BYTES, r1)
    DMA_CMD_STATIC_BASE_GET(r6, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_STATS_DUMP_2)
    DMA_HOST_MEM2MEM_DST_SETUP(r6, CB_UNIT_SIZE_BYTES, d.{stats.dma_addr}.dx)
    
    b           prepare_feedback
    nop

query_qp:
    add         r3, r0, d.{id_ver}.wx
    SQCB_ADDR_GET(r1, r3[23:0], K_SQCB_BASE_ADDR_HI)
    RQCB_ADDR_GET(r2, r3[23:0], K_RQCB_BASE_ADDR_HI)

    phvwr       p.rdma_feedback.aq_completion.op, AQ_OP_TYPE_QUERY_QP
    add         r4, r0, d.{query.rq_dma_addr}.dx
    phvwrpair   p.rdma_feedback.query_qp.rq_id, r3, p.rdma_feedback.query_qp.dma_addr, r4

    // sqcb2 address
    add         r1, r1, (CB_UNIT_SIZE_BYTES * 2)
    CAPRI_RESET_TABLE_2_ARG()
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, rdma_aq_tx_query_sqcb2_process, r1)

    DMA_CMD_STATIC_BASE_GET(r1, AQ_TX_DMA_CMD_START_FLIT_ID, AQ_TX_DMA_CMD_QUERY_QP_SQ)
    DMA_PHV2MEM_SETUP2(r1, c0, query_sq, query_sq, d.{query.sq_dma_addr}.dx)

    // rqcb2 address
    add         r2, r2, (CB_UNIT_SIZE_BYTES * 2)

    b prepare_feedback
    phvwr       CAPRI_PHV_FIELD(TO_SQCB2_INFO_P, cb_addr), r2       // BD Slot

modify_qp:

    add         r3, r0, d.{id_ver}.wx  //TODO: Need to optimize 
    SQCB_ADDR_GET(r1, r3[23:0], K_SQCB_BASE_ADDR_HI)
    RQCB_ADDR_GET(r2, r3[23:0], K_RQCB_BASE_ADDR_HI)
    
dst_qp:
    // MASK = BIT(20)
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_DEST_QPN], 1, e_psn

    // Invoke sqcb2 for QP
    add         r4, r1, (CB_UNIT_SIZE_BYTES * 2) //BD Slot
    add         r4, r4, FIELD_OFFSET(sqcb2_t, dst_qp)
    add         r5, d.{mod_qp.qkey_dest_qpn}.wx, r0

    
    // byte based memwr are safer as it wont cause alignment issues
    // when field changes its offset in future
    memwr.b     r4, r5[23:16]
    add         r4, r4, 1
    memwr.b     r4, r5[15:8]
    add         r4, r4, 1
    memwr.b     r4, r5[7:0]

    add         r5, r2, r0
    add         r5, r5, FIELD_OFFSET(rqcb0_t, dst_qp)
    add         r4, d.{mod_qp.qkey_dest_qpn}.wx, r0
    memwr.b     r5, r4[23:16]
    add         r5, r5, 1
    memwr.b     r5, r4[15:8]
    add         r5, r5, 1
    memwr.b     r5, r4[7:0]

e_psn:
    // MASK = BIT(12)
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_RQ_PSN], 1, q_key

    // Invoke rqcb1
    add         r5, r2, (CB_UNIT_SIZE_BYTES) //BD Slot
    add         r5, r5, FIELD_OFFSET(rqcb1_t, e_psn)
    add         r4, d.{mod_qp.rq_psn}.wx, r0
    memwr.b     r5, r4[23:16]
    add         r5, r5, 1
    memwr.b     r5, r4[15:8]
    add         r5, r5, 1
    memwr.b     r5, r4[7:0]

q_key:
    // MASK = BIT(6)
    bbne        d.mod_qp.attr_mask[RDMA_UPDATE_QP_OPER_SET_QKEY], 1, mod_qp_done

    add         r4, d.{mod_qp.qkey_dest_qpn}.wx, r0

    phvwr       CAPRI_PHV_FIELD(TO_SQCB0_INFO_P, q_key), r4
    phvwr       CAPRI_PHV_FIELD(TO_SQCB0_INFO_P, q_key_valid), 1
    phvwr       CAPRI_PHV_FIELD(TO_RQCB0_INFO_P, q_key), r4
    phvwr       CAPRI_PHV_FIELD(TO_RQCB0_INFO_P, q_key_valid), 1

    //For RQCB1 in Rx side
    phvwrpair p.p4_to_p4plus.modify_qp_ext.q_key, r4, p.p4_to_p4plus.modify_qp_ext.q_key_valid, 1

mod_qp_done:
    
    mfspr       r7, spr_tbladdr
    CAPRI_RESET_TABLE_1_ARG()
    CAPRI_NEXT_TABLE1_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, rdma_aq_tx_modify_qp_2_process, r7) // BD slot

prepare_feedback:

    CAPRI_RESET_TABLE_0_ARG()
    CAPRI_NEXT_TABLE0_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, rdma_aq_tx_feedback_process, r0)

    nop.e
    nop         //Exit Slot

report_bad_cmd:
    b           prepare_feedback
    phvwrpair   p.rdma_feedback.aq_completion.status, AQ_CQ_STATUS_BAD_CMD, p.rdma_feedback.aq_completion.error, 1

report_bad_idx:
    b           prepare_feedback
    phvwrpair   p.rdma_feedback.aq_completion.status, AQ_CQ_STATUS_BAD_INDEX, p.rdma_feedback.aq_completion.error, 1

report_bad_state:
    b           prepare_feedback
    phvwrpair   p.rdma_feedback.aq_completion.status, AQ_CQ_STATUS_BAD_STATE, p.rdma_feedback.aq_completion.error, 1

report_bad_type:
    b           prepare_feedback
    phvwrpair   p.rdma_feedback.aq_completion.status, AQ_CQ_STATUS_BAD_TYPE, p.rdma_feedback.aq_completion.error, 1

report_bad_attr:
    b           prepare_feedback
    phvwrpair   p.rdma_feedback.aq_completion.status, AQ_CQ_STATUS_BAD_ATTR, p.rdma_feedback.aq_completion.error, 1

exit: 
    phvwr.e       p.common.p4_intr_global_drop, 1
    nop         //Exit Slot
