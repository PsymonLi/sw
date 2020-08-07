//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// internal declarations used by learn module
///
//----------------------------------------------------------------------------

#ifndef __LEARN_LEARN_INTERNAL_HPP__
#define __LEARN_LEARN_INTERNAL_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"

namespace learn {

typedef struct learn_handlers_s {
    /// \brief         parse incoming packet and extract endpoint info
    /// \param[in,out] ctxt     learn context to contain endpoint info
    /// \param[in]     pkt      raw packet buffer including p4 to arm header
    /// \param[out]    reinject pointer to boolean to inform caller whether
    ///                packet should be reinjected or dropped
    /// \return        SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*parse)(_Inout_ learn_ctxt_t *ctxt, _In_ char *pkt,
                       _Out_ bool *reinject);

    /// \brief      validate learnt endpoint info
    /// \param[in]  ctxt     learn context containing endpoint info
    /// \param[out] reinject pointer to boolean to inform caller whether
    ///             packet should be reinjected or dropped
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*validate)(_In_ learn_ctxt_t *ctxt, _Out_ bool *reinject);

    /// \brief         preprocess learnt endpoint info to detect learn type
    /// \param[in,out] ctxt    learn context containing endpoint info
    /// \return        SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*pre_process)(_Inout_ learn_ctxt_t *ctxt);

    /// \brief         process learnt endpoint info
    /// \param[in,out] ctxt    learn context containing endpoint info
    /// \return        SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*process)(_Inout_ learn_ctxt_t *ctxt);

    /// \brief     store learnt endpoint info in learn database
    /// \param[in] ctxt    learn context containing endpoint info
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*store)(_In_ learn_ctxt_t *ctxt);

    /// \brief     notify learnt endpoint info to stakeholders
    /// \param[in] ctxt    learn context containing endpoint info
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t (*notify)(_In_ learn_ctxt_t *ctxt);
} learn_handlers_t;

sdk_ret_t parse_packet(learn_ctxt_t *ctxt, char *pkt, bool *reinject);

/// \brief      update learn statistic counters
/// \param[in]  ctxt       learn context containing endpoint info
/// \param[in]  learn_type learn type of endpoint being processed
/// \param[in]  mtype      endpoint type L2 or L3
void update_counters(_In_ learn_ctxt_t *ctxt, _In_ ep_learn_type_t learn_type,
                     _In_ pds_mapping_type_t mtype);

namespace mode_auto {

sdk_ret_t validate(learn_ctxt_t *ctxt, bool *reinject);
sdk_ret_t pre_process(learn_ctxt_t *ctxt);
sdk_ret_t process(learn_ctxt_t *ctxt);
sdk_ret_t store(learn_ctxt_t *ctxt);
sdk_ret_t notify(learn_ctxt_t *ctxt);

}    // namespace mode_auto

namespace mode_notify {

sdk_ret_t validate(learn_ctxt_t *ctxt, bool *reinject);
sdk_ret_t pre_process(learn_ctxt_t *ctxt);
sdk_ret_t process(learn_ctxt_t *ctxt);
sdk_ret_t store(learn_ctxt_t *ctxt);
sdk_ret_t notify(learn_ctxt_t *ctxt);

}    // namespace mode_notify
}    // namespace learn

#endif    //__LEARN_LEARN_INTERNAL_HPP__
