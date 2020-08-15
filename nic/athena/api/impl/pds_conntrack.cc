//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena connection tracking implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/p4/p4_utils.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/athena/api/include/pds_conntrack.h"
#include "gen/p4gen/athena/include/p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "ftl_dev_impl.hpp"

using namespace sdk;

extern "C" {

pds_ret_t
pds_conntrack_state_create (pds_conntrack_spec_t *spec)
{
    p4pd_error_t      p4pd_ret = P4PD_SUCCESS;
    uint32_t          conntrack_id = PDS_CONNTRACK_ID_MAX;
    conntrack_entry_t entry;

    if (!spec) {
        PDS_TRACE_ERR("spec is null");
        return PDS_RET_INVALID_ARG;
    }
    conntrack_id = spec->key.conntrack_id;
    if (conntrack_id == 0 || conntrack_id >= PDS_CONNTRACK_ID_MAX) {
        PDS_TRACE_ERR("conntrack id %u is beyond range", conntrack_id);
        return PDS_RET_INVALID_ARG;
    }

    entry.clear();
    entry.set_valid_flag(TRUE);
    entry.set_flow_type(spec->data.flow_type);
    entry.set_flow_state(spec->data.flow_state);
    entry.set_timestamp(ftl_dev_if::scanner_conntrack_timestamp(
                                    ftl_dev_impl::mpu_timestamp()));

    p4pd_ret = entry.write(conntrack_id);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to write conntrack table at index %u",
                      conntrack_id);
        return PDS_RET_HW_PROGRAM_ERR;
    }
    return PDS_RET_OK;
}

pds_ret_t
pds_conntrack_state_read (pds_conntrack_key_t *key,
                          pds_conntrack_info_t *info)
{
    p4pd_error_t      p4pd_ret = P4PD_SUCCESS;
    uint32_t          conntrack_id = PDS_CONNTRACK_ID_MAX;
    conntrack_entry_t entry;

    if (!key || !info) {
        PDS_TRACE_ERR("key/info is null");
        return PDS_RET_INVALID_ARG;
    }

    conntrack_id = key->conntrack_id;
    if (conntrack_id == 0 || conntrack_id >= PDS_CONNTRACK_ID_MAX) {
        PDS_TRACE_ERR("conntrack id %u is beyond range", conntrack_id);
        return PDS_RET_INVALID_ARG;
    }

    entry.clear();
    p4pd_ret = entry.read(conntrack_id);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read conntrack table at index %u",
                      conntrack_id);
        return PDS_RET_HW_READ_ERR;
    }
    if (FALSE == entry.get_valid_flag()) {

        // Reading an entry to see if it's valid is a normal action
        // so no need to log.
        // PDS_TRACE_ERR("No entry in conntrack table at index %u",
        //               conntrack_id);
        return PDS_RET_ENTRY_NOT_FOUND;
    }
    info->spec.data.flow_type =
        (pds_flow_type_t)entry.get_flow_type();
    info->spec.data.flow_state =
        (pds_flow_state_t)entry.get_flow_state();
    info->status.timestamp = entry.get_timestamp();

    return PDS_RET_OK;
}

pds_ret_t
pds_conntrack_state_update (pds_conntrack_spec_t *spec)
{
    return pds_conntrack_state_create(spec);
}

pds_ret_t
pds_conntrack_state_delete (pds_conntrack_key_t *key)
{
    p4pd_error_t p4pd_ret;
    uint32_t conntrack_id;
    conntrack_entry_t entry;

    if (!key) {
        PDS_TRACE_ERR("key is null");
        return PDS_RET_INVALID_ARG;
    }
    conntrack_id = key->conntrack_id;
    if (conntrack_id == 0 || conntrack_id >= PDS_CONNTRACK_ID_MAX) {
        PDS_TRACE_ERR("conntrack id %u is beyond range", conntrack_id);
        return PDS_RET_INVALID_ARG;
    }

    entry.clear();
    p4pd_ret = entry.write(conntrack_id);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to clear conntrack table at index %u",
                      conntrack_id);
        return PDS_RET_HW_PROGRAM_ERR;
    }
    return PDS_RET_OK;
}

}
