//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// Mgmt stub global states
//--------------------------------------------------------------

#ifndef __PDS_MS_MGMT_STATE_HPP__
#define __PDS_MS_MGMT_STATE_HPP__

#include "nic/metaswitch/stubs/mgmt/pds_ms_mib_idx_gen.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_uuid_obj.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_error.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_object_store.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_bgp_store.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "gen/proto/types.pb.h"
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

#define PDS_MOCK_MODE() \
            (mgmt_state_t::thread_context().state()->pds_mock_mode())

namespace pds_ms {

enum mgmt_slab_id_e {
    PDS_MS_MGMT_BGP_PEER_SLAB_ID = 1,
    PDS_MS_MGMT_BGP_PEER_AF_SLAB_ID,
    PDS_MS_MGMT_VPC_SLAB_ID,
    PDS_MS_MGMT_SUBNET_SLAB_ID,
    PDS_MS_MGMT_INTERFACE_SLAB_ID,
    PDS_MS_MGMT_MAX_SLAB_ID
};

enum class mgmt_obj_type_e {
    VPC,
    SUBNET
};

using ms_idx_t = uint32_t;

struct mgmt_obj_t {
    mgmt_obj_type_e obj_type;
    ms_idx_t         ms_id;
    pds_obj_key_t   uuid;
    mgmt_obj_t(mgmt_obj_type_e obj_type_, ms_idx_t ms_id_, const pds_obj_key_t& uuid_)
        : obj_type(obj_type_), ms_id(ms_id_), uuid(uuid_) {};
};

// Singleton that holds all global state for the mgmt stub
// Placeholder class currently as we do not have any global
// mgmt stub state yet
class mgmt_state_t {
public:
    struct context_t {
        public:    
            context_t(std::recursive_mutex& m, mgmt_state_t* s) : l_(m), state_(s)  {
                mgmt_state_locked(true, true);
            }
            mgmt_state_t* state(void) {return state_;}
            void release(void) {
                l_.unlock(); state_ = nullptr;
                mgmt_state_locked(true, false);
            }
            ~context_t() {
                mgmt_state_locked(true, false);
            }
            context_t(context_t&& c) {
                state_ = c.state_;
                c.state_ = nullptr;
                l_ = std::move(c.l_);
            }
            context_t& operator=(context_t&& c) {
                state_ = c.state_;
                c.state_ = nullptr;
                l_ = std::move(c.l_);
                return *this;
            }
        private:    
            std::unique_lock<std::recursive_mutex> l_;
            mgmt_state_t* state_;
    };
    
    void gen_random(void) {
        unsigned seed = 
            std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 rand(seed);
        epoch_ = rand();
        PDS_TRACE_DEBUG("Using random epoch: %u", epoch_);
    }

    static void create(void) { 
        SDK_ASSERT (g_state_ == nullptr);
        g_state_ = new mgmt_state_t;
        g_response_ready_ = false;
        g_state_->gen_random();    
    }

    static void destroy(void) {
        delete(g_state_); g_state_ = nullptr;
    }

    // Obtain a mutual exclusion context for thread safe access to the 
    // stub state. Automatic release when the context goes 
    // out of scope. Direct external access to Mgmt state without
    // a context is prohibited.
    static context_t thread_context(void) {
        SDK_ASSERT(g_state_ != nullptr);
        return context_t(g_state_mtx_, g_state_);
    }
    
    // Blocking function call. To be used only from the grpc thread
    // context while waiting for the async response back from nbase
    static types::ApiStatus ms_response_wait(void) {
        std::unique_lock<std::mutex> lock(g_cv_mtx_);
        g_cv_resp_.wait(lock, response_ready);
        g_response_ready_ = false;
        return g_ms_response_;
    }
    
    // Called from the response method in the nbase thread context to
    // unblock the grpc thread
    static void ms_response_ready(types::ApiStatus resp);

    void set_nbase_thread(sdk::lib::thread *tptr) {
        nbase_thread_ = tptr;
    }

