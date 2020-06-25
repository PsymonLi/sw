//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec sa entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/ipsec.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/pds_state.hpp"

namespace api {

ipsec_sa_entry::ipsec_sa_entry() {
    ht_ctxt_.reset();
}

ipsec_sa_entry::~ipsec_sa_entry() {
}

api_base *
ipsec_sa_entry::clone(api_ctxt_t *api_ctxt) {
    ipsec_sa_entry *cloned_ipsec_sa;

    cloned_ipsec_sa = ipsec_sa_db()->alloc();
    if (cloned_ipsec_sa) {
        new (cloned_ipsec_sa) ipsec_sa_entry();
        if (cloned_ipsec_sa->init_config(api_ctxt) != SDK_RET_OK) {
            goto error;
        }
        cloned_ipsec_sa->impl_ = impl_->clone();
        if (unlikely(cloned_ipsec_sa->impl_ == NULL)) {
            PDS_TRACE_ERR("Failed to clone ipsec_sa %s impl", key_.str());
            goto error;
        }
    }
    return cloned_ipsec_sa;

error:

    cloned_ipsec_sa->~ipsec_sa_entry();
    ipsec_sa_db()->free(cloned_ipsec_sa);
    return NULL;
}

sdk_ret_t
ipsec_sa_entry::reserve_resources(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret = SDK_RET_OK;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        ret = impl_->reserve_resources(this, orig_obj, obj_ctxt);
        break;

    case API_OP_UPDATE:
        if (impl_) {
            ret = impl_->reserve_resources(this, orig_obj, obj_ctxt);
        }
        break;

    case API_OP_DELETE:
    default:
        ret = sdk::SDK_RET_INVALID_OP;
    }
    return ret;
}

