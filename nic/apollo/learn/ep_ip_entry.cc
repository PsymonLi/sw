//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// IP Learning entry handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/apollo/learn/auto/ep_aging.hpp"
#include "nic/apollo/learn/notify/ep_dedup.hpp"
#include "nic/apollo/learn/ep_ip_entry.hpp"
#include "nic/apollo/learn/ep_ip_state.hpp"
#include "nic/apollo/learn/ep_mac_state.hpp"
#include "nic/apollo/learn/learn_internal.hpp"

namespace learn {

#define ep_mac_db learn_db()->ep_mac_db
#define ep_ip_db learn_db()->ep_ip_db

ep_ip_entry::ep_ip_entry() {
    pds_learn_mode_t mode = learn_oper_mode();

    ht_ctxt_.reset();
    this->state_ = EP_STATE_LEARNING;
    if (mode == PDS_LEARN_MODE_AUTO) {
        ageout_ts_ = 0;
    } else if (mode == PDS_LEARN_MODE_NOTIFY) {
        dedup_ts_ = 0;
    }
}

ep_ip_entry::~ep_ip_entry() {
}

ep_ip_entry *
ep_ip_entry::factory(ep_ip_key_t *key, mac_addr_t *mac_addr) {
    ep_ip_entry *ep_ip;

    SDK_ASSERT(learn_oper_mode() == PDS_LEARN_MODE_NOTIFY);

    ep_ip = ep_ip_db()->alloc();
    if (ep_ip) {
        new (ep_ip) ep_ip_entry();
        ep_ip->key_ = *key;
        MAC_ADDR_COPY(&ep_ip->mac_addr_, mac_addr);
        dedup_timer_init(&ep_ip->dedup_timer_, (void *)ep_ip,
                         PDS_MAPPING_TYPE_L3);
    }
    return ep_ip;
}

ep_ip_entry *
ep_ip_entry::factory(ep_ip_key_t *key, uint32_t vnic_obj_id) {
    ep_ip_entry *ep_ip;

    SDK_ASSERT(learn_oper_mode() == PDS_LEARN_MODE_AUTO);

    ep_ip = ep_ip_db()->alloc();
    if (ep_ip) {
        new (ep_ip) ep_ip_entry();
        ep_ip->key_ = *key;
        aging_timer_init(&ep_ip->aging_timer_, (void *)ep_ip,
                         PDS_MAPPING_TYPE_L3);
        ep_ip->vnic_obj_id_ = vnic_obj_id;
    }
    return ep_ip;
}

void
ep_ip_entry::destroy(ep_ip_entry *ep_ip) {
    ep_ip->~ep_ip_entry();
    ep_ip_db()->free(ep_ip);
}

string
ep_ip_entry::key2str(void) const {
    pds_learn_mode_t mode = learn_oper_mode();

    if (mode == PDS_LEARN_MODE_NOTIFY) {
        return "IP-(" + std::string(ipaddr2str(&key_.ip_addr)) +
               "," + to_string(key_.lif) + ")";
    } else if (mode == PDS_LEARN_MODE_AUTO) {
        return "IP-(" + std::string(key_.vpc.str()) + ", " +
            std::string(ipaddr2str(&key_.ip_addr)) + ")";
    } else {
        return "";
    }
}

sdk_ret_t
ep_ip_entry::delay_delete(void) {
    return api::delay_delete_to_slab(api::PDS_SLAB_ID_IP_ENTRY, this);
}

sdk_ret_t
ep_ip_entry::add_to_db(void) {
    return ep_ip_db()->insert(this);
}

sdk_ret_t
ep_ip_entry::del_from_db(void) {
    if (ep_ip_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

void
ep_ip_entry::set_state(ep_state_t state) {
    this->state_ = state;
}

ep_mac_entry *
ep_ip_entry::mac_entry(void) {
    pds_obj_key_t vnic_key;
    vnic_entry *vnic;
    ep_mac_key_t mac_key = {0};
    pds_learn_mode_t mode = learn_oper_mode();

    if (mode == PDS_LEARN_MODE_AUTO) {
        // construct mac key from vnic_obj_id
        // get subnet and mac from vnic
        vnic_key = api::uuid_from_objid(vnic_obj_id_);
        vnic = vnic_db()->find(&vnic_key);
        if (unlikely(vnic == nullptr)) {
            return nullptr;
        }
        mac_key.make_key((char *)&vnic->mac(), vnic->subnet());
    } else if (mode == PDS_LEARN_MODE_NOTIFY) {
        // construct mac key from mac addr stored, and lif from ip key
        mac_key.make_key((char *)&mac_addr_, key_.lif);
    }
    return (ep_mac_db()->find(&mac_key));
}

mac_addr_t&
ep_ip_entry::mac_addr(void) {
    ep_mac_key_t *mac_key;

    if (learn_oper_mode() == PDS_LEARN_MODE_NOTIFY) {
        return mac_addr_;
    } else {
        mac_key = (ep_mac_key_t *)this->mac_entry()->key();
        return mac_key->mac_addr;
    }
}

}    // namespace learn
