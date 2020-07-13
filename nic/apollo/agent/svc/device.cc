//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_device.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/svc/device.hpp"
#include "nic/apollo/agent/svc/device_svc.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/device.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_device.hpp"
#include "nic/sdk/platform/fru/fru.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define DEVICE_CONF_FILE "/sysconfig/config0/device.conf"

static inline void
device_conf_update (pds_device_spec_t *spec)
{
    boost::property_tree::ptree pt, output;
    boost::property_tree::ptree::iterator pos;

    try {
        // copy existing data from device.conf
        std::ifstream json_cfg(DEVICE_CONF_FILE);
        read_json(json_cfg, pt);

        for (pos = pt.begin(); pos != pt.end();) {
            output.put(pos->first.data(), pos->second.data());
            ++pos;
        }
    } catch (...) {}

    // update the device profile
    if (spec->device_profile == PDS_DEVICE_PROFILE_DEFAULT) {
        output.put("device-profile", "default");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_2PF) {
        output.put("device-profile", "2pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_3PF) {
        output.put("device-profile", "3pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_4PF) {
        output.put("device-profile", "4pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_5PF) {
        output.put("device-profile", "5pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_6PF) {
        output.put("device-profile", "6pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_7PF) {
        output.put("device-profile", "7pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_8PF) {
        output.put("device-profile", "8pf");
    } else if (spec->device_profile == PDS_DEVICE_PROFILE_32VF) {
        output.put("device-profile", "32vf");
    }

    // update the memory profile
    if (spec->memory_profile == PDS_MEMORY_PROFILE_DEFAULT) {
        output.put("memory-profile", "default");
    } else if (spec->memory_profile == PDS_MEMORY_PROFILE_ROUTER) {
        output.put("memory-profile", "router");
    }

    // update the forwarding mode
    if (spec->dev_oper_mode == PDS_DEV_OPER_MODE_NONE) {
        output.put("oper-mode", "");
    } else if (spec->dev_oper_mode == PDS_DEV_OPER_MODE_HOST) {
        output.put("oper-mode", "host");
    } else if (spec->dev_oper_mode == PDS_DEV_OPER_MODE_BITW_SMART_SWITCH) {
        // bump-in-the-wire smart switch mode
        output.put("oper-mode", "bitw_smart_switch");
    } else if (spec->dev_oper_mode == PDS_DEV_OPER_MODE_BITW_SMART_SERVICE) {
        // bump-in-the-wire smart service mode
        output.put("oper-mode", "bitw_smart_service");
    } else if (spec->dev_oper_mode == PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH) {
        // bump-in-the-wire classic switch mode
        output.put("oper-mode", "bitw_classic_switch");
    }

    // set the port admin state
    output.put("port-admin-state", "PORT_ADMIN_STATE_ENABLE");

    try {
        // write back the updated configuration
        boost::property_tree::write_json(DEVICE_CONF_FILE, output);
    } catch (...) {
        PDS_TRACE_ERR("Failed to update {}", DEVICE_CONF_FILE);
    }
}

static inline void
pds_device_state_update (pds_device_spec_t *curr, pds_device_spec_t *spec)
{
    curr->device_ip_addr = spec->device_ip_addr;
    MAC_ADDR_COPY(curr->device_mac_addr, spec->device_mac_addr);
    curr->gateway_ip_addr = spec->gateway_ip_addr;
    curr->bridging_en = spec->bridging_en;
    curr->learn_spec = spec->learn_spec;
    curr->overlay_routing_en = spec->overlay_routing_en;
    curr->symmetric_routing_en = spec->symmetric_routing_en;
    curr->ip_mapping_priority = spec->ip_mapping_priority;
    curr->fw_action_xposn_scheme = spec->fw_action_xposn_scheme;
    curr->tx_policer = spec->tx_policer;
}

// function to copy only the mutables fields into the new spec
static inline void
pds_device_copy_mutable_attrs (pds_device_spec_t *new_spec,
                               pds_device_spec_t *spec)
{
    new_spec->device_ip_addr = spec->device_ip_addr;
    MAC_ADDR_COPY(new_spec->device_mac_addr, spec->device_mac_addr);
    new_spec->gateway_ip_addr = spec->gateway_ip_addr;
    new_spec->learn_spec = spec->learn_spec;
    new_spec->ip_mapping_priority = spec->ip_mapping_priority;
    new_spec->fw_action_xposn_scheme = spec->fw_action_xposn_scheme;
    new_spec->tx_policer = spec->tx_policer;
}

sdk_ret_t
pds_svc_device_create (const pds::DeviceRequest *proto_req,
                       pds::DeviceResponse *proto_rsp)
{
    pds_batch_ctxt_t bctxt;
    sdk_ret_t ret = SDK_RET_OK;
    pds_device_spec_t *api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }
    api_spec = core::agent_state::state()->device();
    if (api_spec == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OUT_OF_MEM);
        return SDK_RET_OOM;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, device object "
                          "creation failed");
            ret = SDK_RET_ERR;
            goto end;
        }
        batched_internally = true;
    }

    ret = pds_device_proto_to_api_spec(api_spec, proto_req->request());
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Invalid device object specification, err %u", ret);
        goto end;
    }
    pds_ms::device_create(api_spec);
    if (!core::agent_state::state()->pds_mock_mode()) {
        ret = pds_device_create(api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            if (batched_internally) {
                pds_batch_destroy(bctxt);
            }
            goto end;
        }
    }

    // update device.conf with persistent attrs
    // TODO: ideally this should be done during commit time (and we need to
    //       handle aborting/rollback here)
    device_conf_update(api_spec);

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }

end:

    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

sdk_ret_t
pds_svc_device_update (const pds::DeviceRequest *proto_req,
                       pds::DeviceResponse *proto_rsp)
{
    sdk_ret_t ret;
    pds_batch_ctxt_t bctxt;
    pds_device_spec_t api_spec, mutable_api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return SDK_RET_INVALID_ARG;
    }

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, device object update "
                          "failed");
            ret = SDK_RET_ERR;
            goto end;
        }
        batched_internally = true;
    }

    // copy existing device spec
    memcpy(&mutable_api_spec, core::agent_state::state()->device(),
           sizeof(pds_device_spec_t));
    // get spec in update request
    ret = pds_device_proto_to_api_spec(&api_spec, proto_req->request());
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Invalid device object specification, err %u", ret);
        goto end;
    }
    // copy mutable fields from update request to mutable api spec
    pds_device_copy_mutable_attrs(&mutable_api_spec, &api_spec);
    if (!core::agent_state::state()->pds_mock_mode()) {
        ret = pds_device_update(&mutable_api_spec, bctxt);
        if (ret != SDK_RET_OK) {
            if (batched_internally) {
                pds_batch_destroy(bctxt);
            }
            goto end;
        }
        // copy mutable api spec to agent copy of spec
        pds_device_state_update(core::agent_state::state()->device(),
                                &mutable_api_spec);
    }

    // update device.conf with persistent attrs
    // TODO: ideally this should be done during commit time (and we need to
    //       handle aborting/rollback here)
    device_conf_update(&api_spec);

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }

end:

    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

sdk_ret_t
pds_svc_device_delete (const pds::DeviceDeleteRequest *proto_req,
                       pds::DeviceDeleteResponse *proto_rsp)
{
    pds_batch_ctxt_t bctxt;
    sdk_ret_t ret = SDK_RET_OK;
    pds_device_spec_t *api_spec;
    bool batched_internally = false;
    pds_batch_params_t batch_params;

    api_spec = core::agent_state::state()->device();

    // create an internal batch, if this is not part of an existing API batch
    bctxt = proto_req->batchctxt().batchcookie();
    if (bctxt == PDS_BATCH_CTXT_INVALID) {
        batch_params.epoch = core::agent_state::state()->new_epoch();
        batch_params.async = false;
        bctxt = pds_batch_start(&batch_params);
        if (bctxt == PDS_BATCH_CTXT_INVALID) {
            PDS_TRACE_ERR("Failed to create a new batch, device object delete "
                          "failed");
            ret = SDK_RET_ERR;
            goto end;
        }
        batched_internally = true;
    }

    if (!core::agent_state::state()->pds_mock_mode()) {
        ret = pds_device_delete(bctxt);
        if (ret != SDK_RET_OK) {
            if (batched_internally) {
                pds_batch_destroy(bctxt);
            }
            goto end;
        }
        memset(api_spec, 0, sizeof(pds_device_spec_t));
    }

    // delete the device.conf file
    unlink(DEVICE_CONF_FILE);

    if (batched_internally) {
        // commit the internal batch
        ret = pds_batch_commit(bctxt);
    }

end:

    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return ret;
}

