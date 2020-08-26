
/*
 *  Stage 4 Table 0
 */

#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s5_t0_tcp_rx_k.h"

struct phv_ p;
struct s5_t0_tcp_rx_k_ k;

%%
    .align
    .param tcp_rx_dma_serq_stage_start         
tcp_rx_s5_t0_launch_dma_serq:
    CAPRI_NEXT_TABLE_READ_OFFSET_e(0, TABLE_LOCK_EN,
                tcp_rx_dma_serq_stage_start, k.common_phv_qstate_addr,
                TCP_TCB_RX_DMA_OFFSET, TABLE_SIZE_512_BITS)
    nop
