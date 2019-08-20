//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/include/base.hpp"
#include "nic/hal/hal.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/include/pd.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/src/internal/eth.hpp"
#include "nic/utils/host_mem/host_mem.hpp"

namespace hal {

#define MAX_LIFS (2048)
#define ETH_RSS_MAX_QUEUES (128)
#define ETH_RSS_LIF_INDIR_TBL_LEN ETH_RSS_MAX_QUEUES

hal_ret_t
eth_rss_init (uint32_t hw_lif_id, lif_rss_info_t *rss, lif_queue_info_t *qinfo)
{
    uint32_t num_queues;

    HAL_TRACE_VERBOSE("{}: Entered", __FUNCTION__);

    //SDK_ASSERT(hw_lif_id < MAX_LIFS);
    SDK_ASSERT(rss != NULL);
    SDK_ASSERT(qinfo != NULL);

    pd::pd_rss_params_table_entry_add_args_t args;
    pd::pd_func_args_t          pd_func_args = {0};
    args.hw_lif_id = hw_lif_id;
    args.rss_type = rss->type;
    args.rss_key = (uint8_t *)&rss->key;
    pd_func_args.pd_rss_params_table_entry_add = &args;
    pd::hal_pd_call(pd::PD_FUNC_ID_RSS_PARAMS_TABLE_ADD, &pd_func_args);

    num_queues = qinfo[intf::LIF_QUEUE_PURPOSE_RX].num_queues;
    SDK_ASSERT(num_queues < ETH_RSS_MAX_QUEUES);

    if (num_queues > 0) {
        for (unsigned int index = 0; index < ETH_RSS_LIF_INDIR_TBL_LEN; index++) {
            HAL_TRACE_DEBUG("{}: hw_lif_id {} index {} type {} qid {}\n",
                __FUNCTION__, hw_lif_id, index, rss->type, rss->indir[index]);
            pd::pd_rss_indir_table_entry_add_args_t args;
            args.hw_lif_id = hw_lif_id;
            args.index = index;
            args.enable = rss->type;
            args.qid = rss->indir[index];
            pd_func_args.pd_rss_indir_table_entry_add = &args;
            pd::hal_pd_call(pd::PD_FUNC_ID_RSS_INDIR_TABLE_ADD, &pd_func_args);
        }
    }

    HAL_TRACE_DEBUG("{}: done", __FUNCTION__);
    return HAL_RET_OK;
}

} // namespace hal