static inline void
device_spec_fill (pds_device_spec_t *spec)
{
    spec->device_profile = api::g_pds_state.device_profile();
    spec->memory_profile = api::g_pds_state.memory_profile();
}

static inline sdk_ret_t
device_info_fill (pds_device_info_t *info)
{
    device_spec_fill(&info->spec);
    api::device_entry::fill_status(&info->status);
    return SDK_RET_OK;
}

sdk_ret_t
pds_svc_device_get (const pds::DeviceGetRequest *proto_req,
                    pds::DeviceGetResponse *proto_rsp)
{
    sdk_ret_t ret = SDK_RET_OK;
    pds_device_spec_t *api_spec = core::agent_state::state()->device();
    pds_device_info_t info;

    if (api_spec->dev_oper_mode != PDS_DEV_OPER_MODE_NONE) {
        memcpy(&info.spec, api_spec, sizeof(pds_device_spec_t));
        if (!core::agent_state::state()->pds_mock_mode()) {
            ret = pds_device_read(&info);
        }
    } else {
        memset(&info, 0, sizeof(pds_device_info_t));
        if (!core::agent_state::state()->pds_mock_mode()) {
            ret = device_info_fill(&info);
        }
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Device object not found");
        return ret;
    }
    pds_device_api_spec_to_proto(
            proto_rsp->mutable_response()->mutable_spec(), &info.spec);
    pds_device_api_status_to_proto(
            proto_rsp->mutable_response()->mutable_status(), &info.status);
    pds_device_api_stats_to_proto(
            proto_rsp->mutable_response()->mutable_stats(), &info.stats);
    return ret;
}

Status
DeviceSvcImpl::DeviceCreate(ServerContext *context,
                            const pds::DeviceRequest *proto_req,
                            pds::DeviceResponse *proto_rsp) {
    pds_svc_device_create(proto_req, proto_rsp);
    return Status::OK;
}

Status
DeviceSvcImpl::DeviceUpdate(ServerContext *context,
                            const pds::DeviceRequest *proto_req,
                            pds::DeviceResponse *proto_rsp) {
    pds_svc_device_update(proto_req, proto_rsp);
    return Status::OK;
}

Status
DeviceSvcImpl::DeviceDelete(ServerContext *context,
                            const pds::DeviceDeleteRequest *proto_req,
                            pds::DeviceDeleteResponse *proto_rsp) {
    pds_svc_device_delete(proto_req, proto_rsp);
    return Status::OK;
}

Status
DeviceSvcImpl::DeviceGet(ServerContext *context,
                         const pds::DeviceGetRequest *proto_req,
                         pds::DeviceGetResponse *proto_rsp) {
    pds_svc_device_get(proto_req, proto_rsp);
    return Status::OK;
}

Status
DeviceSvcImpl::DeviceStatsReset(ServerContext *context,
                                const Empty *req,
                                Empty *rsp) {
    pds_device_stats_reset();
    return Status::OK;
}
