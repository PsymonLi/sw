#include "capri.h"
#include "req_tx.h"
#include "sqcb.h"
#include "nic/p4/common/defines.h"

    #c1 - last
    #c2 - first
    #c3 - UD service, Needed only for send & send_imm
    #c4 - incr_lsn
    #c5 - check credits
    #c6 - adjust_psn/incr_psn
    #c7 - incr_rrq_pindex

struct req_tx_phv_t p;
struct req_tx_s5_t2_k k;
struct sqcb2_t d;

#define IN_P t2_s2s_sqcb_write_back_info_rd
#define IN_RD_P t2_s2s_sqcb_write_back_info_rd
#define IN_SEND_WR_P t2_s2s_sqcb_write_back_info_send_wr
#define IN_TO_S_P to_s5_sqcb_wb_add_hdr_info

#define K_SPEC_CINDEX CAPRI_KEY_RANGE(IN_TO_S_P, spec_cindex_sbit0_ebit7, spec_cindex_sbit8_ebit15)
#define K_OP_TYPE     CAPRI_KEY_FIELD(IN_P, op_type)
#define K_AH_HANDLE   CAPRI_KEY_RANGE(IN_SEND_WR_P, op_send_wr_ah_handle_sbit0_ebit7, op_send_wr_ah_handle_sbit8_ebit31)
#define K_AH_SIZE     CAPRI_KEY_FIELD(IN_TO_S_P, ah_size)
#define K_IMM_DATA    CAPRI_KEY_FIELD(IN_SEND_WR_P, op_send_wr_imm_data_or_inv_key)
#define K_INV_KEY     K_IMM_DATA
#define K_RD_READ_LEN CAPRI_KEY_FIELD(IN_RD_P, op_rd_read_len)
#define K_RD_LOG_PMTU CAPRI_KEY_FIELD(IN_RD_P, op_rd_log_pmtu)
#define K_WQE_ADDR    CAPRI_KEY_FIELD(IN_TO_S_P, wqe_addr)
#define K_TO_S5_DATA k.{to_stage_5_to_stage_data_sbit0_ebit63...to_stage_5_to_stage_data_sbit112_ebit127}
#define K_READ_REQ_ADJUST CAPRI_KEY_RANGE(IN_TO_S_P, read_req_adjust_sbit0_ebit7, read_req_adjust_sbit8_ebit23)
#define K_SPEC_MSG_PSN CAPRI_KEY_RANGE(IN_P, current_sge_offset_sbit0_ebit2, current_sge_offset_sbit27_ebit31)
#define K_SPEC_ENABLE  CAPRI_KEY_FIELD(IN_TO_S_P, spec_enable)


#define ADD_HDR_T t3_s2s_add_hdr_info
#define SQCB_WRITE_BACK_T t2_s2s_sqcb_write_back_info

#define RDMA_PKT_MIDDLE      0
#define RDMA_PKT_LAST        1
#define RDMA_PKT_FIRST       2
#define RDMA_PKT_ONLY        3

// Below are UDP option header sample values used. Same values are used in DOL cases too for verification.
#define ROCE_OPT_TS_VALUE   0x3344
#define ROCE_OPT_TS_ECHO    0x5566
#define ROCE_OPT_MSS        0x1719

%%
    .param    req_tx_write_back_process
    .param    req_tx_add_headers_2_process
    .param    req_tx_sqwqe_fetch_wrid_process

.align
req_tx_add_headers_process:
    // if speculative cindex matches cindex, then this wqe is being
    // processed in the right order and state update is allowed. Otherwise
    // discard and continue with speculation until speculative cindex
    // matches current cindex. If sepculative cindex doesn't match cindex, then
    // revert speculative cindex to cindex , which would allow next speculation
    // to continue from yet to be processed wqe.

    // Similarly, drop if dcqcn rate enforcement doesn't allow this packet
    seq            c1, K_SPEC_CINDEX, d.sq_cindex
    bcf            [!c1], spec_fail
    nop

    // Msg-speculation check. Drop if this is not the right msg-spec packet, only when spec-enable is set.
    bbeq           K_SPEC_ENABLE, 0, skip_msg_psn_check
    seq            c1, K_SPEC_MSG_PSN, d.sq_msg_psn // BD-slot
    bcf            [!c1], spec_fail

