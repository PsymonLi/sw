//-------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS propagate ORFs received from DSC at RR to upstream peers
//--------------------------------------------------------------------

#include "nic/metaswitch/stubs/mgmt/pds_ms_bgp_orf.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_utils.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_ctm.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_tbl_idx.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_bgp_orf_ue.h"
#include "nic/metaswitch/stubs/mgmt/pds_ms_rr_worker.hpp"
#include "nic/metaswitch/stubs/mgmt/gen/svc/internal_gen.hpp"
#include "nic/metaswitch/stubs/mgmt/gen/mgmt/pds_ms_internal_utils_gen.hpp"

namespace pds_ms {

enum class bgp_orf_job_type_e {
    ADD,
    DEL,
    REMOVE_ALL,
    APPLY,
    NBR_DOWN
};

struct bgp_orf_job_cmn_t
{
    bgp_orf_job_cmn_t(bgp_orf_job_type_e t, const bgp_peer_t& p)
        : type(t), peer(p) {};
    bgp_orf_job_type_e type;
    bgp_peer_t peer;
};

void bgp_orf_handle_update(rr_worker_job_t* job);
void bgp_orf_handle_apply(rr_worker_job_t* job);
void bgp_orf_handle_nbr_down(rr_worker_job_t* job);

class bgp_orf_upd_job_t : public rr_worker_job_t
{
public:    
    bgp_orf_upd_job_t(const bgp_peer_t& p, const QB_EXTENDED_COMMUNITY* excm,
                      bgp_orf_job_type_e t)
        : cmn(t, p), ext_comm(*excm) {};

    bgp_orf_upd_job_t(const bgp_peer_t& p, bgp_orf_job_type_e t)
        : cmn(t, p) {};

    bgp_orf_job_cmn_t cmn;
    QB_EXTENDED_COMMUNITY ext_comm;

