//
//  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
//

#include <assert.h>
#include <nic/sdk/lib/ipc/ipc.hpp>
#include "pdsa_hdlr.hpp"
#include "pdsa_vpp_hdlr.h"
#include <map>


static std::map<int, sdk::ipc::handler_cb> g_cb_func;
static std::map<int, const void *> g_cb_ctx;

// message handing callbacks
static pds_ipc_msg_cb msg_handle[PDS_IPC_MSG_OP_MAX][PDS_MSG_ID_MAX] = {0};

// message rx counter
static unsigned int g_type_count[PDS_MSG_TYPE_MAX] = {0};
static unsigned int g_id_count[PDS_MSG_ID_MAX] = {0};

// called from VPP main poll loop, whenere data is available on the zmq
// IPC file descriptor. Calls the SDK IPC library to read messages and
// dispatch them
void
pds_ipc_read_fd (int fd) {
    auto cb_func_it = g_cb_func.find(fd);
    auto cb_ctx_it = g_cb_ctx.find(fd);

    if (cb_func_it == g_cb_func.end()) {
        ipc_log_error("Attempting read on unknown fd %d", fd);
        return;
    }

    sdk::ipc::handler_cb cb_func = cb_func_it->second;
    const void *cb_ctx = cb_ctx_it->second;

    cb_func(fd, cb_ctx);
}

// callback called when initializing IPC. Provides a file desciptor to
// be monitored via poll, and a callback function to be called when
// data is available on the file descriptor
static void
pds_vpp_fd_watch_cb (int fd, sdk::ipc::handler_cb cb, const void *set_ctx,
                    const void *ctx) {
    // confirm that the context is the ptr to this fn
    assert(ctx == (const void *)pds_vpp_fd_watch_cb);

    // store provided data
    g_cb_func[fd] = cb;
    g_cb_ctx[fd] = set_ctx;

    pds_vpp_fd_register(fd);
}

/// \brief dummy "OK" callback, used until actual code is ready
static void
pds_ipc_ok_type_cb (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctx) {
    sdk::sdk_ret_t ret;
    uint64_t       type = (uint64_t )ctx;

    g_type_count[type]++;

    ipc_log_notice("Received msg with type:%d [count:%d] Returning OK", type,
                   g_type_count[type]);

    // always return ok
    ret = sdk::SDK_RET_OK;
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk::sdk_ret_t));
}

/// \brief dummy "NOK" callback, used for unsupported messages
static void
pds_ipc_invalid_type_cb (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctx) {
    sdk::sdk_ret_t ret;
    uint64_t       type = (uint64_t )ctx;

    g_type_count[type]++;

    ipc_log_notice("Received msg with type:%d [count:%d] Returning Invalid OP",
                   type, g_type_count[type]);

    // always return ok
    ret = sdk::SDK_RET_INVALID_OP;
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk::sdk_ret_t));
}

// register callbacks from plugins for messages
// return 0 indicates  registered successfully
// return non-zero indicates registration fail (invalid param)
int
pds_ipc_register_callback (pds_msg_id_t msgid,
                          pds_ipc_msg_op_t operation,
                          pds_ipc_msg_cb cb_fn) {
    if ((msgid >= PDS_MSG_ID_MAX) || (operation >= PDS_IPC_MSG_OP_MAX)) {
        return -1;
    }

    msg_handle[operation][msgid] = cb_fn;
    return 0;
}

// stage one of batch processing. reserve resources for all messages in batch.
// Any failures will trigger release of resources already allocated prior to
// failure
static sdk::sdk_ret_t
msglist_reserve (pds_msg_list_t *msglist) {
    uint32_t rsv_count;
    sdk::sdk_ret_t ret = sdk::SDK_RET_OK;

    for (rsv_count = 0; rsv_count < msglist->num_msgs; rsv_count++) {
        pds_msg_t *msg = &msglist->msgs[rsv_count];
        pds_ipc_msg_cb reserve_fn = msg_handle[PDS_IPC_MSG_OP_RESERVE][msg->id];

        if (msg->id > PDS_MSG_ID_MAX) {
            goto release;
        }

        g_id_count[msg->id]++;

        if (reserve_fn) {
            ret = reserve_fn(msg, NULL);
            // if reserve fails, release all reserved ops
            if (ret != sdk::SDK_RET_OK) {
                ipc_log_error("Reserve fail for msg indx:%d id:%d [count:%d] "
                              "ret:%d epoch:%u", rsv_count, msg->id,
                              g_id_count[msg->id], ret, msglist->epoch);
                goto release;
            }
        }
    }

    ipc_log_notice("Reserve completed for %d messages [epoch:%u]",
                   msglist->num_msgs, msglist->epoch);
    return ret;

release:
    ipc_log_notice("Releasing resources for %d messages [epoch:%u]", rsv_count,
                   msglist->epoch);
    // release resources only up to point of failure
    for (uint32_t i = 0; i < rsv_count; i++) {
        pds_msg_t *msg = &msglist->msgs[i];
        pds_ipc_msg_cb release_fn = msg_handle[PDS_IPC_MSG_OP_RELEASE][msg->id];

        if (release_fn) {
            // TODO: how do we handle release failures
            release_fn(msg, NULL);
        }
    }
    return ret;
}

