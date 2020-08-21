//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/asic/pd/scheduler.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/sdk/asic/common/asic_mem.hpp"
#include "nic/infra/upgrade/api/include/shmstore.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/core.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/include/pds_nexthop.hpp"
#include "nic/apollo/api/include/pds_upgrade.hpp"
#include "nic/apollo/api/nexthop.hpp"
#include "nic/apollo/api/nexthop_group.hpp"
#include "nic/apollo/api/tep.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/vpc.hpp"
#include "nic/apollo/api/device.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/internal/upgrade_pstate.hpp"

namespace api {

// pds agent config objects. these will be allocated from pds agent store
#define PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_NAME    "pds_agent_upgobjs"

static void
asic_quiesce_queue_credit_read (bool ingress, upgrade_pstate_t *pstate)
{
    sdk::asic::pd::port_queue_credit_t credit;
    sdk::asic::pd::queue_credit_t oqs[64];
    uint32_t oq;
    uint32_t max_index = sizeof(pstate->port_ingress_oq_credits) / sizeof(uint32_t);
    p4pd_pipeline_t pipe = ingress ? P4_PIPELINE_INGRESS : P4_PIPELINE_EGRESS;

    credit.num_queues = 64;
    credit.queues = oqs;

    sdk::asic::pd::asicpd_quiesce_queue_credits_read(pipe, &credit);
    SDK_ASSERT(credit.num_queues <= max_index);
    for (uint32_t i = 0; i < credit.num_queues; i++) {
        oq = credit.queues[i].oq;
        SDK_ASSERT(oq < max_index);
        if (ingress) {
            pstate->port_ingress_oq_credits[oq] = credit.queues[i].credit;
        } else {
            pstate->port_egress_oq_credits[oq] = credit.queues[i].credit;
        }
    }
    if (ingress) {
        pstate->port_ingress_num_oqs = credit.num_queues;
    } else {
        pstate->port_egress_num_oqs = credit.num_queues;
    }
}

static void
asic_quiesce_queue_credit_restore (bool ingress, upgrade_pstate_t *pstate)
{
    sdk::asic::pd::port_queue_credit_t credit;
    sdk::asic::pd::queue_credit_t oqs[64];
    p4pd_pipeline_t pipe = ingress ? P4_PIPELINE_INGRESS : P4_PIPELINE_EGRESS;

    credit.queues = oqs;
    credit.num_queues = ingress ? pstate->port_ingress_num_oqs :
                                  pstate->port_egress_num_oqs;
    SDK_ASSERT(credit.num_queues <= (sizeof(oqs) /
                                     sizeof(sdk::asic::pd::queue_credit_t)));
    for (uint32_t i = 0; i < credit.num_queues; i++) {
        credit.queues[i].oq = i;
        if (ingress) {
             credit.queues[i].credit = pstate->port_ingress_oq_credits[i];
        } else {
             credit.queues[i].credit = pstate->port_egress_oq_credits[i];
        }
    }
    sdk::asic::pd::asicpd_quiesce_queue_credits_restore(pipe, &credit);
}

upgrade_pstate_t *
upgrade_pstate_create_or_open (bool create)
{
    sdk::lib::shmstore *store;
    module_version_t curr_version;
    module_version_t prev_version;
    upgrade_pstate_t *upgrade_pstate;

    std::tie(curr_version, prev_version) = api::g_upg_state->module_version(
                                              core::PDS_THREAD_ID_API,
                                              MODULE_VERSION_HITLESS);
    if (!sdk::platform::sysinit_mode_hitless(g_upg_state->init_mode())) {
        store = api::g_upg_state->backup_shmstore(PDS_AGENT_OPER_SHMSTORE_ID);
        SDK_ASSERT(store != NULL);
        if (create) {
            upgrade_pstate = (upgrade_pstate_t *)store->create_segment(UPGRADE_PSTATE_NAME,
                                                                       sizeof(upgrade_pstate_t));
            new (upgrade_pstate) upgrade_pstate_t();
            // read the port queue credits
            asic_quiesce_queue_credit_read(true, upgrade_pstate);
            asic_quiesce_queue_credit_read(false, upgrade_pstate);
        } else {
            upgrade_pstate = (upgrade_pstate_t *)store->open_segment(UPGRADE_PSTATE_NAME);
        }
    } else {
        store = api::g_upg_state->restore_shmstore(PDS_AGENT_OPER_SHMSTORE_ID);
        SDK_ASSERT(store != NULL);
        upgrade_pstate = (upgrade_pstate_t *)store->open_segment(UPGRADE_PSTATE_NAME);
        // TODO : version management if previous and current versions are different
        SDK_ASSERT(prev_version.minor == curr_version.minor);
    }
    return upgrade_pstate;
}

static bool
backup_stateful_obj_cb (void *obj, void *info)
{
    sdk_ret_t ret;
    string keystr;
    uint32_t obj_id;
    upg_obj_info_t *upg_info;

    if (!obj) {
        return false;
    }
    upg_info = (upg_obj_info_t *)info;
    obj_id = upg_info->obj_id;

    switch (obj_id) {
    case OBJ_ID_NEXTHOP_GROUP:
        keystr = ((nexthop_group *)obj)->key2str();
        ret = ((nexthop_group *)obj)->backup(upg_info);
        break;

    case OBJ_ID_NEXTHOP:
        keystr = ((nexthop *)obj)->key2str();
        ret = ((nexthop *)obj)->backup(upg_info);
        break;

    case OBJ_ID_TEP:
        keystr = ((tep_entry *)obj)->key2str();
        ret = ((tep_entry *)obj)->backup(upg_info);

    case OBJ_ID_VNIC:
        keystr = ((vnic_entry *)obj)->key2str();
        ret = ((vnic_entry *)obj)->backup(upg_info);
        break;

    case OBJ_ID_VPC:
        keystr = ((vpc_entry *)obj)->key2str();
        ret = ((vpc_entry *)obj)->backup(upg_info);
        break;

    case OBJ_ID_DEVICE:
        keystr = ((device_entry *)obj)->key2str();
        ret = ((device_entry *)obj)->backup(upg_info);
        break;

    default:
        SDK_ASSERT(0);
    }

    if (ret == SDK_RET_OK && upg_info->skipped) {
        PDS_TRACE_VERBOSE("Stateful obj id %u key %s intentionally skipped",
                          obj_id, keystr.c_str());
        return false; // continue walk
    }
    if (ret != SDK_RET_OK) {
        api::g_upg_state->set_backup_status(false);
        PDS_TRACE_ERR("Backup stateful obj id %u failed for key %s, err %u",
                      obj_id, keystr.c_str(), ret);
        return true; // stop the walk
    }
    // position mem by incrementing it by size for next obj
    upg_info->mem += upg_info->size;
    // adjust size left in persistent storage and total size stashed
    upg_info->backup.size_left -= upg_info->size;
    upg_info->backup.total_size += upg_info->size;
    upg_info->backup.stashed_obj_count += 1;
    return false;
}

static void
backup_statless_obj_cb (void *key, void *val, void *info)
{
    sdk_ret_t ret;
    uint32_t obj_id;
    upg_obj_info_t *upg_info;
    pds_obj_key_t *pkey = (pds_obj_key_t *)key;

    if (!key || !val) {
        return;
    }
    upg_info = (upg_obj_info_t *)info;
    obj_id = upg_info->obj_id;

    switch (obj_id) {
    case OBJ_ID_MAPPING:
        mapping_entry *entry;
        entry = mapping_entry::build(pkey);
        ret = entry->backup(upg_info);
        mapping_entry::soft_delete(entry);
        break;

    default:
        SDK_ASSERT(0);
    }

    if (ret == SDK_RET_OK && upg_info->skipped) {
        PDS_TRACE_VERBOSE("Stateless obj id %u key %s intentionally skipped",
                          obj_id, pkey->str());
        return;
    }
    if (ret != SDK_RET_OK) {
        api::g_upg_state->set_backup_status(false);
        PDS_TRACE_ERR("Backup stateless obj id %u failed for key %s, err %u",
                      obj_id, pkey->str(), ret);
    } else {
        // position mem by incrementing it by size for next obj
        upg_info->mem += upg_info->size;
        // adjust size left in persistent storage and total size stashed
        upg_info->backup.size_left -= upg_info->size;
        upg_info->backup.total_size += upg_info->size;
        upg_info->backup.stashed_obj_count += 1;
    }
    return;
}

static inline sdk_ret_t
backup_tep (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *tep_ht;
    upg_ctxt *ctx = info->ctx;

    tep_ht = tep_db()->tep_ht();
    ret = (tep_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_nexthop (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *nexthop_ht;
    upg_ctxt *ctx = info->ctx;

    nexthop_ht = nexthop_db()->nh_ht();
    ret = (nexthop_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_nexthop_group (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *nh_group_ht;
    upg_ctxt *ctx = info->ctx;

    nh_group_ht = nexthop_group_db()->nh_group_ht();
    ret = (nh_group_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_vnic (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *vnic_ht;
    upg_ctxt *ctx = info->ctx;

    vnic_ht = vnic_db()->vnic_ht();
    ret = (vnic_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_vpc (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *vpc_ht;
    upg_ctxt *ctx = info->ctx;

    vpc_ht = vpc_db()->vpc_ht();
    ret = (vpc_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_device (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    upg_ctxt *ctx = info->ctx;

    ret = device_db()->walk(backup_stateful_obj_cb, (void *)info);
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_mapping (upg_obj_info_t *info)
{
    sdk::lib::kvstore *kvs;
    sdk_ret_t ret;
    upg_ctxt *ctx = info->ctx;

    kvs = api::g_pds_state.kvstore();
    ret = (kvs->iterate(backup_statless_obj_cb, (void *)info, "mapping"));
    // adjust the offset in persistent storage in the end of walk
    ctx->incr_obj_offset(info->backup.total_size);
    return ret;
}

static sdk_ret_t
upg_ev_compat_check (upg_ev_params_t *params)
{
    upgrade_pstate_t *pstate = upgrade_pstate_create_or_open(true);

    if (pstate == NULL) {
        PDS_TRACE_ERR("Failed to create pstate %s", UPGRADE_PSTATE_NAME);
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_start (upg_ev_params_t *params)
{
    upg_stale_shmstores_remove();
    // below file is used by B to offsets its mem regions during
    // A to B hitless upgrade
    api::g_pds_state.mempartition()->dump_regions_info(
        upg_shmstore_volatile_path_hitless().c_str());
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_backup (upg_ev_params_t *params)
{
    sdk_ret_t ret;
    uint32_t obj_size;
    upg_obj_info_t info;
    upg_obj_stash_meta_t *hdr;
    upg_ctxt *ctx;

    ctx = upg_shmstore_objctx_create(PDS_AGENT_CFG_SHMSTORE_ID,
                                     PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_NAME);
    SDK_ASSERT(ctx);
    // set the backup status to true. will set to false if there is a failure
    g_upg_state->set_backup_status(true);
    hdr = (upg_obj_stash_meta_t *)ctx->mem();
    obj_size = ctx->obj_size();

    for (uint32_t id = (uint32_t )OBJ_ID_NONE + 1; id < OBJ_ID_MAX; id++) {
        memset(&info, 0, sizeof(upg_obj_info_t));
        // initialize meta for obj
        hdr[id].obj_id = id;
        hdr[id].offset = ctx->obj_offset();
        // initialize upg_info
        info.obj_id = id;
        info.mem = (char *)hdr + hdr[id].offset;
        info.backup.size_left = obj_size - hdr[id].offset;
        info.ctx = ctx;
        switch (id) {
        case OBJ_ID_NEXTHOP_GROUP:
            ret = backup_nexthop_group(&info);
            // update total number of nh group objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u nexthop group objs", hdr[id].obj_count);
            break;

        case OBJ_ID_MAPPING:
            ret = backup_mapping(&info);
            // update total number of mapping objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u mapping objs", hdr[id].obj_count);
            break;

        case OBJ_ID_NEXTHOP:
            // TODO: revisit it later if its needed
            // ret = backup_nexthop(&info);
            // update total number of nexthop objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u nexthop objs", hdr[id].obj_count);
            break;

        case OBJ_ID_TEP:
            ret = backup_tep(&info);
            // update total number of tep objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u tep objs", hdr[id].obj_count);
            break;

        case OBJ_ID_VNIC:
            ret = backup_vnic(&info);
            // update total number of vnic objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u vnic objs", hdr[id].obj_count);
            break;

        case OBJ_ID_VPC:
            ret = backup_vpc(&info);
            // update total number of vpc objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u vpc objs", hdr[id].obj_count);
            break;

        case OBJ_ID_DEVICE:
            ret = backup_device(&info);
            // update total number of device objs stashed
            hdr[id].obj_count = info.backup.stashed_obj_count;
            PDS_TRACE_INFO("Stashed %u device objs", hdr[id].obj_count);
            break;

        default:
            break;
        }
    }   //end for

    ctx->destroy(ctx);
    if (g_upg_state->backup_status() == false) {
        PDS_TRACE_ERR("Backup failed");
        return SDK_RET_ERR;
    }

    // save the existing configuration for rollback if there is a switchover
    // failure
    ret = impl_base::pipeline_impl()->upgrade_config_save();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pipeline config save failed, err %u", ret);
    }
    return ret;
}

// dont have spec yet, trying to bypass spec' field specific checks during factory
static inline void
create_obj_spec (api_ctxt_t *api_ctxt, uint32_t obj_id)
{
    pds_vpc_spec_t spec;

    switch (obj_id) {
    case OBJ_ID_VPC:
        memset(&spec, 0, sizeof(pds_vpc_spec_t));
        // apulu doesn't support any other encap type
        spec.fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
        api_ctxt->api_params->vpc_spec = spec;
        break;

    default:
        break;
    }
}

static inline sdk_ret_t
restore_obj (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    api_base *api_obj;
    api_ctxt_t *api_ctxt;

    api_ctxt = api::api_ctxt_alloc((obj_id_t )info->obj_id, API_OP_NONE);
    if (api_ctxt == NULL) {
        return SDK_RET_OOM;
    }
    // create base obj assuming api_ctxt->api_param->spec is 0'd and
    // invoke restore, attributes to be filled down the line in same flow
    create_obj_spec(api_ctxt, info->obj_id);
    api_obj = api_base::factory(api_ctxt);
    SDK_ASSERT(api_obj != NULL);
    ret = api_obj->restore(info);
    if (ret == SDK_RET_OK) {
        api_obj->add_to_db();
        // this will cleanup resource in case of rollback during config replay
        api_obj->set_rsvd_rsc();
        api_obj->set_in_restore_list();
    }
    api_ctxt_free(api_ctxt);
    return ret;
}

sdk_ret_t
upg_hitless_restore_api_objs (void)
{
    sdk_ret_t ret = SDK_RET_OK;
    uint32_t obj_count;
    char *mem;
    upg_obj_stash_meta_t *hdr;
    upg_obj_info_t info;
    upg_ctxt *ctx;

    ctx = upg_shmstore_objctx_open(PDS_AGENT_CFG_SHMSTORE_ID,
                                   PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_NAME);
    SDK_ASSERT(ctx);
    hdr = (upg_obj_stash_meta_t *)ctx->mem();

    for (uint32_t id = (uint32_t )OBJ_ID_NONE + 1; id < OBJ_ID_MAX; id++) {
        obj_count = hdr[id].obj_count;
        if (!obj_count) {
            continue;
        }
        PDS_TRACE_INFO("Started restoring %u objs for id %u", obj_count, id);
        memset(&info, 0, sizeof(upg_obj_info_t));
        info.ctx = ctx;
        // initialize the mem reference for each unique obj
        mem = (char *)hdr + hdr[id].offset;
        while (obj_count--) {
            info.mem = mem;
            info.obj_id = id;
            ret = restore_obj(&info);
            // will continue restoring even if something fails
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Restore for obj id %u failed, err %u", id, ret);
                // TODO: evaluate once dev is over
                SDK_ASSERT(0);
            }
            mem += info.size;
        }   // end while
        PDS_TRACE_INFO("Finished restoring %u objs for id %u",
                       hdr[id].obj_count, id);
    }   // end for

    ctx->destroy(ctx);
    return ret;
}

static sdk_ret_t
upg_ev_ready (upg_ev_params_t *params)
{
    upgrade_pstate_t *pstate = upgrade_pstate_create_or_open(false);

    if (pstate == NULL) {
        PDS_TRACE_ERR("Failed to open pstate %s", UPGRADE_PSTATE_NAME);
        return SDK_RET_ERR;
    }
    asic_quiesce_queue_credit_restore(true, pstate);
    asic_quiesce_queue_credit_restore(false, pstate);
    g_upg_state->set_pstate(pstate);
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_pre_switchover (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
asic_quiesce (bool enable)
{
    sdk_ret_t ret;

    if (enable) {
        ret = sdk::asic::pd::asicpd_quiesce_start();
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Upgrade pipeline quiesce start failed");
        }
        // sim it always fail, so ignore the error
        if (api::g_pds_state.platform_type() != platform_type_t::PLATFORM_TYPE_HW) {
            ret = SDK_RET_OK;
        }
    } else {
        ret = sdk::asic::pd::asicpd_quiesce_stop();
        if(ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Upgrade pipeline quiesce stop failed");
        }
        // sim it always fail, so ignore the error
        if (api::g_pds_state.platform_type() != platform_type_t::PLATFORM_TYPE_HW) {
            ret = SDK_RET_OK;
        }
    }
    return ret;
}

static sdk_ret_t
upg_ev_switchover (upg_ev_params_t *params)
{
    sdk_ret_t ret, rv;
    upgrade_pstate_t *pstate = g_upg_state->pstate();

    ret = asic_quiesce(true);
    if (ret == SDK_RET_OK) {
        pstate->pipeline_switchover_done = true;
        ret = impl_base::pipeline_impl()->upgrade_switchover();
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Pipeline switchover failed, ret %u", ret);
        }
    }
    // do quiesce stop irrespective of the failure. don't want to stop the
    // pipeline and return from here
    rv = asic_quiesce(false);
    ret = ret == SDK_RET_OK ? rv : ret;
    if (ret == SDK_RET_OK) {
        pstate->linkmgr_switchover_done = true;
        ret = sdk::linkmgr::port_upgrade_switchover();
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Port switchover failed, ret %u", ret);
        }
    }
    return ret;
}

static sdk_ret_t
upg_ev_config_replay (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static bool
walk_stale_api_obj_cb (void *obj, void *info)
{
    upg_obj_info_t *upg_info = (upg_obj_info_t *)info;

    if (!obj) {
        return false;
    }

    // TODO: if we restored the object but it was never replayed
    // dump all such keys for an obj
    if (((api_base *)obj)->in_restore_list()) {
        PDS_TRACE_ERR("obj id %u, key %s was not replayed", upg_info->obj_id,
                       ((api_base *)obj)->key2str().c_str());
        upg_info->backup.stashed_obj_count++;
    }
    return false;   // continue the walk
}

static void
restore_stale_obj_check (uint32_t obj_id, upg_obj_info_t *info)
{
    switch (obj_id) {
    case OBJ_ID_NEXTHOP_GROUP:
        ht *nh_group_ht;
        nh_group_ht = nexthop_group_db()->nh_group_ht();
        nh_group_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_MAPPING:
        ht *mapping_ht;
        mapping_ht = mapping_db()->mapping_ht();
        mapping_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_NEXTHOP:
        ht *nexthop_ht;
        nexthop_ht = nexthop_db()->nh_ht();
        nexthop_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_TEP:
        ht *tep_ht;
        tep_ht = tep_db()->tep_ht();
        tep_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_VNIC:
        ht *vnic_ht;
        vnic_ht = vnic_db()->vnic_ht();
        vnic_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_VPC:
        ht *vpc_ht;
        vpc_ht = vpc_db()->vpc_ht();
        vpc_ht->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    case OBJ_ID_DEVICE:
        device_db()->walk(walk_stale_api_obj_cb, (void *)info);
        break;

    default:
        PDS_TRACE_ERR("obj id %u is not being checked for stale obj presence",
                      info->obj_id);
        break;
    }
}

static sdk_ret_t
upg_ev_sync (upg_ev_params_t *params)
{
    uint32_t id;
    upg_ctxt *ctx;
    upg_obj_stash_meta_t *hdr;
    upg_obj_info_t info;

    ctx = upg_shmstore_objctx_open(PDS_AGENT_CFG_SHMSTORE_ID,
                                   PDS_AGENT_UPGRADE_SHMSTORE_OBJ_SEG_NAME);
    SDK_ASSERT(ctx);
    hdr = (upg_obj_stash_meta_t *)ctx->mem();

    for (id = (uint32_t )OBJ_ID_NONE + 1; id < OBJ_ID_MAX; id++) {
        if (hdr[id].obj_count) {
            memset(&info, 0, sizeof(upg_obj_info_t));
            info.obj_id = id;
            // detect and assert for objs which were restored but never replayed
            PDS_TRACE_DEBUG("check obj id %u for stale objs, total %u were stashed",
                            id, hdr[id].obj_count);
            restore_stale_obj_check(id, &info);
            if (info.backup.stashed_obj_count) {
                PDS_TRACE_ERR("obj id %u, has %u stale objs", id,
                              info.backup.stashed_obj_count);
                SDK_ASSERT(0);
            }
        }
    }
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_repeal (upg_ev_params_t *params)
{
    sdk_ret_t ret, rv;
    upgrade_pstate_t *pstate = upgrade_pstate_create_or_open(false);

    if (pstate == NULL) {
        PDS_TRACE_ERR("Failed to open pstate %s", UPGRADE_PSTATE_NAME);
        return SDK_RET_ERR;
    }
    ret = threads_suspend_or_resume(false);
    if ((ret == SDK_RET_OK) && pstate->pipeline_switchover_done) {
        ret = asic_quiesce(true);
        if (ret == SDK_RET_OK) {
            ret = impl_base::pipeline_impl()->upgrade_switchover();
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Pipeline switchover failed, ret %u", ret);
            }
        }
        // do quiesce stop irrespective of the failure. don't want to stop the
        // pipeline and return from here
        rv = asic_quiesce(false);
        ret = ret == SDK_RET_OK ? rv : ret;
        if (ret == SDK_RET_OK && pstate->linkmgr_switchover_done) {
            ret = sdk::linkmgr::port_upgrade_switchover();
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Port switchover failed, ret %u", ret);
            }
        }
    }
    // clear of the states
    pstate->pipeline_switchover_done = false;
    pstate->linkmgr_switchover_done = false;
    return ret != SDK_RET_OK ? sdk_ret_t::SDK_RET_UPG_CRITICAL : ret;
}

static bool
mpartition_reset_regions_by_kind_walk (mpartition_region_t *reg, void *ctx)
{
    // reset all regions used by A during A to B upgrade
    // now we reset state regions also. but later when the support for state
    // table re-use exists, should do this based on a switch
    if (reg->kind == sdk::platform::utils::region_kind_t::MEM_REGION_KIND_CFGTBL ||
        reg->kind == sdk::platform::utils::region_kind_t::MEM_REGION_KIND_OPERTBL) {
        sdk::asic::asic_reset_mem_region(reg);
    }
    return false;
}

static sdk_ret_t
upg_ev_finish (upg_ev_params_t *params)
{
    sdk_ret_t ret;
    upgrade_pstate_t *pstate = upgrade_pstate_create_or_open(false);

    // clear of the states
    pstate->pipeline_switchover_done = false;
    pstate->linkmgr_switchover_done = false;

    api::g_pds_state.mempartition()->walk(mpartition_reset_regions_by_kind_walk, NULL);
    ret = pds_teardown();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("PDS teardown failed");
    }
    return ret;
}

sdk_ret_t
upg_hitless_init (pds_init_params_t *params)
{
    upg_ev_hitless_t ev_hdlr;

    // fill upgrade events for graceful
    memset(&ev_hdlr, 0, sizeof(ev_hdlr));
    // thread name is used for just identification here
    strncpy(ev_hdlr.thread_name, "hal", sizeof(ev_hdlr.thread_name));
    ev_hdlr.compat_check_hdlr = upg_ev_compat_check;
    ev_hdlr.start_hdlr = upg_ev_start;
    ev_hdlr.backup_hdlr = upg_ev_backup;
    ev_hdlr.ready_hdlr = upg_ev_ready;
    ev_hdlr.config_replay_hdlr = upg_ev_config_replay;
    ev_hdlr.sync_hdlr = upg_ev_sync;
    ev_hdlr.switchover_hdlr = upg_ev_switchover;
    ev_hdlr.pre_switchover_hdlr = upg_ev_pre_switchover;
    ev_hdlr.repeal_hdlr = upg_ev_repeal;
    ev_hdlr.finish_hdlr = upg_ev_finish;

    // register for upgrade events
    upg_ev_thread_hdlr_register(ev_hdlr);

    return SDK_RET_OK;
}

}    // namespace api
