
#include "nic/sdk/platform/ring/ring.hpp"

using sdk::platform::ring;
using sdk::platform::ring_msg_batch_t;
ring tcp_actl_q;

extern "C" {

typedef struct {
    uint8_t     msg_type;
    uint8_t     unused_1;
    uint8_t     unused_2;
    uint8_t     reason;
    uint32_t    qid;
} tcp_cleanup_msg_t;

void pds_tcp_cleanup_proc(uint32_t hw_qid, uint8_t reason);
void tcp_poll_proc()
{
    int i;
    ring_msg_batch_t    batch;
    uint32_t            qid;
    uint8_t             reason;
    tcp_cleanup_msg_t  *cleanup_msg;
    // SDK_TRACE_VERBOSE("tcp_poll_proc");

    batch.msg_cnt = 0;
    tcp_actl_q.poll(&batch);
    
    for (i = 0; i < batch.msg_cnt; i++) {
        cleanup_msg = (tcp_cleanup_msg_t *)&batch.dword[i];
        qid = cleanup_msg->qid;
        reason = cleanup_msg->reason;
        SDK_TRACE_DEBUG("tcp_close: ACTL_MSG tcp_qid %d reason %d "
              "msg 0x%016lx", qid, reason, batch.dword[i]);
        pds_tcp_cleanup_proc(qid, reason);
    }
}
} // extern "C"
