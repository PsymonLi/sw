//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// IP Learning entry handling
///
//----------------------------------------------------------------------------

#ifndef __LEARN_EP_IP_ENTRY_HPP__
#define __LEARN_EP_IP_ENTRY_HPP__

#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/learn/learn.hpp"

namespace learn {

/// \defgroup EP_LEARN - Endpoint IP Learning Entry functionality
/// @{

class ep_mac_entry;

/// \brief    IP Learning entry
class ep_ip_entry {
public:
    /// \brief          factory method to create an EP IP entry
    /// \param[in]      key         key info for IP address learnt
    /// \param[in]      vnic_obj_id object id of associated vnic
    /// \return         new instance of IP entry or NULL, in case of error
    static ep_ip_entry *factory(ep_ip_key_t *key, uint32_t vnic_obj_id);

    /// \brief          factory method to create an EP IP entry
    /// \param[in]      key         key info for IP address learnt
    /// \param[in]      mac_addr    mac addr associated with this IP entry
    /// \return         new instance of IP entry or NULL, in case of error
    static ep_ip_entry *factory(ep_ip_key_t *key, mac_addr_t *mac_addr);

    /// \brief          free memory allocated to IP entry
    /// \param[in]      ep_ip    pointer to IP entry
    static void destroy(ep_ip_entry *ep_ip);

    /// \brief          add this entry to EP database
    /// \return         SDK_RET_OK on sucess, failure status code on error
    sdk_ret_t add_to_db(void);

    /// \brief          del this entry from EP database
    /// \return         SDK_RET_OK on success, SDK_RET_ENTRY_NOT_FOUND
    ///                 if entry not found in db
    sdk_ret_t del_from_db(void);

    /// \brief          initiate delay deletion of this object
    sdk_ret_t delay_delete(void);

    /// \brief          get the state of this entry
    /// \return         state of this entry
    ep_state_t state(void) const { return state_; }

    /// \brief          set the state of this entry
    /// \param[in]      state    state to be set on this entry
    void set_state(ep_state_t state);

    /// \brief          helper function to get key given EP IP entry
    /// \param[in]      entry    pointer to EP IP instance
    /// \return         pointer to the EP IP instance's key
    static void *ep_ip_key_func_get(void *entry) {
        ep_ip_entry *ep_ip = (ep_ip_entry *)entry;
        return (void *)&(ep_ip->key_);
    }

    /// \brief          get MAC entry associated with this IP entry
    /// \return         pointer to MAC entry, which this IP entry associates to
    ep_mac_entry *mac_entry(void);

    /// \brief          get MAC addr associated with this IP entry
    /// \return         MAC addr for this entry
    mac_addr_t& mac_addr(void);

    /// \brief          get vnic object id of this entry
    /// \return         vnic object id
    uint32_t vnic_obj_id(void) const { return vnic_obj_id_; }

    /// \brief          set the vnic object id for this entry
    /// \param[in]      vnic object id
    void set_vnic_obj_id(uint32_t vnic_obj_id) { vnic_obj_id_ = vnic_obj_id; }

    /// \brief          compare vnic obj id with the one assoc with this entry
    /// \return         true if vnic object ids are equal else false
    bool vnic_compare(uint32_t vnic_obj_id_) const;

    /// \brief          get aging timer of this entry
    /// \return         pointer to event timer
    sdk::event_thread::timer_t *aging_timer(void) { return &aging_timer_; }

    /// \brief          get dedup timer of this entry
    /// \return         pointer to event timer
    sdk::event_thread::timer_t *dedup_timer(void) { return &dedup_timer_; }

    /// \brief          get aging timer start timestamp
    /// \return         agining start timestamp in seconds
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

    /// \brief          get key to this entry
    /// \return         pointer to key
    const ep_ip_key_t *key(void) const { return &key_; }

    /// \brief          return stringified key of the object (for debugging)
    string key2str(void) const;

    /// \brief          return number of ARP probes sent
    uint8_t arp_probe_count(void) const { return arp_probe_count_; }

    /// \brief          reset number of ARP probes sent counter
    void arp_probe_count_reset(void) { arp_probe_count_ = 0; }

    /// \brief          increment number of ARP probes sent counter
    void arp_probe_count_incr(void) { arp_probe_count_++; }

private:
    /// \brief          constructor
    ep_ip_entry();

    /// \brief          destructor
    ~ep_ip_entry();

private:
    ep_ip_key_t                        key_;     ///< IP learning entry key
    union {
        // used in auto mode
        struct {
            ///< timer to ageout IP entry
            sdk::event_thread::timer_t aging_timer_;
            ///< aging timer expiry time
            uint64_t                   ageout_ts_;
            ///< key for vnic associated
            uint32_t                   vnic_obj_id_;
            ///< number of ARP probe sent
            uint8_t                    arp_probe_count_;
        };
        // used in notify mode
        struct {
            ///< timer to dedup IP learnt
            sdk::event_thread::timer_t dedup_timer_;
            ///< dedup timer expiry time
            uint64_t                   dedup_ts_;
            ///< mac addr with which IP is associated
            mac_addr_t                 mac_addr_;
        };
    };
    ep_state_t                         state_;   ///< state of this entry
    ht_ctxt_t                          ht_ctxt_; ///< hash table context

    ///< EP IP state class is a friend of IP entry
    friend class ep_ip_state;
};

/// \@}    // end of EP_IP_ENTRY

}    // namespace learn

using learn::ep_ip_entry;

#endif    // __LEARN_EP_IP_ENTRY_HPP__
