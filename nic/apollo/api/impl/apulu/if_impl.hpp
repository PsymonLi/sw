//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// interface implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __IF_IMPL_HPP__
#define __IF_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_IF_IMPL - interface datapath functionality
/// \ingroup PDS_IF
/// @{

/// \brief  interface implementation
class if_impl : public impl_base {
public:
    /// \brief      factory method to allocate & initialize interface
    ///             impl instance
    /// \param[in]  spec    interface information
    /// \return     new instance of interface or NULL, in case of error
    static if_impl *factory(pds_if_spec_t *spec);

    /// \brief      release all the state associated with the given interface,
    ///             if any, and free the memory
    /// \param[in]  impl    interface impl instance to be freed
    // NOTE: h/w entries should have been cleaned up (by calling
    //       impl->cleanup_hw() before calling this
    static void destroy(if_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(if_impl *impl);

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

    /// \brief      program all h/w tables relevant to this object except
    ///             stage 0 table(s), if any
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief      cleanup all h/w tables relevant to this object except
    ///             stage 0 table(s), if any, by updating packed entries with
    ///             latest epoch#
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

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
        return SDK_RET_OK;
    }

    /// \brief      update all h/w tables relevant to this object except stage 0
    ///             table(s), if any, by updating packed entries with latest
    ///             epoch#
    /// \param[in]  orig_obj old version of the unmodified object
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief      activate the epoch in the dataplane by programming stage 0
    ///             tables, if any
    /// \param[in]  api_obj  (cloned) API api object being activated
    /// \param[in]  orig_obj previous/original unmodified object
    /// \param[in]  epoch    epoch being activated
    /// \param[in]  api_op   api operation
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

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
                                    api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to interface key
    /// \param[out] info pointer to interface info
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief      return h/w id of this interface
    /// \return     h/w id corresponding to this interface
    uint16_t hw_id(void) const { return hw_id_; }

    /// \brief      return h/w port corresponding to this interface
    /// \param[in]  intf    interface entry pointer
    static uint32_t port(if_entry *intf);

private:
    /// \brief  constructor
    if_impl() {
        hw_id_ = 0xFFFF;
    }

    /// \brief  destructor
    ~if_impl() {}

    /// \brief     program l3 interface related attributes in the datapath
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_l3_if_(if_entry *intf, pds_if_spec_t *spec);

    /// \brief     configure ip address on control interface (ctrl0)
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_l3_if_(std::string, if_entry *intf, pds_if_spec_t *spec);

    /// \brief     remove ip address on mgmt interface (ctrl0)
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t deactivate_l3_if_(std::string, if_entry *intf);

    /// \brief     configure ip address on mgmt interface (ctrl0)
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_control_if_(if_entry *intf, pds_if_spec_t *spec);

    /// \brief     activate host interface
    /// \param[in] intf  host interface obj being programmed
    /// \param[in] orig_intf original interface obj being updated
    /// \param[in] obj_ctxt transient state associated with this API
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_host_if_(if_entry *intf, if_entry *orig_intf,
                                pds_if_spec_t *spec, api_obj_ctxt_t *obj_ctxt);

    /// \brief     remove ip address on control interface (ctrl0)
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t deactivate_control_if_(if_entry *intf);

    /// \brief     program interface related tables during interface create by
    ///            enabling stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] intf  interface obj being programmed
    /// \param[in] spec  interface configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, if_entry *intf,
                               pds_if_spec_t *spec);

    /// \brief      program subnet related tables during subnet update by
    ///             enabling stage0 tables corresponding to the new epoch
    /// \param[in]  epoch epoch being activated
    /// \param[in]  intf cloned interface obj being programmed
    /// \param[in]  orig_intf original interface obj being updated
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_update_(pds_epoch_t epoch, if_entry *intf,
                               if_entry *orig_intf, api_obj_ctxt_t *obj_ctxt);

    /// \brief     program interface related tables during interface delete by
    ///            disabling stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] intf interface obj being deleted
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_delete_(pds_epoch_t epoch, if_entry *intf);

private:
    uint16_t    hw_id_;     ///< h/w id of this interface
};

/// @}

}    // namespace impl
}    // namespace api

#endif    // __IF_IMPL_HPP__
