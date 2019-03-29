//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/plugins/network/net_plugin.hpp"

namespace hal {
namespace plugins {
namespace network {

//------------------------------------------------------------------------------
// Network plugin
//------------------------------------------------------------------------------
const std::string FTE_FEATURE_STAGE_MARKER("pensando.io/network:stage-marker");
const std::string FTE_FEATURE_FWDING("pensando.io/network:fwding");
const std::string FTE_FEATURE_FWDING_PRE_STAGE("pensando.io/network:fwding-pre-stage");
const std::string FTE_FEATURE_TUNNEL("pensando.io/network:tunnel");
const std::string FTE_FEATURE_QOS("pensando.io/network:qos");
const std::string FTE_FEATURE_RDMA("pensando.io/network:rdma");
const std::string FTE_FEATURE_INGRESS_CHECKS("pensando.io/network:ingress-checks");

extern "C" hal_ret_t network_init(hal_cfg_t *hal_cfg) {
    // Register  update for forwarding
    fte::feature_info_t fwding_info = {
        state_size:  0,
        state_init_fn: NULL,
        sess_del_cb: NULL,
        sess_get_cb: NULL,
        sess_upd_cb: fwding_exec,
    };
    // Register update for ingress checks
    fte::feature_info_t ingress_info = {
        state_size:  0,
        state_init_fn: NULL,
        sess_del_cb: NULL,
        sess_get_cb: NULL,
        sess_upd_cb: ingress_checks_exec,
    };

    fte::register_feature(FTE_FEATURE_STAGE_MARKER,  stage_exec);
    fte::register_feature(FTE_FEATURE_FWDING, fwding_exec, fwding_info);
    fte::register_feature(FTE_FEATURE_FWDING_PRE_STAGE, fwding_pre_stage_exec);
    fte::register_feature(FTE_FEATURE_TUNNEL, tunnel_exec);
    fte::register_feature(FTE_FEATURE_QOS,  qos_exec);
    fte::register_feature(FTE_FEATURE_RDMA, rdma_exec);
    fte::register_feature(FTE_FEATURE_INGRESS_CHECKS,  ingress_checks_exec, ingress_info);

    return HAL_RET_OK;
}

extern "C" void network_exit() {
    fte::unregister_feature(FTE_FEATURE_STAGE_MARKER);
    fte::unregister_feature(FTE_FEATURE_FWDING);
    fte::unregister_feature(FTE_FEATURE_FWDING_PRE_STAGE);
    fte::unregister_feature(FTE_FEATURE_TUNNEL);
    fte::unregister_feature(FTE_FEATURE_QOS);
    fte::unregister_feature(FTE_FEATURE_RDMA);
    fte::unregister_feature(FTE_FEATURE_INGRESS_CHECKS);
}


}
}
}
