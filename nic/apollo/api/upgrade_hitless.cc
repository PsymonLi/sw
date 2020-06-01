//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/asic/pd/scheduler.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/core.hpp"
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
#include "nic/apollo/api/impl/lif_impl.hpp"

namespace api {

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

    default:
        SDK_ASSERT(0);
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
    upg_shm *shm = api::g_upg_state->backup_shm();

    tep_ht = tep_db()->tep_ht();
    ret = (tep_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_nexthop (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *nexthop_ht;
    upg_shm *shm = api::g_upg_state->backup_shm();

    nexthop_ht = nexthop_db()->nh_ht();
    ret = (nexthop_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_nexthop_group (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *nh_group_ht;
    upg_shm *shm = api::g_upg_state->backup_shm();

    nh_group_ht = nexthop_group_db()->nh_group_ht();
    ret = (nh_group_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_vnic (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *vnic_ht;
    upg_shm *shm = api::g_upg_state->backup_shm();

    vnic_ht = vnic_db()->vnic_ht();
    ret = (vnic_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_vpc (upg_obj_info_t *info)
{
    sdk_ret_t ret;
    ht *vpc_ht;
    upg_shm *shm = api::g_upg_state->backup_shm();

    vpc_ht = vpc_db()->vpc_ht();
    ret = (vpc_ht->walk(backup_stateful_obj_cb, (void *)info));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static inline sdk_ret_t
backup_mapping (upg_obj_info_t *info)
{
    sdk::lib::kvstore *kvs;
    sdk_ret_t ret;
    upg_shm *shm = api::g_upg_state->backup_shm();

    kvs = api::g_pds_state.kvstore();
    ret = (kvs->iterate(backup_statless_obj_cb, (void *)info, "mapping"));
    // adjust the offset in persistent storage in the end of walk
    shm->api_upg_ctx()->incr_obj_offset(info->backup.total_size);
    return ret;
}

static sdk_ret_t
upg_ev_compat_check (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_start (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_backup (upg_ev_params_t *params)
{
    sdk_ret_t ret;
    uint32_t obj_size;
    upg_obj_info_t info;
    upg_obj_stash_meta_t *hdr;
    upg_shm *shm = api::g_upg_state->backup_shm();

    SDK_ASSERT(shm->api_upg_ctx() != NULL);
    // get and initialize a segment from shread memory for write
    ret = shm->api_upg_ctx()->init(PDS_UPGRADE_API_OBJ_STORE_NAME,
                                   PDS_UPGRADE_API_OBJ_STORE_SIZE, true);
    SDK_ASSERT(ret == SDK_RET_OK);
    // set the backup status to true. will set to false if there is a failure
    g_upg_state->set_backup_status(true);
    hdr = (upg_obj_stash_meta_t *)shm->api_upg_ctx()->mem();
    obj_size = shm->api_upg_ctx()->obj_size();

    for (uint32_t id = (uint32_t )OBJ_ID_NONE + 1; id < OBJ_ID_MAX; id++) {
        memset(&info, 0, sizeof(upg_obj_info_t));
        // initialize meta for obj
        hdr[id].obj_id = id;
        hdr[id].offset = shm->api_upg_ctx()->obj_offset();
        // initialize upg_info
        info.obj_id = id;
        info.mem = (char *)hdr + hdr[id].offset;
        info.backup.size_left = obj_size - hdr[id].offset;
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
            ret = backup_nexthop(&info);
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

        default:
            break;
        }
    }   //end for

    if (g_upg_state->backup_status() == false) {
        PDS_TRACE_ERR("Backup failed");
        return SDK_RET_ERR;
    }

    ret = impl_base::pipeline_impl()->upgrade_backup();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pipeline backup failed, err %u", ret);
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

static sdk_ret_t
upg_ev_restore (upg_ev_params_t *params)
{
    std::size_t seg_size;
    sdk_ret_t ret;
    uint32_t obj_count;
    upg_shm *shm = g_upg_state->restore_shm();
    char *mem;
    upg_obj_stash_meta_t *hdr;
    upg_obj_info_t info;

    // get the size of shared memory segment created already during backup
    seg_size = shm->shm_mgr()->get_segment_size(PDS_UPGRADE_API_OBJ_STORE_NAME);
    SDK_ASSERT(seg_size != 0);
    SDK_ASSERT(shm->api_upg_ctx() != NULL);
    // now get the segment handle and open for read.
    // size is ignored if its read operation
    ret = shm->api_upg_ctx()->init(PDS_UPGRADE_API_OBJ_STORE_NAME, seg_size, false);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get shared memory segment for restore, err %u",
                      ret);
        return ret;
    }
    hdr = (upg_obj_stash_meta_t *)shm->api_upg_ctx()->mem();

    for (uint32_t id = (uint32_t )OBJ_ID_NONE + 1; id < OBJ_ID_MAX; id++) {
        obj_count = hdr[id].obj_count;
        if (!obj_count) {
            continue;
        }
        PDS_TRACE_INFO("Started restoring %u objs for id %u", obj_count, id);
        memset(&info, 0, sizeof(upg_obj_info_t));
        // initialize the mem reference for each unique obj
        mem = (char *)hdr + hdr[id].offset;
        while (obj_count--) {
            info.mem = mem;
            info.obj_id = id;
            ret = restore_obj(&info);
            // will continue restoring even if something fails
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Restore for obj id %u failed, err %u", id, ret);
            }
            mem += info.size;
        }   // end while
        PDS_TRACE_INFO("Finished restoring %u objs for id %u",
                       hdr[id].obj_count, id);
    }   // end for

    return ret;
}

static sdk_ret_t
upg_ev_ready (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_quiesce (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_switchover (upg_ev_params_t *params)
{
    sdk_ret_t ret = impl_base::pipeline_impl()->upgrade_switchover();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Upgrade pipeline switchover failed, err %u", ret);
    }
    return ret;
}

static sdk_ret_t
upg_ev_config_replay (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_sync (upg_ev_params_t *params)
{
    return SDK_RET_OK;
}

static sdk_ret_t
upg_ev_repeal (upg_ev_params_t *params)
{
   return SDK_RET_OK;
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
    ev_hdlr.quiesce_hdlr = upg_ev_quiesce;
    ev_hdlr.switchover_hdlr = upg_ev_switchover;
    ev_hdlr.repeal_hdlr = upg_ev_repeal;
    ev_hdlr.restore_hdlr = upg_ev_restore;

    // register for upgrade events
    upg_ev_thread_hdlr_register(ev_hdlr);

    return SDK_RET_OK;
}

}    // namespace api
