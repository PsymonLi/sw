//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// lif datapath implementation
///
//----------------------------------------------------------------------------

#ifndef __LIF_IMPL_HPP__
#define __LIF_IMPL_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/include/sdk/qos.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/framework/if_impl_base.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"

#define SDK_MAX_NAME_LEN            16

namespace api {
namespace impl {

// forward declaration
class lif_impl_state;

/// \defgroup PDS_LIF_IMPL - lif entry datapath implementation
/// \ingroup PDS_LIF
/// \@{
/// \brief LIF implementation

class lif_impl : public if_impl_base {
public:
    /// \brief  factory method to allocate and initialize a lif entry
    /// \param[in] spec    lif configuration parameters
    /// \return    new instance of lif or NULL, in case of error
    static lif_impl *factory(pds_lif_spec_t *spec);

    /// \brief  release all the s/w state associate with the given lif,
    ///         if any, and free the memory
    /// \param[in] impl    lif impl instance to be freed
    ///                  NOTE: h/w entries should have been cleaned up (by
    ///                  calling impl->cleanup_hw() before calling this
    static void destroy(lif_impl *impl);

    /// \brief     helper function to get key given lif entry
    /// \param[in] entry    pointer to lif instance
    /// \return    pointer to the lif instance's key
    static void *lif_key_func_get(void *entry) {
        lif_impl *lif = (lif_impl *)entry;
        return (void *)&(lif->key_);
    }

    /// \brief     helper function to get (internal) lif idgiven lif entry
    /// \param[in] entry    pointer to lif instance
    /// \return    pointer to the lif instance's key
    static void *lif_id_func_get(void *entry) {
        lif_impl *lif = (lif_impl *)entry;
        return (void *)&(lif->id_);
    }

    /// \brief    handle all programming during lif creation
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create(pds_lif_spec_t *spec);

    ///< \brief    program lif tx policer for given lif
    ///< param[in] policer    policer parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_tx_policer(sdk::qos::policer_t *policer);

    ///< \brief    program lif tx scheduler for given lif
    ///< param[in] spec   lif information
    /// \return    SDK_RET_OK on success, failure status code on error
    static sdk_ret_t program_tx_scheduler(pds_lif_spec_t *spec);

    ///< \brief    reserve lif tx scheduler for given lif
    ///< param[in] spec   lif information
    /// \return    SDK_RET_OK on success, failure status code on error
    static sdk_ret_t reserve_tx_scheduler(pds_lif_spec_t *spec);

    /// \brief     ifindex this lif is pinned to
    /// \return    pinned interface index
    if_index_t pinned_ifindex(void) const {
        return pinned_if_idx_;
    }

    /// \brief     return type of the lif
    /// \return    lif type
    lif_type_t type(void) const { return type_; }

    /// \brief     return lif key
    /// \return    key of the lif
    const pds_obj_key_t& key(void) const { return key_; }

    /// \brief     return (internal) lif id
    /// \return    id of the lif
    pds_lif_id_t id(void) const { return id_; }

    /// \brief     return encoded ifindex of lif
    /// \return    ifindex of the lif
    if_index_t ifindex(void) const { return ifindex_; }

    /// \brief     return vnic hw id of this lif
    /// \return    vnic hw id
    uint16_t vnic_hw_id(void) const { return vnic_hw_id_; }

    /// \brief     set/update the name of the lif
    /// \param[in] name    name of the device corresponding to this lif
    void set_name(const char *name);

    /// \brief     return the name of the lif
    /// \return    return lif name
    const char *name(void) const { return name_; }

    /// \brief     set/update the state of the lif
    /// \param[in] state    operational state to update the lif with
    void set_state(lif_state_t state) {
        state_ = state;
    }

    /// \brief     return the operational state of the lif
    /// \return    operational state of the lif
    lif_state_t state(void) const { return state_; }

    /// \brief     set/update the admin state of the lif
    /// \param[in] state    admin state to update the lif with
    void set_admin_state(lif_state_t state);

    /// \brief     return the admin state of the lif
    /// \return    admin state of the lif
    lif_state_t admin_state(void) const { return admin_state_; }

    /// \brief     set/update the mac address of the lif
    /// \param[in] mac    mac address of the device corresponding to this lif
    void set_mac(mac_addr_t mac) {
        memcpy(mac_, mac, ETH_ADDR_LEN);
    }

    /// \brief     set total queue count
    /// \param[in] total_qcount    total queue count
    void set_total_qcount(uint32_t total_qcount) {
        total_qcount_ = total_qcount;
    }

    /// \brief     set cos bitmap
    /// \param[in] cos_bmp    cos bitmap
    void set_cos_bmp(uint16_t cos_bmp) {
        cos_bmp_ = cos_bmp;
    }

    /// \brief     return total queue count
    /// \return    total queue count
    uint32_t total_qcount(void) const { return total_qcount_; }

    /// \brief     return cos bitmap
    /// \return    cos bmp
    uint16_t cos_bmp(void) const { return cos_bmp_; }

    /// \brief     return the MAC address corresponding to this lif
    /// \return    ethernet MAC address of this lif
    mac_addr_t& mac(void) { return mac_; }

    /// \brief    return the nexthop index corresponding to this lif
    /// \return   nexthop index of the lif
    uint32_t nh_idx(void) const { return nh_idx_; }

    /// \brief    save the tx scheduler offset and number of entries to this lif
    void set_tx_sched_info(uint32_t offset, uint32_t num_entries) {
        tx_sched_offset_ = offset;
        tx_sched_num_entries_ = num_entries;
    }