// stage two of batch processing. proces all messages in batch.  Any failures
// will trigger rollback of already processed messages, and release of
// resources for messages
static sdk::sdk_ret_t
msglist_process (pds_msg_list_t *msglist) {
    uint32_t run_count;
    sdk::sdk_ret_t ret = sdk::SDK_RET_OK;

    for (run_count = 0; run_count < msglist->num_msgs; run_count++) {
        pds_msg_t *msg = &msglist->msgs[run_count];
        pds_ipc_msg_cb run_fn = msg_handle[PDS_IPC_MSG_OP_PROCESS][msg->id];

        if (run_fn) {
            ret = run_fn(msg, NULL);
            // if process fails, rollback & release
            if (ret != sdk::SDK_RET_OK) {
                ipc_log_error("Processing fail for msg indx:%d id:%d ret:%d "
                              "[epoch:%u]", run_count, msg->id, ret,
                              msglist->epoch);
                goto rollback;
            }
        }
    }

    return ret;

rollback:
    // rollback operations only up to point of failure
    ipc_log_notice("Rollback operation for %d messages [epoch:%u]", run_count,
                   msglist->epoch);
    for (uint32_t i = 0; i < run_count; i++) {
        pds_msg_t *msg = &msglist->msgs[i];
        pds_ipc_msg_cb rollbk_fn = msg_handle[PDS_IPC_MSG_OP_ROLLBACK][msg->id];

        if (rollbk_fn) {
            // TODO: how do we handle rollback failure
            rollbk_fn(msg, NULL);
        }
    }
    // release resources for all
    ipc_log_notice("Releasing resources for %d messages [epoch:%u]",
                   msglist->num_msgs, msglist->epoch);
    for (uint32_t i = 0; i < msglist->num_msgs; i++) {
        pds_msg_t *msg = &msglist->msgs[i];
        pds_ipc_msg_cb release_fn = msg_handle[PDS_IPC_MSG_OP_RELEASE][msg->id];

        if (release_fn) {
            // TODO: how do we handle release failure
            release_fn(msg, NULL);
        }
    }
    return ret;
}

// handler for batch messages from HAL
static void
pds_ipc_msglist_cb (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctx) {
    sdk::sdk_ret_t  ret;
    pds_msg_list_t *msglist = NULL;

    if (ipc_msg->length() < sizeof(pds_msg_list_t)) {
        ret = sdk::SDK_RET_INVALID_ARG;
        goto error;
    }

    msglist = (pds_msg_list_t *)ipc_msg->data();

    g_type_count[PDS_MSG_TYPE_CFG]++;
    ipc_log_notice("Received msglist with %d message(s) [epoch:%u]",
                   msglist->num_msgs, msglist->epoch);

    // Stage 1: Reserve resources
    ret = msglist_reserve(msglist);
    if (ret != sdk::SDK_RET_OK) {
        goto error;
    }

    // Stage 2: Process
    ret = msglist_process(msglist);

error:
    if (ret != sdk::SDK_RET_OK) {
        ipc_log_error("Execution fail of msglist [epoch:%u] ret:%d",
                      (msglist ? msglist->epoch : 0), ret);
    } else {
        ipc_log_notice("Successful Execution of msglist [epoch:%u]",
                       msglist->epoch);
    }
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk::sdk_ret_t));
}

// handler for PDS IPC command call. command callbacks will have more than a
// return code, so instead they return a pds_msg_t pointer
static void
pds_ipc_cmd_msg_cb (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctx) {
    pds_msg_t *msg, response;
    sdk::sdk_ret_t retcode;
    pds_ipc_msg_cb command_fn;

    g_type_count[PDS_MSG_TYPE_CMD]++;

    // validate received message
    if (ipc_msg->length() != sizeof(pds_msg_t)) {
        retcode = sdk::SDK_RET_INVALID_ARG;
        goto error;
    }

    msg = (pds_msg_t *)ipc_msg->data();
    if (msg->id >= PDS_MSG_ID_MAX) {
        retcode = sdk::SDK_RET_INVALID_ARG;
        goto error;
    }

    // if callback is not registered, return invalid operation
    command_fn = msg_handle[PDS_IPC_MSG_OP_COMMAND][msg->id];
    if (command_fn == NULL) {
        retcode = sdk::SDK_RET_INVALID_OP;
        goto error;
    }

    // actually execute the callback, and get response
    retcode = command_fn(msg, &response);
    if (retcode == sdk::SDK_RET_OK) {
        ipc_log_notice("Execution success of command msg [count:%d]",
                      g_type_count[PDS_MSG_TYPE_CMD]);
        sdk::ipc::respond(ipc_msg, (const void *)&response, sizeof(pds_msg_t));
        return;
    }

error:
    ipc_log_error("Execution fail of command msg [count:%d] ret:%d",
                  g_type_count[PDS_MSG_TYPE_CMD], retcode);
    sdk::ipc::respond(ipc_msg, (const void *)&retcode, sizeof(sdk::sdk_ret_t));
}

// VPP IPC initialization. Register VPP with IPC infra, and install callbacks
// for handling base message types
void
pds_ipc_init (void)
{
    sdk::ipc::ipc_init_async(PDS_IPC_ID_VPP, pds_vpp_fd_watch_cb,
                             (const void *)pds_vpp_fd_watch_cb);

    // register "NONE" messages with INVALID handler
    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_NONE, pds_ipc_invalid_type_cb,
                                  (const void *)PDS_MSG_TYPE_NONE);

    // register placeholder callbacks.
    // TODO: must be removed when actual handlers are implemented
    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_EVENT, pds_ipc_ok_type_cb,
                                  (const void *)PDS_MSG_TYPE_EVENT);

    // register handler for configuration batches
    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_CFG, pds_ipc_msglist_cb, NULL);

    // register handler for command messages
    sdk::ipc::reg_request_handler(PDS_MSG_TYPE_CMD, pds_ipc_cmd_msg_cb, NULL);

    ipc_log_notice("Registered callbacks for IPC messages");
}
