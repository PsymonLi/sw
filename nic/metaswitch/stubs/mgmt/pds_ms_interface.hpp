// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __PDS_MS_INTERFACE_HPP__
#define __PDS_MS_INTERFACE_HPP__

#include "nic/apollo/agent/core/interface.hpp"
#include "nic/apollo/api/include/pds_if.hpp"

namespace pds_ms {
#define LOOPBACK_IF_ID  0
// called from pdsagent svc layer
// TODO: temporary until we stop MS hijack for loopback interface
sdk_ret_t interface_delete(pds_obj_key_t* key, pds_batch_ctxt_t bctxt);

// called from pds hal api thread
// (unless in PDS_MOCK_MODE in which case it is called from pdsagent svc)
sdk_ret_t interface_create(pds_if_spec_t *spec);
sdk_ret_t interface_delete(pds_obj_key_t* key);
sdk_ret_t interface_update(pds_if_spec_t *spec);

};

#endif    // __PDS_MS_INTERFACE_HPP__
