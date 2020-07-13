//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// device implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __DEVICE_IMPL_HPP__
#define __DEVICE_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_device.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_DEVICE_IMPL - device functionality
/// \ingroup PDS_DEVICE
/// @{

/// \brief device implementation
class device_impl : public impl_base {
public:
    /// \brief     factory method to allocate & initialize device impl
    ///            instance
    /// \param[in] spec specification
    /// \return    new instance of device or NULL, in case of error
    static device_impl *factory(pds_device_spec_t *spec);

    /// \brief     release all the s/w state associated with the given device,
    ///            if any, and free the memory
    /// \param[in] impl device to be freed
    /// \NOTE      h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this)
    static void destroy(device_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(device_impl *impl);

    /// \brief     stash this object into persistent storage
    /// \param[in] info pointer to the info object
    /// \param[in] upg_info contains location to put stashed object
    /// \return    sdk_ret_ok or error code
    virtual sdk_ret_t backup(obj_info_t *info, upg_obj_info_t *upg_info) override;

    /// \brief     restore stashed object from persistent storage
    /// \param[in] info pointer to the info object
    /// \param[in] upg_info contains location to read stashed object
    /// \return    sdk_ret_ok or error code
    virtual sdk_ret_t restore(obj_info_t *info, upg_obj_info_t *upg_info) override;

    /// \brief     program all h/w tables relevant to this object except
    ///            stage 0 table(s), if any
    /// \param[in] api_obj  api object holding the resources
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief     cleanup all h/w tables relevant to this object except
    ///            stage 0 table(s), if any, by updating packed entries
    ///            with latest epoch#
    /// \param[in] api_obj  api object holding the resources
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief     update all h/w tables relevant to this object except
    ///            stage 0 table(s), if any, by updating packed entries
    ///            with latest epoch#
    /// \param[in] curr_obj current version of the object
    /// \param[in] prev_obj previous version of the object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief     activate the epoch in the dataplane by programming
    ///            stage 0 tables, if any
    /// \param[in] api_obj  (cloned) API api object being activated
    /// \param[in] orig_obj previous/original unmodified object
    /// \param[in] epoch    epoch being activated
    /// \param[in] api_op   api operation
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

     /// \brief      read spec, statistics and status from hw tables
     /// \param[in]  api_obj API object
     /// \param[in]  key not used
     /// \param[out] info pointer to device info
     /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief      reset all the statistics associated with the device
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reset_stats(void) override;

private:
    /// \brief constructor
    device_impl() {}

    /// \brief destructor
    ~device_impl() {}

    /// \brief      activate h/w tables for this obj from persistent storage
    /// \param[in]  info pointer to the info object
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_restore_(obj_info_t *info);

    /// \brief      populate specification with hardware information
    /// \param[out] spec specification
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_spec_(pds_device_spec_t *spec);

    /// \brief      populate operational status of device object
    /// \param[out] status operational status
    void fill_status_(pds_device_status_t *spec);

    /// \brief      populate ingress drop stats with hardware information
    /// \param[out] ing_drop_stats Ingress drop statistics
    /// \return     Number of stats entries
    uint32_t fill_ing_drop_stats_(pds_device_drop_stats_t *ing_drop_stats);

    /// \brief      populate egress drop stats with hardware information
    /// \param[out] egr_drop_stats Egress drop statistics
    /// \return     Number of stats entries
    uint32_t fill_egr_drop_stats_(pds_device_drop_stats_t *egr_drop_stats);

    /// \brief      clear ingress drop stats
    void clear_ing_drop_stats_(void);

    /// \brief      clear egress drop stats
    void clear_egr_drop_stats_(void);
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __DEVICE_IMPL_HPP__