skip_msg_psn_check:
    phvwr          p.common.to_stage_6_to_stage_data, K_TO_S5_DATA // BD-slot
    SQCB0_ADDR_GET(r1)
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, req_tx_write_back_process, r1)

    // initialize  cf to 0
    crestore       [c7-c1], r0, 0xfe // Branch Delay Slot

    bbeq           CAPRI_KEY_FIELD(IN_P, poll_failed), 1, poll_fail
    seq            c1, CAPRI_KEY_FIELD(IN_P, last_pkt), 1 // Branch Delay Slot

    bbeq           CAPRI_KEY_FIELD(IN_P, rate_enforce_failed), 1, rate_enforce_fail
    seq            c2, CAPRI_KEY_FIELD(IN_P, first), 1 // Branch Delay Slot

    bbeq           K_GLOBAL_FLAG(_error_disable_qp), 1, error_disable_exit
    seq            c3, d.rnr_timeout, 0 //BD-Slot

    bcf            [!c3], rate_enforce_fail
    nop // BD-Slot

    bbeq           CAPRI_KEY_FIELD(IN_TO_S_P, fence), 1, fence
    // get DMA cmd entry based on dma_cmd_index
    DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RDMA_HEADERS) // Branch Delay Slot

    bbne           d.need_credits, 1, add_headers
    cmov           r2, c2, K_OP_TYPE, d.curr_op_type // Branch Delay Slot

check_credit_update:
    scwlt24        c5, d.lsn_rx, d.ssn
    sne.c5         c5, d.rexmit_psn, d.tx_psn
    bcf            [c5], credit_check_fail
    tblwr.!c5      d.need_credits, 0 // Branch Delay Slot

