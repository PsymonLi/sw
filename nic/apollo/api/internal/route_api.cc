//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file implements internal route related APIs
///
//----------------------------------------------------------------------------

#include "nic/infra/core/trace.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_route.hpp"
#include "nic/apollo/api/internal/pds.hpp"
#include "nic/apollo/api/internal/pds_route.hpp"
#include "nic/apollo/api/pds_state.hpp"

void
pds_route_table_spec_s::deepcopy_(const pds_route_table_spec_t& route_table) {
    // self-assignment guard
    if (this == &route_table) {
        return;
    }

    if (route_table.route_info == NULL) {
        if (route_info) {
            // incorrect usage !!
            SDK_ASSERT(FALSE);
        } else {
            // no routes to copy, just copy the key
             memcpy(this, &route_table, sizeof(pds_route_table_spec_t));
             return;
        }
    }

    // free internally allocated memory, if any
    if (route_info && priv_mem_) {
        PDS_TRACE_VERBOSE("Freeing internally allocated route info");
        SDK_FREE(PDS_MEM_ALLOC_ID_ROUTE_TABLE, route_info);
    }
    PDS_TRACE_VERBOSE("Deep copying route table spec");
    route_info =
        (route_info_t *)SDK_MALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                            ROUTE_INFO_SIZE(route_table.route_info->num_routes));
    SDK_ASSERT(route_info != NULL);
    priv_mem_ = true;
    key = route_table.key;
    memcpy(route_info, route_table.route_info,
           ROUTE_INFO_SIZE(route_table.route_info->num_routes));
}

void
pds_route_table_spec_s::move_(pds_route_table_spec_t&& route_table) {
    // self-assignment guard
    if (this == &route_table) {
        return;
    }

    if (route_table.route_info == NULL) {
        if (route_info) {
            // incorrect usage !!
            SDK_ASSERT(FALSE);
        }
        // fall through
    }
    // free internally allocated memory, if any
    if (route_info && priv_mem_) {
        PDS_TRACE_VERBOSE("Freeing internally allocated route info");
        SDK_FREE(PDS_MEM_ALLOC_ID_ROUTE_TABLE, route_info);
    }
    // shallow copy the spec
    PDS_TRACE_VERBOSE("Shallow copying route table spec");
    memcpy(this, &route_table, sizeof(pds_route_table_spec_t));
    // transfer the ownership of the memory
    route_table.route_info = NULL;
}

namespace api {

#define PDS_MAX_UNDERLAY_ROUTES                   1
uint32_t g_num_routes = 0;
typedef struct route_entry_s {
    uint8_t valid:1;
    uint8_t rsvd:7;
    pds_route_spec_t spec;
} route_entry_t;
static route_entry_t g_route_db[PDS_MAX_UNDERLAY_ROUTES];

static bool
tep_upd_walk_cb_ (void *obj, void *ctxt)
{
    sdk_ret_t ret;
    pds_tep_spec_t spec;
    tep_entry *tep = (tep_entry *)obj;
    pds_batch_ctxt_t bctxt = (pds_batch_ctxt_t)ctxt;

    if (tep->nh_type() == PDS_NH_TYPE_OVERLAY) {
        // tunnel pointing to another tunnel, skip this
        return false;
    }
    spec.key = tep->key();
    spec.remote_ip = tep->ip();
    memcpy(spec.mac, tep->mac(), ETH_ADDR_LEN);
    spec.type = tep->type();
    spec.encap = tep->encap();

    if (g_num_routes == 0) {
        // TEP walk triggered by route delete
        spec.nh_type = PDS_NH_TYPE_BLACKHOLE;
    } else {
        // TEP walk triggered by route add/update,
        // 1st route is the best route !!
        spec.nh_type = g_route_db[0].spec.attrs.nh_type;
        if (spec.nh_type == PDS_NH_TYPE_UNDERLAY_ECMP) {
            spec.nh_group = g_route_db[0].spec.attrs.nh_group;
        } else if (spec.nh_type == PDS_NH_TYPE_UNDERLAY) {
            spec.nh = g_route_db[0].spec.attrs.nh;
        }
    }
    ret = pds_tep_update(&spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update TEP %s, err %u", spec.key.str(), ret);
        // update other TEPs still !!
    }
    return false;
}

static sdk_ret_t
pds_update_teps (void)
{
    pds_batch_ctxt_t bctxt;
    pds_batch_params_t batch_params = { 0 };

    batch_params.epoch = PDS_INTERNAL_API_EPOCH_START;
    batch_params.async = true;
    // no need to pass callback as there is no action we can take there
    bctxt = pds_batch_start(&batch_params);
    tep_db()->walk(tep_upd_walk_cb_, (void *)bctxt);
    return pds_batch_commit(bctxt);
}

static bool
mirror_session_upd_walk_cb_ (void *obj, void *ctxt)
{
    sdk_ret_t ret;
    vpc_entry *vpc;
    pds_obj_key_t vpc_key;
    pds_mirror_session_info_t info = { 0 };
    mirror_session *ms = (mirror_session *)obj;
    pds_batch_ctxt_t bctxt = (pds_batch_ctxt_t)ctxt;

    // check if this is ERSPAN mirror session or not
    if (unlikely(ms->type() != PDS_MIRROR_SESSION_TYPE_ERSPAN)) {
        // this mirror session is of no interest
        return false;
    }

    // check if ERSPAN destination is in the underlay
    vpc_key = ms->erspan_vpc();
    vpc = vpc_db()->find(&vpc_key);
    if (vpc->type() != PDS_VPC_TYPE_UNDERLAY) {
        // this mirror session is of no interest
        return false;
    }

    // read the spec of this mirror session
    ret = ms->read(&info);
    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read mirror session %s to update "
                      "reachability, err %u", ms->key2str().c_str(), ret);
        // continue the walk and update other mirror sessions still !!
        return false;
    }

    // and post an update
    ret = pds_mirror_session_update(&info.spec, bctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update mirror session %s, err %u",
                      info.spec.key.str(), ret);
        // continue the walk and update other mirror sessions still !!
    }
    return false;
}

