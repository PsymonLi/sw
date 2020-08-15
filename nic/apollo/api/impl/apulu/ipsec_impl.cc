//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of ipsec sa
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/ipsec/ipsec.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/ipsec.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/ipsec_impl.hpp"
#include "nic/apollo/p4/include/apulu_defines.h"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "gen/p4gen/p4/include/ftl.h"

namespace api {
namespace impl {

/// \defgroup PDS_IPSEC_SA_IMPL - ipsec entry datapath implementation
/// \ingroup PDS_IPSEC
/// @{

ipsec_sa_impl *
ipsec_sa_impl::factory(void *spec, bool encrypt_sa) {
    ipsec_sa_impl *impl;

    impl = ipsec_sa_impl_db()->alloc();
    if (encrypt_sa) {
        new (impl) ipsec_sa_encrypt_impl((pds_ipsec_sa_encrypt_spec_t *)spec);
        impl->set_encrypt_sa(true);
    } else {
        new (impl) ipsec_sa_decrypt_impl((pds_ipsec_sa_decrypt_spec_t *)spec);
        impl->set_encrypt_sa(false);
    }
    return impl;
}

void
ipsec_sa_impl::destroy(ipsec_sa_impl *impl) {
    if (impl->is_encrypt_sa()) {
        ((ipsec_sa_encrypt_impl *)impl)->~ipsec_sa_encrypt_impl();
    } else {
        ((ipsec_sa_decrypt_impl *)impl)->~ipsec_sa_decrypt_impl();
    }
    ipsec_sa_impl_db()->free(impl);
}

impl_base *
ipsec_sa_impl::clone(void) {
    ipsec_sa_impl *cloned_impl;

    cloned_impl = ipsec_sa_impl_db()->alloc();
    if (encrypt_sa_) {
        new (cloned_impl) ipsec_sa_encrypt_impl();
    } else {
        new (cloned_impl) ipsec_sa_decrypt_impl();
    }
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
ipsec_sa_impl::free(ipsec_sa_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                                         api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    rte_indexer *idxr;
    // need spec only for key which is in the same location for encrypt and
    // decrypt spec
    pds_ipsec_sa_encrypt_spec_t *spec =
        &obj_ctxt->api_params->ipsec_sa_encrypt_spec;
    uint8_t qtype;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();

        // reserve nexthop index
        ret = nexthop_impl_db()->nh_idxr()->alloc(&nh_idx_);
        if (ret != SDK_RET_OK) {
            goto error;
        }

        if (encrypt_sa_) {
            idxr = ipsec_sa_impl_db()->ipsec_sa_encrypt_idxr();
            qtype = IPSEC_ENCRYPT_QTYPE;
        } else {
            idxr = ipsec_sa_impl_db()->ipsec_sa_decrypt_idxr();
            qtype = IPSEC_DECRYPT_QTYPE;
        }

        // allocate hw id for this sa
        if ((ret = idxr->alloc(&idx)) !=
                SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to allocate hw id for ipsec sa %s err %u",
                          spec->key.str(), ret);
            goto error;
        }
        hw_id_ = idx;
        base_pa_ = lif_mgr::get_lif_qstate_addr(
                       api::g_pds_state.ipsec_lif_qstate(),
                       APULU_IPSEC_LIF, qtype, hw_id_);
        PDS_TRACE_DEBUG("Alloc hw_id %u base_addr 0x%lx", hw_id_, base_pa_);
        break;

    case API_OP_UPDATE:
        // TODO

    default:
        break;
    }
    return SDK_RET_OK;

error:
    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }
    return ret;
}

