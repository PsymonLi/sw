//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// MAC Learning entry handling
///
//----------------------------------------------------------------------------

#include "nic/infra/core/mem.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/learn/auto/ep_aging.hpp"
#include "nic/apollo/learn/notify/ep_dedup.hpp"
#include "nic/apollo/learn/ep_mac_entry.hpp"
#include "nic/apollo/learn/ep_mac_state.hpp"
#include "nic/apollo/learn/learn_state.hpp"
#include "nic/apollo/learn/learn_internal.hpp"

namespace learn {

#define ep_mac_db learn_db()->ep_mac_db

ep_mac_entry::ep_mac_entry() {
    pds_learn_mode_t mode = learn_oper_mode();

    this->state_ = EP_STATE_LEARNING;
    if (mode == PDS_LEARN_MODE_AUTO) {
        ageout_ts_ = 0;
    } else if (mode == PDS_LEARN_MODE_NOTIFY) {
        dedup_ts_ = 0;
    }
    ht_ctxt_.reset();
}

ep_mac_entry::~ep_mac_entry() {
    if (learn_oper_mode() == PDS_LEARN_MODE_AUTO) {
        if (this->vnic_obj_id_) {
            learn_db()->vnic_obj_id_free(this->vnic_obj_id_);
        }
    }
    this->ip_list_.clear();
}

ep_mac_entry *
ep_mac_entry::factory(ep_mac_key_t *key, pds_encap_t *encap) {
    ep_mac_entry *ep_mac;

    SDK_ASSERT(learn_oper_mode() == PDS_LEARN_MODE_NOTIFY);

    ep_mac = ep_mac_db()->alloc();
    if (ep_mac) {
        new (ep_mac) ep_mac_entry();
        memcpy(&ep_mac->key_, key, sizeof(ep_mac_key_t));
        dedup_timer_init(&ep_mac->dedup_timer_, (void *)ep_mac,
                         PDS_MAPPING_TYPE_L2);
        memcpy(&ep_mac->encap_, encap, sizeof(pds_encap_t));
    }
    return ep_mac;
}

ep_mac_entry *
ep_mac_entry::factory(ep_mac_key_t *key, uint32_t vnic_obj_id) {
    ep_mac_entry *ep_mac;

    SDK_ASSERT(learn_oper_mode() == PDS_LEARN_MODE_AUTO);

    ep_mac = ep_mac_db()->alloc();
    if (ep_mac) {
        new (ep_mac) ep_mac_entry();
        memcpy(&ep_mac->key_, key, sizeof(ep_mac_key_t));
        aging_timer_init(&ep_mac->aging_timer_, (void *)ep_mac,
                         PDS_MAPPING_TYPE_L2);
        ep_mac->vnic_obj_id_ = vnic_obj_id;
    }
    return ep_mac;
}

void
ep_mac_entry::destroy(ep_mac_entry *ep_mac) {
    ep_mac->~ep_mac_entry();
    ep_mac_db()->free(ep_mac);
}

string
ep_mac_entry::key2str(void) const {
    pds_learn_mode_t mode = learn_oper_mode();

    if (mode == PDS_LEARN_MODE_NOTIFY) {
        return "MAC-(" + std::string(macaddr2str(key_.mac_addr))
            + "," + to_string(key_.lif) + ")";
    } else if (mode == PDS_LEARN_MODE_AUTO) {
        return "MAC-(" + std::string(key_.subnet.str()) + ", " +
            std::string(macaddr2str(key_.mac_addr)) + ")";
    } else {
        return "";
    }
}

sdk_ret_t
ep_mac_entry::delay_delete(void) {
    return api::delay_delete_to_slab(api::PDS_SLAB_ID_MAC_ENTRY, this);
}

sdk_ret_t
ep_mac_entry::add_to_db(void) {
    return ep_mac_db()->insert(this);
}

sdk_ret_t
ep_mac_entry::del_from_db(void) {
    if (ep_mac_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

void
ep_mac_entry::set_state(ep_state_t state) {
    this->state_ = state;
}

void
ep_mac_entry::add_ip(ep_ip_entry *ip) {
    this->ip_list_.push_front(ip);
}

void
ep_mac_entry::del_ip(ep_ip_entry *ip) {
    this->ip_list_.remove(ip);
}

void
ep_mac_entry::walk_ip_list(state_walk_cb_t walk_cb, void *ctxt) {
    ip_entry_list_t::iterator it;
    bool stop_iter = false;

    it = this->ip_list_.begin();
    while(it != this->ip_list_.end()) {
        stop_iter = walk_cb(*(it++), ctxt);
        if(stop_iter) {
            break;
        }
    }
}

}    // namespace learn