add_headers:
    .brbegin
    br             r2[2:0]    
    // To start with, num_addr is 1 (bth)
    DMA_PHV2PKT_SETUP_MULTI_ADDR_0(r6, bth, bth, 1)

    .brcase OP_TYPE_SEND
        .csbegin
        cswitch [c2, c1]
        nop
        .brcase RDMA_PKT_MIDDLE
            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _send),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _middle),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_MIDDLE, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot

        .brcase RDMA_PKT_LAST
            phvwr          BTH_ACK_REQ, 1
            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _send),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _last),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_LAST, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot

        .brcase RDMA_PKT_FIRST
            // check_credits = TRUE
            setcf          c5, [c0]

            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _send),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _first),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_FIRST, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot

        .brcase RDMA_PKT_ONLY
            // Update num addrs to 2 if UD service (bth, deth)
            seq            c3, d.service, RDMA_SERV_TYPE_UD

            // check_credits = TRUE  : Keep this instruction here to avoid hazard for c3
            setcf          c5, [c0]

            phvwr.!c3      BTH_ACK_REQ, 1

            DMA_PHV2PKT_SETUP_CMDSIZE_C(r6, 2, c3)
            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _send),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _only),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_ONLY, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        .csend

    .brcase OP_TYPE_SEND_INV
        phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _inv_rkey),  1, \
                       CAPRI_PHV_FIELD(phv_global_common, _send),  1
        .csbegin
        cswitch [c2, c1]
        tblwr.c2       d.inv_key, K_INV_KEY // Branch Delay Slot
        .brcase RDMA_PKT_MIDDLE
            phvwr          CAPRI_PHV_FIELD(phv_global_common, _middle),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_MIDDLE, d.service, RDMA_OPC_SERV_TYPE_SHIFT
        .brcase RDMA_PKT_LAST
            phvwr          BTH_ACK_REQ, 1
            // dma_cmd[2] - IETH hdr
            phvwr          IETH_R_KEY, d.inv_key
            // num addrs 2 (bth, ieth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, ieth, ieth, 1)

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _last),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_LAST_WITH_INV, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        .brcase RDMA_PKT_FIRST
            // check_credits = TRUE
            setcf          c5, [c0]

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _first),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_FIRST, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        .brcase RDMA_PKT_ONLY
            phvwr          BTH_ACK_REQ, 1
            // check_credits = TRUE
            setcf          c5, [c0]

            // dma_cmd[2] - IETH hdr; num addrs 2 (bth, ieth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, ieth, ieth, 1)
            phvwr          IETH_R_KEY, K_INV_KEY

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _only),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_ONLY_WITH_INV, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        .csend

    .brcase OP_TYPE_SEND_IMM
        phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _immdt),  1, \
                       CAPRI_PHV_FIELD(phv_global_common, _send),  1
        .csbegin
        cswitch [c2, c1]
        tblwr.c2       d.imm_data, K_IMM_DATA  // Branch Delay Slot
        
        .brcase RDMA_PKT_MIDDLE
            phvwr          CAPRI_PHV_FIELD(phv_global_common, _middle),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_MIDDLE, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
        .brcase RDMA_PKT_LAST
            phvwr          BTH_ACK_REQ, 1
            // dma_cmd[2] - IMMETH hdr, num addrs 2 (bth, immeth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, immeth, immeth, 1)
            phvwr          IMMDT_DATA, d.imm_data
            phvwr          CAPRI_PHV_FIELD(phv_global_common, _last),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_LAST_WITH_IMM, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        
        .brcase RDMA_PKT_FIRST
            // check_credits = TRUE
            setcf          c5, [c0]

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _first),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_FIRST, d.service, RDMA_OPC_SERV_TYPE_SHIFT
        
        .brcase RDMA_PKT_ONLY
            // check_credits = TRUE
            setcf          c5, [c0]
        
            // dma_cmd[2] - IMMETH hdr, num addrs 2 (bth, immeth) or 3 for UD (bth, deth, immeth)
            seq            c3, d.service, RDMA_SERV_TYPE_UD
            phvwr.!c3          BTH_ACK_REQ, 1

            DMA_PHV2PKT_SETUP_CMDSIZE_C(r6, 2, !c3)
            DMA_PHV2PKT_SETUP_CMDSIZE_C(r6, 3, c3)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N_C(r6, immeth, immeth, 1, !c3)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N_C(r6, immeth, immeth, 2, c3)

            phvwr          IMMDT_DATA, K_IMM_DATA

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _only),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_SEND_ONLY_WITH_IMM, d.service, RDMA_OPC_SERV_TYPE_SHIFT
        .csend

    .brcase OP_TYPE_READ
        // inc_rrq_pindex = TRUE; adjust_psn = TRUE; inc_lsn = TRUE
        crestore       [c7, c6, c4], 0xd0, 0xd0
        
        // dma_cmd[2] - RETH hdr, num addrs 2 (bth, reth)
        DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
        DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, reth, reth, 1)
        
        // rrqwqe_p = rrq_base_addr + rrq_p_index * sizeof(rrqwqe_t)
        sll            r3, d.rrq_base_addr, RRQ_BASE_ADDR_SHIFT
        add            r3, r3, d.rrq_pindex, LOG_RRQ_WQE_SIZE
        
        sub            r5, d.tx_psn, K_READ_REQ_ADJUST
        mincr          r5, 24, r0
        phvwrpair      p.rrqwqe.psn, r5, p.{rrqwqe.e_psn, rrqwqe.msn}, d.{tx_psn, ssn}
        
        // dma_cmd[3]
        DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RRQWQE)
        DMA_HBM_PHV2MEM_SETUP(r6, rrqwqe.read_rsp_or_atomic, rrqwqe.read.pad, r3)
        DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RRQWQE_BASE_SGES)
        add           r3, r3, 32 // copy rrqwqe_base_sge after 32 bytes from start
        DMA_HBM_PHV2MEM_SETUP(r6, rrqwqe_base_sges, rrqwqe_base_sges, r3)
        
        phvwr          CAPRI_PHV_FIELD(phv_global_common, _read_req),  1
        b              rrq_full_chk
        add            r2, RDMA_PKT_OPC_RDMA_READ_REQ, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot

    .brcase OP_TYPE_WRITE
        .csbegin
        cswitch [c2, c1]
        nop
        .brcase RDMA_PKT_MIDDLE
            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _write),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _middle),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_MIDDLE, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
        .brcase RDMA_PKT_LAST
            phvwr          BTH_ACK_REQ, 1

            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _write),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _last),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_LAST, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
        .brcase RDMA_PKT_FIRST
            // inc_lsn = TRUE
            crestore       [c4], 0x10, 0x10

            // dma_cmd[2] - RETH hdr, num addrs 2 (bth, reth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, reth, reth, 1)

            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _write),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _first),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_FIRST, d.service, RDMA_OPC_SERV_TYPE_SHIFT
        
        .brcase RDMA_PKT_ONLY
            phvwr          BTH_ACK_REQ, 1
            // inc_lsn = TRUE
            crestore       [c4], 0x10, 0x10

            // dma_cmd[2] - RETH hdr, num addrs 2 (bth, reth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, reth, reth, 1)

            phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _write),  1, \
                           CAPRI_PHV_FIELD(phv_global_common, _only),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_ONLY, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        .csend

  
    .brcase OP_TYPE_WRITE_IMM
        phvwrpair      CAPRI_PHV_FIELD(phv_global_common, _immdt),  1, \
                       CAPRI_PHV_FIELD(phv_global_common, _write),  1
        .csbegin
        cswitch [c2, c1]
        tblwr.c2       d.imm_data, K_IMM_DATA  // Branch Delay Slot
        .brcase RDMA_PKT_MIDDLE
            phvwr          CAPRI_PHV_FIELD(phv_global_common, _middle),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_MIDDLE, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
    .brcase RDMA_PKT_LAST
            phvwr          BTH_ACK_REQ, 1
            // check_credits = TRUE
            setcf          c5, [c0]

            // dma_cmd[2] - IMMETH hdr, num addrs 2 (bth, immeth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, immeth, immeth, 1)
            phvwr          IMMDT_DATA, d.imm_data

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _last),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_LAST_WITH_IMM, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
        .brcase RDMA_PKT_FIRST
            // dma_cmd[2] - RETH hdr, num addrs 2 (bth, reth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, reth, reth, 1)
            phvwr          CAPRI_PHV_FIELD(phv_global_common, _first),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_FIRST, d.service, RDMA_OPC_SERV_TYPE_SHIFT // Branch Delay Slot
        
        .brcase RDMA_PKT_ONLY
            phvwr          BTH_ACK_REQ, 1
            // check_credits = TRUE
            setcf          c5, [c0]

            // dma_cmd[2] : addr1 - RETH hdr, num addrs 3 (bth, reth, immeth)
            DMA_PHV2PKT_SETUP_CMDSIZE(r6, 3)
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, reth, reth, 1)

            // dma_cmd[2] : addr2 - IMMETH hdr
            DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, immeth, immeth, 2)
            phvwr          IMMDT_DATA, K_IMM_DATA

            phvwr          CAPRI_PHV_FIELD(phv_global_common, _only),  1
            b              op_type_end
            add            r2, RDMA_PKT_OPC_RDMA_WRITE_ONLY_WITH_IMM, d.service, RDMA_OPC_SERV_TYPE_SHIFT //branch delay slot
        .csend

    .brcase OP_TYPE_CMP_N_SWAP
        // inc_rrq_pindex = TRUE; inc_lsn = TRUE;
        crestore       [c7, c4], 0x90, 0x90
        
        // dma_cmd[2] - ATOMICETH hdr, num addrs 2 (bth, atomiceth)
        DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
        DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, atomiceth, atomiceth, 1)
        
        // rrqwqe_p = rrq_base_addr + rrq_pindex & sizeof(rrqwqe_t)
        sll            r3, d.rrq_base_addr, RRQ_BASE_ADDR_SHIFT
        add            r3, r3, d.rrq_pindex, LOG_RRQ_WQE_SIZE
        
        phvwr          RRQWQE_ATOMIC_OP_TYPE, K_OP_TYPE
        phvwrpair      p.rrqwqe.psn, d.tx_psn, p.{rrqwqe.e_psn, rrqwqe.msn}, d.{tx_psn, ssn}
        
        // dma_cmd[3] 
        DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RRQWQE)
        DMA_HBM_PHV2MEM_SETUP(r6, rrqwqe.read_rsp_or_atomic, rrqwqe.atomic, r3)
        
        phvwr          CAPRI_PHV_FIELD(phv_global_common, _atomic_cswap),  1
        b              rrq_full_chk
        add            r2, RDMA_PKT_OPC_CMP_SWAP, d.service, RDMA_OPC_SERV_TYPE_SHIFT
   
    .brcase OP_TYPE_FETCH_N_ADD
        // inc_rrq_pindex = TRUE; inc_lsn = TRUE;
        crestore       [c7, c4], 0x90, 0x90

        // dma_cmd[2] - ATOMICETH hdr, num addrs 2 (bth, atomiceth)
        DMA_PHV2PKT_SETUP_CMDSIZE(r6, 2)
        DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, atomiceth, atomiceth, 1)
        
        // rrqwqe_p = rrq_base_addr + rrq_pindex & sizeof(rrqwqe_t)
        sll            r3, d.rrq_base_addr, RRQ_BASE_ADDR_SHIFT
        add            r3, r3, d.rrq_pindex, LOG_RRQ_WQE_SIZE
        
        phvwr          RRQWQE_ATOMIC_OP_TYPE, K_OP_TYPE
        phvwrpair      p.rrqwqe.psn, d.tx_psn, p.{rrqwqe.e_psn, rrqwqe.msn}, d.{tx_psn, ssn}
        
        // dma_cmd[3] - rrqwqe
        DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RRQWQE)
        DMA_HBM_PHV2MEM_SETUP(r6, rrqwqe.read_rsp_or_atomic, rrqwqe.atomic, r3)
        
        phvwr          CAPRI_PHV_FIELD(phv_global_common, _atomic_fna),  1
        b              rrq_full_chk
        add            r2, RDMA_PKT_OPC_FETCH_ADD, d.service, RDMA_OPC_SERV_TYPE_SHIFT
    .brend

