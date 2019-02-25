/*
* Copyright (c) 2018, Pensando Systems Inc.
*/

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <bitset>
#include <cmath>
#include <map>
#include <thread>
#include <chrono>
#include <sstream>
#include <grpc++/grpc++.h>

#include "logger.hpp"
#include "hal_client.hpp"
#include "nicmgr_utils.hpp"

using namespace kh;
using namespace types;
using namespace intf;
using namespace endpoint;
using namespace l2segment;
using namespace multicast;
using namespace accelRGroup;
//using namespace internal;

using namespace grpc;
using namespace std;

HalClient::HalClient(enum ForwardingMode fwd_mode)
{
    string url = std::string("localhost:50054");
    if (getenv("HAL_GRPC_PORT")) {
        url = string("localhost:") + getenv("HAL_GRPC_PORT");
    }

    NIC_LOG_DEBUG("Connecting to HAL @ {}", url);
    channel = CreateChannel(url, InsecureChannelCredentials());

    NIC_LOG_DEBUG("Waiting for HAL to be ready ...");
    auto state = channel->GetState(true);
    while (state != GRPC_CHANNEL_READY) {
        // Wait for State change or deadline
        channel->WaitForStateChange(state, gpr_time_from_seconds(1, GPR_TIMESPAN));
        state = channel->GetState(true);
    }

    vrf_stub_ = Vrf::NewStub(channel);
    intf_stub_ = Interface::NewStub(channel);
    internal_stub_ = Internal::NewStub(channel);
    ep_stub_ = Endpoint::NewStub(channel);
    l2seg_stub_ = L2Segment::NewStub(channel);
    multicast_stub_ = Multicast::NewStub(channel);
    rdma_stub_ = Rdma::NewStub(channel);
    accel_rgroup_stub_ = AccelRGroup::NewStub(channel);
    crypto_stub_ = Internal::NewStub(channel);
    port_stub_ = Port::NewStub(channel);

    this->fwd_mode = fwd_mode;
}

uint64_t
HalClient::LifCreate(hal_lif_info_t *hal_lif_info)
{
    NIC_LOG_DEBUG("Creating with Lif: type: {}, uplink_port: {}, hw_lif_id: {}, rdma_en: {}",
                 hal_lif_info->type,
                 hal_lif_info->pinned_uplink_port_num,
                 hal_lif_info->hw_lif_id,
                 hal_lif_info->enable_rdma);

    // Nicmgr should always allocate hw_lif_id and pass to HAL
    NIC_ASSERT(hal_lif_info->hw_lif_id != 0);

    Lif *eth_lif = Lif::Factory(hal_lif_info);
    hal_lif_info->pushed_to_hal = true;

    eth_lif_map[hal_lif_info->hw_lif_id] = eth_lif;

    NIC_LOG_DEBUG("hw_lif_id {} eth_lif->id {} ",
                 hal_lif_info->hw_lif_id,
                 eth_lif->GetHwLifId());

    // Passed hw_lif_id should be same as HAL returned
    NIC_ASSERT(hal_lif_info->hw_lif_id == eth_lif->GetHwLifId());

    NIC_LOG_DEBUG("lif-{} Created", hal_lif_info->hw_lif_id);

    return 0;
}

int HalClient::PgmBaseAddrGet(const char *prog_name, uint64_t *base_addr)
{
    ClientContext               context;
    GetProgramAddressRequestMsg req_msg;
    ProgramAddressResponseMsg   resp_msg;

    auto req = req_msg.add_request();
    req->set_handle("p4plus");
    req->set_prog_name(prog_name);
    req->set_resolve_label(false);

    auto status = internal_stub_->GetProgramAddress(&context, req_msg, &resp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(), status.error_message());
        return -1;
    }

    *base_addr = resp_msg.response(0).addr();
    return 0;
}

int
HalClient::AccelRGroupAdd(const std::string& rgroup_name)
{
    AccelRGroupAddRequestMsg    req_msg;
    AccelRGroupAddResponseMsg   rsp_msg;
    ClientContext               context;
    Status                      status;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    status = accel_rgroup_stub_->AccelRGroupAdd(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {}", rsp.api_status(),
                     rgroup_name);
        return -1;
    }
    return 0;
}

int
HalClient::AccelRGroupDel(const std::string& rgroup_name)
{
    AccelRGroupDelRequestMsg    req_msg;
    AccelRGroupDelResponseMsg   rsp_msg;
    ClientContext               context;
    Status                      status;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    status = accel_rgroup_stub_->AccelRGroupDel(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {}", rsp.api_status(),
                     rgroup_name);
        return -1;
    }

    return 0;
}

