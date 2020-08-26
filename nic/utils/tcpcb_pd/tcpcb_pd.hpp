// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __TCPCB_PD_HPP__
#define __TCPCB_PD_HPP__

#include "tcpcb_pd.h"
#include "nic/sdk/platform/utils/lif_mgr/lif_mgr.hpp"

namespace sdk {
namespace tcp {

//------------------------------------------------------------------------------
// APIs
//------------------------------------------------------------------------------
sdk_ret_t tcpcb_pd_create(tcpcb_pd_t *cb);
sdk_ret_t tcpcb_pd_set_rxdma(tcpcb_pd_t *cb);
sdk_ret_t tcpcb_pd_set_txdma(tcpcb_pd_t *cb);

sdk_ret_t tcpcb_pd_get(tcpcb_pd_t *cb);
sdk_ret_t tcpcb_pd_get_rxdma(tcpcb_pd_t *cb);
sdk_ret_t tcpcb_pd_get_txdma(tcpcb_pd_t *cb);

sdk_ret_t tcpcb_pd_get_stats(tcpcb_stats_pd_t *cb);
sdk_ret_t tcpcb_pd_get_stats_rxdma(tcpcb_stats_pd_t *cb);
sdk_ret_t tcpcb_pd_get_stats_txdma(tcpcb_stats_pd_t *cb);

// For ARM socket app
void sock_qstate_init(lif_mgr *sock_lif, int qid, uint8_t action_id);

} // namespace tcp
} // namespace sdk

#endif // __TCPCB_PD_HPP__
