//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/vmotion/vmotion.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/include/hal_mem.hpp"
#include "nic/hal/core/core.hpp"
#include "nic/hal/iris/delphi/delphi_events.hpp"
#include <malloc.h>

namespace hal {

vmotion_dst_host_fsm_def* vmotion::dst_host_fsm_def_ = vmotion_dst_host_fsm_def::factory();
vmotion_src_host_fsm_def* vmotion::src_host_fsm_def_ = vmotion_src_host_fsm_def::factory();

hal_ret_t
vmotion_init (int vmotion_port)
{
    vmotion *vmn = g_hal_state->get_vmotion();

    if (vmn) {
        HAL_TRACE_ERR("vmotion already initialized port:{} vmn:{:p}", vmotion_port, (void *)vmn);
        return HAL_RET_OK;
    }

    HAL_TRACE_INFO("vmotion init port:{}", vmotion_port);

    vmn = vmotion::factory(VMOTION_MAX_THREADS, (vmotion_port ? vmotion_port : VMOTION_PORT));

    g_hal_state->set_vmotion(vmn);

    return HAL_RET_OK;
}

hal_ret_t
vmotion_deinit ()
{
    vmotion *vmn = g_hal_state->get_vmotion();

    HAL_TRACE_INFO("vmotion deinit vmn:{:p}", (void *)vmn);

    if (!vmn) {
        return HAL_RET_OK;
    }
    // Reset the vMotion pointer in HAL Global
    g_hal_state->set_vmotion(NULL);

    // vMotion destroy
    vmotion::destroy(vmn);
    return HAL_RET_OK;
}
//-----------------------------------------------------------------------------
// spawn vmotion thread
//-----------------------------------------------------------------------------
vmotion *
vmotion::factory(uint32_t max_threads, uint32_t port)
{
    vmotion     *vmn;
    void        *mem = (vmotion *) g_hal_state->vmotion_slab()->alloc();

    if (!mem) {
        HAL_TRACE_ERR("OOM failed to allocate memory for vmotion");
        return NULL;
    }

    vmn = new (mem) vmotion();
    if (vmn->init(max_threads, port) != HAL_RET_OK) {
        return NULL;
    }

    return vmn;
}

void
vmotion::destroy(vmotion *vmn)
{
    // deinit
    hal_ret_t ret = vmn->deinit();

    if (ret != HAL_RET_RETRY) {
        hal::delay_delete_to_slab(HAL_SLAB_VMOTION, vmn);
    }
    HAL_TRACE_INFO("vMotion Destroy. Ret:{}", ret);
}

static void
vmotion_delay_destroy_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    vmotion *vmn = (vmotion *)ctxt;
    if (vmn->delay_deinit() != HAL_RET_RETRY) {
        hal::delay_delete_to_slab(HAL_SLAB_VMOTION, vmn);
    }
}

void
vmotion::vmotion_schedule_delay_destroy(void)
{
    if (sdk::lib::timer_schedule(HAL_TIMER_ID_THREAD_DELAY_DEL, VMOTION_DESTROY_DELAY_TIME,
                                 reinterpret_cast<void *>(this),
                                 vmotion_delay_destroy_cb, false) == NULL) {
        HAL_TRACE_ERR("vMotion error in starting delay destroy timer");
    }
}

hal_ret_t
vmotion::init(uint32_t max_threads, uint32_t vmotion_port)
{
    vmotion_.max_threads     = max_threads;
    vmotion_.port            = vmotion_port;

    // Init TLS Context
    tls_context_ = TLSContext::factory();
    if (tls_context_ == NULL) {
        HAL_TRACE_ERR("vMotion TLS Init failed");
        return HAL_RET_ERR;
    }

    vmotion_.threads_idxr    = rte_indexer::factory(max_threads, false, false);
    SDK_ASSERT(vmotion_.threads_idxr != NULL);

    // spawn master server thread
    vmotion_.vmotion_master =
        sdk::event_thread::event_thread::factory("vmn-mas", HAL_THREAD_ID_VMOTION,
                                                 sdk::lib::THREAD_ROLE_CONTROL,
                                                 0x0,    // use all control cores
                                                 master_thread_init,  // Thread Init Fn
                                                 master_thread_exit,  // Thread Exit Fn
                                                 NULL,  // Thread Event CB
                                                 sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                                                 sdk::lib::thread::sched_policy_by_role(sdk::lib::THREAD_ROLE_CONTROL),
                                                 THREAD_FLAGS_NONE);

    // Start the master thread
    vmotion_.vmotion_master->start(this);

    return HAL_RET_OK;
}

