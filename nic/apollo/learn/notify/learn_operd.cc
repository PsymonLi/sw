//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// operd helper utilities for learn
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/logger.hpp"
#include "nic/infra/operd/event/event.hpp"
#include "nic/infra/operd/event/event_type.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"
#include "nic/apollo/learn/learn_impl_base.hpp"

namespace learn {
namespace mode_notify {

// TODO: DHCP options may not fit into this size. need to revisit it.
#define LEARN_OPERD_PKT_MAX_LEN 128 ///< sending first 128 bytes from packet

static inline operd_event_data_t *
get_operd_msgbuf (void)
{
    static operd_event_data_t *operd_msgbuf = nullptr;
    size_t size;

    if (operd_msgbuf == nullptr) {
        size = sizeof(operd_event_data_t) + sizeof(learn_operd_msg_t)
                   + LEARN_OPERD_PKT_MAX_LEN;
        operd_msgbuf = (operd_event_data_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_LEARN,
                                                        size);
    }
    return operd_msgbuf;
}

static inline void
learn_operd_write_data (operd_event_data_t *buffer, size_t buffer_len)
{
    learn_db()->operd_region()->write(OPERD_DECODER_EVENT, sdk::operd::INFO,
                                      buffer, buffer_len);
    return;
}

static uint16_t
fill_pktinfo (learn_ctxt_t *ctxt, learn_operd_msg_t *msg)
{
    dpdk_mbuf *mbuf = (dpdk_mbuf *)ctxt->pkt_ctxt.mbuf;
    unsigned char *pkt = (unsigned char *)learn_lif_mbuf_data_start(mbuf);
    uint32_t pkt_len = dpdk_device::get_data_len(mbuf);

    // skip p4 to arm header
    pkt += impl::p4_to_arm_hdr_sz();
    pkt_len -= impl::p4_to_arm_hdr_sz();
    msg->pkt_len = (pkt_len >= LEARN_OPERD_PKT_MAX_LEN) ?
                               LEARN_OPERD_PKT_MAX_LEN: pkt_len;
    memcpy(msg->pkt_data, pkt, msg->pkt_len);
    msg->host_if = api::uuid_from_objid(ctxt->ifindex);

    return (sizeof(learn_operd_msg_t) + msg->pkt_len);
}

void
generate_learn_event (learn_ctxt_t *ctxt)
{
    operd_event_data_t *buffer;
    uint16_t buffer_len;

    buffer = get_operd_msgbuf();
    buffer->event_id = operd::event::LEARN_PKT;
    buffer_len = fill_pktinfo(ctxt, (learn_operd_msg_t *)buffer->message);
    buffer_len += sizeof(operd_event_data_t);
    learn_operd_write_data(buffer, buffer_len);
}

}    // namespace mode_notify
}    // namespace learn
