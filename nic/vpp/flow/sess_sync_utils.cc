//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
#include <nic/sdk/include/sdk/base.hpp>
#include <sess_sync.pb.h>
#include "pdsa_hdlr.h"

using namespace sess_sync;

static const FlowState encoded_state[] = {
    [PDS_FLOW_STATE_CONN_INIT]        = FLOW_STATE_INIT,
    [PDS_FLOW_STATE_CONN_SETUP]       = FLOW_STATE_SETUP,
    [PDS_FLOW_STATE_ESTABLISHED]      = FLOW_STATE_ESTABLISHED,
    [PDS_FLOW_STATE_KEEPALIVE_SENT]   = FLOW_STATE_KEEPALIVE,
    [PDS_FLOW_STATE_HALF_CLOSE_IFLOW] = FLOW_STATE_HALFCLOSE_IFLOW,
    [PDS_FLOW_STATE_HALF_CLOSE_RFLOW] = FLOW_STATE_HALFCLOSE_RFLOW,
    [PDS_FLOW_STATE_CLOSE]            = FLOW_STATE_CLOSE
};

static pds_flow_state decoded_state[] = {
    [FLOW_STATE_NONE]            = PDS_FLOW_STATE_CONN_INIT, // To avoid compilation error
    [FLOW_STATE_INIT]            = PDS_FLOW_STATE_CONN_INIT,
    [FLOW_STATE_SETUP]           = PDS_FLOW_STATE_CONN_SETUP,
    [FLOW_STATE_ESTABLISHED]     = PDS_FLOW_STATE_ESTABLISHED,
    [FLOW_STATE_KEEPALIVE]       = PDS_FLOW_STATE_KEEPALIVE_SENT,
    [FLOW_STATE_HALFCLOSE_IFLOW] = PDS_FLOW_STATE_HALF_CLOSE_IFLOW,
    [FLOW_STATE_HALFCLOSE_RFLOW] = PDS_FLOW_STATE_HALF_CLOSE_RFLOW,
    [FLOW_STATE_CLOSE]           = PDS_FLOW_STATE_CLOSE
};

static const FlowPacketType encoded_type[] = {
    [PDS_FLOW_L2L_INTRA_SUBNET]              = FLOW_PKT_TYPE_L2L_INTRA_SUBNET,
    [PDS_FLOW_L2L_INTER_SUBNET]              = FLOW_PKT_TYPE_L2L_INTER_SUBNET,
    [PDS_FLOW_L2R_INTRA_SUBNET]              = FLOW_PKT_TYPE_L2R_INTRA_SUBNET,
    [PDS_FLOW_L2R_INTER_SUBNET]              = FLOW_PKT_TYPE_L2R_INTER_SUBNET,
    [PDS_FLOW_R2L_INTRA_SUBNET]              = FLOW_PKT_TYPE_R2L_INTRA_SUBNET,
    [PDS_FLOW_R2L_INTER_SUBNET]              = FLOW_PKT_TYPE_R2L_INTER_SUBNET,
    [PDS_FLOW_L2N_ASYMMETRIC_ROUTE]          = FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE,
    [PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAPT]     = FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE_NAPT,
    [PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAT]      = FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE_NAT,
    [PDS_FLOW_L2N_SYMMETRIC_ROUTE]           = FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE,
    [PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAPT]      = FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_NAPT,
    [PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAT]       = FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_NAT,
    [PDS_FLOW_L2N_SYMMETRIC_ROUTE_TWICE_NAT] = FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_TWICE_NAT,
    [PDS_FLOW_L2N_INTRA_VCN_ROUTE]           = FLOW_PKT_TYPE_L2N_INTRA_VCN_ROUTE,
    [PDS_FLOW_N2L_ASYMMETRIC_ROUTE]          = FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE,
    [PDS_FLOW_N2L_ASYMMETRIC_ROUTE_NAT]      = FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE_NAT,
    [PDS_FLOW_N2L_SYMMETRIC_ROUTE]           = FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE,
    [PDS_FLOW_N2L_SYMMETRIC_ROUTE_NAT]       = FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE_NAT,
    [PDS_FLOW_N2L_ASYMMETRIC_ROUTE_SVC_NAT]  = FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE_SVC_NAT,
    [PDS_FLOW_N2L_SYMMETRIC_ROUTE_SVC_NAT]   = FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE_SVC_NAT,
    [PDS_FLOW_N2L_INTRA_VCN_ROUTE]           = FLOW_PKT_TYPE_N2L_INTRA_VCN_ROUTE
};

