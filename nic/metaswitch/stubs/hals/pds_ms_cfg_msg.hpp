//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS Cfg Msg handler
//---------------------------------------------------------------

#include "nic/sdk/lib/ipc/ipc.hpp"

#ifndef __PDS_MS_CFG_MSG_HPP__
#define __PDS_MS_CFG_MSG_HPP__

namespace pds_ms {

void
pds_msg_cfg_callback (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctxt);

} // End namespace

#endif
