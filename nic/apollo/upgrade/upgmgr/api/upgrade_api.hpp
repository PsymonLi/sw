//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
///----------------------------------------------------------------------------
///
/// \file
/// This file defines upgrade API
///
///----------------------------------------------------------------------------

#ifndef __API_UPGRADE_API_HPP__
#define __API_UPGRADE_API_HPP__

#define UPGMGR_INIT_MODE_FILE "/update/upgmgr_init_mode.txt"
#define MAX_FW_PKGNAME_LEN 256

namespace api {

/// \brief     upgrade event request
///            ipc from grpc main thread to request processing thread
typedef enum upg_ev_req_msg_id_s {
    UPG_REQ_MSG_ID_NONE  = 0,
    UPG_REQ_MSG_ID_START = 1,
} upg_ev_req_msg_id_t;

/// \brief     upgrade event params
///            passed from grpc main thread to request processing thread
typedef struct upg_ev_req_msg_s {
    upg_ev_req_msg_id_t id;                  ///< message id
    sdk::platform::sysinit_mode_t upg_mode;  ///< upgrade mode
    char fw_pkgname[MAX_FW_PKGNAME_LEN];     ///< firmware package name with path
    // TODO fill other details
    upg_status_t rsp_status;                ///< upgrade status
} upg_ev_req_msg_t;

}    // namespace api

/// \brief     upgrade initialization params
typedef struct upg_init_params_s {
    std::string tools_dir;
} upg_init_params_t;

/// \brief     initialize upgrade manager
/// \param[in] params is initialization parameter
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t upg_init(upg_init_params_t *params);

/// \brief     check if current upgrade stage is config replay
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t upg_config_replay_ready_check(void);

/// \brief     report successful replay of configuration.
void upg_config_replay_done(void);

/// \brief     abort the upgrade
void upg_abort(void);

using api::upg_ev_req_msg_id_t;
using api::upg_ev_req_msg_t;

#endif    // __API_UPGRADE_API_HPP__
