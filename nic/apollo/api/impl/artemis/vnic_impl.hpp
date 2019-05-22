//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vnic implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __VNIC_IMPL_HPP__
#define __VNIC_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/vpc.hpp"
#include "nic/apollo/api/subnet.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/route.hpp"
#include "nic/apollo/api/policy.hpp"
#include "gen/p4gen/artemis/include/p4pd.h"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_VNIC_IMPL - vnic functionality
/// \ingroup PDS_VNIC
/// @{

/// \brief  VNIC implementation
class vnic_impl : public impl_base {
public:
    /// \brief      factory method to allocate & initialize vnic impl instance
    /// \param[in]  pds_vnic vnic information
    /// \return     new instance of vnic or NULL, in case of error
    static vnic_impl *factory(pds_vnic_spec_t *pds_vnic);

    /// \brief      release all the s/w state associated with the given vnic,
    ///             if any, and free the memory
    /// \param[in]  impl vnic impl instance to be freed
    // NOTE: h/w entries should have been cleaned up (by calling
    //       impl->cleanup_hw() before calling this
    static void destroy(vnic_impl *impl);

    /// \brief      allocate/reserve h/w resources for this object
    /// \param[in]  orig_obj old version of the unmodified object
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        obj_ctxt_t *obj_ctxt) override;

    /// \brief  free h/w resources used by this object, if any
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /// \brief      free h/w resources used by this object, if any
    ///             (this API is invoked during object deletes)
    /// \param[in]  api_obj    api object holding the resources
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t nuke_resources(api_base *api_obj) override;

    /// \brief      program all h/w tables relevant to this object except
    ///             stage 0 table(s), if any
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /// \brief      re-program all hardware tables relevant to this object
    ///             except stage 0 table(s), if any and this reprogramming
    ///             must be based on existing state and any of the state
    ///             present in the dirty object list (like clone objects etc.)
    /// \param[in]  api_obj API object being activated
    /// \param[in]  api_op API operation
    /// \return     #SDK_RET_OK on success, failure status code on error
    // NOTE: this method is called when an object is in the dependent/puppet
    //       object list
    virtual sdk_ret_t reprogram_hw(api_base *api_obj, api_op_t api_op) override;

    /// \brief      cleanup all h/w tables relevant to this object except
    ///             stage 0 table(s), if any, by updating packed entries with
    ///             latest epoch#
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /// \brief      update all h/w tables relevant to this object except stage 0
    ///             table(s), if any, by updating packed entries with latest
    ///             epoch#
    /// \param[in]  orig_obj old version of the unmodified object
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                obj_ctxt_t *obj_ctxt) override;

    /// \brief      activate the epoch in the dataplane by programming stage 0
    ///             tables, if any
    /// \param[in]  epoch    epoch being activated
    /// \param[in]  api_op   api operation
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj,
                                  pds_epoch_t epoch,
                                  api_op_t api_op,
                                  obj_ctxt_t *obj_ctxt) override;

    /// \brief      re-activate config in the hardware stage 0 tables relevant
    ///             to this object, if any, this reactivation must be based on
    ///             existing state and any of the state present in the dirty
    ///             object list (like clone objects etc.) only and not directly
    ///             on db objects
    // \param[in]   api_obj API object being activated
    /// \param[in]  api_op API operation
    /// \return     #SDK_RET_OK on success, failure status code on error
    // NOTE: this method is called when an object is in the dependent/puppet
    //       object list
    virtual sdk_ret_t reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                                    api_op_t api_op) override;

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  key  pointer to vnic key
    /// \param[out] info pointer to vnic info
    /// \param[in]  arg  Not used
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(obj_key_t *key, obj_info_t *info,
                              void *arg = NULL) override;

    uint16_t hw_id(void) { return hw_id_; }

private:
    /// \brief  constructor
    vnic_impl() {
        hw_id_ = 0xFFFF;
        vnic_by_slot_hash_idx_ = 0xFFFF;
    }

    /// \brief  destructor
    ~vnic_impl() {}

    /// \brief      program VNIC_MAPPING table and activate the config
    /// \param[in]  api_op          api operation
    /// \param[in]  api_obj         vnic entry object
    /// \param[in]  epoch           epoch being activated
    /// \param[in]  vpc             vpc entry
    /// \param[in]  subnet          subnet entry
    /// \param[in]  spec            vnic configuration
    /// \param[in]  v4_route_table  IPv4 routing table entry
    /// \param[in]  v6_route_table  IPv6 routing table entry
    /// \param[in]  v4_policy       egress IPv4 security policy
    /// \param[in]  v6_policy       egress IPv6 security policy
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_vnic_mapping_create_(pds_epoch_t epoch,
                                            vpc_entry *vpc,
                                            pds_vnic_spec_t *spec,
                                            vnic_entry *vnic,
                                            route_table *v4_route_table,
                                            route_table *v6_route_table,
                                            policy *v4_policy,
                                            policy *v6_policy);

    /// \brief      program vnic related tables during vnic create by enabling
    ///             stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  vnic  vnic obj being programmed
    /// \param[in]  spec  vnic configuration
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_vnic_create_(pds_epoch_t epoch, vnic_entry *vnic,
                                    pds_vnic_spec_t *spec);

    /// \brief      program vnic related tables during vnic delete by disabling
    ///             stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  vnic  vnic obj being programmed
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_vnic_delete_(pds_epoch_t epoch, vnic_entry *vnic);

private:
    uint16_t    hw_id_;                 ///< hardware id
    handle_t    vnic_mapping_handle_;
};

/// @}

}    // namespace impl
}    // namespace api

#endif    // __VNIC_IMPL_HPP__