int
HalClient::AccelRGroupRingAdd(const std::string& rgroup_name,
           const std::vector<std::pair<const std::string,uint32_t>>& ring_vec)
{
    AccelRGroupRingAddRequestMsg    req_msg;
    AccelRGroupRingAddResponseMsg   rsp_msg;
    ClientContext                   context;
    Status                          status;
    int                             i;

    assert(ring_vec.size());
    for (i = 0; i < (int)ring_vec.size(); i++) {
        auto req = req_msg.add_request();
        req->set_rgroup_name(rgroup_name);
        req->set_ring_name(ring_vec[i].first);
        req->set_ring_handle(ring_vec[i].second);
    }
    status = accel_rgroup_stub_->AccelRGroupRingAdd(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    assert(rsp_msg.response_size() == (int)ring_vec.size());
    for (i = 0; i < rsp_msg.response_size(); i++) {
        auto rsp = rsp_msg.response(i);
        if (rsp.api_status() != types::API_STATUS_OK) {
            NIC_FUNC_ERR("API status {} rgroup_name {} ring_name {}",
                         rsp.api_status(), rgroup_name, ring_vec[i].first);
            return -1;
        }
    }

    return 0;
}

int
HalClient::AccelRGroupRingDel(const std::string& rgroup_name,
           const std::vector<std::pair<const std::string,uint32_t>>& ring_vec)
{
    AccelRGroupRingDelRequestMsg    req_msg;
    AccelRGroupRingDelResponseMsg   rsp_msg;
    ClientContext                   context;
    Status                          status;
    int                             i;

    assert(ring_vec.size());
    for (i = 0; i < (int)ring_vec.size(); i++) {
        auto req = req_msg.add_request();
        req->set_rgroup_name(rgroup_name);
        req->set_ring_name(ring_vec[i].first);
    }
    status = accel_rgroup_stub_->AccelRGroupRingDel(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    assert(rsp_msg.response_size() == (int)ring_vec.size());
    for (i = 0; i < rsp_msg.response_size(); i++) {
        auto rsp = rsp_msg.response(i);
        if (rsp.api_status() != types::API_STATUS_OK) {
            NIC_FUNC_ERR("API status {} rgroup_name {} ring_name {}",
                         rsp.api_status(), rgroup_name, ring_vec[i].first);
            return -1;
        }
    }

    return 0;
}

int
HalClient::AccelRGroupResetSet(const std::string& rgroup_name,
                               uint32_t sub_ring,
                               bool reset_sense)
{
    AccelRGroupResetSetRequestMsg   req_msg;
    AccelRGroupResetSetResponseMsg  rsp_msg;
    ClientContext                   context;
    Status                          status;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    req->set_reset_sense(reset_sense);
    status = accel_rgroup_stub_->AccelRGroupResetSet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {} last_ring_handle {} last_sub_ring {}",
                     rsp.api_status(), rgroup_name, rsp.last_ring_handle(),
                     rsp.last_sub_ring());
        return -1;
    }
    return 0;
}

int
HalClient::AccelRGroupEnableSet(const std::string& rgroup_name,
                               uint32_t sub_ring,
                               bool enable_sense)
{
    AccelRGroupEnableSetRequestMsg  req_msg;
    AccelRGroupEnableSetResponseMsg rsp_msg;
    ClientContext                   context;
    Status                          status;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    req->set_enable_sense(enable_sense);
    status = accel_rgroup_stub_->AccelRGroupEnableSet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {} last_ring_handle {} last_sub_ring {}",
                     rsp.api_status(), rgroup_name, rsp.last_ring_handle(),
                     rsp.last_sub_ring());
        return -1;
    }
    return 0;
}

int
HalClient::AccelRGroupPndxSet(const std::string& rgroup_name,
                              uint32_t sub_ring,
                              uint32_t val,
                              bool conditional)
{
    AccelRGroupPndxSetRequestMsg    req_msg;
    AccelRGroupPndxSetResponseMsg   rsp_msg;
    ClientContext                   context;
    Status                          status;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    req->set_val(val);
    req->set_conditional(conditional);
    status = accel_rgroup_stub_->AccelRGroupPndxSet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {} last_ring_handle {} last_sub_ring {}",
                     rsp.api_status(), rgroup_name, rsp.last_ring_handle(),
                     rsp.last_sub_ring());
        return -1;
    }
    return 0;
}

