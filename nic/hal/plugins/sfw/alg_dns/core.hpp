//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#include "nic/include/fte.hpp"
#include "lib/list/list.hpp"
#include "nic/hal/plugins/sfw/alg_utils/core.hpp"

namespace hal {
namespace plugins {
namespace alg_dns {

using namespace hal::plugins::alg_utils;

/*
 * Externs
 */
extern alg_state_t *g_dns_state;

/*
 * Constants
 */
const std::string FTE_FEATURE_ALG_DNS("pensando.io/sfw:alg_dns");

/*
 * Forward declarations
 */
typedef struct dns_info_ dns_info_t;

/*
 * Function prototypes
 */

// plugin.cc
fte::pipeline_action_t alg_dns_exec (fte::ctx_t &ctx);
void dnsinfo_cleanup_hdlr (l4_alg_status_t *l4_sess);
typedef hal_ret_t (*dns_cb_t) (fte::ctx_t& ctx, l4_alg_status_t *exp_flow);
fte::pipeline_action_t alg_dns_session_delete_cb(fte::ctx_t &ctx);
fte::pipeline_action_t alg_dns_session_get_cb(fte::ctx_t &ctx);
hal_ret_t alg_dns_init(hal_cfg_t *hal_cfg);
void alg_dns_exit();

/*
 * Data Structures
 */
typedef struct dns_info_ {
    uint16_t        dnsid;
    uint32_t        parse_errors;
    void           *timer;
    bool            response_rcvd;
} dns_info_t;

}  // namespace alg_dns
}  // namespace plugins
}  // namespace hal