rrq_full_chk:
    add            r1, d.rrq_pindex, 1
    mincr          r1, d.log_rrq_size, 0
    seq            c3, r1, d.rrq_cindex
    bcf            [c3], rrq_full
    phvwr.c3       CAPRI_PHV_FIELD(SQCB_WRITE_BACK_T, rate_enforce_failed), 1 // Branch Delay Slot

op_type_end:
    tblwr.c2       d.curr_op_type, K_OP_TYPE // Branch Delay Slot

    tblwr          d.fence, 0
    tblwr          d.fence_done, 1

    // sqcb2 maintains copy of sq_cindex to enable speculation check. Increment
    //the copy on completion of wqe and write it into sqcb2
    tblmincri.c1    d.sq_cindex, d.log_sq_size, 1

    // sqcb2 maintains copy of sq_msg_psn to enable msg speculation check. Increment the copy 
    // for first/mid packets and Reset on completion of wqe.
    tblwr.c1        d.sq_msg_psn, 0
    tblmincri.!c1   d.sq_msg_psn, 24, 1

    // phv_p->bth.pkey = 0xffff
    phvwr          BTH_PKEY, DEFAULT_PKEY

    // store wqe_start_psn for backtrack from middle of multi-packet msg
    tblwr.c2       d.wqe_start_psn, d.tx_psn

    // phv_p->bth.psn = sqcb1_p->tx_psn
    phvwrpair      BTH_OPCODE, r2, BTH_PSN, d.tx_psn // Branch Delay Slot

