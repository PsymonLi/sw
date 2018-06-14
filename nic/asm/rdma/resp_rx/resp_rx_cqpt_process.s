#include "capri.h"
#include "resp_rx.h"
#include "cqcb.h"
#include "common_phv.h"

struct resp_rx_phv_t p;
struct resp_rx_s6_t2_k k;

#define PAGE_ADDR_P     r1
#define CQWQE_P         r1
#define DMA_CMD_BASE    r2
#define TMP             r3
#define EQCB_ADDR       r5
#define CQCB_PA_ADDR    r4
#define CQCB_PA_INDEX   r4

#define ARG_P           r7

    #c2: no_dma
    #c3: no_translate
    
//TODO: verify if it is in right order
//#define PHV_CQWQE_START cqwqe.id.wrid
//#define PHV_CQWQE_END   cqwqe.r_key

#define EQ_INFO_P t2_s2s_cqcb_to_eq_info

#define IN_P t2_s2s_cqcb_to_pt_info
#define K_PA_NEXT_INDEX   CAPRI_KEY_RANGE(IN_P, pt_next_pg_index_sbit0_ebit2, pt_next_pg_index_sbit11_ebit15)
#define K_CQCB_ADDR CAPRI_KEY_RANGE(IN_P, cqcb_addr_sbit0_ebit4, cqcb_addr_sbit29_ebit33)
#define K_EQCB_ADDR CAPRI_KEY_RANGE(IN_P, eqcb_addr_sbit0_ebit2, eqcb_addr_sbit27_ebit33)
#define K_CQID CAPRI_KEY_RANGE(IN_P, cq_id_sbit0_ebit7, cq_id_sbit8_ebit23)

%%
    .param  resp_rx_eqcb_process

.align
resp_rx_cqpt_process:

    bbeq            CAPRI_KEY_FIELD(IN_P, report_error), 1, fire_eqcb
    crestore [c3, c2], CAPRI_KEY_RANGE(IN_P, no_translate, no_dma), 0x3 //BD Slot

    bcf             [c2 & c3], fire_eqcb
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, RESP_RX_DMA_CMD_START_FLIT_ID, RESP_RX_DMA_CMD_CQ) //BD slot
    
    //page_addr_p = (u64 *) (d_p + sizeof(u64) * cqcb_to_pt_info_p->page_seg_offset);
    sub             PAGE_ADDR_P, (HBM_NUM_PT_ENTRIES_PER_CACHE_LINE-1), CAPRI_KEY_FIELD(IN_P, page_seg_offset)  
    sll             PAGE_ADDR_P, PAGE_ADDR_P, CAPRI_LOG_SIZEOF_U64_BITS

    tblrdp.dx   PAGE_ADDR_P, PAGE_ADDR_P, 0, CAPRI_SIZEOF_U64_BITS
    
    // Lets cache the translated page physical address
    // *cq_cb->pt_next_pa = page_addr_p
    add             CQCB_PA_ADDR, K_CQCB_ADDR, offsetof(struct cqcb_t, pt_next_pa) 
    memwr.d         CQCB_PA_ADDR, PAGE_ADDR_P
    add             CQCB_PA_INDEX, K_CQCB_ADDR, offsetof(struct cqcb_t, pt_next_pg_index)
    //memwr.h         CQCB_PA_INDEX, CAPRI_KEY_FIELD(IN_P, pa_next_index)
    memwr.h         CQCB_PA_INDEX, K_PA_NEXT_INDEX

    bcf             [c2], fire_eqcb
    DMA_CMD_STATIC_BASE_GET(DMA_CMD_BASE, RESP_RX_DMA_CMD_START_FLIT_ID, RESP_RX_DMA_CMD_CQ) //BD slot    

        // cqwqe_p = (cqwqe_t *)(*page_addr_p + cqcb_to_pt_info_p->page_offset);
    add             CQWQE_P, PAGE_ADDR_P, CAPRI_KEY_RANGE(IN_P, page_offset_sbit0_ebit7, page_offset_sbit8_ebit15)

    DMA_PHV2MEM_SETUP(DMA_CMD_BASE, c1, cqwqe, cqwqe, CQWQE_P)

fire_eqcb:
    bbne CAPRI_KEY_FIELD(IN_P, fire_eqcb), 1, cqpt_exit
    add  EQCB_ADDR, r0, K_EQCB_ADDR // BD Slot

    CAPRI_RESET_TABLE_2_ARG()
    phvwrpair CAPRI_PHV_FIELD(EQ_INFO_P, qid), \
              K_CQID, \
              CAPRI_PHV_RANGE(EQ_INFO_P, eqe_type, eqe_code), \
              CAPRI_KEY_RANGE(IN_P, eqe_type, eqe_code)

    CAPRI_NEXT_TABLE2_READ_PC_E(CAPRI_TABLE_LOCK_EN, CAPRI_TABLE_SIZE_512_BITS, resp_rx_eqcb_process, EQCB_ADDR) //Exit Slot
    
cqpt_exit:
    DMA_SET_END_OF_CMDS_C(struct capri_dma_cmd_phv2mem_t, DMA_CMD_BASE, !c1)
    CAPRI_SET_TABLE_2_VALID(0)
    nop.e
    nop