hal_ret_t
vmotion::deinit()
{
    std::vector<vmotion_ep_dbg_t *>::iterator it = vmotion_dbg_history_.begin();

    HAL_TRACE_INFO("vMotion Deinit. Active VMN_EPs:{}", vmn_eps_.size());

    // Exit master thread
    vmotion_.vmotion_master->stop();

    // Free TLS Context
    TLSContext::destroy(tls_context_);
    tls_context_ = NULL;

    // There is a possibility that, there could be some active vMotion going on.
    // Then post event to those vMotion threads to exit
    if (!vmn_eps_.empty()) {

        std::vector<uint32_t> thrd_ids;
        for (auto vmn_ep : vmn_eps_) {
            thrd_ids.push_back(vmn_ep->get_thread_id());
        }

        for (auto thrd_id : thrd_ids) {
            // Destroy vMotion EP
            vmotion_thread_evt_t event = VMOTION_EVT_EP_MV_ABORT;

            sdk::event_thread::message_send(thrd_id, (void *)event);
        }
        // Till vMotion threads close, delay the vMotion pointer cleanup
        vmotion_schedule_delay_destroy();

        return HAL_RET_RETRY;
    }

    // Free vMotion debug history
    while (it != vmotion_dbg_history_.end()) {
        HAL_FREE(HAL_MEM_ALLOC_VMOTION, *it);
        it = vmotion_dbg_history_.erase(it);
    }

    // Free threads indexer
    rte_indexer::destroy(vmotion_.threads_idxr);
    vmotion_.threads_idxr = NULL;

    return HAL_RET_OK;
}

hal_ret_t
vmotion::delay_deinit()
{
    std::vector<vmotion_ep_dbg_t *>::iterator it = vmotion_dbg_history_.begin();

    HAL_TRACE_INFO("vMotion Delay Deinit Attempt:{}", delay_del_attempt_cnt_);

    if (!vmn_eps_.empty() && delay_del_attempt_cnt_ < VMOTION_DELAY_DEL_MAX_ATTEMPT) {
        delay_del_attempt_cnt_++;
        vmotion_schedule_delay_destroy();
        return HAL_RET_RETRY;
    }

    // Destroy EPs
    for (std::vector<vmotion_ep *>::iterator vmn_ep = vmn_eps_.begin();
         vmn_ep != vmn_eps_.end(); vmn_ep++) {
        vmotion_ep::destroy(*vmn_ep);
    }

    // Free vMotion debug history
    while (it != vmotion_dbg_history_.end()) {
        HAL_FREE(HAL_MEM_ALLOC_VMOTION, *it);
        it = vmotion_dbg_history_.erase(it);
    }

    // Free threads indexer
    rte_indexer::destroy(vmotion_.threads_idxr);
    vmotion_.threads_idxr = NULL;

    return HAL_RET_OK;
}

hal_ret_t
vmotion::alloc_thread_id(uint32_t *tid)
{
    hal_ret_t ret = HAL_RET_OK;

    // allocate tid
    VMOTION_WLOCK
    if (vmotion_.threads_idxr->alloc(tid) != SDK_RET_OK) {
        HAL_TRACE_ERR("unable to allocate thread id Size:{} Usage:{}",
                      vmotion_.threads_idxr->size(), vmotion_.threads_idxr->usage());
        ret = HAL_RET_ERR;
    }
    VMOTION_WUNLOCK

    (*tid) += HAL_THREAD_ID_VMOTION_THREADS_MIN;

    return ret;
}

