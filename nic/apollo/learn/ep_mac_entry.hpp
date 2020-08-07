//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// MAC Learning entry handling
///
//----------------------------------------------------------------------------

#ifndef __LEARN_EP_MAC_ENTRY_HPP__
#define __LEARN_EP_MAC_ENTRY_HPP__

#include <list>
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/learn/learn.hpp"

using std::list;
using api::state_walk_cb_t;

namespace learn {

/// \defgroup EP_LEARN - Endpoint MAC Learning Entry functionality
/// @{

class ep_ip_entry;
typedef list<ep_ip_entry *> ip_entry_list_t;

/// \brief    MAC Learning entry
class ep_mac_entry {
public:
    /// \brief          factory method to create an EP MAC entry
    /// \param[in]      key      key info for MAC learnt
    /// \param[in]      encap    vlan encap for this MAC
    /// \return         new instance of MAC entry or NULL, in case of error
    static ep_mac_entry *factory(ep_mac_key_t *key, pds_encap_t *encap);

    /// \brief          factory method to create an EP MAC entry
    /// \param[in]      key            key info for MAC learnt
    /// \param[in]      vnic_obj_id    vnic object id associated with this MAC
    /// \return         new instance of MAC entry or NULL, in case of error
    static ep_mac_entry *factory(ep_mac_key_t *key, uint32_t vnic_obj_id);

    /// \brief          free memory allocated to MAC entry
    /// \param[in]      ep_mac    pointer to MAC entry
    static void destroy(ep_mac_entry *ep_mac);

    /// \brief          add this entry to EP database
    /// \return         SDK_RET_OK on sucess, failure status code on error
    sdk_ret_t add_to_db(void);

    /// \brief          del this entry from EP database
    /// \return         SDK_RET_OK on success, SDK_RET_ENTRY_NOT_FOUND
    ///                 if entry not found in db
    sdk_ret_t del_from_db(void);

    /// \brief          initiate delay deletion of this object
    sdk_ret_t delay_delete(void);

    /// \brief          get the key of this entry
    /// \return         pointer to key of this entry
    const ep_mac_key_t *key(void) const { return &key_; }

    /// \brief          get the state of this entry
    /// \return         state of this entry
    ep_state_t state(void) const { return state_; }

    /// \brief          set the state of this entry
    /// \param[in]      state    state to be set on this entry
    void set_state(ep_state_t state);

    /// \brief          helper function to get key given EP MAC entry
    /// \param[in]      entry    pointer to EP MAC instance
    /// \return         pointer to the EP MAC instance's key
    static void *ep_mac_key_func_get(void *entry) {
        ep_mac_entry *ep_mac = (ep_mac_entry *)entry;
        return (void *)&(ep_mac->key_);
    }

    /// \brief          link IP entry with this MAC endpoint
    /// \param[in]      IP    IP entry learnt with this mac
    /// \return         SDK_RET_OK on success, error code on failure
    void add_ip(ep_ip_entry *ip);

    /// \brief          unlink IP entry from this MAC endpoint
    /// \param[in]      IP    IP entry to be unlinked from this mac
    /// \return         SDK_RET_OK on success, error code on failure
    void del_ip(ep_ip_entry *ip);

    // safely walk all IP entries associated with this ep
    void walk_ip_list(state_walk_cb_t walk_cb, void *ctxt);

    /// \brief          get vnic object id of this entry
    /// \return         vnic object id
    uint32_t vnic_obj_id(void) const { return vnic_obj_id_; }

    /// \brief          set vnic object id of this entry
    /// \param[in]      idx vnic object id for this entry
    void set_vnic_obj_id(uint32_t idx) { vnic_obj_id_ = idx; }

    /// \brief          get aging timer of this entry
    /// \return         pointer to event timer
    sdk::event_thread::timer_t *aging_timer(void) { return &aging_timer_; }

    /// \brief          get dedup timer of this entry
    /// \return         pointer to event timer
    sdk::event_thread::timer_t *dedup_timer(void) { return &dedup_timer_; }

    /// \brief          get aging timer start timestamp
    /// \return         aging start timestamp in seconds
    uint64_t ageout_ts(void) const { return ageout_ts_; }

    /// \brief          set aging timer start timestamp
    /// \param[in]      timestamp
    void set_ageout_ts(uint64_t ts) { ageout_ts_ = ts; }

    /// \brief          get dedup timer finish timestamp
    /// \return         dedup finish timestamp in seconds
    uint64_t dedup_ts(void) const { return dedup_ts_; }

    /// \brief          set dedup timer finish timestamp
    /// \param[in]      timestamp
    void set_dedup_ts(uint64_t ts) { dedup_ts_ = ts; }

    /// \brief          get number of associated IP mappings
    /// \return         count of IP addresses
    uint16_t ip_count(void) const { return (uint16_t) ip_list_.size(); }

    /// \brief          get encap associated with this entry
    /// \return         encap of this entry
    pds_encap_t encap(void) const { return encap_; }

    /// \brief          set encap for this entry
    /// \param[in]      encap type and value
    void set_encap(pds_encap_t encap) { encap_ = encap; }

    /// \brief          return stringified key of the object (for debugging)
    string key2str(void) const;

private:
    /// \brief          constructor
    ep_mac_entry();

    /// \brief          destructor
    ~ep_mac_entry();

private:
    ep_mac_key_t                       key_;     ///< MAC learning entry key
    ep_state_t                         state_;   ///< state of this entry
    union {
        // used in auto mode
        struct {
            ///< timer to ageout MAC
            sdk::event_thread::timer_t aging_timer_;
            ///< aging timer expiry time
            uint64_t                   ageout_ts_;
            ///< key for vnic associated
            uint32_t                   vnic_obj_id_;
        };
        // used in notify mode
        struct {
            ///< timer to dedup MAC
            sdk::event_thread::timer_t dedup_timer_;
            ///< dedup timer expiry time
            uint64_t                   dedup_ts_;
            ///< vlan encap
            pds_encap_t                encap_;
        };
    };
    ht_ctxt_t                          ht_ctxt_; ///< hash table context
    ip_entry_list_t                    ip_list_; ///< list of linked IP entries

    friend class ep_mac_state;
};
/// \@}    // end of EP_MAC_ENTRY

}    // namespace learn

using learn::ep_mac_entry;

#endif    // __LEARN_EP_MAC_ENTRY_HPP__
