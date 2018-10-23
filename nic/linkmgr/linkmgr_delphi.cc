//-----------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include <iostream>
#include "grpc++/grpc++.h"
#include "nic/linkmgr/linkmgr_delphi.hpp"
#include "nic/linkmgr/linkmgr.hpp"
#include "nic/linkmgr/linkmgr_svc.hpp"

#include "nic/include/base.hpp"
#include "nic/include/trace.hpp"
#include "nic/include/hal.hpp"
#include "nic/sdk/include/sdk/port_mac.hpp"

namespace linkmgr {

using grpc::Status;
using delphi::error;
using port::PortResponse;
using port::PortOperStatus;
using delphi::objects::PortSpec;
using delphi::objects::PortStatus;
using delphi::objects::PortSpecPtr;
using delphi::objects::PortStatusPtr;

// port reactors
port_svc_ptr_t g_port_rctr;

// linkmgr_get_port_reactor gets the port reactor object
port_svc_ptr_t linkmgr_get_port_reactor () {
    return g_port_rctr;
}

// linkmgr_init_port_reactors creates a port reactor
Status linkmgr_init_port_reactors (delphi::SdkPtr sdk) {
    // create the PortSpec reactor
    g_port_rctr = std::make_shared<port_svc>(sdk);

    // mount objects
    PortSpec::Mount(sdk, delphi::ReadMode);

    // Register PortSpec reactor
    PortSpec::Watch(sdk, g_port_rctr);

    // mount status objects
    PortStatus::Mount(sdk, delphi::ReadWriteMode);

    HAL_TRACE_DEBUG("Linkmgr: Mounted port objects from delphi...");

    return Status::OK;
}

// OnPortSpecCreate gets called when PortSpec object is created
error port_svc::OnPortSpecCreate(PortSpecPtr portSpec) {
    PortResponse    response;
    port_args_t     port_args  = {0};
    hal_ret_t       hal_ret    = HAL_RET_OK;
    hal_handle_t    hal_handle = 0;

    // validate port params
    if (validate_port_create (*portSpec.get(), &response) == false) {
        HAL_TRACE_ERR("port create validation failed");
        return error::New("port create validation failed");
    }

    // set the params in port_args
    populate_port_create_args (*portSpec.get(), &port_args);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);

    // create the port
    hal_ret = linkmgr::port_create(&port_args, &hal_handle);
    if (hal_ret != HAL_RET_OK) {
        HAL_TRACE_ERR("port create failed with error {}", hal_ret);
        return error::New("port create failed");
    }

    hal::hal_cfg_db_close();

    HAL_TRACE_DEBUG("Linkmgr: Port {} got created", port_args.port_num);

    return error::OK();
}

// OnPortSpecUpdate gets called when PortSpec object is updated
error port_svc::OnPortSpecUpdate(PortSpecPtr portSpec) {
    PortResponse    response;
    port_args_t     port_args  = {0};
    hal_ret_t       hal_ret    = HAL_RET_OK;

    // validate port params
    if (validate_port_update (*portSpec.get(), &response) == false) {
        HAL_TRACE_ERR("port update validation failed");
        return error::New("port update validation failed");
    }

    // set the params in port_args
    populate_port_update_args (*portSpec.get(), &port_args);

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);

    // create the port
    hal_ret = linkmgr::port_update(&port_args);
    if (hal_ret != HAL_RET_OK) {
        HAL_TRACE_ERR("port update failed with error {}", hal_ret);
        return error::New("port update failed");
    }

    hal::hal_cfg_db_close();

    HAL_TRACE_DEBUG("Linkmgr: Port {} updated", port_args.port_num);

    return error::OK();
}

// OnPortSpecDelete gets called when PortSpec object is deleted
error port_svc::OnPortSpecDelete(PortSpecPtr portSpec) {
    PortResponse    response;
    port_args_t     port_args  = {0};
    hal_ret_t       hal_ret    = HAL_RET_OK;

    // set port number in port_args
    sdk::linkmgr::port_args_init(&port_args);
    port_args.port_num = portSpec->key_or_handle().port_id();

    hal::hal_cfg_db_open(hal::CFG_OP_WRITE);

    // create the port
    hal_ret = linkmgr::port_delete(&port_args);
    if (hal_ret != HAL_RET_OK) {
        HAL_TRACE_ERR("port delete failed with error {}", hal_ret);
        return error::New("port delete failed");
    }
    hal::hal_cfg_db_close();

    HAL_TRACE_DEBUG("Linkmgr: Port {} got deleted", port_args.port_num);

    return error::OK();
}

// update_port_status updates port status in delphi
error port_svc::update_port_status(google::protobuf::uint32 port_id,
                                    PortOperStatus status) {
    // create port status object
    PortStatusPtr port = std::make_shared<PortStatus>();
    port->mutable_key_or_handle()->set_port_id(port_id);
    port->set_oper_status(status);

    // add it to database
    sdk_->QueueUpdate(port);

    HAL_TRACE_DEBUG("Updated port status object for port_id {} oper state {} "
                    "port: {}", port_id, status, port);

    return error::OK();
}

}    // namespace linkmgr