hal_ret_t
vmotion::release_thread_id(uint32_t tid)
{
    hal_ret_t ret = HAL_RET_OK;

    tid -= HAL_THREAD_ID_VMOTION_THREADS_MIN;

    // free tid
    VMOTION_WLOCK
    if (vmotion_.threads_idxr->free(tid) != SDK_RET_OK) {
        HAL_TRACE_ERR("unable to free thread id: {}", tid);
        ret = HAL_RET_ERR;
    }
    VMOTION_WUNLOCK

    return ret;
}

void
vmotion::master_thread_init(void *ctxt)
{
    vmotion       *vmn = (vmotion *)ctxt;
    struct sockaddr_in address;
    int                master_socket;
    int                opt = true;

    // Set thread detached
    pthread_detach(pthread_self());

    // create master socket
    master_socket = socket(AF_INET , SOCK_STREAM , 0);

    // set master socket to allow multiple connections
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // configure address settings
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(vmn->get_vmotion_port());

    // bind socket to localhost:port
    bind(master_socket, (struct sockaddr *) &address, sizeof(address));

    // listen
    if (listen(master_socket, vmn->get_max_threads()) != 0) {
        // close socket
        HAL_TRACE_ERR("vmotion: Master thread listen failed");
        close(master_socket);
        return;
    }

    vmn->get_master_thread_io()->ctx = ctxt;
    vmn->set_master_sock(master_socket);

    // initialize and start a watcher to accept client requests
    sdk::event_thread::io_init(vmn->get_master_thread_io(), master_thread_cb,
                               master_socket, EVENT_READ);
    sdk::event_thread::io_start(vmn->get_master_thread_io());
}

void
vmotion::master_thread_exit(void *ctxt)
{
    vmotion       *vmn = (vmotion *)ctxt;

    HAL_TRACE_INFO("Master vMotion thread thread exit");

    vmotion::delay_delete_thread(vmn->get_master_event_thrd());

    close(vmn->get_master_sock());
}

// call back when a new client is trying to connect.
// - create a new thread.
// - pass the thread object and fd to the new thread.
void
vmotion::master_thread_cb(sdk::event_thread::io_t *io, int sock_fd, int events)
{
    vmotion  *vmn = (vmotion *)io->ctx;

    if (EV_ERROR & events) {
        HAL_TRACE_ERR("invalid event.");
        return;
    }
    vmn->spawn_src_host_thread(sock_fd);
}

void
vmotion::run_vmotion(ep_t *ep, vmotion_thread_evt_t event)
{
    vmotion_ep *vmn_ep = get_vmotion_ep(ep);

    if ((!vmn_ep) && (event == VMOTION_EVT_EP_MV_ABORT)) {
        return;
    }

    HAL_TRACE_INFO("vMotion: vmn_ep:{:p} event:{}", (void *)vmn_ep, event);

    if (event == VMOTION_EVT_EP_MV_COLD) {
        // vMotion is happening from a non-pensando device to locally here.
        // We do - Disable TCP Normalization for the EP that migrated
        vmotion_ep_migration_normalization_cfg(ep, true, false);
        return;
    }

    if (event == VMOTION_EVT_EP_MV_START) {
        if (!vmn_ep) {
            vmn_ep = create_vmotion_ep(ep, VMOTION_TYPE_MIGRATE_IN);

            if (!vmn_ep) {
                // vMotion creation fails, possibly due to resources full.
                // Create Normalization entry to make the active sessions to work. 
                vmotion_ep_migration_normalization_cfg(ep, true, false);
                return;
            }
        } else {
            HAL_TRACE_ERR("vmotion ep already exists");
        }
    }

    if (vmn_ep) {
        sdk::event_thread::message_send(vmn_ep->get_thread_id(), (void *)event);
    }
}

vmotion_ep *
vmotion::get_vmotion_ep(ep_t *ep)
{
    auto it = std::find_if(vmn_eps_.begin(), vmn_eps_.end(), [ep](vmotion_ep *e) {
            return (e->get_ep() == ep); });
    return (it == vmn_eps_.end() ? NULL : *it);
}

vmotion_ep *
vmotion::create_vmotion_ep(ep_t *ep, ep_vmotion_type_t type)
{
    auto vmn_ep = vmotion_ep::factory(ep, type);

    if (!vmn_ep) {
        return NULL;
    }

    VMOTION_WLOCK
    vmn_eps_.push_back(vmn_ep);
    VMOTION_WUNLOCK

    return vmn_ep;
}

