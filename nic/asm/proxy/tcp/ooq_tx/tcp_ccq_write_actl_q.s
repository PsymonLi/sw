#include "nic/p4/common/defines.h"
#include "tcp-constants.h"
#include "tcp-shared-state.h"
#include "tcp-macros.h"
#include "tcp-table.h"
#include "tcp-phv.h"
#include "tcp_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_s1_t0_ooq_tcp_tx_k.h"

struct phv_ p;
struct s1_t0_ooq_tcp_tx_k_ k;
struct s1_t0_ooq_tcp_tx_write_actl_q_d d;

%%
    .align
    .param          TCP_ACTL_Q_BASE

tcp_ccq_s1_write_actl_q:
    CAPRI_CLEAR_TABLE0_VALID
    addui       r4, r0, hiword(TCP_ACTL_Q_BASE)
    addi        r4, r4, loword(TCP_ACTL_Q_BASE)
    /*
     * Use ACTL ring 0
     * Skipping TCP_ACTL_Q_BASE_FOR_ID(), since ring id is 0
     */
    // Load prod in r6
    add         r6, r0, d.{tcp_actl_q_pindex}.wx
   
    // Prep the ring entry = (msg_type | reason << 24 | qid << 32) 
    add         r3, TCP_ACTL_MSG_TYPE_CLEANUP, k.common_phv_fid, TCP_ACTL_MSG_QID_SHIFT
    or          r3, r3, k.to_s1_close_reason, TCP_ACTL_MSG_CLOSE_REASON_SHIFT
    TCP_ACTL_Q_ENQUEUE(r5, // Reg for use by the macro
                   r3, // ring_elem
                   r6, // prod idx
                   r4, // Base ring addr
                   actl_q_descr_dword,
                   actl_q_descr_dword,
                   actl_q_entry_dma_cmd,
                   1,
                   1,
                   c1)
    nop.e
    nop
