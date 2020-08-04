// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __CAPRI_QUIESCE_H__
#define __CAPRI_QUIESCE_H__

#include "include/sdk/base.hpp"
#include "lib/p4/p4_api.hpp"

namespace sdk {
namespace platform {
namespace capri {

sdk_ret_t capri_quiesce_start(void);
sdk_ret_t capri_quiesce_stop(void);
sdk_ret_t capri_quiesce_init(void);
void capri_quiesce_queue_credits_read(p4pd_pipeline_t pipe,
                                      sdk::asic::pd::port_queue_credit_t *credit);
void capri_quiesce_queue_credits_restore(p4pd_pipeline_t pipe,
                                         sdk::asic::pd::port_queue_credit_t *credit);

} // namespace capri
} // namespace platform
} // namespace sdk

#endif  /* __CAPRI_QUIESCE_H__ */