check_credits:
    // set check_credits to false if credit check is disabled
    sne.c5         c5, d.disable_credits, 1
    bcf            [!c5], adjust_psn
    // inc lsn for read, atomic, write (without imm)
    tblmincri.c4   d.lsn, 24, 1

    // if (check_credits && (sqcb1_p->ssn > sqcb1_p->lsn))
    //     phv_p->bth.a = 1
    //     write_back_info_p->set_credits = TRUE
    sne            c5, d.lsn_rx, d.lsn_tx
    tblwr.c5       d.lsn, d.lsn_rx
    tblwr.c5       d.lsn_tx, d.lsn_rx

    scwlt24        c5, d.lsn, d.ssn
    bcf            [!c5], adjust_psn
    phvwr.c5       BTH_ACK_REQ, 1
    // Disable TX scheduler for this QP until ack is received with credits to
    // send subsequent packets
#if !(defined(HAPS) || defined(HW))
    DOORBELL_NO_UPDATE_DISABLE_SCHEDULER(K_GLOBAL_LIF, K_GLOBAL_QTYPE, K_GLOBAL_QID, SQ_RING_ID, r3, r5) // Branch Delay Slot
#endif

adjust_psn:
    b.!c6          inc_psn_ssn
    // if (adjust_psn)
    // tx_psn = read_len >> log_pmtu
    srl            r3, K_RD_READ_LEN, K_RD_LOG_PMTU // Branch Delay Slot
    tblmincr       d.tx_psn, 24, r3

    // if read_len is < pmtu, then increment tx_psn by 1
    sne            c6, r3, r0

    // set exp_rsp_psn to be the last read response psn
    sub            r7, d.tx_psn, 1
    mincr          r7, 24, 0

    // tx_psn += (read_len & ((1 << log_pmtu) -1)) ? 1 : 0
    add.c6         r3, K_RD_READ_LEN, r0
    mincr.c6       r3, K_RD_LOG_PMTU, r0
    sle.c6         c6, r3, r0