    /// \brief     return if initializtion of lifis done or not
    /// \return    true if lif initialization is done
    bool create_done(void) const { return create_done_; }

    /// \brief    retrieve tx scheduler offset & number of entries from this lif
    /// \return   true if it is valid, false otherwise
    bool tx_sched_info(uint32_t *offset, uint32_t *num_entries) {
        if (tx_sched_offset_ == INVALID_INDEXER_INDEX) {
            return false;
        }
        *offset = tx_sched_offset_;
        *num_entries = tx_sched_num_entries_;
        return true;
    }

    /// \brief      reset all the statistics associated with the lif
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t reset_stats(void);

    /// \brief      track pps for lif
    /// \param[in]  interval    sampling interval in secs
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t track_pps(api_base *api_obj, uint32_t interval);

    /// \brief      dump all the statistics associated with the lif
    /// \param[in]  fd  file descriptor to which output is dumped
    void dump_stats(api_base *api_obj, uint32_t fd);

private:
    ///< constructor
    ///< \param[in] spec    lif configuration parameters
    lif_impl(pds_lif_spec_t *spec);

    ///< destructor
    ~lif_impl() {}

    ///< \brief    install all the NACLs needed for the oob lif/uplink traffic
    ///            forwarding
    ///< \param[in] uplink_nh_idx    nexthop index allocated for oob uplink
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_oob_mnic_nacls_(uint32_t uplink_nh_idx);

    ///< \brief    program necessary h/w entries for oob mnic lif
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_oob_mnic_(pds_lif_spec_t *spec);

    ///< \brief    install all the NACLs needed for the inband lif/uplink
    ///            traffic forwarding
    ///< \param[in] uplink_nh_idx    nexthop index allocated for inband uplink
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_inb_mnic_nacls_(uint32_t uplink_nh_idx);

    ///< \brief    program necessary h/w entries for inband mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_inb_mnic_(pds_lif_spec_t *spec);

    ///< \brief    install all the NACLs needed for redirecting traffic to vpp
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_datapath_mnic_nacls_(void);

    ///< \brief    program necessary h/w entries for datapath lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_datapath_mnic_(pds_lif_spec_t *spec);

    ///< \brief    install all the NACLs needed for the internal management
    ///            mnic traffic
    ///< \param[in] host_mgmt_lif    host mgmt. lif instance
    ///< \param[in] host_mgmt_lif_nh_idx    host mgmt. lif's nexthop index
    ///< \param[in] int_mgmt_lif    internal mgmt. lif instance
    ///< \param[in] int_mgmt_lif_nh_idx    internal mgmt. lif's nexthop index
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_internal_mgmt_mnic_nacls_(lif_impl *host_mgmt_lif,
                                                uint32_t host_mgmt_lif_nh_idx,
                                                lif_impl *int_mgmt_lif,
                                                uint32_t int_mgmt_lif_nh_idx);

    ///< \brief    program necessary entries for internal mgmt. mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_internal_mgmt_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for host (data) lifs
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_host_lif_(pds_lif_spec_t *spec);

    ///< \brief    install all NACLs needed for PDS_LEARN_MODE_AUTO
    ///< \param[in] lif    learn lif instance
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_learn_auto_mode_nacls_(lif_impl *lif);

    ///< \brief    install all NACLs needed for PDS_LEARN_MODE_NOTIFY
    ///< \param[in] lif    learn lif instance
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_learn_notify_mode_nacls_(lif_impl *lif,
                                               pds_learn_source_t *source);

    ///< \brief    program necessary entries for host (data) lifs
    ///< \param[in] mode      learn mode
    ///< \param[in] source    learn source configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t install_learn_nacls_(pds_learn_mode_t mode,
                                   pds_learn_source_t *source);

    ///< \brief    program necessary entries for learn lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_learn_lif_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for control lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_control_lif_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for P2P MNIC lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_p2p_mnic_(pds_lif_spec_t *spec);

private:
    pds_obj_key_t    key_;            ///< lif key
    pds_lif_id_t     id_;             ///< (s/w & h/w) internal lif id
    if_index_t       pinned_if_idx_;  ///< pinnned if index, if any
    lif_type_t       type_;           ///< type of the lif
    /// name of the lif, if any
    char             name_[SDK_MAX_NAME_LEN];
    mac_addr_t       mac_;            ///< MAC address of lif
    uint32_t         tx_sched_offset_;///< tx scheduler entry offset
    /// number of tx scheduler entries used
    uint32_t         tx_sched_num_entries_;
    uint32_t         total_qcount_;   ///< total queue count
    uint16_t         cos_bmp_;        ///< cos bmp

    /// operational state
    // TODO: we can have state per pipeline in this class
    //       ideally, we should have the concrete class inside pipeline specific
    //       dir and this should be a base class !!
    if_index_t       ifindex_;        ///< ifindex of this lif
    uint32_t         nh_idx_;         ///< nexthop idx of this lif
    uint16_t         vnic_hw_id_;     ///< vnic hw id
    lif_state_t      admin_state_;    ///< admin state
    lif_state_t      state_;          ///< operational state
    bool             create_done_;    ///< TRUE if lif create is done
    ht_ctxt_t        ht_ctxt_;        ///< hash table context
    ht_ctxt_t        id_ht_ctxt_;     ///< id based hash table context
    /// lif_impl_state is friend of lif_impl
    friend class lif_impl_state;
} __PACK__;

/// \@}

}    // namespace impl
}    // namespace api

#endif    //    __LIF_IMPL_HPP__
