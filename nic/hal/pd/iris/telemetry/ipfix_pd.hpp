// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __IPFIX_PD_HPP__
#define __IPFIX_PD_HPP__

#include "nic/include/base.hpp"
#include "nic/include/hal_cfg.hpp"

namespace hal {
namespace pd {

hal_ret_t ipfix_module_init(hal_cfg_t *hal_cfg);
hal_ret_t ipfix_init(uint16_t export_id, uint64_t pktaddr,
                     uint16_t payload_start, uint16_t payload_size);
void ipfix_doorbell_ring_cb(void *timer, uint32_t timer_id, void *ctxt);

}    // namespace pd
}    // namespace hal

#endif    // _IPFIX_PD_HPP__

