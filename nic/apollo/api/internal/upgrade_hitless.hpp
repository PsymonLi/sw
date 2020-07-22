//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines hitless event handlers during process upgrade
///
//----------------------------------------------------------------------------

/// \defgroup UPGRADE Upgrade event handlers
/// \ingroup  UPG_EVENT_HDLR
/// @{
///

#ifndef __API_UPGRADE_HITLESS_HPP__
#define __API_UPGRADE_HITLESS_HPP__

namespace api {

/// \brief upgrade event handlers to the registered thread
/// functions for upgrading process/thread A to B
/// A is the currently running process and B is the new process
/// each process/thread should implement this and act on these events.
typedef struct upg_ev_hitless_s {
    /// registering thread name
    /// used for debug traces. no other significance.
    char thread_name[SDK_MAX_NAME_LEN];

    /// compat checks should be done here (on A)
    upg_ev_hdlr_t compat_check_hdlr;

    /// start of a new upgrade, mount check the existance of B should be
    /// done here (on A).
    upg_ev_hdlr_t start_hdlr;

    /// software states should be saved here (on A).
    upg_ev_hdlr_t backup_hdlr;

    /// upgrade specific bringup should be done here (on B)
    /// the bringup shouldn't touch the shared hw resources with A
    /// [Ex: polling LIFs, ports, etc )
    /// below functions should check the bringup status of A
    upg_ev_hdlr_t ready_hdlr;

    /// config replay (on B)
    upg_ev_hdlr_t config_replay_hdlr;

    /// operational state syncing (on B)
    upg_ev_hdlr_t sync_hdlr;

    /// prepare for switching to B. threads are paused to a safe state here
    upg_ev_hdlr_t pre_switchover_hdlr;

    /// pipeline switch should be done here (on B)
    /// if it is success, it can start shared resource access
    upg_ev_hdlr_t switchover_hdlr;

    /// repeal an upgrade (on A / B)
    /// an repeal (on A)
    ///   after backup, requires the thread to cleanup its saved states
    ///   after sw_quiesce, requires the thread to resume and goes to ready state
    /// an repeal (on B)
    ///   after restore, requires the thread to cleanup its state and go to
    ///   sofware quiesce state.
    upg_ev_hdlr_t repeal_hdlr;

    /// called on B. requires the threads to undo any changes done.
    /// similar to repeal on A
    upg_ev_hdlr_t rollback_hdlr;

    /// completed the upgrade (on A / B)
    upg_ev_hdlr_t finish_hdlr;
} upg_ev_hitless_t;

/// register the thread to the main upgrade event handler
void upg_ev_thread_hdlr_register(upg_ev_hitless_t &hdlr);

}   // namespace api

using api::upg_ev_hitless_t;

/// @}

#endif   // __API_UPGRADE_HITLESS_HPP__