int
HalClient::AccelRGroupInfoGet(const std::string& rgroup_name,
                              uint32_t sub_ring,
                              accel_rgroup_rinfo_rsp_cb_t rsp_cb_func,
                              void *user_ctx,
                              uint32_t& ret_num_entries)
{
    AccelRGroupInfoGetRequestMsg    req_msg;
    AccelRGroupInfoGetResponseMsg   rsp_msg;
    ClientContext                   context;
    Status                          status;
    int                             i;

    ret_num_entries = 0;
    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    status = accel_rgroup_stub_->AccelRGroupInfoGet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {}",
                     rsp.api_status(), rgroup_name);
        return -1;
    }

    ret_num_entries = (uint32_t)rsp.ring_info_spec_size();
    for (i = 0; i < (int)ret_num_entries; i++) {
        accel_rgroup_rinfo_rsp_t rinfo = {0};
        auto spec = rsp.ring_info_spec(i);

        rinfo.ring_handle = spec.ring_handle();
        rinfo.sub_ring = spec.sub_ring();
        rinfo.base_pa = spec.base_pa();
        rinfo.pndx_pa = spec.pndx_pa();
        rinfo.shadow_pndx_pa = spec.shadow_pndx_pa();
        rinfo.opaque_tag_pa = spec.opaque_tag_pa();
        rinfo.opaque_tag_size = spec.opaque_tag_size();
        rinfo.ring_size = spec.ring_size();
        rinfo.desc_size = spec.desc_size();
        rinfo.pndx_size = spec.pndx_size();
        rinfo.sw_reset_capable = spec.sw_reset_capable();
        rinfo.sw_enable_capable = spec.sw_enable_capable();
        (*rsp_cb_func)(user_ctx, rinfo);
    }
    return 0;
}

int
HalClient::AccelRGroupIndicesGet(const std::string& rgroup_name,
                                 uint32_t sub_ring,
                                 accel_rgroup_rindices_rsp_cb_t rsp_cb_func,
                                 void *user_ctx,
                                 uint32_t& ret_num_entries)
{
    AccelRGroupIndicesGetRequestMsg     req_msg;
    AccelRGroupIndicesGetResponseMsg    rsp_msg;
    ClientContext                       context;
    Status                              status;
    int                                 i;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    status = accel_rgroup_stub_->AccelRGroupIndicesGet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {}",
                     rsp.api_status(), rgroup_name);
        return -1;
    }

    ret_num_entries = (uint32_t)rsp.ring_indices_spec_size();
    for (i = 0; i < (int)ret_num_entries; i++) {
        accel_rgroup_rindices_rsp_t rindices = {0};
        auto spec = rsp.ring_indices_spec(i);

        rindices.ring_handle = spec.ring_handle();
        rindices.sub_ring = spec.sub_ring();
        rindices.pndx = spec.pndx();
        rindices.cndx = spec.cndx();
        (*rsp_cb_func)(user_ctx, rindices);
    }
    return 0;
}

int
HalClient::AccelRGroupMetricsGet(const std::string& rgroup_name,
                                 uint32_t sub_ring,
                                 accel_rgroup_rmetrics_rsp_cb_t rsp_cb_func,
                                 void *user_ctx,
                                 uint32_t& ret_num_entries)
{
    AccelRGroupMetricsGetRequestMsg     req_msg;
    AccelRGroupMetricsGetResponseMsg    rsp_msg;
    ClientContext                       context;
    Status                              status;
    int                                 i;

    auto req = req_msg.add_request();
    req->set_rgroup_name(rgroup_name);
    req->set_sub_ring(sub_ring);
    status = accel_rgroup_stub_->AccelRGroupMetricsGet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto rsp = rsp_msg.response(0);
    if (rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} rgroup_name {}",
                     rsp.api_status(), rgroup_name);
        return -1;
    }

    ret_num_entries = (uint32_t)rsp.ring_metrics_spec_size();
    for (i = 0; i < (int)ret_num_entries; i++) {
        accel_rgroup_rmetrics_rsp_t rmetrics = {0};
        auto spec = rsp.ring_metrics_spec(i);

        rmetrics.ring_handle = spec.ring_handle();
        rmetrics.sub_ring = spec.sub_ring();
        rmetrics.input_bytes = spec.input_bytes();
        rmetrics.output_bytes = spec.output_bytes();
        rmetrics.soft_resets = spec.soft_resets();
        (*rsp_cb_func)(user_ctx, rmetrics);
    }
    return 0;
}