sdk_ret_t
ipsec_sa_entry::release_resources(void) {
    if (impl_) {
        impl_->release_resources(this);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::nuke_resources_(void) {
    if (impl_) {
        impl_->nuke_resources(this);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::init_config(api_ctxt_t *api_ctxt) {
    pds_ipsec_sa_encrypt_spec_t *spec = &api_ctxt->api_params->ipsec_sa_encrypt_spec;

    PDS_TRACE_VERBOSE("Initializing ipsec SA %s", spec->key.str());

    key_ = spec->key;
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::program_create(api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->program_hw(this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::cleanup_config(api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->cleanup_hw(this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::program_update(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->update_hw(orig_obj, this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                                        api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        PDS_TRACE_DEBUG("Activating ipsec SA %s config", key_.str());
        return impl_->activate_hw(this, orig_obj, epoch, api_op, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::reprogram_config(api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->reprogram_hw(this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::reactivate_config(pds_epoch_t epoch, api_obj_ctxt_t *obj_ctxt) {
    if (impl_) {
        return impl_->reactivate_hw(this, epoch, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_entry::add_to_db(void) {
    return ipsec_sa_db()->insert(this);
}

sdk_ret_t
ipsec_sa_entry::del_from_db(void) {
    if (ipsec_sa_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
ipsec_sa_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_IPSEC, this);
}

ipsec_sa_entry *
ipsec_sa_entry::factory(void *spec, bool encrypt_sa) {
    ipsec_sa_entry *ipsec_sa;

    // create ipsec_sa_encrypt entry with defaults, if any
    ipsec_sa = (ipsec_sa_entry *)ipsec_sa_db()->alloc();
    if (ipsec_sa) {
        if (encrypt_sa) {
            new (ipsec_sa) ipsec_sa_encrypt_entry();
            ipsec_sa->impl_ = impl_base::factory(
                    impl::IMPL_OBJ_ID_IPSEC_SA_ENCRYPT, spec);
            ipsec_sa->set_encrypt_sa(true);
        } else {
            new (ipsec_sa) ipsec_sa_decrypt_entry();
            ipsec_sa->impl_ = impl_base::factory(
                    impl::IMPL_OBJ_ID_IPSEC_SA_DECRYPT, spec);
            ipsec_sa->set_encrypt_sa(false);
        }
    }
    return ipsec_sa;
}

void
ipsec_sa_entry::destroy(ipsec_sa_entry *ipsec_sa) {
    ipsec_sa->nuke_resources_();
    if (ipsec_sa->impl_) {
        if (ipsec_sa->is_encrypt_sa()) {
            impl_base::destroy(impl::IMPL_OBJ_ID_IPSEC_SA_ENCRYPT, ipsec_sa->impl_);
        } else {
            impl_base::destroy(impl::IMPL_OBJ_ID_IPSEC_SA_DECRYPT, ipsec_sa->impl_);
        }
    }
    ipsec_sa->~ipsec_sa_entry();
    ipsec_sa_db()->free(ipsec_sa);
}

// encrypt entry
ipsec_sa_encrypt_entry::ipsec_sa_encrypt_entry() {
}

ipsec_sa_encrypt_entry::~ipsec_sa_encrypt_entry() {
}

sdk_ret_t
ipsec_sa_encrypt_entry::free(ipsec_sa_encrypt_entry *ipsec_sa_encrypt) {
    if (ipsec_sa_encrypt->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_IPSEC_SA_ENCRYPT, ipsec_sa_encrypt->impl_);
    }
    ipsec_sa_encrypt->~ipsec_sa_encrypt_entry();
    ipsec_sa_db()->free(ipsec_sa_encrypt);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_encrypt_entry::populate_msg(pds_msg_t *msg, api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.op = obj_ctxt->api_op;
    msg->cfg_msg.obj_id = OBJ_ID_IPSEC_SA_ENCRYPT;
    if (obj_ctxt->api_op == API_OP_DELETE) {
        msg->cfg_msg.ipsec_sa_encrypt.key = obj_ctxt->api_params->key;
    } else {
        msg->cfg_msg.ipsec_sa_encrypt.spec = obj_ctxt->api_params->ipsec_sa_encrypt_spec;
        if (impl_) {
            impl_->populate_msg(msg, this, obj_ctxt);
        }
    }
    return SDK_RET_OK;
}

void
ipsec_sa_encrypt_entry::fill_spec_(pds_ipsec_sa_encrypt_spec_t *spec) {
    memcpy(&spec->key, &key_, sizeof(pds_obj_key_t));
    // TODO get from impl
}

sdk_ret_t
ipsec_sa_encrypt_entry::read(pds_ipsec_sa_encrypt_info_t *info) {
    fill_spec_(&info->spec);
    if (impl_) {
        return impl_->read_hw(this, (impl::obj_key_t *)(&info->spec.key),
                              (impl::obj_info_t *)info);
    }
    return SDK_RET_OK;
}

// decrypt entry
ipsec_sa_decrypt_entry::ipsec_sa_decrypt_entry() {
}

ipsec_sa_decrypt_entry::~ipsec_sa_decrypt_entry() {
}


sdk_ret_t
ipsec_sa_decrypt_entry::free(ipsec_sa_decrypt_entry *ipsec_sa_decrypt) {
    if (ipsec_sa_decrypt->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_IPSEC_SA_DECRYPT, ipsec_sa_decrypt->impl_);
    }
    ipsec_sa_decrypt->~ipsec_sa_decrypt_entry();
    ipsec_sa_db()->free(ipsec_sa_decrypt);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_decrypt_entry::populate_msg(pds_msg_t *msg, api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.op = obj_ctxt->api_op;
    msg->cfg_msg.obj_id = OBJ_ID_IPSEC_SA_DECRYPT;
    if (obj_ctxt->api_op == API_OP_DELETE) {
        msg->cfg_msg.ipsec_sa_decrypt.key = obj_ctxt->api_params->key;
    } else {
        msg->cfg_msg.ipsec_sa_decrypt.spec = obj_ctxt->api_params->ipsec_sa_decrypt_spec;
        if (impl_) {
            impl_->populate_msg(msg, this, obj_ctxt);
        }
    }
    return SDK_RET_OK;
}

void
ipsec_sa_decrypt_entry::fill_spec_(pds_ipsec_sa_decrypt_spec_t *spec) {
    memcpy(&spec->key, &key_, sizeof(pds_obj_key_t));
    // TODO get from impl
}

sdk_ret_t
ipsec_sa_decrypt_entry::read(pds_ipsec_sa_decrypt_info_t *info) {
    fill_spec_(&info->spec);
    if (impl_) {
        return impl_->read_hw(this, (impl::obj_key_t *)(&info->spec.key),
                              (impl::obj_info_t *)info);
    }
    return SDK_RET_OK;
}


}    // namespace api