hal_ret_t
vmotion::delete_vmotion_ep(vmotion_ep *vmn_ep)
{
    // If entry exists in vmn_eps_, delete it
    VMOTION_WLOCK
    auto it = std::find(vmn_eps_.begin(), vmn_eps_.end(), vmn_ep);
    if (it != vmn_eps_.end()) {
        // Delete the from the EP List
        vmn_eps_.erase(it);
    }
    VMOTION_WUNLOCK

    // Record vMotion end time
    vmn_ep->debug_record_time(&vmotion_ep_dbg_t::end_time);

    vmotion_dbg_history_entry_add(vmn_ep);

    // Destroy EP
    vmotion_ep::destroy(vmn_ep);

    // Malloc holds lot of free memory.  By trimming the memory the memory is released back
    // used this in cases where we free memory in large blocks
    malloc_trim(0);

    return HAL_RET_OK;
}

hal_ret_t
vmotion::vmotion_handle_ep_del(ep_t *ep)
{
    if (!VMOTION_IS_ENABLED()) {
        return HAL_RET_OK;
    }
    if (ep->vmotion_state == MigrationState::IN_PROGRESS) {
        // Loop the sessions, and start aging timer if any sync sessions received
        endpoint_migration_done(ep, ep->vmotion_state);
    }

    run_vmotion(ep, VMOTION_EVT_EP_MV_ABORT);

    return HAL_RET_OK;
}

vmotion_ep *
vmotion_ep::factory(ep_t *ep, ep_vmotion_type_t type)
{
    vmotion_ep *mem = (vmotion_ep *)g_hal_state->vmotion_ep_slab()->alloc();
    if (!mem) {
        HAL_TRACE_ERR("OOM failed to allocate memory for vmotion ep");
        return NULL;
    }
    vmotion_ep* vmn_ep = new (mem) vmotion_ep();

    if (vmn_ep->init(ep, type) != HAL_RET_OK) {
        vmn_ep->get_vmotion()->delete_vmotion_ep(vmn_ep);
        return NULL;
    }
    return vmn_ep;
}

void
vmotion_ep::destroy(vmotion_ep *vmn_ep)
{
    // Free the state machine
    delete vmn_ep->get_sm();
    // Free the memory
    hal::delay_delete_to_slab(HAL_SLAB_VMOTION_EP, vmn_ep);
}

hal_ret_t
vmotion_ep::init(ep_t *ep, ep_vmotion_type_t type)
{
    hal_ret_t ret = HAL_RET_OK;

    if ((type != VMOTION_TYPE_MIGRATE_IN) && (type != VMOTION_TYPE_MIGRATE_OUT)) {
        return HAL_RET_ERR;
    }

    vmotion_ptr_        = g_hal_state->get_vmotion();
    vmotion_type_       = type;
    ep_hdl_             = ep->hal_handle;
    old_homing_host_ip_ = ep->old_homing_host_ip;
    memcpy(&mac_, &ep->l2_key.mac_addr, ETH_ADDR_LEN);

    ep->vmotion_type = type;

    if (type == VMOTION_TYPE_MIGRATE_IN) {
        sm_ = new fsm_state_machine_t(get_vmotion()->get_dst_host_fsm_def_func,
                                      STATE_DST_HOST_INIT,
                                      STATE_DST_HOST_END,
                                      STATE_DST_HOST_END,
                                      STATE_DST_HOST_END,
                                      (fsm_state_ctx)this,
                                      NULL);

        ret = spawn_dst_host_thread();
    } else {
        sm_ = new fsm_state_machine_t(get_vmotion()->get_src_host_fsm_def_func,
                                      STATE_SRC_HOST_INIT,
                                      STATE_SRC_HOST_END,
                                      STATE_SRC_HOST_END,
                                      STATE_SRC_HOST_END,
                                      (fsm_state_ctx)this,
                                      NULL);
    }

    // Stats
    get_vmotion()->incr_stats(&vmotion_stats_t::total_vmotion);
    if (type == VMOTION_TYPE_MIGRATE_IN) {
        get_vmotion()->incr_stats(&vmotion_stats_t::mig_in_vmotion);
    } else {
        get_vmotion()->incr_stats(&vmotion_stats_t::mig_out_vmotion);
    }

    // Record vMotion start time
    debug_record_time(&vmotion_ep_dbg_t::start_time);

    if ((type == VMOTION_TYPE_MIGRATE_IN) && (ret != HAL_RET_OK)) {
        dst_host_end_thread_crt_failure (this);
    }

    return ret;
}