static sdk_ret_t
pds_update_mirror_sessions (void)
{
    pds_batch_ctxt_t bctxt;
    pds_batch_params_t batch_params = { 0 };

    batch_params.epoch = PDS_INTERNAL_API_EPOCH_START;
    batch_params.async = true;
    // no need to pass callback as there is no action we can take there
    bctxt = pds_batch_start(&batch_params);
    mirror_session_db()->walk(mirror_session_upd_walk_cb_, (void *)bctxt);
    return pds_batch_commit(bctxt);
}

static sdk_ret_t
update_objects (void)
{
    // first update the TEPs
    pds_update_teps();
    // and then update the mirror sessions as ERSPAN collector IP's reachability
    // might have changed
    pds_update_mirror_sessions();
    return SDK_RET_OK;
}

sdk_ret_t
pds_underlay_route_update (_In_ pds_route_spec_t *spec)
{
    bool found = false;

    for (uint32_t i = 0; i < g_num_routes; i++) {
        if (g_route_db[i].valid &&
            (ip_prefix_is_equal(&g_route_db[i].spec.attrs.prefix,
                                &spec->attrs.prefix))) {
            PDS_TRACE_DEBUG("Updating underlay route %s",
                            ippfx2str(&spec->attrs.prefix));
            g_route_db[i].spec = *spec;
            found = true;
            break;
        }
    }
    if (!found) {
        PDS_TRACE_DEBUG("Creating underlay route %s",
                        ippfx2str(&spec->attrs.prefix));
        if (g_num_routes >= PDS_MAX_UNDERLAY_ROUTES) {
            PDS_TRACE_ERR("Failed to create route %s, underlay route table "
                          "is full", ippfx2str(&spec->attrs.prefix));
            return SDK_RET_NO_RESOURCE;
        }
        g_route_db[g_num_routes].valid = TRUE;
        g_route_db[g_num_routes].spec = *spec;
        g_num_routes++;
    }
    // update all the affected objects
    update_objects();
    return SDK_RET_OK;
}

sdk_ret_t
pds_underlay_route_delete (_In_ ip_prefix_t *prefix)
{
    for (uint32_t i = 0; i < g_num_routes; i++) {
        if (g_route_db[i].valid &&
            (ip_prefix_is_equal(&g_route_db[i].spec.attrs.prefix, prefix))) {
            // replace this with the last valid route and fill this slot
            g_route_db[i] = g_route_db[g_num_routes - 1];
            g_route_db[g_num_routes - 1].valid = FALSE;
            g_num_routes--;
            // update all the affected objects
            update_objects();
            return SDK_RET_OK;
        }
    }
    PDS_TRACE_ERR("Failed to delete route %s, route not found",
                  ippfx2str(prefix));
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
pds_underlay_nexthop (_In_ ipv4_addr_t ip_addr, _Out_ pds_nh_type_t *nh_type,
                      _Out_ pds_obj_key_t *nh)
{
    if (g_num_routes == 0) {
        // TEP walk triggered by route delete
        *nh_type = PDS_NH_TYPE_BLACKHOLE;
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    // 1st route is the best route !!
    *nh_type = g_route_db[0].spec.attrs.nh_type;
    if (*nh_type == PDS_NH_TYPE_UNDERLAY_ECMP) {
        *nh = g_route_db[0].spec.attrs.nh_group;
    } else if (*nh_type == PDS_NH_TYPE_UNDERLAY) {
        *nh = g_route_db[0].spec.attrs.nh;
    } else {
        *nh_type = PDS_NH_TYPE_BLACKHOLE;
    }
    return SDK_RET_OK;
}

}    // namespace api
