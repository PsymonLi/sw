//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// PDS-MS stub store for destination ip addresses whose underlay reachability
// is tracked
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/common/pds_ms_ip_track_store.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_error.hpp"
#include "nic/sdk/include/sdk/ip.hpp"

namespace pds_ms {

template<> sdk::lib::slab* slab_obj_t<ip_track_obj_t>::slab_ = nullptr;

void
ip_track_slab_init (slab_uptr_t slabs_[], sdk::lib::slab_id_t slab_id)
{
    slabs_[slab_id].
        reset(sdk::lib::slab::factory("PDS-MS-DESTIP-TRACK",
                                      slab_id, sizeof(ip_track_obj_t),
                                      128,
                                      true, true, true));
    if (unlikely (!slabs_[slab_id])) {
        throw Error("SLAB creation failed for Overlay ECMP HAL Index");
    }
    ip_track_obj_t::set_slab(slabs_[slab_id].get());
}

ip_track_obj_t::ip_track_obj_t(const pds_obj_key_t& pds_obj_key,
                               const ip_addr_t& destip,
                               obj_id_t pds_obj_id)
    : pds_obj_key_(pds_obj_key), destip_(destip), pds_obj_id_(pds_obj_id) {
    // Allocate index
    // Enter thread-safe context to access/modify global state
    auto state_ctxt = state_t::thread_context();
    auto rs = state_ctxt.state()->
        ip_track_internal_idx_alloc(&internal_index_);
    if (rs != SDK_RET_OK) {
        throw Error(std::string("DestIP Internal tracking index alloc failed for ")
                    .append(ipaddr2str(&destip)).append(" with err ")
                    .append(std::to_string(rs)));
    }
}

ip_track_obj_t::~ip_track_obj_t() {
    // Free index
    // Enter thread-safe context to access/modify global state
    auto state_ctxt = pds_ms::state_t::thread_context();
    auto rs = state_ctxt.state()->ip_track_internal_idx_free(internal_index_);
    if (rs != SDK_RET_OK) {
        PDS_TRACE_ERR("DestIP Internal tracking index %d free failed for %s %s"
                      " with err %d",
                       internal_index_, pds_obj_key_.str(),
                       ipaddr2str(&destip_), rs);
        return;
    }
    PDS_TRACE_VERBOSE("Freed destip internal track index %d", internal_index_);
}

ip_prefix_t ip_track_obj_t::internal_ip_prefix() const {
    ip_prefix_t internal_ip_pfx = {0};
    internal_ip_pfx.len = 32;
    internal_ip_pfx.addr.af = IP_AF_IPV4;
    internal_ip_pfx.addr.addr.v4_addr = internal_index_; 
    return internal_ip_pfx;
}

} // End namespace
