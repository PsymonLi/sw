// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#include <netinet/in.h>

#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_common.hpp"
#include "include/sdk/platform.hpp"
#include "asic/rw/asicrw.hpp"
#include "nic/include/tcp_common.h"
#include "tcpcb_pd.hpp"
#include "tcpcb_pd_internal.hpp"
#include "gen/p4gen/tcp_proxy_rxdma/include/tcp_proxy_rxdma_p4plus_ingress.h"

namespace sdk {
namespace tcp {

sdk_ret_t
tcpcb_pd_create(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_set_rxdma(cb));

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_set_txdma(cb));

    return ret;
}

sdk_ret_t
tcpcb_pd_get(tcpcb_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_get_rxdma(cb));

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_get_txdma(cb));

    return ret;
}

sdk_ret_t
tcpcb_pd_get_stats(tcpcb_stats_pd_t *cb)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_get_stats_rxdma(cb));

    CALL_AND_CHECK_FN(sdk::tcp::tcpcb_pd_get_stats_txdma(cb));

    return ret;
}

} // namespace tcp
} // namespace sdk
