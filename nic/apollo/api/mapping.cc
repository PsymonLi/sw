/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    mapping.cc
 *
 * @brief   mapping entry handling
 */

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/mapping.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"

namespace api {

/**
 * @defgroup PDS_MAPPING_ENTRY - mapping entry functionality
 * @ingroup PDS_MAPPING
 * @{
 */

mapping_entry::mapping_entry() {
    is_local_ = false;
    public_ip_valid_ = false;
    ht_ctxt_.reset();
    skey_ht_ctxt_.reset();
}

mapping_entry *
mapping_entry::factory(pds_mapping_spec_t *spec) {
    mapping_entry *mapping;

    // create mapping entry with defaults, if any
    mapping = mapping_db()->alloc();
    if (mapping) {
        new (mapping) mapping_entry();
        if (spec->is_local) {
            mapping->is_local_ = true;
        }
        mapping->impl_ =
            impl_base::factory(impl::IMPL_OBJ_ID_MAPPING, spec);
        if (mapping->impl_ == NULL) {
            mapping_entry::destroy(mapping);
            return NULL;
        }
    }
    return mapping;
}

mapping_entry::~mapping_entry() {
}

void
mapping_entry::destroy(mapping_entry *mapping) {
    mapping->nuke_resources_();
    if (mapping->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_MAPPING, mapping->impl_);
    }
    mapping->~mapping_entry();
    mapping_db()->free(mapping);
}

api_base *
mapping_entry::clone(api_ctxt_t *api_ctxt) {
    mapping_entry *cloned_mapping;

    cloned_mapping = mapping_db()->alloc();
    if (cloned_mapping) {
        new (cloned_mapping) mapping_entry();
        if (cloned_mapping->init_config(api_ctxt) != SDK_RET_OK) {
            goto error;
        }
        cloned_mapping->impl_ = impl_->clone();
        if (unlikely(cloned_mapping->impl_ == NULL)) {
            PDS_TRACE_ERR("Failed to clone mapping %s impl", key2str().c_str());
            goto error;
        }
    }
    return cloned_mapping;

error:

    cloned_mapping->~mapping_entry();
    mapping_db()->free(cloned_mapping);
    return NULL;
}

sdk_ret_t
mapping_entry::free(mapping_entry *mapping) {
    if (mapping->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_MAPPING, mapping->impl_);
    }
    mapping->~mapping_entry();
    mapping_db()->free(mapping);
    return SDK_RET_OK;
}

mapping_entry *
mapping_entry::build(pds_mapping_key_t *skey) {
    mapping_entry *mapping;

    // create mapping entry with defaults, if any
    mapping = mapping_db()->alloc();
    if (mapping) {
        new (mapping) mapping_entry();
        memcpy(&mapping->skey_, skey, sizeof(*skey));
        mapping->impl_ = impl_base::build(impl::IMPL_OBJ_ID_MAPPING,
                                          &mapping->skey_, mapping);
        if (mapping->impl_ == NULL) {
            mapping_entry::destroy(mapping);
            return NULL;
        }
    }
    return mapping;
}

mapping_entry *
mapping_entry::build(pds_obj_key_t *key) {
    pds_mapping_key_t skey;
    mapping_entry *mapping = NULL;

    // find the 2nd-ary key corresponding to this primary key
    if (mapping_db()->skey(key, &skey) == SDK_RET_OK) {
        // and then build the object
        mapping = mapping_entry::build(&skey);
        if (mapping) {
            memcpy(&mapping->key_, key, sizeof(*key));
        }
    }
    return mapping;
}

void
mapping_entry::soft_delete(mapping_entry *mapping) {
    if (mapping->impl_) {
        impl_base::soft_delete(impl::IMPL_OBJ_ID_MAPPING, mapping->impl_);
    }
    mapping->del_from_db();
    mapping->~mapping_entry();
    mapping_db()->free(mapping);
}

sdk_ret_t
mapping_entry::reserve_resources(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, orig_obj, obj_ctxt);
}

sdk_ret_t
mapping_entry::release_resources(void) {
    return impl_->release_resources(this);
}

sdk_ret_t
mapping_entry::nuke_resources_(void) {
    if (this->impl_) {
        return impl_->nuke_resources(this);
    }
    return SDK_RET_OK;
}

