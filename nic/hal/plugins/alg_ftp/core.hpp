//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#include "nic/include/fte.hpp"
#include "nic/hal/plugins/alg_utils/core.hpp"

namespace hal {
namespace plugins {
namespace alg_ftp {

using namespace hal::plugins::alg_utils;

/*
 * Externs
 */
extern alg_state_t *g_ftp_state;
extern tcp_buffer_slab_t g_ftp_tcp_buffer_slabs;

/*
 * Constants
 */
const std::string FTE_FEATURE_ALG_FTP("pensando.io/alg_ftp:alg_ftp");

/*
 * Function prototypes
 */

// plugin.cc
fte::pipeline_action_t alg_ftp_exec(fte::ctx_t &ctx);
fte::pipeline_action_t alg_ftp_session_delete_cb(fte::ctx_t &ctx);
fte::pipeline_action_t alg_ftp_session_get_cb(fte::ctx_t &ctx);

void ftpinfo_cleanup_hdlr(l4_alg_status_t *l4_sess);

}  // namespace alg_ftp
}  // namespace plugins
}  // namespace hal