int
HalClient::CryptoKeyIndexUpdate(uint32_t key_index,
                                types::CryptoKeyType key_type,
                                void *key,
                                uint32_t key_size)
{
    CryptoKeyCreateWithIdRequestMsg     create_req_msg;
    CryptoKeyCreateWithIdResponseMsg    create_rsp_msg;
    CryptoKeyUpdateRequestMsg           update_req_msg;
    CryptoKeyUpdateResponseMsg          update_rsp_msg;
    ClientContext                       context;
    ClientContext                       context2;
    Status                              status;

    auto create_req = create_req_msg.add_request();
    create_req->set_keyindex(key_index);
    create_req->set_allow_dup_alloc(true);
    status = crypto_stub_->CryptoKeyCreateWithId(&context, create_req_msg,
                                                 &create_rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC create status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    auto create_rsp = create_rsp_msg.response(0);
    if (create_rsp.keyindex() != key_index) {
        NIC_FUNC_ERR("GRPC create expected key_index {} actual {}",
                     key_index, create_rsp.keyindex());
        return -1;
    }

    auto update_req = update_req_msg.add_request();
    auto update_spec = update_req->mutable_key();
    update_spec->set_key(key, key_size);
    update_spec->set_keyindex(key_index);
    update_spec->set_key_size(key_size);
    update_spec->set_key_type(key_type);

    status = crypto_stub_->CryptoKeyUpdate(&context2, update_req_msg,
                                           &update_rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC update status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }
    auto update_rsp = update_rsp_msg.response(0);
    if (update_rsp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API update status {} key_index {}",
                     update_rsp.api_status(), key_index);
        return -1;
    }
    return 0;
}

uint32_t
HalClient::PortSpeedInMbps(port::PortSpeed speed_enum)
{
    uint32_t speed = 0;

    switch (speed_enum) {
        case PortSpeed::PORT_SPEED_NONE:
            speed = 0;
            break;
        case PortSpeed::PORT_SPEED_1G:
            speed = 1000;
            break;
        case PortSpeed::PORT_SPEED_10G:
            speed = 10000;
            break;
        case PortSpeed::PORT_SPEED_25G:
            speed = 25000;
            break;
        case PortSpeed::PORT_SPEED_40G:
            speed = 40000;
            break;
        case PortSpeed::PORT_SPEED_50G:
            speed = 50000;
            break;
        case PortSpeed::PORT_SPEED_100G:
            speed = 100000;
            break;
        default:
            NIC_FUNC_ERR("Invalid speed {}", speed);
            speed = 0;
    }

    return speed;
}

port::PortSpeed
HalClient::PortSpeedEnum(uint32_t speed)
{
    port::PortSpeed speed_enum;

    switch (speed) {
        case 0:
            speed_enum = port::PortSpeed::PORT_SPEED_NONE;
            break;
        case 1000:
            speed_enum = port::PortSpeed::PORT_SPEED_1G;
            break;
        case 10000:
            speed_enum = port::PortSpeed::PORT_SPEED_10G;
            break;
        case 25000:
            speed_enum = port::PortSpeed::PORT_SPEED_25G;
            break;
        case 40000:
            speed_enum = port::PortSpeed::PORT_SPEED_40G;
            break;
        case 50000:
            speed_enum = port::PortSpeed::PORT_SPEED_50G;
            break;
        case 100000:
            speed_enum = port::PortSpeed::PORT_SPEED_100G;
            break;
        default:
            NIC_FUNC_ERR("Invalid speed_enum {}", speed);
            speed_enum = port::PortSpeed::PORT_SPEED_NONE;
    }

    return speed_enum;
}

int
HalClient::PortStatusGet(uint8_t portnum, hal_port_status_t &st)
{
    PortGetRequestMsg       req_msg;
    PortGetResponseMsg      rsp_msg;
    ClientContext           context;
    Status                  status;

    auto req = req_msg.add_request();
    req->mutable_key_or_handle()->set_port_id(portnum);
    status = port_stub_->PortGet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC update status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    auto port = rsp_msg.response(0);
    if (port.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} port {}", port.api_status(), portnum);
        return -1;
    }

    st.id = port.status().key_or_handle().port_id();
    st.status = port.status().oper_status();
    st.speed = PortSpeedInMbps(port.status().port_speed());
    NIC_FUNC_DEBUG("{}: port_speed_enum {} port_speed {}", portnum,
        port.status().port_speed(), st.speed);
    st.xcvr.state = port.status().xcvr_status().state();
    st.xcvr.phy = port.status().xcvr_status().cable_type();
    st.xcvr.pid = port.status().xcvr_status().pid();
    memcpy(st.xcvr.sprom,
        port.status().xcvr_status().xcvr_sprom().c_str(),
        MIN(port.status().xcvr_status().xcvr_sprom().size(), 
        sizeof (st.xcvr.sprom)));

    NIC_FUNC_DEBUG("{}: id {} status {} speed {} "
        " xcvr.state {} xcvr.phy {} xcvr.pid {}",
        portnum, st.id, st.status, st.speed,
        st.xcvr.state, st.xcvr.phy, st.xcvr.pid);

    return 0;
}

