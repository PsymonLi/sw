//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// subnet implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __SUBNET_IMPL_HPP__
#define __SUBNET_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/api/subnet.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/vpc_impl.hpp"
#include "gen/p4gen/apulu/include/p4pd.h"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_SUBNET_IMPL - subnet datapath functionality
/// \ingroup PDS_SUBNET
/// @{

/// \brief  subnet implementation
class subnet_impl : public impl_base {
public:
    /// \brief      factory method to allocate & initialize subnet impl instance
    /// \param[in]  spec    subnet information
    /// \return     new instance of subnet or NULL, in case of error
    static subnet_impl *factory(pds_subnet_spec_t *spec);

    /// \brief      release all the state associated with the given subnet,
    ///             if any, and free the memory
    /// \param[in]  impl    subnet impl instance to be freed
    // NOTE: h/w entries should have been cleaned up (by calling
    //       impl->cleanup_hw() before calling this
    static void destroy(subnet_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(subnet_impl *impl);

    /// \brief      allocate/reserve h/w resources for this object
    /// \param[in]  api_obj  object for which resources are being reserved
    /// \param[in]  orig_obj old version of the unmodified object
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *api_obj, api_base *orig_obj,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief  free h/w resources used by this object, if any
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /// \brief      free h/w resources used by this object, if any
    ///             (this API is invoked during object deletes)
    /// \param[in]  api_obj    api object holding the resources
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t nuke_resources(api_base *api_obj) override;

    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] api_obj     api object associated with the impl instance
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg, api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      program all h/w tables relevant to this object except
    ///             stage 0 table(s), if any
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      cleanup all h/w tables relevant to this object except
    ///             stage 0 table(s), if any, by updating packed entries with
    ///             latest epoch#
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      update all h/w tables relevant to this object except stage 0
    ///             table(s), if any, by updating packed entries with latest
    ///             epoch#
    /// \param[in]  orig_obj old version of the unmodified object
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      activate the epoch in the dataplane by programming stage 0
    ///             tables, if any
    /// \param[in]  api_obj  (cloned) API object being activated
    /// \param[in]  orig_obj previous/original unmodified object
    /// \param[in]  epoch    epoch being activated
    /// \param[in]  api_op   api operation
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      re-program all hardware tables relevant to this object
    ///             except stage 0 table(s), if any and this reprogramming
    ///             must be based on existing state and any of the state
    ///             present in the dirty object list (like clone objects etc.)
    /// \param[in]  api_obj API object being activated
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    // NOTE: this method is called when an object is in the dependent/puppet
    //       object list
    virtual sdk_ret_t reprogram_hw(api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override {
        // other object updates don't affect subnet h/w programming when subnet
        // is sitting in the dependent object list, any updates to route
        // table(s) and/or policy table(s) that subnet is pointing to are
        // propagated to  respective vnics by now
        return SDK_RET_OK;
    }

    /// \brief      re-activate config in the hardware stage 0 tables relevant
    ///             to this object, if any, this reactivation must be based on
    ///             existing state and any of the state present in the dirty
    ///             object list (like clone objects etc.) only and not directly
    ///             on db objects
    /// \param[in]  api_obj  (cloned) API api object being activated
    /// \param[in]  epoch    epoch being activated
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    /// NOTE: this method is called when an object is in the dependent/puppet
    ///       object list
    virtual sdk_ret_t reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                                    api_obj_ctxt_t *obj_ctxt);

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to subnet key
    /// \param[out] info pointer to subnet info
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief     return subnet's h/w id
    /// \return    h/w id assigned to the subnet
    uint16_t hw_id(void) { return hw_id_; }

    /// \brief      fill the subnet spec
    /// \param[out] spec specification
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_spec_(pds_subnet_spec_t *spec);

    /// \brief      fill the subnet status
    /// \param[out] status status
    void fill_status_(pds_subnet_status_t *status);

    /// \brief      get the key from entry in hash table context
    /// \param[in]  entry in the hash table context
    /// \return     hw id from the entry
    static void *key_get(void *entry) {
        subnet_impl *subnet = (subnet_impl *)entry;
        return (void *)&(subnet->hw_id_);
    }

    /// \brief      accessor API for key
    /// \return     pointer to the subnet object's key
    pds_obj_key_t *key(void) { return &key_; }

    /// \brief      accessor API for hash table context
    ht_ctxt_t *ht_ctxt(void) { return &ht_ctxt_; }

private:
    /// \brief  constructor
    subnet_impl() {
        hw_id_ = 0xFFFF;
        vni_hdl_ = handle_t::null();
        v4_vr_ip_mapping_hdl_ = handle_t::null();
        v6_vr_ip_mapping_hdl_ = handle_t::null();
        ht_ctxt_.reset();
    }

    /// \brief  constructor with spec
    subnet_impl(pds_subnet_spec_t *spec) {
        subnet_impl();
        key_ = spec->key;
    }

    /// \brief  destructor
    ~subnet_impl() {}

    /// \brief    given a VxLAN vnid, reserve an entry in the VNI table
    /// \param[in] spec     subnet configuration
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t reserve_vni_entry_(pds_subnet_spec_t *spec);

    ///\ brief    reserve resources needed for given VR IP of the subnet
    /// \param[in] vpc      VPC impl instance of this subnet
    /// \param[in] vr_ip    IPv4/IPv6 VR IP of this subnet
    /// \param[in] spec     subnet configuration
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t reserve_vr_ip_resources_(vpc_impl *vpc, ip_addr_t *vr_ip,
                                       pds_subnet_spec_t *spec);

    /// \brief reserve the mapping table resources needed for VR IP of subnet
    /// \param[in]  bd_hw_id h/w id of the subnet
    /// \param[in]  spec    subnet configuration
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_vr_ip_mapping_entries_(uint16_t bd_hw_id,
                                         pds_subnet_spec_t *spec);

    /// \brief      program subnet related tables during subnet create by
    ///             enabling stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  subnet    subnet obj being programmed
    /// \param[in]  spec    subnet configuration
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, subnet_entry *subnet,
                               pds_subnet_spec_t *spec);

    /// \brief      program subnet related tables during subnet update by
    ///             enabling stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  subnet cloned subnet obj being programmed
    /// \param[in]  orig_subnet original/unmodified subnet obj
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_update_(pds_epoch_t epoch, subnet_entry *subnet,
                               subnet_entry *orig_subnet,
                               api_obj_ctxt_t *obj_ctxt);

    /// \brief      program subnet related tables during subnet delete by
    ///             disabling stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  subnet    subnet obj being programmed
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_delete_(pds_epoch_t epoch, subnet_entry *subnet);

private:
    uint16_t hw_id_;
    handle_t vni_hdl_;
    ///< handles in MAPPING table for IPv4 and IPv6 VR IPs
    handle_t v4_vr_ip_mapping_hdl_;
    handle_t v6_vr_ip_mapping_hdl_;
    /// PI specific info
    struct {
        pds_obj_key_t key_;
    };
    ht_ctxt_t ht_ctxt_;
};

/// @}

}    // namespace impl
}    // namespace api

#endif    // __SUBNET_IMPL_HPP__
