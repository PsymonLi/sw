//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// PDS Metaswitch stub Subnet Spec store used by Mgmt
//---------------------------------------------------------------

#ifndef __PDS_MS_BGP_STORE_HPP__
#define __PDS_MS_BGP_STORE_HPP__

#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "include/sdk/ip.hpp"
#include <unordered_map>

namespace pds_ms {
// bgp peer info
struct bgp_peer_t {
    ip_addr_t local_addr;
    ip_addr_t peer_addr;

    bgp_peer_t(const ip_addr_t& l, const ip_addr_t& p) :
        local_addr(l), peer_addr(p) {}

    bgp_peer_t(const ATG_INET_ADDRESS* l, const ATG_INET_ADDRESS* p)
    {
        ms_to_pds_ipaddr(*l, &local_addr);
        ms_to_pds_ipaddr(*p, &peer_addr);
    }
    std::string str(void) {
        std::string str = ipaddr2str(&local_addr);
        str.append(", ").append(ipaddr2str(&peer_addr));
        return str;
    }
};

struct bgp_peer_props_t {
    bool rrclient;
};

static inline bool
operator ==(const pds_ms::bgp_peer_t& a,
            const pds_ms::bgp_peer_t& b)
{
    return (IPADDR_EQ(&a.local_addr, &b.local_addr) &&
            IPADDR_EQ(&a.peer_addr, &b.peer_addr));
}

class bgp_peer_hash {
public:
    std::size_t operator()(const bgp_peer_t& peer) const {
        return hash_algo::fnv_hash((void *)&peer, sizeof(peer));
    }
};

// structure to store pending peer info
struct bgp_peer_pend_obj_t {
    ip_addr_t local_addr;
    ip_addr_t peer_addr;
    bool      add_oper;
    bool      rrclient;
    bgp_peer_pend_obj_t (const ip_addr_t& l, const ip_addr_t& p,
                         bool add, bool rrc) :
        local_addr(l), peer_addr(p), add_oper(add), rrclient(rrc) {}
};

// bgp peer store
class bgp_peer_store_t {
public:
    void add(const ip_addr_t& l, const ip_addr_t& p, bool rrclient) {
        SDK_TRACE_DEBUG ("bgp_peer_store: add %s/%s",
                          ipaddr2str(&l), ipaddr2str(&p));
        list_[bgp_peer_t(l, p)] = bgp_peer_props_t{.rrclient = rrclient};
    }
    void del(const ip_addr_t& l, const ip_addr_t& p) {
        SDK_TRACE_DEBUG ("bgp_peer_store: del %s/%s",
                          ipaddr2str(&l), ipaddr2str(&p));
        list_.erase(bgp_peer_t(l, p));
    }
    bool find(const ip_addr_t& l, const ip_addr_t& p) const {
        auto it = list_.find(bgp_peer_t(l,p));
        return (it != list_.end());
    }
    bool rrclient(const ip_addr_t& l, const ip_addr_t& p) const {
        auto it = list_.find(bgp_peer_t(l,p));
        if (it == list_.end()) {
            return false;
        }
        return it->second.rrclient;
    }

private:  
    // bgp peer -> is rr client
    std::unordered_map<bgp_peer_t, bgp_peer_props_t, bgp_peer_hash> list_;
};

} // End namespace

#endif
