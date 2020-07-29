//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines interface APIs for internal module interactions
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/lib/metrics/metrics.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/if_state.hpp"
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/impl/lif_impl_state.hpp"
#include "gen/platform/mem_regions.hpp"

namespace api {

static inline if_entry *
pds_if_entry_find (const if_index_t *key)
{
    return (if_db()->find((if_index_t *)key));
}

sdk_ret_t
pds_if_read (_In_ const if_index_t *key, _Out_ pds_if_info_t *info)
{
    if_entry *entry;

    if (key == NULL || info == NULL) {
        return SDK_RET_INVALID_ARG;
    }

    if ((entry = pds_if_entry_find(key)) == NULL) {
        PDS_TRACE_ERR("Failed to find interface 0x%x", *key);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    return entry->read(info);
}

sdk_ret_t
port_get (_In_ const if_index_t *key, _Out_ pds_if_info_t *info)
{
    return pds_if_read(key, info);
}

static inline void
if_spec_from_host_if_spec (pds_if_spec_t *if_spec,
                           pds_host_if_spec_t *spec)
{
    memset(if_spec, 0, sizeof(pds_if_spec_t));
    memcpy(&if_spec->key, &spec->key, sizeof(pds_obj_key_t));
    if_spec->admin_state = g_pds_state.default_pf_state();
    if_spec->type = IF_TYPE_HOST;
    return;
}

static inline sdk_ret_t
pds_host_if_create (pds_if_spec_t *spec)
{
    api::api_ctxt_t *api_ctxt;

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_IF, API_OP_CREATE);
    if (likely(api_ctxt != NULL)) {
        api_ctxt->api_params->if_spec = *spec;
        return process_api(PDS_BATCH_CTXT_INVALID, api_ctxt);
    }
    return SDK_RET_OOM;
}

static inline sdk_ret_t
pds_host_if_delete (pds_obj_key_t *key)
{
    api::api_ctxt_t *api_ctxt;

    api_ctxt = api::api_ctxt_alloc(OBJ_ID_IF, API_OP_DELETE);
    if (likely(api_ctxt != NULL)) {
        api_ctxt->api_params->key = *key;
        return process_api(PDS_BATCH_CTXT_INVALID, api_ctxt);
    }
    return SDK_RET_OOM;
}

sdk_ret_t
pds_host_if_create (pds_host_if_spec_t *spec)
{
    sdk_ret_t ret;
    api::if_entry *intf;
    pds_if_spec_t if_spec;
    api::impl::lif_impl *lif;
    pds_lif_spec_t *lif_spec = &spec->lif;
    static mem_addr_t lif_stats_base_addr =
        g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    static uint64_t block_size =
        g_pds_state.mempartition()->block_size(MEM_REGION_LIF_STATS_NAME);

    if ((lif_spec->type == sdk::platform::LIF_TYPE_HOST) ||
        (lif_spec->type == sdk::platform::LIF_TYPE_CONTROL)){
        // convert to if spec and create
        if_spec_from_host_if_spec(&if_spec, spec);
        ret = pds_host_if_create(&if_spec);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Host interface %s create failed, ret %u", spec->key.str(), ret);
            return ret;
        }
        intf = if_db()->find(&if_spec.key);
        intf->set_host_if_name(spec->name);
        intf->set_host_if_mac(spec->mac);
    }

    // allocate lif impl
    lif = api::impl::lif_impl::factory(lif_spec);
    if (unlikely(lif == NULL)) {
        if (lif_spec->type == sdk::platform::LIF_TYPE_HOST) {
            pds_host_if_delete(&if_spec.key);
        }
        return sdk::SDK_RET_OOM;
    }

    // insert lif impl in lif_impl_state
    api::impl::lif_impl_db()->insert(lif);
    PDS_TRACE_DEBUG("Inserted lif %u %s %u %s",
                    lif_spec->id, lif_spec->name, lif_spec->type,
                    macaddr2str(lif_spec->mac));

    // register for lif metrics
    sdk::metrics::row_address(g_pds_state.hostif_metrics_handle(),
                              *(sdk::metrics::key_t *)spec->key.id,
                              (void *)(lif_stats_base_addr +
                              (lif_spec->id * block_size)));
    return SDK_RET_OK;
}

sdk_ret_t
pds_host_if_create_notify (pds_obj_key_t *key)
{
    sdk_ret_t ret;
    pds_event_t event;
    pds_if_info_t if_info = { 0 };

    event.event_id = PDS_EVENT_ID_HOST_IF_CREATE;
    ret = pds_if_read(key, &if_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Read host if %s failed", key->str());
        return ret;
    }
    event.if_info.spec = if_info.spec;
    event.if_info.status = if_info.status;
    g_pds_state.event_notify(&event);
    return ret;
}

sdk_ret_t
pds_host_if_update_name (pds_obj_key_t *key, std::string name)
{
    pds_event_t event;
    api::if_entry *intf;
    pds_if_info_t if_info = { 0 };

    intf = if_db()->find(key);
    if (intf) {
        intf->set_host_if_name(name.c_str());
        intf->read(&if_info);
        // notify host if update
        event.event_id = PDS_EVENT_ID_HOST_IF_UPDATE;
        event.if_info.spec = if_info.spec;
        event.if_info.status = if_info.status;
        g_pds_state.event_notify(&event);
    }
    return SDK_RET_OK;
}

}    // namespace api