    rr_worker_job_hdlr_t hdlr_cb() {
        switch (cmn.type) {
        // append updates to a pending batch
        case bgp_orf_job_type_e::ADD:
        case bgp_orf_job_type_e::DEL:
        case bgp_orf_job_type_e::REMOVE_ALL:
            return bgp_orf_handle_update;
        // apply the pending batched updates
        case bgp_orf_job_type_e::APPLY:
            return bgp_orf_handle_apply;
        // neighbor down - flush all
        case bgp_orf_job_type_e::NBR_DOWN:
            return bgp_orf_handle_nbr_down;
        }
        return nullptr;
    }
};

static void
route_map_upd_txn_ (void)
{
    uint correlator = PDS_MS_CTM_GRPC_CORRELATOR;

    if (!mgmt_state_t::thread_context().state()->route_map_created()) {
        PDS_TRACE_DEBUG("No upstream peer on RR yet, ignore ORF");
        return;
    }
    // only 1 thread can issue MS MIB requests at a given time
    std::lock_guard<std::mutex> lck(pds_ms::mgmt_state_t::grpc_lock());

    PDS_MS_START_TXN(correlator);
    bgp_route_map_upd_rr(correlator);
    PDS_MS_END_TXN(correlator);

    auto ret = pds_ms::mgmt_state_t::ms_response_wait();

    if (ret != types::API_STATUS_OK) {
        PDS_TRACE_ERR("apply BGP ORF failed in configuring route-map in MS %s",
                       pds_ms_api_ret_str(ret));
        return;
    }
    PDS_TRACE_DEBUG("applied BGP ORF successfully");
}

// Handle updates to ORF received from downstream RR clients
// Append them to this BGP peer's pending batch
void
bgp_orf_handle_update (rr_worker_job_t* job)
{
    auto mgmt_thread_ctxt = mgmt_state_t::thread_context();
    auto mgmt_state = mgmt_thread_ctxt.state();

    auto upd_job = dynamic_cast<bgp_orf_upd_job_t*>(job);
    auto& peer = upd_job->cmn.peer;

    if (!mgmt_state->bgp_peer_store().rrclient(peer.local_addr,
                                               peer.peer_addr)) {
        // ignore ORFs received from upstream
        PDS_TRACE_DEBUG("Ignore BGP ORF update received from upstream peer %s",
                        peer.str().c_str());
        return;
    }
    auto& peer_orfs = mgmt_state->orf_rr.peers[peer];

    PDS_TRACE_DEBUG("BGP ORF peer %s handle update", peer.str().c_str());

    switch (upd_job->cmn.type) {
    case bgp_orf_job_type_e::ADD:
    {
        ms_rt_t rt(upd_job->ext_comm.value);
        pds_ms::ms_rt_t allzero;
        allzero.reset();

        if (rt == allzero) {
            peer_orfs.pend_add.clear();
            peer_orfs.pend_del = peer_orfs.committed;
            PDS_TRACE_DEBUG("Batch append - all-zero RT, flush %ld prev RTs",
                            peer_orfs.pend_del.size());
        } else {
            auto it = peer_orfs.committed.find(rt);
            if (it == peer_orfs.committed.end()) {
                PDS_TRACE_DEBUG("Batch append - RT %s add by BGP peer %s",
                                rt.str(), peer.str().c_str());
                peer_orfs.pend_add.insert(rt);
            }
            peer_orfs.pend_del.erase(rt);
        }
        break;
    }
    case bgp_orf_job_type_e::DEL:
    {
        ms_rt_t rt(upd_job->ext_comm.value);
        auto it = peer_orfs.committed.find(rt);

        if (it != peer_orfs.committed.end()) {
            PDS_TRACE_DEBUG("Batch append - RT %s del by BGP peer %s",
                            rt.str(), peer.str().c_str());
            peer_orfs.pend_del.insert(rt);
        }
        peer_orfs.pend_add.erase(rt);
        break;
    }
    case bgp_orf_job_type_e::REMOVE_ALL:
        peer_orfs.pend_add.clear();
        peer_orfs.pend_del = peer_orfs.committed;
        PDS_TRACE_DEBUG("Batch append - remove-all, flush %ld prev RTs",
                        peer_orfs.pend_del.size());
        break;
    case bgp_orf_job_type_e::NBR_DOWN:
    case bgp_orf_job_type_e::APPLY:
        // these operations have their own handler functions
        SDK_ASSERT(0);
        break;
    }
}

// Apply pending batched updates for given BGP peer
void
bgp_orf_handle_apply (rr_worker_job_t* job)
{
    try {
        bool orf_chg = false;

        // derive final orf list from list of pending rt updates
        {
            auto upd_job = dynamic_cast<bgp_orf_upd_job_t*>(job);
            auto mgmt_thread_ctxt = mgmt_state_t::thread_context();
            auto mgmt_state = mgmt_thread_ctxt.state();

            auto& peer = upd_job->cmn.peer;
            auto& peers = mgmt_state->orf_rr.peers;
            auto& peer_orfs = peers[peer];
            auto& orfs = mgmt_state->orf_rr.orfs;

            PDS_TRACE_DEBUG("BGP ORF peer %s handle apply", peer.str().c_str());

            if (peer_orfs.pend_add.empty() && peer_orfs.pend_del.empty()) {
                return;
            }
            for (auto& rt: peer_orfs.pend_add) {
                PDS_TRACE_DEBUG("RT %s add peer %s", rt.str(),
                                peer.str().c_str());
                auto it = orfs.find(rt);
                if (it == orfs.end()) {
                    // first BGP peer advertising new RT - add to route-map
                    orf_chg = true;
                    PDS_TRACE_INFO("++++ Add RT %s to route-map on RR",
                                   rt.str());
                }
                orfs[rt].insert(peer);
                peer_orfs.committed.insert(rt);
            }
            for (auto& rt: peer_orfs.pend_del) {
                auto it = orfs.find(rt);
                if (it == orfs.end()) {
                    SDK_ASSERT(0);
                }
                PDS_TRACE_DEBUG("RT %s remove peer %s", rt.str(),
                                peer.str().c_str());
                orfs[rt].erase(peer);
                if (orfs[rt].empty()) {
                    // last BGP peer withdrew RT - remove from route-map
                    orfs.erase(rt);
                    orf_chg = true;
                    PDS_TRACE_INFO("++++ Remove RT %s from route-map on RR",
                                   rt.str());
                }
                peer_orfs.committed.erase(rt);
            }
            peer_orfs.pend_add.clear();
            peer_orfs.pend_del.clear();
            if (peer_orfs.committed.empty()) {
                PDS_TRACE_DEBUG("Cleanup all ORFs on peer %s",
                                peer.str().c_str());
                peers.erase(peer);
            }
        }
        if (orf_chg) {
            // configure route map if there is change in ORF
            route_map_upd_txn_();
        }
    } catch (const pds_ms::Error& e) {
        PDS_TRACE_ERR("BGP ORF RR apply failed %s", e.what());
    }
}

// BGP session down - flush all RTs received from this neighbor immediately
void
bgp_orf_handle_nbr_down (rr_worker_job_t* job)
{
    auto upd_job = dynamic_cast<bgp_orf_upd_job_t*>(job);
    try {
        bool orf_chg = false;
        {
            auto mgmt_thread_ctxt = mgmt_state_t::thread_context();
            auto mgmt_state = mgmt_thread_ctxt.state();

            auto& peer = upd_job->cmn.peer;
            auto& peers = mgmt_state->orf_rr.peers;
            auto& peer_orfs = peers[peer];
            auto& orfs = mgmt_state->orf_rr.orfs;

            PDS_TRACE_DEBUG("BGP ORF peer %s handle neighbor down",
                            peer.str().c_str());

            for (auto& rt: peer_orfs.committed) {
                auto it = orfs.find(rt);
                if (it == orfs.end()) {
                    SDK_ASSERT(0);
                }
                orfs[rt].erase(peer);
                if (orfs[rt].empty()) {
                    // last BGP peer removed - remove from route-map
                    orfs.erase(rt);
                    PDS_TRACE_INFO("++++ Remove RT %s from route-map on RR",
                                   rt.str());
                    orf_chg = true;
                }
            }
            peer_orfs.committed.clear();
            peers.erase(peer);
        }
        if (orf_chg) {
            // configure route map if there is change in ORF
            route_map_upd_txn_();
        }
    } catch (const pds_ms::Error& e) {
        PDS_TRACE_ERR("BGP ORF RR remove-all failed %s", e.what());
    }
}

} // end namespace

