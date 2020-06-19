//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// Route stub store
//---------------------------------------------------------------

#ifndef __PDS_MS_ROUTE_STORE_HPP__
#define __PDS_MS_ROUTE_STORE_HPP__

#include <nbase.h>
extern "C"
{
#include <ntltimer.h>
}
#include "nic/metaswitch/stubs/common/pds_ms_defs.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_object_store.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_slab_object.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/apollo/api/include/pds_route.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include <vector>
#include <unordered_map>

static inline bool operator==(const ip_prefix_t& a,
                              const ip_prefix_t& b) {
    return (ip_prefix_is_equal((ip_prefix_t *)&a, (ip_prefix_t *)&b));
}

namespace pds_ms {

// Minimum number of routes for initial vector size. This is to avoid
// multiple re-allocs while adding routes
#define PDS_MS_MIN_NUM_ROUTES           1024
// Commit batch timer in milliseconds
#define PDS_MS_COMMIT_BATCH_TIMER_MS    200
// Max hardware route table capacity (16K routes)
// TODO: This should eventually come from a hardware catalog file
#define PDS_MS_MAX_NUM_ROUTES           16383

class route_table_obj_t : public slab_obj_t<route_table_obj_t>,
                          public base_obj_t {
public:
    route_table_obj_t(pds_obj_key_t key, uint8_t af, bool underlay=false)
        : key_(key), af_(af) {
        if (underlay) {
            // We dont fill the route table for Underlay 
            // Use minimum capacity
            routes_capacity_ = 2;
        }
        realloc_();
        // Initialize the timer
        NTL_TIMER_INIT_TIMER_CB(&batch_commit_timer_);
    }
    pds_obj_key_t key(void) const {return key_;}
    void update_store(state_t* state, bool op_delete) override;
    void print_debug_str(void) override {};
    bool add_upd_route(pds_route_t &route);
    const pds_route_t* get_route(ip_prefix_t &pfx);
    bool del_route(ip_prefix_t &pfx);
    int num_routes(void) { return (route_index_.size()); }
    route_info_t *routes(void) { return routes_; }
    int pending_batch_sz() { return pending_batch_sz_; }
    void reset_pending_batch_sz() { pending_batch_sz_ = 0; }
    NTL_TIMER_FIXED_CB *batch_commit_timer() { return &batch_commit_timer_; }

    ~route_table_obj_t() {
        if (routes_ != nullptr) {
            free (routes_);
            routes_ = nullptr;
        }
        routes_capacity_ = 0;
        // Terminate the timer
        NTL_TIMER_TERM_TIMER_CB(&batch_commit_timer_);
    }
    
private:
    // Max routes that will fit in allocated buffer size
    uint32_t routes_capacity_ = PDS_MS_MIN_NUM_ROUTES; 
    route_info_t*  routes_ = nullptr;

    pds_obj_key_t key_;
    uint8_t af_;
    // Store the index of the route in the vector
    std::unordered_map<ip_prefix_t, int, ip_prefix_hash> route_index_;
    // Size of the pending batch to be committed
    int pending_batch_sz_ =  0;
    // Batch commit timer
    NTL_TIMER_FIXED_CB batch_commit_timer_;

    void realloc_();
};

class route_table_store_t : public obj_store_t <pds_obj_key_t, route_table_obj_t, pds_obj_key_hash> {
};

void route_table_slab_init (slab_uptr_t slabs[], sdk::lib::slab_id_t slab_id);

} // End namespace

#endif