    sdk::lib::thread *nbase_thread(void) {
        return nbase_thread_;
    }
    
    uint32_t epoch(void) { return epoch_; }

    bool pds_mock_mode(void) const { return pds_mock_mode_;  }
    void set_pds_mock_mode(bool val) { pds_mock_mode_ = val; }

    bool rr_mode(void) const { return rr_mode_;  }
    void set_rr_mode(bool val) { rr_mode_ = val; }

    void set_pending_uuid_create(const pds_obj_key_t& uuid, 
                                 uuid_obj_uptr_t&& obj);
    void release_pending_uuid() {
        uuid_pending_create_.clear();
        uuid_pending_delete_.clear();
    }
    void set_pending_uuid_delete(const pds_obj_key_t& uuid) {
        PDS_TRACE_DEBUG("UUID %s in pending Delete list", uuid.str());
        uuid_pending_delete_.push_back(uuid);
    }
    // Commit all pending UUID operations to permanent store
    void commit_pending_uuid();
    void remove_uuid(const pds_obj_key_t& uuid) {
        uuid_store_.erase(uuid);
    }
    uuid_obj_t* lookup_uuid(const pds_obj_key_t& uuid);
    void walk_uuid(const std::function<bool(const pds_obj_key_t&, uuid_obj_t*)>& cb_fn) {
        for (auto& obj: uuid_store_) {
            if (!cb_fn (obj.first, obj.second.get())) {break;}
        }
    }

    // Throws exception if the given VPC is not found in the UUID store
    vpc_uuid_obj_t* vpc_uuid_obj(const pds_obj_key_t& vpc_key);

    mib_idx_gen_indexer_t&  mib_indexer() {return mib_indexer_;}
    void set_rt_pending_add(const uint8_t *rt_str, rt_type_e type,
                             pend_rt_t::ms_id_t id) {
        PDS_TRACE_DEBUG("Push RT %s (type=%d, id=%d) to add list",
                         rt_str, static_cast<uint32_t>(type), id);
        rt_pending_add_.emplace_back(rt_str, type, id);
    }
    void set_rt_pending_delete(const uint8_t *rt_str, rt_type_e type,
                             pend_rt_t::ms_id_t id) {
        PDS_TRACE_DEBUG("Push RT %s (type=%d, id=%d) to del list",
                         rt_str, static_cast<uint32_t>(type), id);
        rt_pending_delete_.emplace_back(rt_str, type, id);
    }
    void clear_rt_pending() {
        rt_pending_add_.clear();
        rt_pending_delete_.clear();
    }
    vector<pend_rt_t>& get_rt_pending_add() { return rt_pending_add_;}
    vector<pend_rt_t>& get_rt_pending_delete() { return rt_pending_delete_;}
    static void redo_rt_pending(vector<pend_rt_t>&, bool);

    // bgp peer store
    bgp_peer_store_t& bgp_peer_store(void) {return bgp_peer_store_;}
    void set_bgp_peer_pend(const ip_addr_t& l, const ip_addr_t& p, bool add) {
        bgp_peer_pend_.emplace_back(l,p,add);
    }
    void clear_bgp_peer_pend(void) {
        bgp_peer_pend_.clear();
    }
    vector<bgp_peer_pend_obj_t>& get_bgp_peer_pend(void) {return bgp_peer_pend_;}
    static void redo_bgp_peer_pend(vector<bgp_peer_pend_obj_t>&);
    static std::mutex& grpc_lock() {return g_grpc_lock_;}

    bool overlay_routing_en() {return overlay_routing_en_;}
    void set_overlay_routing_en(bool enable) {overlay_routing_en_ = enable;}

    mgmt_obj_t* check_vni_match(pds_vnid_id_t vni,
                                mgmt_obj_type_e obj_type,
                                ms_idx_t ms_id,
                                const pds_obj_key_t& uuid);
    void set_vni(pds_vnid_id_t vni, mgmt_obj_type_e obj_type,
                 ms_idx_t id, const pds_obj_key_t& uuid);
    void reset_vni(pds_vnid_id_t vni);
    void set_route_map_created() {route_map_created_ = true;}
    bool route_map_created() {return route_map_created_;}
    bool is_amx_open() {return amx_open_;}
    void set_amx_open(bool open) {amx_open_ = open;}