inc_psn_ssn:
    // exp_rsp_psn is the psn for which ack_req bit is set or psn of last read
    // rsp packet of read_req or psn of rsp packet of atomic_req
    add.!c6        r7, d.tx_psn, r0

    // sqcb1_p->tx_psn++
    tblmincri.!c6  d.tx_psn, 24, 1

    // if (rrqwqe_to_hdr_info_p->last)
    //     sqcb1_p->ssn++;
    tblmincri.c1   d.ssn, 24, 1

    SQCB1_ADDR_GET(r1)
    add            r2, FIELD_OFFSET(sqcb1_t, tx_psn), r1
    // memwr to sqcb1 - tx_psn[63:40], ssn[39:16], rsvd2[15:0]
    add            r3, r0, d.{tx_psn, ssn}, 16
    memwr.d        r2, r3

rrq_p_index_chk:
    // do we need to increment rrq_pindex ?
    bcf             [!c7], local_ack_timer
    SQCB0_ADDR_GET(r2)

    // incr_rrq_p_index
    tblmincri       d.rrq_pindex, d.log_rrq_size, 1
    add             r3, r1, RRQ_P_INDEX_OFFSET
    phvwr           p.rrq_p_index, d.rrq_pindex

    // dma_cmd[4] - incr rrq_p_index for read/atomic
    DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RRQ_PINDEX)
    DMA_HBM_PHV2MEM_SETUP(r6, rrq_p_index, rrq_p_index, r3)

