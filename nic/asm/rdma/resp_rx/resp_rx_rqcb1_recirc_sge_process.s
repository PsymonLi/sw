#include "capri.h"
#include "resp_rx.h"
#include "rqcb.h"
#include "common_phv.h"

struct resp_rx_phv_t p;
// this is an mpu only program, hence there is no d-vector
struct resp_rx_s1_t0_k k;

#define WQE_OFFSET      r1
#define NUM_VALID_SGES  r2
#define ADDR_TO_LOAD    r3
#define WQE_PTR         r5

#define RQCB_TO_WQE_P   t0_s2s_rqcb_to_wqe_info

#define IN_P        t0_s2s_rqcb_to_rqcb1_info
#define IN_TO_S_P   to_s1_recirc_info
#define IN_TO_S_CURR_WQE_PTR      CAPRI_KEY_RANGE(IN_TO_S_P, curr_wqe_ptr_sbit0_ebit47, curr_wqe_ptr_sbit48_ebit63)
#define IN_TO_S_CURR_SGE_OFFSET   CAPRI_KEY_RANGE(IN_TO_S_P, current_sge_offset_sbit0_ebit15, current_sge_offset_sbit16_ebit31)
#define IN_TO_S_REM_PYLD_BYTES    CAPRI_KEY_RANGE(IN_TO_S_P, remaining_payload_bytes_sbit0_ebit7, remaining_payload_bytes_sbit8_ebit15)

%%
    .param    resp_rx_rqwqe_process

.align
resp_rx_rqcb1_recirc_sge_process:
    
    //  wqe_offset = RX_SGE_OFFSET +
    //  rqcb1_p->current_sge_id * sizeof(sge_t);

    add         r7, r0, K_GLOBAL_FLAGS

    add     WQE_OFFSET, RQWQE_SGE_OFFSET, CAPRI_KEY_FIELD(IN_TO_S_P, current_sge_id), LOG_SIZEOF_SGE_T 

    IS_ANY_FLAG_SET(c1, r7, RESP_RX_FLAG_FIRST|RESP_RX_FLAG_ONLY)

    // if it is the first or only packet getting recirc'd,
    // rqcb1 is not yet populated with wqe_ptr and num_sges field.
    // In these cases, take the wqe_ptr and num_sges from recirc info

    cmov    WQE_PTR, c1, IN_TO_S_CURR_WQE_PTR, CAPRI_KEY_RANGE(IN_P, curr_wqe_ptr_sbit0_ebit7, curr_wqe_ptr_sbit56_ebit63)
    add     ADDR_TO_LOAD, WQE_PTR, WQE_OFFSET

    cmov    NUM_VALID_SGES, c1, CAPRI_KEY_FIELD(IN_TO_S_P, num_sges), CAPRI_KEY_FIELD(IN_P, num_sges)

    sub     NUM_VALID_SGES, NUM_VALID_SGES, CAPRI_KEY_FIELD(IN_TO_S_P, current_sge_id)
    
    CAPRI_RESET_TABLE_0_ARG()
    CAPRI_SET_FIELD_RANGE2_IMM(RQCB_TO_WQE_P, recirc_path, in_progress, (1 << 1)|1)
    phvwrpair   CAPRI_PHV_FIELD(RQCB_TO_WQE_P, remaining_payload_bytes), \
                IN_TO_S_REM_PYLD_BYTES, \
                CAPRI_PHV_FIELD(RQCB_TO_WQE_P, current_sge_id), \
                CAPRI_KEY_FIELD(IN_TO_S_P, current_sge_id)

    phvwrpair   CAPRI_PHV_FIELD(RQCB_TO_WQE_P, current_sge_offset), \
                IN_TO_S_CURR_SGE_OFFSET, \
                CAPRI_PHV_FIELD(RQCB_TO_WQE_P, num_valid_sges), \
                NUM_VALID_SGES

    // currently our DMA commands exhaust upon one recirculation, hence 
    // below static formula to compute dma_cmd_index works.
    // for a generic case, we need to make this multiple of recirc_count

    phvwrpair   CAPRI_PHV_FIELD(RQCB_TO_WQE_P, curr_wqe_ptr), \
                WQE_PTR, \
                CAPRI_PHV_FIELD(RQCB_TO_WQE_P, dma_cmd_index), \
                (RESP_RX_DMA_CMD_PYLD_BASE + (MAX_PYLD_DMA_CMDS_PER_SGE * 2))

    // invoke rqwqe
    CAPRI_NEXT_TABLE0_READ_PC_E(CAPRI_TABLE_LOCK_DIS, CAPRI_TABLE_SIZE_512_BITS, resp_rx_rqwqe_process, ADDR_TO_LOAD)