    // should we advertise BGP Graceful restart capaility
    static bool bgp_gr_supported();
    // is hitless upgrade (graceful restart) in progress
    bool is_upg_ht_mode(void);
    bool is_upg_ht_in_progress(void);
    void set_upg_ht_start(void);
    void set_upg_ht_repeal(void);
    void set_upg_ht_restart(void) {
        upg_ht_b_starting_up_ = true;
    }
    void set_upg_ht_sync(void) {
        upg_ht_b_starting_up_ = false;
    }
    void set_upg_ht_rollback(void) {
        upg_ht_b_starting_up_ = false;
    }

private:
    static mgmt_state_t* g_state_;
    // Predicate to avoid spurious wake-up calls
    static bool g_response_ready_;
    static std::mutex g_cv_mtx_;
    static std::mutex g_grpc_lock_;
    static std::recursive_mutex g_state_mtx_;
    static std::condition_variable g_cv_resp_;
    static types::ApiStatus g_ms_response_;

    uint32_t epoch_;
    bool pds_mock_mode_ = false;
    bool rr_mode_ = false;
    std::unordered_map<pds_obj_key_t, uuid_obj_uptr_t, pds_obj_key_hash> uuid_store_;
    std::unordered_map<pds_obj_key_t, uuid_obj_uptr_t, pds_obj_key_hash> uuid_pending_create_;
    std::vector<pds_obj_key_t> uuid_pending_delete_;
    std::vector<pend_rt_t> rt_pending_add_;
    std::vector<pend_rt_t> rt_pending_delete_;
    mib_idx_gen_indexer_t mib_indexer_;
    slab_uptr_t slabs_ [PDS_MS_MGMT_MAX_SLAB_ID];
    bgp_peer_store_t  bgp_peer_store_;
    std::vector<bgp_peer_pend_obj_t> bgp_peer_pend_;
    bool overlay_routing_en_ = false;
        
    std::unordered_map<pds_vnid_id_t, mgmt_obj_t> vni_store_; 
    bool route_map_created_ = false;
    bool amx_open_ = false;
    bool upg_ht_a_going_down_ = false;
    bool upg_ht_b_starting_up_ = false;
    // gRPC lock for hitless upgrade
    std::unique_lock<std::mutex> upg_ht_grpc_lock_;

private:
    mgmt_state_t(void);
    static bool response_ready() {
        return g_response_ready_;
    }
    // Store the nbase thread ptr. Used to set thread ready upon nbase
    // init completion
    sdk::lib::thread *nbase_thread_;
};

class mgmt_uuid_guard_t {
public:
    ~mgmt_uuid_guard_t() {
        auto mgmt_ctxt = mgmt_state_t::thread_context();
        mgmt_ctxt.state()->release_pending_uuid();
    }
};

bool mgmt_check_vni(pds_vnid_id_t vni, mgmt_obj_type_e obj_type,
                    ms_idx_t ms_id, const pds_obj_key_t& uuid);
static inline
void mgmt_set_vni(pds_vnid_id_t vni, mgmt_obj_type_e obj_type,
                  ms_idx_t ms_id, const pds_obj_key_t& uuid) {
    auto mgmt_ctxt = mgmt_state_t::thread_context();
    mgmt_ctxt.state()->set_vni(vni, obj_type, ms_id, uuid);
}

static inline
void mgmt_reset_vni(pds_vnid_id_t vni) {
    auto mgmt_ctxt = mgmt_state_t::thread_context();
    mgmt_ctxt.state()->reset_vni(vni);
}

// Function prototypes
bool  mgmt_state_init(void);
void  mgmt_state_destroy(void);

}

#endif    // __PDS_MS_MGMT_STATE_HPP__