int
HalClient::PortConfigGet(uint8_t portnum, hal_port_config_t &cfg)
{
    PortGetRequestMsg       req_msg;
    PortGetResponseMsg      rsp_msg;
    ClientContext           context;
    Status                  status;

    auto req = req_msg.add_request();
    req->mutable_key_or_handle()->set_port_id(portnum);
    status = port_stub_->PortGet(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC update status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    auto port = rsp_msg.response(0);
    if (port.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("API status {} port {}", port.api_status(), portnum);
        return -1;
    }

    cfg.state = port.spec().admin_state();
    cfg.speed = PortSpeedInMbps(port.spec().port_speed());
    NIC_FUNC_DEBUG("{}: port_speed_enum {} port_speed {}", portnum,
        port.spec().port_speed(), cfg.speed);
    cfg.mtu = port.spec().mtu();
    cfg.an_enable = port.spec().auto_neg_enable();
    cfg.fec_type = port.spec().fec_type();
    cfg.pause_type = port.spec().pause();
    cfg.loopback_mode = port.spec().loopback_mode();

    NIC_FUNC_DEBUG("{}: state {} speed {} mtu {} "
        "an_enable {} fec_type {} pause_type {} loopback_mode {}",
        portnum, cfg.state, cfg.speed, cfg.mtu,
        cfg.an_enable, cfg.fec_type, cfg.pause_type, cfg.loopback_mode);

    return 0;
}

int
HalClient::PortConfigSet(uint8_t portnum, hal_port_config_t &cfg)
{
    PortGetRequestMsg       get_req_msg;
    PortGetResponseMsg      get_rsp_msg;
    ClientContext           get_context;
    Status                  get_status;

    auto get_req = get_req_msg.add_request();
    get_req->mutable_key_or_handle()->set_port_id(portnum);
    get_status = port_stub_->PortGet(&get_context, get_req_msg, &get_rsp_msg);
    if (!get_status.ok()) {
        NIC_FUNC_ERR("GRPC update status {} {}", get_status.error_code(),
                     get_status.error_message());
        return -1;
    }

    auto port = get_rsp_msg.response(0);
    if (port.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("PortGet API status {} port {}", port.api_status(), portnum);
        return -1;
    }

    PortRequestMsg          req_msg;
    PortResponseMsg         rsp_msg;
    ClientContext           context;
    Status                  status;

    auto req = req_msg.add_request();
    req->CopyFrom(port.spec());
    req->set_admin_state((port::PortAdminState)cfg.state);
    req->set_port_speed(PortSpeedEnum(cfg.speed));
    req->set_mtu(cfg.mtu);
    req->set_auto_neg_enable(cfg.an_enable);
    req->set_fec_type((port::PortFecType)cfg.fec_type);
    req->set_pause((port::PortPauseType)cfg.pause_type);
    req->set_loopback_mode((port::PortLoopBackMode)cfg.loopback_mode);
    status = port_stub_->PortUpdate(&context, req_msg, &rsp_msg);
    if (!status.ok()) {
        NIC_FUNC_ERR("GRPC update status {} {}", status.error_code(),
                     status.error_message());
        return -1;
    }

    auto resp = rsp_msg.response(0);
    if (resp.api_status() != types::API_STATUS_OK) {
        NIC_FUNC_ERR("PortUpdate API status {} port {}", resp.api_status(), portnum);
        return -1;
    }

    NIC_FUNC_DEBUG("{}: state {} speed {} mtu {} "
        "an_enable {} fec_type {} pause_type {} loopback_mode {}",
        portnum, cfg.state, cfg.speed, cfg.mtu,
        cfg.an_enable, cfg.fec_type, cfg.pause_type, cfg.loopback_mode);

    return (0);
}