sdk_ret_t
mapping_entry::init_config(api_ctxt_t *api_ctxt) {
    pds_mapping_spec_t *spec = &api_ctxt->api_params->mapping_spec;

    if (spec->is_local) {
        if (spec->skey.type != PDS_MAPPING_TYPE_L3) {
            // local L2 mapping will come down in the form of vnics, not as
            // local mappings
            PDS_TRACE_ERR("local mapppings can't be non-L3");
            return SDK_RET_INVALID_ARG;
        }
    }
    memcpy(&key_, &spec->key, sizeof(key_));
    memcpy(&skey_, &spec->skey, sizeof(skey_));
    if ((spec->skey.type == PDS_MAPPING_TYPE_L2) && spec->num_tags) {
        PDS_TRACE_ERR("tags are not supported on L2 mappings, "
                      "configured %u tags", spec->num_tags);
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

sdk_ret_t
mapping_entry::program_create(api_obj_ctxt_t *obj_ctxt) {
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
mapping_entry::cleanup_config(api_obj_ctxt_t *obj_ctxt) {
    return impl_->cleanup_hw(this, obj_ctxt);
}

sdk_ret_t
mapping_entry::compute_update(api_obj_ctxt_t *obj_ctxt) {
    pds_mapping_spec_t *spec = &obj_ctxt->api_params->mapping_spec;

    // L2R, R2L updates not supported
    if (is_local_ != spec->is_local) {
        PDS_TRACE_ERR("R2L/L2R mapping moves are not supported via update, "
                      "mapping %s must be deleted and re-added",
                      key2str().c_str());
        return SDK_RET_INVALID_ARG;
    }

    // check for any changes in the public IP configuration
    if (is_local_) {
        if ((public_ip_valid_ == false) && spec->public_ip_valid) {
            // public IP wasn't there but configured now
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_PUBLIC_IP_ADD;
        } else if (public_ip_valid_ && (spec->public_ip_valid == false)) {
            // public IP was present, but now removed
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_PUBLIC_IP_DEL;
        } else if (public_ip_valid_ && spec->public_ip_valid &&
                   (memcmp(&public_ip_, &spec->public_ip,
                           sizeof(public_ip_)))) {
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_PUBLIC_IP_UPD;
        }
        // no change in public IP information
    }

    // check for any changes in the tag configuration
    if (spec->skey.type == PDS_MAPPING_TYPE_L3) {
        if ((num_tags_ == 0) && (spec->num_tags != 0)) {
            // tags didn't exist before but added now
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_TAGS_ADD;
        } else if (num_tags_ && (spec->num_tags == 0)) {
            // tags were there before but now cleared
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_TAGS_DEL;
        } else if (num_tags_ && spec->num_tags) {
            // tag content can be compared later for optimization
            obj_ctxt->upd_bmap |= PDS_MAPPING_UPD_TAGS_UPD;
        }
    }
    PDS_TRACE_DEBUG("mapping %s update bmap 0x%lx", key2str().c_str(),
                    obj_ctxt->upd_bmap);
    return SDK_RET_OK;
}

sdk_ret_t
mapping_entry::program_update(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Updating mapping %s", key_.str());
    return impl_->update_hw(orig_obj, this, obj_ctxt);
}

sdk_ret_t
mapping_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                               api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    return impl_->activate_hw(this, orig_obj, epoch, api_op, obj_ctxt);
}

void
mapping_entry::fill_spec_(pds_mapping_spec_t *spec) {
    memcpy(&spec->key, &key_, sizeof(pds_obj_key_t));
    memcpy(&spec->skey, &skey_, sizeof(pds_mapping_key_t));
    memcpy(&spec->is_local, &is_local_, sizeof(bool));
    memcpy(&spec->public_ip_valid, &public_ip_valid_, sizeof(bool));
}

sdk_ret_t
mapping_entry::read(pds_obj_key_t *key, pds_mapping_info_t *info) {
    pds_mapping_key_t  skey;

    // find the 2nd-ary key corresponding to this primary key
    if (mapping_db()->skey(key, &skey) == SDK_RET_OK) {
        // and then read from h/w
        memcpy(&info->spec.key, key, sizeof(*key));
        memcpy(&info->spec.skey, &skey, sizeof(skey));
        return impl_->read_hw(this, (impl::obj_key_t *)&skey,
                              (impl::obj_info_t *)info);
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
mapping_entry::read(pds_mapping_key_t *skey, pds_mapping_info_t *info) {
    return impl_->read_hw(this, (impl::obj_key_t *)skey,
                          (impl::obj_info_t *)info);
}

// even though mapping object is stateless, we need to temporarily insert
// into the db as back-to-back operations on the same object can be issued
// in same batch
sdk_ret_t
mapping_entry::add_to_db(void) {
    return mapping_db()->insert(this);
}

sdk_ret_t
mapping_entry::del_from_db(void) {
    if (mapping_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
mapping_entry::update_db(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (mapping_db()->remove((mapping_entry *)orig_obj)) {
        return mapping_db()->insert(this);
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
mapping_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_MAPPING, this);
}

sdk_ret_t
mapping_entry::backup(upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    pds_mapping_info_t info;

    memset(&info, 0, sizeof(pds_mapping_info_t));
    fill_spec_(&info.spec);
    // let impl decide whether we need to save this obj
    ret = impl_->backup((impl::obj_info_t *)&info, upg_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to backup mapping %s err %u", key_.str(), ret);
    }
    return ret;
}

sdk_ret_t
mapping_entry::restore(upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    api_ctxt_t *api_ctxt;
    pds_mapping_info_t info;

    memset(&info, 0, sizeof(pds_mapping_info_t));
    // fetch info from proto buf
    ret = impl_->restore((impl::obj_info_t *)&info, upg_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore mapping, err %u", ret);
        return ret;
    }
    // restore PI fields
    api_ctxt = api::api_ctxt_alloc((obj_id_t)upg_info->obj_id, API_OP_NONE);
    if (api_ctxt == NULL) {
        return SDK_RET_OOM;
    }
    api_ctxt->api_params->mapping_spec = info.spec;
    ret = init_config(api_ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore mapping %s, err %u",
                      info.spec.key.str(), ret);
        return ret;
    }
    is_local_ = info.spec.is_local;
    api_ctxt_free(api_ctxt);
    return ret;
}

}    // namespace api
