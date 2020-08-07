//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// process packets received on learn lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"
#include "nic/apollo/learn/learn_internal.hpp"
#include "nic/apollo/learn/utils.hpp"

namespace learn {

using namespace sdk::types;
extern learn_handlers_t g_handler_tbl;

static inline sdk_ret_t
add_tx_pkt_hdr (void *mbuf, learn_ctxt_t *ctxt)
{
    impl::p4_tx_info_t tx_info;
    char *tx_hdr;

    // remove cpu to arm rx hdr and insert arm to cpu tx hdr
    tx_hdr = learn_lif_mbuf_rx_to_tx(mbuf);
    if (tx_hdr == nullptr) {
        ctxt->pkt_ctxt.pkt_drop_reason = PKT_DROP_REASON_MBUF_ERR;
        return SDK_RET_ERR;
    }

    tx_info.slif = ctxt->pkt_ctxt.impl_info.lif;
    tx_info.nh_type = impl::LEARN_NH_TYPE_NONE;
    return impl::arm_to_p4_tx_hdr_fill(learn_lif_mbuf_data_start(mbuf),
                                       &tx_info);
}

static sdk_ret_t
reinject_pkt_to_p4 (void *mbuf, learn_ctxt_t *ctxt)
{
    // check if packet needs to be dropped
    if (ctxt->pkt_ctxt.impl_info.hints & LEARN_HINT_ARP_REPLY) {
        learn_lif_drop_pkt(mbuf, PKT_DROP_REASON_ARP_REPLY);
        return SDK_RET_OK;
    } else if (ctxt->pkt_ctxt.impl_info.hints & LEARN_HINT_RARP) {
        learn_lif_drop_pkt(mbuf, PKT_DROP_REASON_RARP);
        return SDK_RET_OK;
    }
    // reinject packet to p4
    if (likely(add_tx_pkt_hdr(mbuf, ctxt) == SDK_RET_OK)) {
        learn_lif_send_pkt(mbuf);
        return SDK_RET_OK;
    }
    return SDK_RET_ERR;
}

void
process_learn_pkt (void *mbuf)
{
    learn_ctxt_t ctxt = { 0 };
    learn_batch_ctxt_t lbctxt;
    sdk_ret_t ret = SDK_RET_OK;
    char *pkt_data = learn_lif_mbuf_data_start(mbuf);
    bool reinject = false;
#ifdef BATCH_SUPPORT
    sdk_ret_t   batch_ret = SDK_RET_OK;
    pds_batch_params_t batch_params {learn_db()->epoch_next(), false, nullptr,
                                      nullptr};
#endif
    bool has_learn_info = false;
    bool drop_pkt_on_err = true;

    ctxt.ctxt_type = LEARN_CTXT_TYPE_PKT;
    memset(&lbctxt.counters, 0, sizeof(lbctxt.counters));
    ctxt.lbctxt = &lbctxt;
    ctxt.pkt_ctxt.mbuf = mbuf;

    // default drop reason
    ctxt.pkt_ctxt.pkt_drop_reason = PKT_DROP_REASON_PARSE_ERR;

    // parse P4 and packet headers and extract data
    ret = g_handler_tbl.parse(&ctxt, pkt_data, &reinject);
    if (unlikely(ret != SDK_RET_OK)) {
        if (reinject) {
            if (reinject_pkt_to_p4(mbuf, &ctxt) == SDK_RET_OK) {
                drop_pkt_on_err = false;
            }
        }
        goto error;
    }

    // detect new learns and moves if any
    ret = g_handler_tbl.pre_process(&ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        goto error;
    }

    if (ctxt.needs_logging()) {
        PDS_TRACE_DEBUG("Learn context %s", ctxt.str());
    }
    has_learn_info = true;
    ctxt.pkt_ctxt.pkt_drop_reason = PKT_DROP_REASON_LEARNING_FAIL;

    reinject = false;
    // all required info is gathered, validate the learn
    ret = g_handler_tbl.validate(&ctxt, &reinject);
    if (unlikely(ret != SDK_RET_OK)) {
        if (reinject) {
            if (reinject_pkt_to_p4(mbuf, &ctxt) == SDK_RET_OK) {
                drop_pkt_on_err = false;
            }
        }
        goto error;
    }

    // now process the learn start API batch
#ifdef BATCH_SUPPORT
    ctxt.bctxt = pds_batch_start(&batch_params);
    if (unlikely(ctxt.bctxt == PDS_BATCH_CTXT_INVALID)) {
        PDS_TRACE_ERR("Failed to start a batch for %s", ctxt.str());
        ctxt.pkt_drop_reason = PKT_DROP_REASON_RES_ALLOC_FAIL;
        goto error;
    }
#else
    // each API is committed individually
    ctxt.bctxt = PDS_BATCH_CTXT_INVALID;
#endif

    // process learn info and update batch with required apis
    ret = g_handler_tbl.process(&ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        goto error;
    }

#ifdef BATCH_SUPPORT
    // submit the apis in sync mode
    batch_ret = pds_batch_commit(ctxt.bctxt);
    ctxt.bctxt = PDS_BATCH_CTXT_INVALID;
    if (unlikely(batch_ret != SDK_RET_OK)) {
        goto error;
    }
#endif

    // apis executed successfully, update sw state and notify cp
    ret = g_handler_tbl.store(&ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        goto error;
    }
    update_batch_counters(&lbctxt, true);
    LEARN_COUNTER_INCR(rx_pkt_type[ctxt.pkt_ctxt.pkt_type]);

    // broadcast learn events
    (void)g_handler_tbl.notify(&ctxt);

    // reinject the packet to p4 if needed
    if (reinject_pkt_to_p4(mbuf, &ctxt) == SDK_RET_OK) {
        return;
    }
    // packet not reinjected, drop it

error:

    if (has_learn_info) {
        PDS_TRACE_ERR("Failed to process %s",
                       ctxt.log_str(PDS_MAPPING_TYPE_L2));
        if (ctxt.ip_learn_type != LEARN_TYPE_INVALID) {
            PDS_TRACE_ERR("Failed to process %s",
                          ctxt.log_str(PDS_MAPPING_TYPE_L3));
        }
    }
    if (ret != SDK_RET_OK) {
        update_batch_counters(&lbctxt, false);
        LEARN_COUNTER_INCR(rx_pkt_type[ctxt.pkt_ctxt.pkt_type]);
    }
    if (drop_pkt_on_err) {
        learn_lif_drop_pkt(mbuf, ctxt.pkt_ctxt.pkt_drop_reason);
    }
    // clean up any allocated resources
    ctxt.reset();
}

}    // namespace learn
