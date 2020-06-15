//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __VPORT_IMPL_HPP__
#define __VPORT_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_vport.hpp"
#include "gen/p4gen/apulu/include/p4pd.h"
#include "gen/p4gen/p4plus_rxdma/include/p4plus_rxdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_VPORT_IMPL - vport functionality
/// \ingroup PDS_VPORT
/// @{

// initial epoch value when vport is created
#define PDS_IMPL_VPORT_EPOCH_START          0x0

/// \brief vport implementation
class vport_impl : public impl_base {
public:
    /// \brief     factory method to allocate & initialize vport impl instance
    /// \param[in] spec vport specification
    /// \return    new instance of vport or NULL, in case of error
    static vport_impl *factory(pds_vport_spec_t *spec);

    /// \brief     release all the s/w state associated with the given vport,
    ///            if any, and free the memory
    /// \param[in] impl vport impl instance to be freed
    /// \NOTE      h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this)
    static void destroy(vport_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(vport_impl *impl);

    /// \brief     allocate/reserve h/w resources for this object
    /// \param[in] api_obj  object for which resources are being reserved
    /// \param[in] orig_obj old version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *api_obj, api_base *orig_obj,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     free h/w resources used by this object, if any
    /// \param[in] api_obj API object holding this resource
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /// \brief     free h/w resources used by this object, if any
    ///            (this API is invoked during object deletes)
    /// \param[in] api_obj API object holding the resources
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t nuke_resources(api_base *api_obj) override;

    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] api_obj     api object associated with the impl instance
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg, api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     program all h/w tables relevant to this object except
    ///            stage 0 table(s), if any
    /// \param[in] api_obj API object holding this resource
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     cleanup all h/w tables relevant to this object except stage 0
    ///            table(s), if any, by updating packed entries with latest
    ///            epoch#
    /// \param[in] api_obj  API object holding this resource
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     update all h/w tables relevant to this object except stage 0
    ///            table(s), if any, by updating packed entries with latest
    ///            epoch#
    /// \param[in] curr_obj current version of the unmodified object
    /// \param[in] prev_obj previous version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     activate the epoch in the dataplane by programming stage 0
    ///            tables, if any
    /// \param[in] api_obj  (cloned) API object being activated
    /// \param[in] orig_obj previous/original unmodified object
    /// \param[in] epoch    epoch being activated
    /// \param[in] api_op   api operation
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     re-program config in the hardware
    ///            re-program all hardware tables relevant to this object
    ///            except stage 0 table(s), if any and this reprogramming must
    ///            be based on existing state and any of the state present in
    ///            the dirty object list (like clone objects etc.)
    /// \param[in] api_obj API object being activated
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    #SDK_RET_OK on success, failure status code on error
    /// \NOTE      this method is called when an object is in the
    ///            dependent/puppet object list
    virtual sdk_ret_t reprogram_hw(api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     re-activate config in the hardware stage 0 tables relevant to
    ///            this object, if any, this reactivation must be based on
    ///            existing state and any of the state present in the dirty
    ///            object list
    ///            (like cloned objects etc.) only and not directly on db
    ///            objects
    /// \param[in]  api_obj  (cloned) API api object being activated
    /// \param[in]  epoch    epoch being activated
    /// \param[in]  obj_ctxt transient state associated with this API
    /// \return    #SDK_RET_OK on success, failure status code on error
    /// \NOTE      this method is called when an object is in the
    ///            dependent/puppet object list
    virtual sdk_ret_t reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                                    api_obj_ctxt_t *obj_ctxt) override {
        // no tables programmed in activation stage will be affected because of
        // updates to other objects like route tables, policy tables etc.
        return SDK_RET_OK;
    }

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to vport key
    /// \param[out] info pointer to vport info
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief     return vport's current epoch
    /// \return    epoch of the vport
    uint8_t epoch(void) const { return epoch_; }

    /// \brief     return vport's h/w id
    /// \return    h/w id assigned to the vport
    uint16_t hw_id(void) const { return hw_id_; }

    /// \brief     return nexthop idx allocated for this vport
    /// \return    nexthop index corresponding to this vport
    uint16_t nh_idx(void) const { return nh_idx_; }

    /// \brief      get the key from entry in hash table context
    /// \param[in]  entry in the hash table context
    /// \return     hw id from the entry
    static void *key_get(void *entry) {
        vport_impl *vport = (vport_impl *)entry;
        return (void *)&(vport->hw_id_);
    }

    /// \brief      accessor API for hash table context
    ht_ctxt_t *ht_ctxt(void) { return &ht_ctxt_; }

private:
    /// \brief constructor
    vport_impl() {
        epoch_ = PDS_IMPL_VPORT_EPOCH_START;
        hw_id_ = 0xFFFF;
        nh_idx_ = 0xFFFF;
        ht_ctxt_.reset();
    }

    /// \brief  constructor with spec
    vport_impl(pds_vport_spec_t *spec) {
        vport_impl();
    }

    /// \brief destructor
    ~vport_impl() {}

    /// \brief     add an entry to VLAN table
    /// \param[in] epoch epoch being activated
    /// \param[in] vport  vport obj being programmed
    /// \param[in] spec  vport configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_vlan_entry_(pds_epoch_t epoch, vport_entry *vport,
                              pds_vport_spec_t *spec);

    /// \brief     program vport related tables during vport create by enabling
    ///            stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vport  vport obj being programmed
    /// \param[in] spec  vport configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, vport_entry *vport,
                               pds_vport_spec_t *spec);

    /// \brief     program vport related tables during vport delete by disabling
    ///            stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vport  vport obj being programmed
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_delete_(pds_epoch_t epoch, vport_entry *vport);

    /// \brief      program vport related tables during vport update by
    ///             enabling stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vport  vport obj being programmed
    /// \param[in] orig_vport  original vport obj that is being modified
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_update_(pds_epoch_t epoch, vport_entry *vport,
                               vport_entry *orig_vport,
                               api_obj_ctxt_t *obj_ctxt);

    /// \brief      fill the vport stats
    /// \param[out] stats statistics
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_stats_(pds_vport_stats_t *stats);

    /// \brief      fill the vport status
    /// \param[out] status status
    void fill_status_(pds_vport_status_t *status);

    /// \brief      retrieve vport stats from vpp, if any
    /// \param[out] stats statistics
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_vpp_stats_(pds_vport_stats_t *stats);

private:
    // P4 datapath specific state
    uint8_t epoch_;      ///< datapath epoch of the vport
    uint16_t hw_id_;     ///< hardware id
    uint16_t nh_idx_;    ///< nexthop table index for this vport
    ht_ctxt_t ht_ctxt_;  ///< hash table ctxt for this entry
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __VPORT_IMPL_HPP__
