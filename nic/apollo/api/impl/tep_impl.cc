/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    tep_impl.cc
 *
 * @brief   datapath implementation of tep
 */

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/tep.hpp"
#include "nic/apollo/api/impl/tep_impl.hpp"
#include "nic/apollo/api/impl/oci_impl_state.hpp"
#include "gen/p4gen/apollo/include/p4pd.h"
#include "nic/apollo/p4/include/defines.h"
#include "nic/sdk/lib/p4/p4_api.hpp"

namespace api {
namespace impl {

/**
 * @defgroup OCI_TEP_IMPL - tep entry datapath implementation
 * @ingroup OCI_TEP
 * @{
 */

/**
 * @brief    factory method to allocate & initialize tep impl instance
 * @param[in] oci_tep    tep information
 * @return    new instance of tep or NULL, in case of error
 */
tep_impl *
tep_impl::factory(oci_tep_spec_t *oci_tep) {
    tep_impl *impl;

    // TODO: move to slab later
    impl = (tep_impl *)SDK_CALLOC(SDK_MEM_ALLOC_OCI_TEP_IMPL,
                                  sizeof(tep_impl));
    new (impl) tep_impl();
    return impl;
}

/**
 * @brief    release all the s/w state associated with the given tep,
 *           if any, and free the memory
 * @param[in] tep     tep to be freed
 * NOTE: h/w entries should have been cleaned up (by calling
 *       impl->cleanup_hw() before calling this
 */
void
tep_impl::destroy(tep_impl *impl) {
    impl->~tep_impl();
    SDK_FREE(SDK_MEM_ALLOC_OCI_TEP_IMPL, impl);
}

/**
 * @brief    allocate/reserve h/w resources for this object
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
tep_impl::reserve_resources(api_base *api_obj) {
    // TODO: if directmap provides a way to reserve() we dont need this indexer
    //       at all !!
    if (tep_impl_db()->tep_idxr()->alloc((uint32_t *)&hw_id_) !=
            sdk::lib::indexer::SUCCESS) {
        return sdk::SDK_RET_NO_RESOURCE;
    }
    return SDK_RET_OK;
}

/**
 * @brief     free h/w resources used by this object, if any
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
tep_impl::release_resources(api_base *api_obj) {
    if (hw_id_ != 0xFF) {
        tep_impl_db()->tep_idxr()->free(hw_id_);
        tep_impl_db()->tep_tx_tbl()->remove(hw_id_);
    }
    return sdk::SDK_RET_INVALID_OP;
}

/**
 * @brief    program all h/w tables relevant to this object except stage 0
 *           table(s), if any
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
#define tep_tx_mpls_udp_action    action_u.tep_tx_mpls_udp_tep_tx
sdk_ret_t
tep_impl::program_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t                  ret;
    oci_tep_spec_t             *tep_spec;
    tep_tx_actiondata_t        tep_tx_data = { 0 };
    nexthop_tx_actiondata_t    nh_tx_data = { 0 };

    tep_spec = &obj_ctxt->api_params->tep_spec;
    tep_tx_data.action_id = TEP_TX_MPLS_UDP_TEP_TX_ID;
    tep_tx_data.tep_tx_mpls_udp_action.dipo = tep_spec->key.ip_addr;

    // TODO: fix this when fte plugin is available
    MAC_UINT64_TO_ADDR(tep_tx_data.tep_tx_mpls_udp_action.dmac, 0x0E0D0A0B0200);
    ret = tep_impl_db()->tep_tx_tbl()->insert_withid(&tep_tx_data, hw_id_);
    SDK_ASSERT(ret == SDK_RET_OK);

    nh_tx_data.action_id = NEXTHOP_TX_NEXTHOP_INFO_ID;
    nh_tx_data.action_u.nexthop_tx_nexthop_info.tep_index = hw_id_;
    if (tep_spec->type == OCI_ENCAP_TYPE_GW_ENCAP) {
        nh_tx_data.action_u.nexthop_tx_nexthop_info.encap_type = GW_ENCAP;
    } else if (tep_spec->type == OCI_ENCAP_TYPE_VNIC) {
        nh_tx_data.action_u.nexthop_tx_nexthop_info.encap_type = VNIC_ENCAP;
    }
    // TODO: fix this once p4/asm is fixed
    //nh_tx_data.action_u.nexthop_tx_nexthop_info.dst_slot_id = 0xAB;
    ret = tep_impl_db()->nh_tx_tbl()->insert(&nh_tx_data, (uint32_t *)&nh_id_);
    SDK_ASSERT(ret == SDK_RET_OK);

    OCI_TRACE_DEBUG("Programmed TEP %s, MAC 0x%lx, hw id %u, nh id",
                    ipv4addr2str(tep_spec->key.ip_addr),
                    0x0E0D0A0B0200, hw_id_, nh_id_);
    return ret;
}

/**
 * @brief    cleanup all h/w tables relevant to this object except stage 0
 *           table(s), if any, by updating packed entries with latest epoch#
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
tep_impl::cleanup_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    //TODO: need to unprogram HW.
    return sdk::SDK_RET_INVALID_OP;
}

/**
 * @brief    update all h/w tables relevant to this object except stage 0
 *           table(s), if any, by updating packed entries with latest epoch#
 * @param[in] orig_obj    old version of the unmodified object
 * @param[in] curr_obj    cloned and updated version of the object
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
tep_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                    obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

/** @} */    // end of OCI_TEP_IMPL

}    // namespace impl
}    // namespace api
