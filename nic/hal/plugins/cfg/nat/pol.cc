//-----------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
// Handles Config APIs for NAT Policy
//-----------------------------------------------------------------------------

#include <google/protobuf/util/json_util.h>
#include "nic/include/hal_cfg.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/fte/acl/acl.hpp"
#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/hal/src/utils/cfg_op_ctxt.hpp"
#include "nic/gen/hal/include/hal_api_stats.hpp"
#include "nat.hpp"


using sdk::lib::ht_ctxt_t;
using acl::acl_ctx_t;

namespace hal {

//-----------------------------------------------------------------------------
// NAT policy object alloc/free routines
//-----------------------------------------------------------------------------

static inline nat_cfg_pol_t *
nat_cfg_pol_alloc (void)
{
    return ((nat_cfg_pol_t *)g_hal_state->nat_cfg_pol_slab()->alloc());
}

static inline void
nat_cfg_pol_free (nat_cfg_pol_t *pol)
{
    hal::delay_delete_to_slab(HAL_SLAB_NAT_CFG_POL, pol);
}

static inline void
nat_cfg_pol_init (nat_cfg_pol_t *pol)
{
    HAL_SPINLOCK_INIT(&pol->slock, PTHREAD_PROCESS_SHARED);
    dllist_reset(&pol->rule_list);
    pol->hal_hdl = HAL_HANDLE_INVALID;
}

static inline void
nat_cfg_pol_uninit (nat_cfg_pol_t *pol)
{
    HAL_SPINLOCK_DESTROY(&pol->slock);
}

static inline nat_cfg_pol_t *
nat_cfg_pol_alloc_init (void)
{
    nat_cfg_pol_t *pol;

    if ((pol = nat_cfg_pol_alloc()) ==  NULL)
        return NULL;

    nat_cfg_pol_init(pol);
    return pol;
}

static inline void
nat_cfg_pol_uninit_free (nat_cfg_pol_t *pol)
{
    nat_cfg_pol_uninit(pol);
    nat_cfg_pol_free(pol);
}

static inline void
nat_cfg_pol_cleanup (nat_cfg_pol_t *pol)
{
    if (pol) {
        nat_cfg_rule_list_cleanup(&pol->rule_list);
        nat_cfg_pol_uninit_free(pol);
    }
}

//-----------------------------------------------------------------------------
// ACL LIB operational handling
//-----------------------------------------------------------------------------

acl_config_t nat_ip_acl_config_glbl;

static hal_ret_t
nat_cfg_pol_rule_acl_build (nat_cfg_pol_t *pol, const acl_ctx_t **acl_ctx)
{
    hal_ret_t ret = HAL_RET_OK;
    nat_cfg_rule_t *rule;
    dllist_ctxt_t *entry;
    uint32_t prio = 0;

    dllist_for_each(entry, &pol->rule_list) {
        rule = dllist_entry(entry, nat_cfg_rule_t, list_ctxt);
        rule->prio = prio++;
        if ((ret = nat_cfg_rule_acl_build(rule, acl_ctx)) != HAL_RET_OK)
            return ret;
    }

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_acl_build (nat_cfg_pol_t *pol, const acl_ctx_t **out_acl_ctx)
{
    hal_ret_t ret = HAL_RET_ERR;
    const acl_ctx_t *acl_ctx;

    if ((acl_ctx = rule_lib_init(nat_acl_ctx_name(pol->key.vrf_id),
                                 &nat_ip_acl_config_glbl)) == NULL)
        return ret;

    if ((ret = nat_cfg_pol_rule_acl_build(pol, &acl_ctx)) != HAL_RET_OK)
        return ret;

    *out_acl_ctx = acl_ctx;
    return ret;
}

static inline void
nat_cfg_pol_rule_acl_cleanup (nat_cfg_pol_t *pol)
{
    nat_cfg_rule_t *rule;
    dllist_ctxt_t *entry;

    dllist_for_each(entry, &pol->rule_list) {
        rule = dllist_entry(entry, nat_cfg_rule_t, list_ctxt);
        nat_cfg_rule_acl_cleanup(rule);
    }
}

static inline hal_ret_t
nat_cfg_pol_acl_cleanup (nat_cfg_pol_t *pol)
{
    const acl_ctx_t *acl_ctx;

    if ((acl_ctx = acl::acl_get(nat_acl_ctx_name(pol->key.vrf_id))) == NULL)
        return HAL_RET_NAT_POLICY_NOT_FOUND;

    nat_cfg_pol_rule_acl_cleanup(pol);
    acl::acl_delete(acl_ctx);
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// NAT config object to its corresponding HAL handle management routines
//
// The NAT config object can be obtained through key or hal handle. The node
// is inserted/deleted in two different hash tables.
//   a) key: hal_handle, data: config object
//   b) key: config object key, data: hal_handle
// In case of #b, a second lookup is done to get to object from hal_handle
//-----------------------------------------------------------------------------

static inline nat_cfg_pol_t *
nat_cfg_pol_hal_hdl_db_lookup (hal_handle_t hal_hdl)
{
    if (hal_hdl == HAL_HANDLE_INVALID)
        return NULL;

    auto hal_hdl_e = hal_handle_get_from_handle_id(hal_hdl);
    if (hal_hdl_e == NULL || hal_hdl_e->obj_id() != HAL_OBJ_ID_NAT_POLICY)
        return NULL;

    return (nat_cfg_pol_t *) hal_handle_get_obj(hal_hdl);
}

static inline nat_cfg_pol_t *
nat_cfg_pol_db_lookup (nat_cfg_pol_key_t *key)
{
    hal_handle_id_ht_entry_t *entry;

    if ((entry = hal_handle_id_ht_entry_db_lookup(
            g_hal_state->nat_policy_ht(), key)) == NULL)
        return NULL;

    return nat_cfg_pol_hal_hdl_db_lookup(entry->handle_id);
}

static inline hal_ret_t
nat_cfg_pol_create_db_handle (nat_cfg_pol_t *pol)
{
    hal_handle_id_ht_entry_t *entry;

    if ((entry = hal_handle_id_ht_entry_alloc_init(
            g_hal_state->hal_handle_id_ht_entry_slab(),
            pol->hal_hdl)) == NULL)
        return HAL_RET_OOM;

    return hal_handle_id_ht_entry_db_add(
        g_hal_state->nat_policy_ht(), &pol->key, entry);
}

static inline hal_ret_t
nat_cfg_pol_delete_db_handle (nat_cfg_pol_key_t *key)
{
    hal_handle_id_ht_entry_t *entry;

    if ((entry = hal_handle_id_ht_entry_db_del(
            g_hal_state->nat_policy_ht(), key)) == NULL)
        return HAL_RET_NAT_POLICY_NOT_FOUND;

    hal_handle_id_ht_entry_uninit_free(entry);
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// CREATE configuration handling
//-----------------------------------------------------------------------------

static inline hal_ret_t
nat_cfg_pol_rule_spec_extract (nat::NatPolicySpec& spec, nat_cfg_pol_t *pol)
{
    hal_ret_t ret = HAL_RET_OK;

    for (int i = 0; i < spec.rules_size(); i++) {
        if ((ret = nat_cfg_rule_spec_handle(
               spec.rules(i), &pol->rule_list)) != HAL_RET_OK)
            return ret;
    }

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_data_spec_extract (nat::NatPolicySpec& spec, nat_cfg_pol_t *pol)
{
    hal_ret_t ret = HAL_RET_OK;

    if ((ret = nat_cfg_pol_rule_spec_extract(spec, pol)) != HAL_RET_OK)
        return ret;

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_key_spec_extract (const kh::NatPolicyKeyHandle& spec,
                              nat_cfg_pol_key_t *key)
{
    vrf_t *vrf;

    key->pol_id = spec.policy_key().nat_policy_id();

    if (spec.policy_key().vrf_key_or_handle().key_or_handle_case() ==
        kh::VrfKeyHandle::kVrfId) {
        key->vrf_id = spec.policy_key().vrf_key_or_handle().vrf_id();
    } else {
        if ((vrf = vrf_lookup_by_handle(spec.policy_key().vrf_key_or_handle().
                                        vrf_handle())) == NULL)
            return HAL_RET_VRF_NOT_FOUND;

        key->vrf_id = vrf->vrf_id;
    }
    return HAL_RET_OK;
}

static inline hal_ret_t
nat_cfg_pol_spec_extract (nat::NatPolicySpec& spec, nat_cfg_pol_t *pol)
{
    hal_ret_t ret;

    if ((ret = nat_cfg_pol_key_spec_extract(
            spec.key_or_handle(), &pol->key)) != HAL_RET_OK)
        return ret;

    if ((ret = nat_cfg_pol_data_spec_extract(spec, pol)) != HAL_RET_OK)
        return ret;

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_key_spec_validate (const kh::NatPolicyKeyHandle& spec, bool create)
{
    if (create) {
        if (spec.policy_handle() != HAL_HANDLE_INVALID)
            return HAL_RET_INVALID_ARG;
    }

    if (spec.policy_key().vrf_key_or_handle().key_or_handle_case() ==
        kh::VrfKeyHandle::kVrfId) {
        if (vrf_lookup_by_id(spec.policy_key().vrf_key_or_handle().
                             vrf_id()) == NULL)
            return  HAL_RET_VRF_NOT_FOUND;
    } else {
        if (vrf_lookup_by_handle(spec.policy_key().vrf_key_or_handle().
                                 vrf_handle()) == NULL)
            return HAL_RET_VRF_NOT_FOUND;
    }

    return HAL_RET_OK;
}

static inline hal_ret_t
nat_cfg_pol_spec_validate (nat::NatPolicySpec& spec, bool create)
{
    hal_ret_t ret;

    if (!spec.has_key_or_handle())
        return HAL_RET_INVALID_ARG;

    if ((ret = nat_cfg_pol_key_spec_validate(
            spec.key_or_handle(), create)) != HAL_RET_OK)
        return ret;

    return HAL_RET_OK;
}

hal_ret_t
nat_cfg_pol_create_cfg_handle (nat::NatPolicySpec& spec,
                               nat_cfg_pol_t **out_pol)
{
    hal_ret_t ret = HAL_RET_OK;
    nat_cfg_pol_t *pol;

    if ((ret = nat_cfg_pol_spec_validate(spec, true)) != HAL_RET_OK)
        return ret;

    if ((pol = nat_cfg_pol_alloc_init()) == NULL)
        return HAL_RET_OOM;

    if ((ret = nat_cfg_pol_spec_extract(spec, pol)) != HAL_RET_OK)
        return ret;

    *out_pol = pol;
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// CREATE operational handling
//-----------------------------------------------------------------------------

static hal_ret_t
nat_cfg_pol_create_commit_cb (cfg_op_ctxt_t *cfg_ctxt)
{
    hal_ret_t ret = HAL_RET_INVALID_ARG;
    nat_cfg_pol_create_app_ctxt_t *app_ctx;

    if (!cfg_ctxt || !cfg_ctxt->app_ctxt)
        goto end;

    app_ctx = (nat_cfg_pol_create_app_ctxt_t *) cfg_ctxt->app_ctxt;

    if ((ret = acl::acl_commit(app_ctx->acl_ctx)) != HAL_RET_OK)
        goto end;

    acl_deref(app_ctx->acl_ctx);

end:
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("nat create commit failed");
        //todo: free resources
    }
    return ret;
}

hal_ret_t
nat_cfg_pol_create_oper_handle (nat_cfg_pol_t *pol)
{
    hal_ret_t ret;
    hal_handle_t hal_hdl;
    nat_cfg_pol_create_app_ctxt_t app_ctxt = { 0 };

    // build acl before operating on the callbacks
    if ((ret = nat_cfg_pol_acl_build(pol, &app_ctxt.acl_ctx)) != HAL_RET_OK)
        return ret;

    if ((ret = cfg_ctxt_op_create_handle(
            HAL_OBJ_ID_NAT_POLICY, pol, &app_ctxt, hal_cfg_op_null_cb,
            nat_cfg_pol_create_commit_cb, hal_cfg_op_null_cb,
            hal_cfg_op_null_cb, &hal_hdl)) != HAL_RET_OK)
        return ret;

    // save the hal handle and add policy to databases
    pol->hal_hdl = hal_hdl;
    if ((ret = nat_cfg_pol_create_db_handle(pol)) != HAL_RET_OK)
        return ret;

    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// GET configuration handling
//-----------------------------------------------------------------------------

static inline hal_ret_t
nat_cfg_pol_rule_spec_build (nat_cfg_pol_t *pol, nat::NatPolicySpec *spec)
{
    hal_ret_t ret = HAL_RET_OK;
    dllist_ctxt_t *entry;
    nat_cfg_rule_t *rule;

    dllist_for_each(entry, &pol->rule_list) {
        rule = dllist_entry(entry, nat_cfg_rule_t, list_ctxt);
        auto rule_spec = spec->add_rules();
        if ((ret = nat_cfg_rule_spec_build(rule, rule_spec)) != HAL_RET_OK)
            return ret;
    }

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_data_spec_build (nat_cfg_pol_t *pol, nat::NatPolicySpec *spec)
{
    hal_ret_t ret;

    if ((ret = nat_cfg_pol_rule_spec_build(pol, spec)) != HAL_RET_OK)
        return ret;

    return ret;
}

static inline hal_ret_t
nat_cfg_pol_key_spec_build (nat_cfg_pol_t *pol, kh::NatPolicyKeyHandle *spec)
{
    spec->mutable_policy_key()->mutable_vrf_key_or_handle()->set_vrf_id(
        pol->key.vrf_id);
    spec->mutable_policy_key()->set_nat_policy_id(pol->key.pol_id);
    spec->set_policy_handle(pol->hal_hdl);
    return HAL_RET_OK;
}

static inline hal_ret_t
nat_cfg_pol_spec_build (nat_cfg_pol_t *pol, nat::NatPolicySpec *spec)
{
    hal_ret_t ret;

    if ((ret = nat_cfg_pol_key_spec_build(
            pol, spec->mutable_key_or_handle())) != HAL_RET_OK)
        return ret;

    if ((ret = nat_cfg_pol_data_spec_build(pol, spec)) != HAL_RET_OK)
        return ret;

    return ret;
}

nat_cfg_pol_t *
nat_cfg_pol_key_or_handle_lookup (const kh::NatPolicyKeyHandle& kh)
{
    if (kh.policy_handle() != HAL_HANDLE_INVALID) {
        return nat_cfg_pol_hal_hdl_db_lookup(kh.policy_handle());
    } else {
        nat_cfg_pol_key_t key = {0};
        nat_cfg_pol_key_spec_extract(kh, &key);
        return nat_cfg_pol_db_lookup(&key);
    }
}

static inline bool
nat_policy_get_ht_cb (void *ht_entry, void *ctxt)
{
    nat_cfg_pol_t *pol;
    hal_handle_id_ht_entry_t *entry = (hal_handle_id_ht_entry_t *)ht_entry;
    NatPolicyGetResponseMsg *rsp = (NatPolicyGetResponseMsg *)ctxt;
    nat::NatPolicyGetResponse *response = rsp->add_response();

    pol = (nat_cfg_pol_t *)hal_handle_get_obj(entry->handle_id);
    nat_cfg_pol_spec_build(pol, response->mutable_spec());

    // return false here, so that we don't terminate the walk
    return false;
}

hal_ret_t
nat_cfg_pol_get_cfg_handle (NatPolicyGetRequest& req,
                            NatPolicyGetResponseMsg *rsp)
{
    nat_cfg_pol_t *pol;

    // walk all policies as no key is specified
    if (!req.has_key_or_handle()) {
        g_hal_state->nat_policy_ht()->walk(nat_policy_get_ht_cb, rsp);
        HAL_API_STATS_INC(HAL_API_NAT_POLICY_GET_SUCCESS);
        return HAL_RET_OK;
    }

    auto kh = req.key_or_handle();
    auto response = rsp->add_response();
    if ((pol = nat_cfg_pol_key_or_handle_lookup(kh)) == NULL) {
        response->set_api_status(types::API_STATUS_NOT_FOUND);
        HAL_API_STATS_INC(HAL_API_NAT_POLICY_GET_FAIL);
        return HAL_RET_NAT_POLICY_NOT_FOUND;
    }
    nat_cfg_pol_spec_build(pol, response->mutable_spec());
    HAL_API_STATS_INC(HAL_API_NAT_POLICY_GET_SUCCESS);
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// DELETE configuration handling
//-----------------------------------------------------------------------------

hal_ret_t
nat_cfg_pol_delete_cfg_handle (nat_cfg_pol_t *pol)
{
    nat_cfg_pol_cleanup(pol);
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// DELETE operational handling
//-----------------------------------------------------------------------------

hal_ret_t
nat_cfg_pol_delete_oper_handle (nat_cfg_pol_t *pol)
{
    hal_ret_t ret;

    if ((ret = nat_cfg_pol_delete_db_handle(&pol->key)) != HAL_RET_OK)
        return ret;

    if ((ret = cfg_ctxt_op_delete_handle(
            HAL_OBJ_ID_NAT_POLICY, pol, NULL, hal_cfg_op_null_cb,
            hal_cfg_op_null_cb, hal_cfg_op_null_cb,
            hal_cfg_op_null_cb, pol->hal_hdl)) != HAL_RET_OK)
        return ret;

    if ((ret = nat_cfg_pol_acl_cleanup(pol)) != HAL_RET_OK)
        return ret;

    return HAL_RET_OK;
}

}  // namespace hal