// This function returns TRUE, if packet will be handled by vMotion.
bool
vmotion::process_rarp(mac_addr_t mac)
{
    if (!VMOTION_IS_ENABLED()) {
        HAL_TRACE_VERBOSE("Ignore vMotion event. vMotion not enabled");
        return false;
    }

    auto  ep = find_ep_by_mac(mac);
    if ((!ep) || (ep->vmotion_state != MigrationState::IN_PROGRESS)) {
        HAL_TRACE_VERBOSE("Ignoring RARP Packet. EP {} ptr {:p} vmotion state {}",
                          macaddr2str(mac), (void *)ep, (ep ? ep->vmotion_state : 0));
        return false;
    }

    run_vmotion(ep, VMOTION_EVT_RARP_RCVD);

    return true;
}

void
vmotion_ep::set_last_sync_time() {
    timespec_t  ctime;

    // get current time
    clock_gettime(CLOCK_MONOTONIC, &ctime);

    // Set the time
    sdk::timestamp_to_nsecs(&ctime, &last_sync_time_);
}

void
vmotion_ep::incr_dbg_cnt(uint32_t vmotion_ep_dbg_t::* field, uint32_t count)
{
    vmotion_ep_dbg_.*field += count;
}

void
vmotion_ep::debug_record_time(timespec_t vmotion_ep_dbg_t::* field)
{
    clock_gettime(CLOCK_REALTIME, &(vmotion_ep_dbg_.*field));
}

static bool
vmotion_endpoint_dump (void *ht_entry, void *ctxt)
{
    hal_handle_id_ht_entry_t       *entry = (hal_handle_id_ht_entry_t *)ht_entry;
    internal::VmotionDebugResponse *resp = (internal::VmotionDebugResponse *)ctxt;
    ep_t                           *ep = (ep_t *)hal_handle_get_obj(entry->handle_id);
    internal::VmotionActiveEp      *ep_rsp;

    if (ep->vmotion_state != MigrationState::NONE) {
        ep_rsp = resp->add_active_ep();

        ep_rsp->set_mac_address(macaddr2str(ep->l2_key.mac_addr));
        ep_rsp->set_migration_state(ep->vmotion_state);
        ep_rsp->set_useg_vlan(ep->useg_vlan);
    }

    // Always return false here, so that we walk through all hash table entries.
    return false;
}

void
vmotion_ep::populate_vmotion_ep_dump (internal::VmotionDebugEp *rsp)
{
    char      timebuf[30];

    auto ep = get_ep();

    rsp->set_mac_address(macaddr2str(mac_));
    rsp->set_old_homing_host_ip(ipv4addr2str(old_homing_host_ip_.addr.v4_addr));
    rsp->set_migration_type(vmotion_type_);
    rsp->set_vmotion_state(ep ? ep->vmotion_state : 0);
    rsp->set_flags(flags_);
    rsp->set_state(sm_->get_state());
    rsp->set_sync_cnt(vmotion_ep_dbg_.sync_cnt);
    rsp->set_term_sync_cnt(vmotion_ep_dbg_.term_sync_cnt);
    rsp->set_sync_sess_cnt(vmotion_ep_dbg_.sync_sess_cnt);
    rsp->set_term_sync_sess_cnt(vmotion_ep_dbg_.term_sync_sess_cnt);

    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S",
             localtime(&vmotion_ep_dbg_.start_time.tv_sec));
    rsp->set_start_time(timebuf);
    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S",
             localtime(&vmotion_ep_dbg_.term_sync_time.tv_sec));
    rsp->set_term_sync_time(timebuf);
}