static pds_flow_pkt_type decoded_type[] = {
    [FLOW_PKT_TYPE_NONE]                          = PDS_FLOW_L2L_INTRA_SUBNET, // To avoid compilation error
    [FLOW_PKT_TYPE_L2L_INTRA_SUBNET]              = PDS_FLOW_L2L_INTRA_SUBNET,
    [FLOW_PKT_TYPE_L2L_INTER_SUBNET]              = PDS_FLOW_L2L_INTER_SUBNET,
    [FLOW_PKT_TYPE_L2R_INTRA_SUBNET]              = PDS_FLOW_L2R_INTRA_SUBNET,
    [FLOW_PKT_TYPE_L2R_INTER_SUBNET]              = PDS_FLOW_L2R_INTER_SUBNET,
    [FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE]          = PDS_FLOW_L2N_ASYMMETRIC_ROUTE,
    [FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE_NAPT]     = PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAPT,
    [FLOW_PKT_TYPE_L2N_ASYMMETRIC_ROUTE_NAT]      = PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAT,
    [FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE]           = PDS_FLOW_L2N_SYMMETRIC_ROUTE,
    [FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_NAPT]      = PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAPT,
    [FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_NAT]       = PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAT,
    [FLOW_PKT_TYPE_L2N_SYMMETRIC_ROUTE_TWICE_NAT] = PDS_FLOW_L2N_SYMMETRIC_ROUTE_TWICE_NAT,
    [FLOW_PKT_TYPE_L2N_INTRA_VCN_ROUTE]           = PDS_FLOW_L2N_INTRA_VCN_ROUTE,
    [FLOW_PKT_TYPE_R2L_INTRA_SUBNET]              = PDS_FLOW_R2L_INTRA_SUBNET,
    [FLOW_PKT_TYPE_R2L_INTER_SUBNET]              = PDS_FLOW_R2L_INTER_SUBNET,
    [FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE]          = PDS_FLOW_N2L_ASYMMETRIC_ROUTE,
    [FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE_NAT]      = PDS_FLOW_N2L_ASYMMETRIC_ROUTE_NAT,
    [FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE]           = PDS_FLOW_N2L_SYMMETRIC_ROUTE,
    [FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE_NAT]       = PDS_FLOW_N2L_SYMMETRIC_ROUTE_NAT,
    [FLOW_PKT_TYPE_N2L_ASYMMETRIC_ROUTE_SVC_NAT]  = PDS_FLOW_N2L_ASYMMETRIC_ROUTE_SVC_NAT,
    [FLOW_PKT_TYPE_N2L_SYMMETRIC_ROUTE_SVC_NAT]   = PDS_FLOW_N2L_SYMMETRIC_ROUTE_SVC_NAT,
    [FLOW_PKT_TYPE_N2L_INTRA_VCN_ROUTE]           = PDS_FLOW_N2L_INTRA_VCN_ROUTE
};

uint32_t
pds_encode_flow_state (pds_flow_state flow_state)
{
    return encoded_state[flow_state];
}

bool
pds_decode_flow_state (uint32_t encoded_flow_state, pds_flow_state *flow_state)
{
    if (unlikely((encoded_flow_state < FLOW_STATE_INIT) ||
                 (encoded_flow_state > FLOW_STATE_CLOSE))) {
        return false;
    }
    *flow_state = decoded_state[encoded_flow_state];
    return true;
}

uint32_t
pds_encode_flow_pkt_type (pds_flow_pkt_type flow_type)
{
    return encoded_type[flow_type];
}

bool
pds_decode_flow_pkt_type (uint32_t encoded_flow_type,
                          pds_flow_pkt_type *flow_type)
{
    if (unlikely((encoded_flow_type < FLOW_PKT_TYPE_L2L_INTRA_SUBNET) ||
                 (encoded_flow_type > FLOW_PKT_TYPE_N2L_INTRA_VCN_ROUTE))) {
        return false;
    }
    *flow_type = decoded_type[encoded_flow_type];
    return true;
}

