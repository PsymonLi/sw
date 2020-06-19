//                                                                                                                                                                              
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// service request message handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/agent/svc/specs.hpp"
#include "nic/apollo/agent/svc/nh_svc.hpp"
#include "nic/apollo/agent/svc/vpc_svc.hpp"
#include "nic/apollo/agent/svc/nat_svc.hpp"
#include "nic/apollo/agent/svc/vnic_svc.hpp"
#include "nic/apollo/agent/svc/dhcp_svc.hpp"
#include "nic/apollo/agent/svc/meter_svc.hpp"
#include "nic/apollo/agent/svc/route_svc.hpp"
#include "nic/apollo/agent/svc/device_svc.hpp"
#include "nic/apollo/agent/svc/policy_svc.hpp"
#include "nic/apollo/agent/svc/subnet_svc.hpp"
#include "nic/apollo/agent/svc/tunnel_svc.hpp"
#include "nic/apollo/agent/svc/svc_thread.hpp"
#include "nic/apollo/agent/svc/mapping_svc.hpp"
#include "nic/apollo/agent/svc/policer_svc.hpp"
#include "nic/apollo/agent/svc/service_svc.hpp"
#include "nic/apollo/agent/svc/interface_svc.hpp"
#include "nic/apollo/agent/svc/ipsec_svc.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/trace.hpp"