void
vmotion::populate_vmotion_history_dump (internal::VmotionDebugEp *rsp, vmotion_ep_dbg_t *entry)
{
    char      timebuf[30];

    rsp->set_mac_address(macaddr2str(entry->ep_mac));
    rsp->set_old_homing_host_ip(ipv4addr2str(entry->old_homing_host_ip.addr.v4_addr));
    rsp->set_migration_type(entry->vmotion_type);
    rsp->set_vmotion_state(entry->vmotion_state);
    rsp->set_flags(entry->flags);
    rsp->set_state(entry->sm_state);
    rsp->set_sync_cnt(entry->sync_cnt);
    rsp->set_term_sync_cnt(entry->term_sync_cnt);
    rsp->set_sync_sess_cnt(entry->sync_sess_cnt);
    rsp->set_term_sync_sess_cnt(entry->term_sync_sess_cnt);

    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S",
             localtime(&entry->start_time.tv_sec));
    rsp->set_start_time(timebuf);
    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S",
             localtime(&entry->term_sync_time.tv_sec));
    rsp->set_term_sync_time(timebuf);
    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S",
             localtime(&entry->end_time.tv_sec));
    rsp->set_end_time(timebuf);
}


void
vmotion::populate_vmotion_dump (internal::VmotionDebugResponse *rsp)
{
    rsp->set_vmotion_enable(1);

    for (auto e : vmotion_dbg_history_) {
        populate_vmotion_history_dump(rsp->add_history_ep(), e);
    }

    for (std::vector<vmotion_ep *>::iterator ep = vmn_eps_.begin(); ep != vmn_eps_.end(); ep++) {
        (*ep)->populate_vmotion_ep_dump(rsp->add_ep());
    }

    g_hal_state->ep_l2_ht()->walk(vmotion_endpoint_dump, rsp);

    rsp->mutable_stats()->set_total_vmotion(stats_.total_vmotion);
    rsp->mutable_stats()->set_mig_in_vmotion(stats_.mig_in_vmotion);
    rsp->mutable_stats()->set_mig_out_vmotion(stats_.mig_out_vmotion);
    rsp->mutable_stats()->set_mig_success(stats_.mig_success);
    rsp->mutable_stats()->set_mig_failed(stats_.mig_failed);
    rsp->mutable_stats()->set_mig_aborted(stats_.mig_aborted);
    rsp->mutable_stats()->set_mig_timeout(stats_.mig_timeout);
    rsp->mutable_stats()->set_mig_cold(stats_.mig_cold);
}

void
vmotion::incr_migration_state_stats(MigrationState state)
{
    VMOTION_WLOCK
    if (state == MigrationState::SUCCESS) {
        incr_stats(&vmotion_stats_t::mig_success);
    } else if (state == MigrationState::FAILED) {
        incr_stats(&vmotion_stats_t::mig_failed);
    } else if (state == MigrationState::TIMEOUT) {
        incr_stats(&vmotion_stats_t::mig_timeout);
    } else if (state == MigrationState::ABORTED) {
        incr_stats(&vmotion_stats_t::mig_aborted);
    }
    VMOTION_WUNLOCK
}

void
vmotion::migration_done(ep_t *ep, MigrationState mig_state)
{
    // In migration done, EP session_list_head will be looped, so take read lock.
    // Ensure this function is called only from vMotion threads
    VMOTION_HAL_READ_LOCK();
    endpoint_migration_done(ep, mig_state);
    VMOTION_HAL_READ_UNLOCK();
}

static void
vmotion_thread_delay_del_cb (void *timer, uint32_t timer_id, void *ctxt)
{
    sdk::event_thread::event_thread *thr = (sdk::event_thread::event_thread *)ctxt;

    HAL_TRACE_INFO("vmotion_thread_delay_del_cb thread: {}", thr->thread_id());

    // Free up the thread ID
    if (g_hal_state->get_vmotion()) {
        g_hal_state->get_vmotion()->release_thread_id(thr->thread_id());
    }

    // Free up event thread memory
    sdk::event_thread::event_thread::destroy(thr);
}

void
vmotion::delay_delete_thread(sdk::event_thread::event_thread *thr)
{
    if (sdk::lib::timer_schedule(HAL_TIMER_ID_THREAD_DELAY_DEL, VMOTION_THR_DELAY_DEL_TIME,
                                 reinterpret_cast<void *>(thr),
                                 vmotion_thread_delay_del_cb, false) == NULL) {
        HAL_TRACE_ERR("vMotion error in starting thread delay del timer");
    }
}

