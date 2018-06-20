#pragma once

#include "nic/include/base.h"
#include "nic/include/pd.hpp"
#include "nic/hal/pd/cpupkt_headers.hpp"
#include "nic/hal/src/internal/proxy.hpp"

namespace hal {
namespace pd {

#define MAX_CPU_PKT_QUEUES              (types::WRingType_ARRAYSIZE)
#define MAX_CPU_PKT_QUEUE_INST_INFO     (1024)

#define HAL_MAX_CPU_PKT_DESCR_ENTRIES   1024
#define CPU_PKT_DESCR_SIZE              128

#define CPU_PKT_VALID_BIT_MASK          ((uint64_t)1 << 63)

#define HAL_MAX_CPU_PKT_PAGE_ENTRIES    1024
#define CPU_PKT_PAGE_SIZE               9216

#define CPU_ASQ_PID                     0
#define CPU_ASQ_QTYPE                   0
#define CPU_ASQ_QID                     0
#define CPU_SCHED_RING_ASQ              0


#define DB_ADDR_BASE                   0x800000
#define DB_ADDR_BASE_HOST              0x8400000
#define DB_UPD_SHFT                    17
#define DB_LIF_SHFT                    6
#define DB_TYPE_SHFT                   3

#define DB_PID_SHFT                    48
#define DB_QID_SHFT                    24
#define DB_RING_SHFT                   16

#define DB_IDX_UPD_NOP                 (0x0 << 2)
#define DB_IDX_UPD_CIDX_SET            (0x1 << 2)
#define DB_IDX_UPD_PIDX_SET            (0x2 << 2)
#define DB_IDX_UPD_PIDX_INC            (0x3 << 2)

#define DB_SCHED_UPD_NOP               (0x0)
#define DB_SCHED_UPD_EVAL              (0x1)
#define DB_SCHED_UPD_CLEAR             (0x2)
#define DB_SCHED_UPD_SET               (0x3)


typedef uint64_t  cpupkt_hw_id_t;
struct cpupkt_queue_info_s;
typedef struct cpupkt_queue_info_s cpupkt_queue_info_t;

typedef struct cpupkt_qinst_info_s {
    uint32_t                queue_id;
    cpupkt_hw_id_t          base_addr;
    uint32_t                pc_index;
    cpupkt_hw_id_t          pc_index_addr;
    cpupkt_queue_info_t     *queue_info;
} cpupkt_qinst_info_t;

typedef struct cpupkt_queue_info_s {
    types::WRingType        type;
    pd_wring_meta_t         *wring_meta;
    uint32_t                num_qinst;
    cpupkt_qinst_info_t     *qinst_info[MAX_CPU_PKT_QUEUE_INST_INFO];
} cpupkt_queue_info_t;

typedef struct cpupkt_rx_ctxt_s {
    uint32_t                num_queues;
    cpupkt_queue_info_t     queue[MAX_CPU_PKT_QUEUES];
} __PACK__ cpupkt_rx_ctxt_t;

typedef struct cpupkt_tx_ctxt_s {
    cpupkt_queue_info_t     queue[MAX_CPU_PKT_QUEUES];
} __PACK__ cpupkt_tx_ctxt_t;

typedef struct cpupkt_ctxt_s {
    cpupkt_rx_ctxt_t rx;
    cpupkt_tx_ctxt_t tx;
} __PACK__ cpupkt_ctxt_t;

static inline cpupkt_ctxt_t *
cpupkt_ctxt_init (cpupkt_ctxt_t *ctxt)
{
    if (!ctxt) {
        return NULL;
    }

    memset(ctxt, 0, sizeof(cpupkt_ctxt_t));
    return ctxt;
}

#if 0
cpupkt_ctxt_t* cpupkt_ctxt_alloc_init(void);
hal_ret_t cpupkt_register_rx_queue(cpupkt_ctxt_t* ctxt, types::WRingType type, uint32_t queue_id=0);
hal_ret_t cpupkt_register_tx_queue(cpupkt_ctxt_t* ctxt, types::WRingType type, uint32_t queue_id=0);
hal_ret_t cpupkt_unregister_tx_queue(cpupkt_ctxt_t* ctxt, types::WRingType type, uint32_t queue_id=0);

// receive
hal_ret_t cpupkt_poll_receive(cpupkt_ctxt_t* ctxt,
                              p4_to_p4plus_cpu_pkt_t** flow_miss_hdr,
                              uint8_t** data,
                              size_t* data_len);
hal_ret_t cpupkt_free(p4_to_p4plus_cpu_pkt_t* flow_miss_hdr, uint8_t* data);

// transmit pkt
hal_ret_t cpupkt_send(cpupkt_ctxt_t* ctxt,
                      types::WRingType type,
                      uint32_t queue_id,
                      cpu_to_p4plus_header_t* cpu_header,
                      p4plus_to_p4_header_t* p4_header,
                      uint8_t* data,
                      size_t data_len,
                      uint16_t dest_lif = SERVICE_LIF_CPU,
                      uint8_t  qtype = CPU_ASQ_QTYPE,
                      uint32_t qid = CPU_ASQ_QID,
                      uint8_t  ring_number = CPU_SCHED_RING_ASQ);

hal_ret_t cpupkt_page_alloc(cpupkt_hw_id_t* page_addr);
hal_ret_t cpupkt_descr_alloc(cpupkt_hw_id_t* descr_addr);
hal_ret_t cpupkt_descr_free(cpupkt_hw_id_t descr_addr);

hal_ret_t
cpupkt_program_send_ring_doorbell(uint16_t dest_lif,
                                  uint8_t  qtype,
                                  uint32_t qid,
                                  uint8_t  ring_number);

#endif

}    // namespace pd
}    // namespace hal
