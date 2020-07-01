//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS Cfg Msg handler
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/hals/pds_ms_cfg_msg.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_ip_track.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/core/trace.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include <string>

namespace pds_ms {
void
pds_msg_cfg_callback (sdk::ipc::ipc_msg_ptr ipc_msg, const void *ctxt)
{
    pds_cfg_msg_t *cfg_msg;
    pds_msg_list_t *msg_list;
    sdk_ret_t ret = SDK_RET_OK;
    bool op_delete = false;
    bool op_update = false; /* updates can cause delete or create */

    msg_list = (pds_msg_list_t *)ipc_msg->data();

    for (uint32_t i = 0; i < msg_list->num_msgs; i++) {
        cfg_msg = &msg_list->msgs[i].cfg_msg;
        pds_obj_key_t* key = nullptr;
        ip_addr_t* ip = nullptr;
        std::string obj_type;

        switch (cfg_msg->obj_id) {

        case OBJ_ID_TEP:
            if (mgmt_state_t::thread_context().state()->overlay_routing_en()) {
                // API thread may echo back the tunnel create from EVPN
                // Ignore this
                PDS_TRACE_VERBOSE("Ignore CFG_MSG TEP in Overlay routing mode");
                continue;
            }
            if (cfg_msg->op == API_OP_DELETE) {
                key = &cfg_msg->tep.key;
                op_delete = true;
            } else {
                if (cfg_msg->op == API_OP_CREATE) {
                    if (cfg_msg->tep.spec.nh_type != PDS_NH_TYPE_NONE) {
                        // Ignore TEP create with static NH type
                        continue;
                    }
                } else {
                    op_update = true;
                    // TEP Update could have modified from static NH to
                    // dynamic NH or vice-versa
                    if (cfg_msg->tep.spec.nh_type != PDS_NH_TYPE_NONE) {
                        op_delete = true;
                    }
                }
                key = &cfg_msg->tep.spec.key;
                ip = &cfg_msg->tep.spec.remote_ip;
            }
            obj_type = "TEP";
            break;

        case OBJ_ID_MIRROR_SESSION:
            if (cfg_msg->op == API_OP_DELETE) {
                key = &cfg_msg->mirror_session.key;
                op_delete = true;
            } else {
                auto& spec = cfg_msg->mirror_session.spec;
                if (spec.type != PDS_MIRROR_SESSION_TYPE_ERSPAN) {
                    continue;
                }
                if (cfg_msg->op == API_OP_CREATE) {
                    if (spec.erspan_spec.dst_type != PDS_ERSPAN_DST_TYPE_IP) {
                        continue;
                    }
                } else {
                    op_update = true;
                    // Mirror update can lead to start/stop ip-track if
                    // the ERSPAN dst type is modified.
                    if (spec.erspan_spec.dst_type != PDS_ERSPAN_DST_TYPE_IP) {
                        op_delete = true;
                    }
                }
                key = &cfg_msg->mirror_session.spec.key;
                ip = &cfg_msg->mirror_session.spec.erspan_spec.ip_addr;
            }
            obj_type = "MIRROR";
            break;

        default:
            continue;
        }

        if (op_delete) {
            PDS_TRACE_DEBUG("CFG_MSG %s %s %s delete", obj_type.c_str(),
                            key->str(), (op_update) ? "update" : "");
        } else {
            PDS_TRACE_DEBUG("CFG_MSG %s %s %s %s", obj_type.c_str(),
                            key->str(), ipaddr2str(ip),
                            (op_update) ? "update" : "create");
        }

        if (op_delete) {
            ret = ip_track_del (*key);
            if (ret == SDK_RET_ENTRY_NOT_FOUND) {
                // Ignore deletes for cfg objcts that we dont know about 
                ret = SDK_RET_OK;
            }
        } else {
            ret = ip_track_add (*key, *ip, cfg_msg->obj_id, op_update);
            if (op_update && ret == SDK_RET_ENTRY_EXISTS) {
                // Ignore updates for existing objects
                ret = SDK_RET_OK;
            }
        }
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("CFG_MSG %s %s %s %s %s failed",
                          obj_type.c_str(), key->str(),
                          (ip != nullptr) ? ipaddr2str(ip) : "",
                          (op_update) ? "update" : "",
                          (op_delete) ? "delete" : "create");
        }
    }
    sdk::ipc::respond(ipc_msg, (const void *)&ret, sizeof(sdk_ret_t));
}

} // End namespace
