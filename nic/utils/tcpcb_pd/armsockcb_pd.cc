// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
#include "nic/include/globals.hpp"
#include "include/sdk/platform.hpp"
#include "asic/rw/asicrw.hpp"
#include "nic/sdk/platform/utils/lif_mgr/lif_mgr.hpp"
/*
 * Note 1 - use tcp qstate definition since intrinsic portion is
 * common
 */
#include "gen/p4gen/tcp_proxy_txdma/include/tcp_proxy_txdma_p4plus_ingress.h"

using sdk::platform::utils::lif_mgr;
using sdk::asic::asic_mem_write;

namespace sdk {
namespace tcp {

void
sock_qstate_init(lif_mgr *sock_lif, int qid, uint8_t action_id)
{
   // See Note 1
   s0_t0_tcp_tx_d data = { 0 };
   uint64_t addr;
   sdk_ret_t ret;

   addr = sock_lif->get_lif_qstate_addr(SERVICE_LIF_ARM_SOCKET_APP, 0, qid);

   data.action_id = action_id;
   data.u.read_rx2tx_d.total = 1;

   ret = asic_mem_write(addr, (uint8_t *)&data, CACHE_LINE_SIZE);
   SDK_ASSERT(ret == SDK_RET_OK);
}

} // namespace tcp
} // namespace sdk