hal_ret_t
vmotion::vmotion_ep_quiesce_program(ep_t *ep, bool entry_add)
{
    hal_ret_t ret;

    VMOTION_PD_LOCK
    ret = ep_quiesce(ep, entry_add);
    VMOTION_PD_UNLOCK

    return ret;
}

// This function can be called by the vMotion thread (with lock needed).
// Also from the Main (GRPC) thread in case of Cold vMotion (without lock needed)
hal_ret_t
vmotion::vmotion_ep_migration_normalization_cfg(ep_t *ep, bool disable, bool lock_needed)
{
    hal_ret_t ret;

    if (lock_needed) {
        VMOTION_PD_LOCK
    }

    ret = endpoint_migration_normalization_cfg(ep, disable);

    if (lock_needed) {
        VMOTION_PD_UNLOCK
    }
    return ret;
}

hal_ret_t
vmotion::vmotion_ep_inp_mac_vlan_pgm(ep_t *ep, bool create, bool lock_needed)
{
    hal_ret_t ret;

    if (lock_needed) {
        VMOTION_PD_LOCK
    }

    ret = endpoint_migration_inp_mac_vlan_pgm(ep, create);

    if (lock_needed) {
        VMOTION_PD_UNLOCK
    }
    return ret;
}

hal_ret_t
vmotion::vmotion_ep_migration_if_update(ep_t *ep)
{
    hal_ret_t ret;

    VMOTION_PD_LOCK
    ret = endpoint_migration_if_update(ep);
    VMOTION_PD_UNLOCK

    return ret;
}

void
vmotion::vmotion_dbg_history_entry_add(vmotion_ep *vmn_ep)
{
    vmotion_ep_dbg_t *entry;
    ep_t             *ep = vmn_ep->get_ep();

    VMOTION_WLOCK

    if (vmotion_dbg_history_.size() == VMOTION_MAX_HISTORY_ENTRIES) {
        entry = vmotion_dbg_history_.front();
        vmotion_dbg_history_.erase(vmotion_dbg_history_.begin());
    } else {
        entry = (vmotion_ep_dbg_t *) HAL_CALLOC(HAL_MEM_ALLOC_VMOTION, sizeof(vmotion_ep_dbg_t));
    }

    vmotion_dbg_history_.push_back(entry);

    VMOTION_WUNLOCK

    memcpy(entry, vmn_ep->get_vmotion_ep_dbg(), sizeof(vmotion_ep_dbg_t));
    memcpy(&entry->ep_mac, vmn_ep->get_ep_mac(), ETH_ADDR_LEN);
    memcpy(&entry->old_homing_host_ip, &vmn_ep->get_old_homing_host_ip(), sizeof(ip_addr_t));
    entry->vmotion_type  = vmn_ep->get_vmotion_type();
    entry->vmotion_state = (ep ? ep->vmotion_state : MigrationState::NONE);
    entry->flags         = *vmn_ep->get_flags(); 
    entry->sm_state      = vmn_ep->get_sm()->get_state();
}

void
vmotion::vmotion_notify_event(vmotion_ep *vmn_ep, MigrationState migration_state)
{
    eventtypes::EventTypes  event_type;
    const char *dir = ((vmn_ep->get_vmotion_type() == VMOTION_TYPE_MIGRATE_IN) ? "in" : "out");

    if (migration_state == MigrationState::FAILED) {
        event_type = eventtypes::DSC_ENDPOINT_MIGRATION_FAILED;
    } else if (migration_state == MigrationState::ABORTED) {
        event_type = eventtypes::DSC_ENDPOINT_MIGRATION_ABORTED;
    } else if (migration_state == MigrationState::TIMEOUT) {
        event_type = eventtypes::DSC_ENDPOINT_MIGRATION_TIMEOUT;
    } else {
        return;
    }

    hal_workload_migration_event_notify(event_type, dir, macaddr2str(*vmn_ep->get_ep_mac()));
}

} // namespace hal
