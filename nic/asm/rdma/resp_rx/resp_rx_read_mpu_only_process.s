#include "capri.h"
#include "resp_rx.h"
#include "rqcb.h"
#include "common_phv.h"

struct resp_rx_phv_t p;
// there is no 'd' vector for mpu-only program
struct resp_rx_s1_t1_k k;

#define IN_P            t1_s2s_rqcb_to_read_atomic_rkey_info
#define OUT_P           t1_s2s_rkey_info
#define TO_S_WB1_P      to_s5_wb1_info
#define RKEY_INFO_P     t1_s2s_rkey_info

#define R_KEY r3
#define KT_BASE_ADDR r6
#define KEY_ADDR r2

#define DMA_CMD_BASE r1
#define DB_ADDR r4
#define DB_DATA r5

#define K_LEN CAPRI_KEY_RANGE(IN_P, len_sbit0_ebit7, len_sbit24_ebit31)
#define K_VA CAPRI_KEY_RANGE(IN_P, va_sbit0_ebit7, va_sbit8_ebit63)
#define K_RSQ_PINDEX CAPRI_KEY_RANGE(IN_P, rsq_p_index_sbit0_ebit7, rsq_p_index_sbit8_ebit15)
#define K_PRIV_OPER_ENABLE CAPRI_KEY_FIELD(IN_P, priv_oper_enable)
#define K_RKEY CAPRI_KEY_FIELD(IN_P, r_key)

%%
    .param  resp_rx_rqrkey_process
    .param  resp_rx_rqcb1_write_back_mpu_only_process
    .param  resp_rx_rqrkey_rsvd_rkey_process

.align
resp_rx_read_mpu_only_process:

    seq         c1, K_LEN, r0
    bcf         [c1], rd_zero_len

    CAPRI_RESET_TABLE_1_ARG();  //BD Slot

    phvwrpair   CAPRI_PHV_FIELD(OUT_P, va), K_VA, \
                CAPRI_PHV_FIELD(OUT_P, len), K_LEN

    add     R_KEY, r0, K_RKEY

    seq     c5, R_KEY, RDMA_RESERVED_LKEY_ID
    // c5: rsvd key
    bcf     [!c5], skip_priv_oper
    seq.c5  c5, K_PRIV_OPER_ENABLE, 1 // BD Slot
    // c5: rsvd key + priv oper enabled
    phvwr.!c5   CAPRI_PHV_FIELD(RKEY_INFO_P, rsvd_key_err), 1

skip_priv_oper:
    KT_BASE_ADDR_GET2(KT_BASE_ADDR, r1)
    KEY_ENTRY_ADDR_GET(KEY_ADDR, KT_BASE_ADDR, R_KEY)

    // for read operation, we skip PT and hence there is no need 
    // to set dma_cmd_start_index and dma_cmdeop while calling 
    // key_process program. dma_cmdeop is either set after RSQWQE 
    // DMA command (in stage0) or upon RSQ Doorbell DMA command 
    // (in the current stage).

    // tbl_id: 1, acc_ctrl: REMOTE_READ, cmdeop: 0, skip_pt: 1
    CAPRI_SET_FIELD_RANGE2_IMM(OUT_P, tbl_id, skip_pt, ((TABLE_1 << 10) | (ACC_CTRL_REMOTE_READ << 2) | (0 << 1) | 1))
    
    // set write back related params
    // incr_nxt_to_go_token_id: 1, incr_c_index: 0, 
    phvwr       CAPRI_PHV_RANGE(TO_S_WB1_P, incr_nxt_to_go_token_id, incr_c_index), (1<<1 | 0)
    CAPRI_SET_FIELD2(RKEY_INFO_P, user_key, R_KEY[7:0])

    // invoke rqrkey 
    //CAPRI_NEXT_TABLE1_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, resp_rx_rqrkey_process, KEY_ADDR)
    CAPRI_NEXT_TABLE1_READ_PC_C(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, resp_rx_rqrkey_rsvd_rkey_process, resp_rx_rqrkey_process, KEY_ADDR, c5)

ring_rsq_dbell:

    CAPRI_SETUP_DB_ADDR(DB_ADDR_BASE, DB_SET_PINDEX, DB_SCHED_WR_EVAL_RING, K_GLOBAL_LIF, K_GLOBAL_QTYPE, DB_ADDR)
    CAPRI_SETUP_DB_DATA(K_GLOBAL_QID, RSQ_RING_ID, K_RSQ_PINDEX, DB_DATA)
    // store db_data in LE format
    phvwr   p.db_data1, DB_DATA.dx

    // DMA for RSQ DoorBell
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, RESP_RX_DMA_CMD_RD_ATOMIC_START_FLIT_ID, RESP_RX_DMA_CMD_RSQ_DB)
    DMA_HBM_PHV2MEM_SETUP(DMA_CMD_BASE, db_data1, db_data1, DB_ADDR)
    DMA_SET_WR_FENCE(DMA_CMD_PHV2MEM_T, DMA_CMD_BASE)
    DMA_SET_END_OF_CMDS(DMA_CMD_PHV2MEM_T, DMA_CMD_BASE)

exit:
    nop.e
    nop

rd_zero_len:
    CAPRI_RESET_TABLE_2_ARG()
    CAPRI_SET_FIELD2(TO_S_WB1_P, incr_nxt_to_go_token_id, 1)

    // no need to check rkey
    // invoke an mpu-only program which will bubble down and eventually invoke write back
    CAPRI_NEXT_TABLE2_READ_PC(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_0_BITS, resp_rx_rqcb1_write_back_mpu_only_process, r0)
    b   ring_rsq_dbell
    CAPRI_SET_TABLE_1_VALID(0)  //BD Slot