static sdk_ret_t
pds_handle_cfg (int fd, cfg_ctxt_t *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;
    types::ServiceResponseMessage proto_rsp;
    google::protobuf::Any *any_resp = proto_rsp.mutable_response();

    PDS_TRACE_VERBOSE("Received UDS config message {}", ctxt->cfg);

    switch (ctxt->cfg) {
    case CFG_MSG_VPC_CREATE:
    case CFG_MSG_VPC_UPDATE:
    case CFG_MSG_VPC_DELETE:
    case CFG_MSG_VPC_GET:
        ret = pds_svc_vpc_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_VPC_PEER_CREATE:
    case CFG_MSG_VPC_PEER_DELETE:
    case CFG_MSG_VPC_PEER_GET:
        ret = pds_svc_vpc_peer_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_VNIC_CREATE:
    case CFG_MSG_VNIC_UPDATE:
    case CFG_MSG_VNIC_DELETE:
    case CFG_MSG_VNIC_GET:
        ret = pds_svc_vnic_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_SUBNET_CREATE:
    case CFG_MSG_SUBNET_UPDATE:
    case CFG_MSG_SUBNET_DELETE:
    case CFG_MSG_SUBNET_GET:
        ret = pds_svc_subnet_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_MAPPING_CREATE:
    case CFG_MSG_MAPPING_UPDATE:
    case CFG_MSG_MAPPING_DELETE:
    case CFG_MSG_MAPPING_GET:
        ret = pds_svc_mapping_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_METER_CREATE:
    case CFG_MSG_METER_UPDATE:
    case CFG_MSG_METER_DELETE:
    case CFG_MSG_METER_GET:
        ret = pds_svc_meter_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_POLICER_CREATE:
    case CFG_MSG_POLICER_UPDATE:
    case CFG_MSG_POLICER_DELETE:
    case CFG_MSG_POLICER_GET:
        ret = pds_svc_policer_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_SECURITY_POLICY_CREATE:
    case CFG_MSG_SECURITY_POLICY_UPDATE:
    case CFG_MSG_SECURITY_POLICY_DELETE:
    case CFG_MSG_SECURITY_POLICY_GET:
        ret = pds_svc_security_policy_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_SECURITY_PROFILE_CREATE:
    case CFG_MSG_SECURITY_PROFILE_UPDATE:
    case CFG_MSG_SECURITY_PROFILE_DELETE:
    case CFG_MSG_SECURITY_PROFILE_GET:
        ret = pds_svc_security_profile_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_SECURITY_RULE_CREATE:
    case CFG_MSG_SECURITY_RULE_UPDATE:
    case CFG_MSG_SECURITY_RULE_DELETE:
    case CFG_MSG_SECURITY_RULE_GET:
        ret = pds_svc_security_rule_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_INTERFACE_CREATE:
    case CFG_MSG_INTERFACE_UPDATE:
    case CFG_MSG_INTERFACE_DELETE:
    case CFG_MSG_INTERFACE_GET:
    case CFG_MSG_LIF_GET:
        ret = pds_svc_interface_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_NAT_PORT_BLOCK_CREATE:
    case CFG_MSG_NAT_PORT_BLOCK_DELETE:
    case CFG_MSG_NAT_PORT_BLOCK_GET:
        ret = pds_svc_nat_port_block_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_DHCP_POLICY_CREATE:
    case CFG_MSG_DHCP_POLICY_UPDATE:
    case CFG_MSG_DHCP_POLICY_DELETE:
    case CFG_MSG_DHCP_POLICY_GET:
        ret = pds_svc_dhcp_policy_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_NEXTHOP_CREATE:
    case CFG_MSG_NEXTHOP_UPDATE:
    case CFG_MSG_NEXTHOP_DELETE:
    case CFG_MSG_NEXTHOP_GET:
        ret = pds_svc_nexthop_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_NHGROUP_CREATE:
    case CFG_MSG_NHGROUP_UPDATE:
    case CFG_MSG_NHGROUP_DELETE:
    case CFG_MSG_NHGROUP_GET:
        ret = pds_svc_nhgroup_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_ROUTE_TABLE_CREATE:
    case CFG_MSG_ROUTE_TABLE_UPDATE:
    case CFG_MSG_ROUTE_TABLE_DELETE:
    case CFG_MSG_ROUTE_TABLE_GET:
        ret = pds_svc_route_table_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_ROUTE_CREATE:
    case CFG_MSG_ROUTE_UPDATE:
    case CFG_MSG_ROUTE_DELETE:
    case CFG_MSG_ROUTE_GET:
        ret = pds_svc_route_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_SVC_MAPPING_CREATE:
    case CFG_MSG_SVC_MAPPING_UPDATE:
    case CFG_MSG_SVC_MAPPING_DELETE:
    case CFG_MSG_SVC_MAPPING_GET:
        ret = pds_svc_service_mapping_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_DEVICE_CREATE:
    case CFG_MSG_DEVICE_UPDATE:
    case CFG_MSG_DEVICE_DELETE:
    case CFG_MSG_DEVICE_GET:
        ret = pds_svc_device_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_TUNNEL_CREATE:
    case CFG_MSG_TUNNEL_UPDATE:
    case CFG_MSG_TUNNEL_DELETE:
    case CFG_MSG_TUNNEL_GET:
        ret = pds_svc_tunnel_handle_cfg(ctxt, any_resp);
        break;
    case CFG_MSG_IPSEC_SA_ENCRYPT_CREATE:
    case CFG_MSG_IPSEC_SA_ENCRYPT_DELETE:
    case CFG_MSG_IPSEC_SA_ENCRYPT_GET:
    case CFG_MSG_IPSEC_SA_DECRYPT_CREATE:
    case CFG_MSG_IPSEC_SA_DECRYPT_DELETE:
    case CFG_MSG_IPSEC_SA_DECRYPT_GET:
        ret = pds_svc_ipsec_sa_handle_cfg(ctxt, any_resp);
        break;
    default:
        PDS_TRACE_ERR("Unsupport config message {}", ctxt->cfg);
        return SDK_RET_INVALID_ARG;
    }

    proto_rsp.set_apistatus(sdk_ret_to_api_status(ret));
    if (!proto_rsp.SerializeToFileDescriptor(fd)) {
        PDS_TRACE_ERR("Serializing config message {} response to socket failed", ctxt->cfg);
    }

    return SDK_RET_OK;
}

static sdk_ret_t
pds_handle_cmd (cmd_ctxt_t *ctxt)
{
    sdk_ret_t ret;
    types::ServiceResponseMessage proto_rsp;

    ret = debug::pds_handle_cmd(ctxt);

    proto_rsp.set_apistatus(sdk_ret_to_api_status(ret));
    if (!proto_rsp.SerializeToFileDescriptor(ctxt->sock_fd)) {
        PDS_TRACE_ERR("Serializing command message {} response to socket failed", ctxt->cmd);
    }

    return ret;
}

sdk_ret_t
handle_svc_req (int fd, types::ServiceRequestMessage *proto_req, int cmd_fd)
{
    sdk_ret_t ret;
    svc_req_ctxt_t svc_req;

    memset(&svc_req, 0, sizeof(svc_req_ctxt_t));
    // convert proto svc request to svc req ctxt
    pds_svc_req_proto_to_svc_req_ctxt(&svc_req, proto_req, fd, cmd_fd);

    switch (svc_req.type) {
    case SVC_REQ_TYPE_CFG:
        ret = pds_handle_cfg(fd, &svc_req.cfg_ctxt);
        break;
    case SVC_REQ_TYPE_CMD:
        ret = pds_handle_cmd(&svc_req.cmd_ctxt);
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
    }
    return ret;
}