local_ack_timer:
    // Skip timer logic if not (last/only or read/atomic or ack_req set)
    bcf            [!c1 & !c7 & !c5], load_hdr_template
    tblwr.c5       d.need_credits, 1 // Branch Delay Slot

    // if retransmit timer is disabled (local_ack_timeout = 0), do not track
    // timer expiry.
    // If response is already pending then do not modify lask_ack_or_req_ts.
    // If (rexmit_psn > previous exp_rsp_psn) update last_ack_or_req_ts
    // to current request timestamp and start timer if not runing already. In
    // either case, update exp_rsp_psn to indicate that more responses are
    // expected
    seq            c1, d.local_ack_timeout, 0
    scwle24        c2, d.rexmit_psn, d.exp_rsp_psn
    bcf            [c1 | c2], load_hdr_template
    tblwr          d.exp_rsp_psn, r7 // Branch Delay Slot
    tblwr          d.last_ack_or_req_ts, r4
    bbeq           d.timer_on, 1, load_hdr_template
    CAPRI_START_FAST_TIMER(r1, r2, K_GLOBAL_LIF, K_GLOBAL_QTYPE, K_GLOBAL_QID, TIMER_RING_ID, 100)
    tblwr          d.timer_on, 1

load_hdr_template:

    //For UD, ah_handle comes in send req
    seq            c3, d.service, RDMA_SERV_TYPE_UD
    cmov           r1, c3, K_AH_HANDLE, d.header_template_addr
    cmov           r2, c3, K_AH_SIZE, d.header_template_size

    // phv_p->bth.dst_qp = sqcb1_p->dst_qp if it is not UD service
    phvwr.!c3      BTH_DST_QP, d.dst_qp
    phvwrpair      P4PLUS_TO_P4_APP_ID, P4PLUS_APPTYPE_RDMA, \
                   P4PLUS_TO_P4_FLAGS, (P4PLUS_TO_P4_FLAGS_UPDATE_IP_LEN | P4PLUS_TO_P4_FLAGS_UPDATE_UDP_LEN)

   // HACK: overloading timestamp field d to store flow_index for GFT.
#ifdef GFT
    phvwrpair          p.p4plus_to_p4.flow_index[15:0], d.timestamp, P4PLUS_TO_P4_FLOW_INDEX_VALID, 1
#else
    #phvwr          p.{roce_options.TS_value[15:0]...roce_options.TS_echo[30:16]}, d.{timestamp...timestamp_echo}
    addi           r3, r0, ROCE_OPT_TS_VALUE
    addi           r4, r0, ROCE_OPT_TS_ECHO
    addi           r5, r0, ROCE_OPT_MSS
    phvwrpair      p.roce_options.TS_value[15:0], r3, \
                   p.roce_options.TS_echo[30:16], r4
#endif
    phvwrpair      p.roce_options.MSS_value, r5, p.roce_options.EOL_kind, ROCE_OPT_KIND_EOL

    CAPRI_RESET_TABLE_3_ARG()
    phvwrpair CAPRI_PHV_FIELD(ADD_HDR_T, hdr_template_inline), CAPRI_KEY_FIELD(IN_P, hdr_template_inline), CAPRI_PHV_FIELD(ADD_HDR_T, service), d.service
    phvwrpair CAPRI_PHV_FIELD(ADD_HDR_T, header_template_addr), r1, CAPRI_PHV_FIELD(ADD_HDR_T, header_template_size), r2
    phvwr CAPRI_PHV_RANGE(ADD_HDR_T, roce_opt_ts_enable, roce_opt_mss_enable), d.{roce_opt_ts_enable, roce_opt_mss_enable}

    CAPRI_NEXT_TABLE3_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, req_tx_add_headers_2_process, r0)

    nop.e
    nop

fence:
    tblwr.e         d.fence, 1
    tblwr           d.fence_done, 0