sdk_ret_t
ipsec_sa_impl::release_resources(api_base *api_obj) {
    sdk_ret_t ret;
    sdk_table_api_params_t tparams;

    // release the ipsec sa h/w id
    if (hw_id_ != 0xFFFF) {
        if (encrypt_sa_) {
            ipsec_sa_impl_db()->ipsec_sa_encrypt_idxr()->free(hw_id_);
        } else {
            ipsec_sa_impl_db()->ipsec_sa_decrypt_idxr()->free(hw_id_);
        }
    }

    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl::nuke_resources(api_base *api_obj) {
    release_resources(api_obj);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl::cleanup_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                         api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

sdk_ret_t
ipsec_sa_impl::activate_delete_(pds_epoch_t epoch, ipsec_sa_entry *ipsec_sa) {
    return SDK_RET_ERR;
}

sdk_ret_t
ipsec_sa_impl::reprogram_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

//
// ipsec_sa_encrypt_impl class
//
sdk_ret_t
ipsec_sa_encrypt_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                                    api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.ipsec_sa_encrypt.status.hw_id = hw_id_;
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_encrypt_impl::program_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    pds_ipsec_sa_encrypt_spec_t *spec;
    nexthop_info_entry_t nexthop_info_entry;
    sdk_ret_t ret;

    spec = &obj_ctxt->api_params->ipsec_sa_encrypt_spec;

    // program the nexthop
    PDS_TRACE_DEBUG("Programming encrypt ipsec nh %u", nh_idx_);
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = APULU_IPSEC_LIF;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_IPSEC;
    nexthop_info_entry.qtype = IPSEC_ENCRYPT_QTYPE;
    nexthop_info_entry.qid = hw_id_;
    ret = nexthop_info_entry.write(nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program encrypt ipsec nh idx %u, err %u",
                      nh_idx_, ret);
        return ret;
    }

    ipseccb_encrypt_create(hw_id_, base_pa_, spec);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_encrypt_impl::activate_create_(pds_epoch_t epoch,
                                        ipsec_sa_entry *ipsec_sa,
                                        pds_ipsec_sa_encrypt_spec_t *spec) {
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_encrypt_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                                   pds_epoch_t epoch, api_op_t api_op,
                                   api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_ipsec_sa_encrypt_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->ipsec_sa_encrypt_spec;
        ret = activate_create_(epoch, (ipsec_sa_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (ipsec_sa_entry *)api_obj);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

void
ipsec_sa_encrypt_impl::fill_status_(pds_ipsec_sa_encrypt_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
ipsec_sa_encrypt_impl::fill_stats_(pds_ipsec_sa_encrypt_stats_t *stats) {
    return SDK_RET_ERR;
}

sdk_ret_t
ipsec_sa_encrypt_impl::read_hw(api_base *api_obj, obj_key_t *key,
                               obj_info_t *info) {
    sdk_ret_t rv;
    pds_ipsec_sa_encrypt_info_t *ipsec_sa_info = (pds_ipsec_sa_encrypt_info_t *)info;

    rv = fill_stats_(&ipsec_sa_info->stats);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read hardware stats tables for ipsec sa %s",
                      api_obj->key2str().c_str());
        return rv;
    }
    fill_status_(&ipsec_sa_info->status);
    return SDK_RET_OK;
}

//
// ipsec_sa_decrypt_impl class
//
sdk_ret_t
ipsec_sa_decrypt_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                                    api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.ipsec_sa_decrypt.status.hw_id = hw_id_;
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_decrypt_impl::program_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    pds_ipsec_sa_decrypt_spec_t *spec;
    nexthop_info_entry_t nexthop_info_entry;
    sdk_ret_t ret;

    // program the nexthop
    PDS_TRACE_DEBUG("Programming decrypt ipsec nh %u", nh_idx_);
    memset(&nexthop_info_entry, 0, nexthop_info_entry.entry_size());
    nexthop_info_entry.lif = APULU_IPSEC_LIF;
    nexthop_info_entry.port = TM_PORT_DMA;
    nexthop_info_entry.app_id = P4PLUS_APPTYPE_IPSEC;
    nexthop_info_entry.qtype = IPSEC_DECRYPT_QTYPE;
    nexthop_info_entry.qid = hw_id_;
    ret = nexthop_info_entry.write(nh_idx_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program decrypt ipsec nh idx %u err %u",
                      nh_idx_, ret);
        return ret;
    }

    spec = &obj_ctxt->api_params->ipsec_sa_decrypt_spec;

    ipseccb_decrypt_create(hw_id_, base_pa_, spec);
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_decrypt_impl::activate_create_(pds_epoch_t epoch,
                                        ipsec_sa_entry *ipsec_sa,
                                        pds_ipsec_sa_decrypt_spec_t *spec) {
    return SDK_RET_OK;
}

sdk_ret_t
ipsec_sa_decrypt_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                                   pds_epoch_t epoch, api_op_t api_op,
                                   api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_ipsec_sa_decrypt_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->ipsec_sa_decrypt_spec;
        ret = activate_create_(epoch, (ipsec_sa_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (ipsec_sa_entry *)api_obj);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

void
ipsec_sa_decrypt_impl::fill_status_(pds_ipsec_sa_decrypt_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
ipsec_sa_decrypt_impl::fill_stats_(pds_ipsec_sa_decrypt_stats_t *stats) {
    return SDK_RET_ERR;
}

sdk_ret_t
ipsec_sa_decrypt_impl::read_hw(api_base *api_obj, obj_key_t *key,
                               obj_info_t *info) {
    sdk_ret_t rv;
    pds_ipsec_sa_decrypt_info_t *ipsec_sa_info = (pds_ipsec_sa_decrypt_info_t *)info;

    rv = fill_stats_(&ipsec_sa_info->stats);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read hardware stats tables for ipsec sa %s",
                      api_obj->key2str().c_str());
        return rv;
    }
    fill_status_(&ipsec_sa_info->status);
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
