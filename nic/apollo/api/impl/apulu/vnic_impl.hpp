//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
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
#include "gen/p4gen/apulu/include/p4pd.h"
#include "gen/p4gen/p4plus_rxdma/include/p4plus_rxdma_p4pd.h"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_VNIC_IMPL - vnic functionality
/// \ingroup PDS_VNIC
/// @{

// initial epoch value when vnic is created
#define PDS_IMPL_VNIC_EPOCH_START          0x0

/// \brief vnic implementation
class vnic_impl : public impl_base {
public:
    /// \brief     factory method to allocate & initialize vnic impl instance
    /// \param[in] spec vnic specification
    /// \return    new instance of vnic or NULL, in case of error
    static vnic_impl *factory(pds_vnic_spec_t *spec);

    /// \brief     release all the s/w state associated with the given VNIC,
    ///            if any, and free the memory
    /// \param[in] impl vnic impl instance to be freed
    /// \NOTE      h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this)
    static void destroy(vnic_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(vnic_impl *impl);

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
                                 api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

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
                                    api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to vnic key
    /// \param[out] info pointer to vnic info
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief      reset all the statistics associated with the vnic
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reset_stats(void) override;

    /// \brief     return vnic's current epoch
    /// \return    epoch of the vnic
    uint8_t epoch(void) const { return epoch_; }

    /// \brief     return vnic's h/w id
    /// \return    h/w id assigned to the vnic
    uint16_t hw_id(void) const { return hw_id_; }

    /// \brief     return nexthop idx allocated for this vnic
    /// \return    nexthop index corresponding to this vnic
    uint16_t nh_idx(void) const { return nh_idx_; }

    /// \brief     return vnic's h/w id in IP_MAC_BINDING table table
    /// \return    h/w id assigned to the vnic in IP_MAC_BINDING table
    uint32_t binding_hw_id(void) const { return binding_hw_id_; }

    /// \brief      get the key from entry in hash table context
    /// \param[in]  entry in the hash table context
    /// \return     hw id from the entry
    static void *key_get(void *entry) {
        vnic_impl *vnic = (vnic_impl *) entry;
        return (void *)&(vnic->hw_id_);
    }

    /// \brief      accessor API for key
    pds_obj_key_t *key(void) { return &key_; }

    /// \brief      accessor API for hash table context
    ht_ctxt_t *ht_ctxt(void) { return &ht_ctxt_; }

private:
    /// \brief constructor
    vnic_impl() {
        epoch_ = PDS_IMPL_VNIC_EPOCH_START;
        hw_id_ = 0xFFFF;
        nh_idx_ = 0xFFFF;
        lif_vlan_hdl_ = handle_t::null();
        local_mapping_hdl_ = handle_t::null();
        mapping_hdl_ = handle_t::null();
        binding_hw_id_ = PDS_IMPL_RSVD_IP_MAC_BINDING_HW_ID;
        ht_ctxt_.reset();
    }

    /// \brief  constructor with spec
    vnic_impl(pds_vnic_spec_t *spec) {
        vnic_impl();
        key_ = spec->key;
    }

    /// \brief destructor
    ~vnic_impl() {}

    /// \brief      populate rxdma vnic info table entry's policy tree root
    ///             address
    /// \param[in]  vnic_info_data    vnic info data to be programmed
    /// \param[in]  idx               policy LPM root entry index
    /// \param[in]  addr              policy tree root address
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t populate_rxdma_vnic_info_policy_root_(
                  vnic_info_rxdma_actiondata_t *vnic_info_data,
                  uint32_t idx, mem_addr_t addr);

    /// \brief      program vnic info tables in rxdma and txdma
    /// \param[in]  vnic      vnic being created/updated
    /// \param[in]  vpc       vpc of the vnic
    /// \param[in]  subnet    subnet of the vnic
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_rxdma_vnic_info_(vnic_entry *vnic, vpc_entry *vpc,
                                       subnet_entry *subnet);

    /// \brief      program nexthop correponding to the vnic
    /// \param[in]  oper_mode DSC operational mode
    /// \param[in]  spec      configuration spec of vnic
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_vnic_nh_(pds_device_oper_mode_t oper_mode,
                               pds_vnic_spec_t *spec,
                               nexthop_info_entry_t *nh_data);

    /// \brief      compute the Rx and Tx mirror session bitmaps
    /// \param[in]   spec    configuration spec of vnic
    /// \param[out]  rx_bmap Rx mirror session btmap
    /// \param[out]  tx_bmap Tx mirror session bitmap
    void compute_mirror_session_bmap_(pds_vnic_spec_t *spec,
                                      uint8_t *rx_bmap, uint8_t *tx_bmap);

    /// \brief     add an entry to LOCAL_MAPPING table
    /// \param[in] epoch epoch being activated
    /// \param[in] vpc vpc of this vnic
    /// \param[in] subnet subnet of this vnic
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] spec  vnic configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_local_mapping_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                                       subnet_entry *subnet, vnic_entry *vnic,
                                       pds_vnic_spec_t *spec);

    /// \brief     add an entry to VLAN table
    /// \param[in] epoch epoch being activated
    /// \param[in] vpc vpc of this vnic
    /// \param[in] subnet subnet of this vnic
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] spec  vnic configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_lif_vlan_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                                  subnet_entry *subnet, vnic_entry *vnic,
                                  pds_vnic_spec_t *spec);

    /// \brief     add an entry to MAPPING table
    /// \param[in] epoch epoch being activated
    /// \param[in] vpc vpc of this vnic
    /// \param[in] subnet subnet of this vnic
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] spec  vnic configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_mapping_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                                 subnet_entry *subnet, vnic_entry *vnic,
                                 pds_vnic_spec_t *spec);

    /// \brief     program vnic related tables during vnic create by enabling
    ///            stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] spec  vnic configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, vnic_entry *vnic,
                               pds_vnic_spec_t *spec);

    /// \brief     program vnic related tables during vnic delete by disabling
    ///            stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vnic  vnic obj being programmed
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_delete_(pds_epoch_t epoch, vnic_entry *vnic);

    /// \brief      update DHCP binding(s) corresponding to this vnic
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] orig_vnic  original vnic obj that is being modified
    /// \param[in] subnet subnet of this vnic
    /// \param[in] spec vnic configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t upd_dhcp_binding_(vnic_entry *vnic, vnic_entry *orig_vnic,
                                subnet_entry *subnet, pds_vnic_spec_t *spec);

    /// \brief    send vnic update event
    /// \param[in] vnic vnic instance being updated
    void send_vnic_upd_event_(vnic_entry *vnic);

    /// \brief      program vnic related tables during vnic update by
    ///             enabling stage0 tables corresponding to the new epoch
    /// \param[in] epoch epoch being activated
    /// \param[in] vnic  vnic obj being programmed
    /// \param[in] orig_vnic  original vnic obj that is being modified
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_update_(pds_epoch_t epoch, vnic_entry *vnic,
                               vnic_entry *orig_vnic, api_obj_ctxt_t *obj_ctxt);

    /// \brief      restore h/w resources for this obj from persistent storage
    /// \param[in]  info pointer to the info object
    /// \return     #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t restore_resources(obj_info_t *info) override;

    /// \brief      fill the vnic spec
    /// \param[out] spec specification
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_spec_(pds_vnic_spec_t *spec);

    /// \brief      fill the vnic stats
    /// \param[out] stats statistics
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_stats_(pds_vnic_stats_t *stats);

    /// \brief      fill the vnic status
    /// \param[out] status status
    void fill_status_(pds_vnic_status_t *status);

private:
    // P4 datapath specific state
    uint8_t epoch_;           ///< datapath epoch of the vnic
    uint16_t hw_id_;          ///< hardware id
    uint16_t nh_idx_;         ///< nexthop table index for this vnic
    handle_t lif_vlan_hdl_;   ///<  (if, vlan) table entry handle
    ///< handle for LOCAL_MAPPING and MAPPING table entries (note that handles
    ///< are valid only in a transaction)
    handle_t local_mapping_hdl_;
    handle_t mapping_hdl_;
    // h/w id for IP_MAC_BINDING table
    uint32_t binding_hw_id_;
    /// PI specific info
    struct {
        pds_obj_key_t key_;
    };
    ht_ctxt_t ht_ctxt_;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __VNIC_IMPL_HPP__