spec_fail:
    phvwr.e   p.common.p4_intr_global_drop, 1
    CAPRI_SET_TABLE_2_VALID(0)

credit_check_fail:
    phvwr     CAPRI_PHV_FIELD(IN_P, credit_check_failed), 1
rrq_full:
#if !(defined(HAPS) || defined(HW))
    // For Model, to avoid infinite loop, disable scheduler for this QP and upon
    // receiving response for the outstanding request, scheduler will be
    // re-enabled in the test to process the next read request.
    // In case of hardware, txdma will be scheduled continousuly for this QP
    // until rrq has room for the next request to be sent out
    DOORBELL_NO_UPDATE_DISABLE_SCHEDULER(K_GLOBAL_LIF, K_GLOBAL_QTYPE, K_GLOBAL_QID, SQ_RING_ID, r1, r2)
#endif
rate_enforce_fail:
    phvwr    CAPRI_PHV_FIELD(SQCB_WRITE_BACK_T, rate_enforce_failed), 1 
poll_fail:
exit:
    nop.e
    nop

error_disable_exit:
#if    !(defined(HAPS) || defined(HW))
    /*
     *  Incrementing sq_cindex copy to satisfy model. Ideally, on error disabling we should just exit and be
     *  in the same state which caused the error.
     */
    tblmincri       d.sq_cindex, d.log_sq_size, 1
#endif
    // DMA commands for generating error-completion to RxDMA
    phvwr          p.rdma_feedback.feedback_type, RDMA_COMPLETION_FEEDBACK
    add            r1, r0, offsetof(struct req_tx_phv_t, p4_to_p4plus)
    phvwrp         r1, 0, CAPRI_SIZEOF_RANGE(struct req_tx_phv_t, p4_intr_global, p4_to_p4plus), r0
    DMA_CMD_STATIC_BASE_GET(r6, REQ_TX_DMA_CMD_START_FLIT_ID, REQ_TX_DMA_CMD_RDMA_ERR_FEEDBACK) 
    DMA_PHV2PKT_SETUP_MULTI_ADDR_0(r6, p4_intr_global, p4_to_p4plus, 2)
    DMA_PHV2PKT_SETUP_MULTI_ADDR_N(r6, rdma_feedback, rdma_feedback, 1)

    phvwrpair      p.p4_intr_global.tm_iport, TM_PORT_INGRESS, p.p4_intr_global.tm_oport, TM_PORT_DMA
    phvwrpair      p.p4_intr_global.tm_iq, 0, p.p4_intr_global.lif, K_GLOBAL_LIF
    SQCB0_ADDR_GET(r1)
    phvwrpair      p.p4_intr_rxdma.intr_qid, K_GLOBAL_QID, p.p4_intr_rxdma.intr_qstate_addr, r1
    phvwri         p.p4_intr_rxdma.intr_rx_splitter_offset, RDMA_FEEDBACK_SPLITTER_OFFSET

    phvwrpair      p.p4_intr_rxdma.intr_qtype, K_GLOBAL_QTYPE, p.p4_to_p4plus.p4plus_app_id, P4PLUS_APPTYPE_RDMA
    phvwri         p.p4_to_p4plus.raw_flags, REQ_RX_FLAG_RDMA_FEEDBACK
    phvwri         p.p4_to_p4plus.table0_valid, 1
    //purposefully disabling this optimization - so, fetch wrid program is invoked all the time
    //from there stats program is invoked to update error stats
    //bbeq           CAPRI_KEY_FIELD(IN_P, first), 1, exit
    DMA_SET_END_OF_PKT_END_OF_CMDS(DMA_CMD_PHV2PKT_T, r6)
    // Load wqe in case of mid/last pkt errors to fetch wrid to be posted in error-completion.
    CAPRI_NEXT_TABLE3_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, req_tx_sqwqe_fetch_wrid_process, K_WQE_ADDR)