// User Exit function to track the reception of a extended community
// type ORF entry
void
pen_qbrm_user_orf_ext_comm_type (const ATG_INET_ADDRESS *local_addr,
                                 const ATG_INET_ADDRESS *remote_addr,
                                 const QB_EXTENDED_COMMUNITY* ext_comm_val,
                                 NBB_BYTE add_or_remove)
{
    if (!pds_ms::mgmt_state_t::rr_mode()) {
        return;
    }
    auto upd_type = (add_or_remove == QB_ORF_ADD) ?
        pds_ms::bgp_orf_job_type_e::ADD : pds_ms::bgp_orf_job_type_e::DEL;
    pds_ms::bgp_peer_t peer(local_addr, remote_addr);

    PDS_TRACE_DEBUG("BGP ORF peer %s user-exit ext-comm update",
                    peer.str().c_str());
    pds_ms::rr_worker_job_uptr_t jobp
        (new pds_ms::bgp_orf_upd_job_t(peer, ext_comm_val, upd_type));
    pds_ms::mgmt_state_t::rr_worker()->add_job(std::move(jobp));
}

// User Exit function called when the BGP session with a neighbor goes down
void
pen_qbrm_user_nbr_session_down (const ATG_INET_ADDRESS *local_addr,
                                const ATG_INET_ADDRESS *remote_addr)
{
    if (!pds_ms::mgmt_state_t::rr_mode()) {
        return;
    }
    pds_ms::bgp_peer_t peer(local_addr, remote_addr);

    PDS_TRACE_DEBUG("BGP ORF peer %s user-exit neighbor down",
                    peer.str().c_str());
    pds_ms::rr_worker_job_uptr_t jobp
        (new pds_ms::bgp_orf_upd_job_t(peer,
                                       pds_ms::bgp_orf_job_type_e::NBR_DOWN));
    pds_ms::mgmt_state_t::rr_worker()->add_job(std::move(jobp));
}

// User Exit function to track the reception an ORF with the REMOVE-ALL
// action being set
void
pen_qbrm_user_orf_remove_all (const ATG_INET_ADDRESS *local_addr,
                              const ATG_INET_ADDRESS *remote_addr)
{
    if (!pds_ms::mgmt_state_t::rr_mode()) {
        return;
    }
    pds_ms::bgp_peer_t peer(local_addr, remote_addr);

    PDS_TRACE_DEBUG("BGP ORF peer %s user-exit remove-all update",
                    peer.str().c_str());
    pds_ms::rr_worker_job_uptr_t jobp
        (new pds_ms::bgp_orf_upd_job_t(peer,
                                       pds_ms::bgp_orf_job_type_e::REMOVE_ALL));
    pds_ms::mgmt_state_t::rr_worker()->add_job(std::move(jobp));
}

// User Exit function indicating that the ORF policy can now be applied
// for this neighbor/prefix type.
void
pen_qbrm_user_orf_apply (const ATG_INET_ADDRESS *local_addr,
                         const ATG_INET_ADDRESS *remote_addr)
{
    if (!pds_ms::mgmt_state_t::rr_mode()) {
        return;
    }
    pds_ms::bgp_peer_t peer(local_addr, remote_addr);

    PDS_TRACE_DEBUG("BGP ORF peer %s user-exit apply", peer.str().c_str());
    pds_ms::rr_worker_job_uptr_t jobp
        (new pds_ms::bgp_orf_upd_job_t(peer,
                                       pds_ms::bgp_orf_job_type_e::APPLY));
    pds_ms::mgmt_state_t::rr_worker()->add_job(std::move(jobp));
}
