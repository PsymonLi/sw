//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __NCSI_IPC_HPP__
#define __NCSI_IPC_HPP__

#include "gen/proto/ncsi.pb.h"
#include "nic/include/base.hpp"

using ncsi::VlanFilterRequest;
using ncsi::VlanFilterResponse;
using ncsi::VlanFilterGetRequest;
using ncsi::VlanFilterGetResponseMsg;
using ncsi::MacFilterRequest;
using ncsi::MacFilterResponse;
using ncsi::MacFilterGetRequest;
using ncsi::MacFilterGetResponseMsg;
using ncsi::BcastFilterRequest;
using ncsi::BcastFilterResponse;
using ncsi::BcastFilterDeleteRequest;
using ncsi::BcastFilterDeleteResponse;
using ncsi::BcastFilterGetRequest;
using ncsi::BcastFilterGetResponseMsg;
using ncsi::McastFilterRequest;
using ncsi::McastFilterResponse;
using ncsi::McastFilterDeleteRequest;
using ncsi::McastFilterDeleteResponse;
using ncsi::McastFilterGetRequest;
using ncsi::McastFilterGetResponseMsg;
using ncsi::VlanModeRequest;
using ncsi::VlanModeResponse;
using ncsi::VlanModeGetRequest;
using ncsi::VlanModeGetResponseMsg;
using ncsi::VlanModeGetResponse;
using ncsi::ChannelRequest;
using ncsi::ChannelResponse;
using ncsi::ChannelGetRequest;
using ncsi::ChannelGetResponseMsg;
using google::protobuf::Message;

namespace hal {

#define NCSI_MSG_ID(ENTRY)                                                 \
    ENTRY(NCSI_MSG_VLAN_FILTER,            0, "NCSI_MSG_VLAN_FILTER")      \
    ENTRY(NCSI_MSG_MAC_FILTER,             1, "NCSI_MSG_MAC_FILTER")       \
    ENTRY(NCSI_MSG_BCAST_FILTER,           2, "NCSI_MSG_BCAST_FILTER")     \
    ENTRY(NCSI_MSG_MCAST_FILTER,           3, "NCSI_MSG_MCAST_FILTER")     \
    ENTRY(NCSI_MSG_VLAN_MODE,              4, "NCSI_MSG_VLAN_MODE")        \
    ENTRY(NCSI_MSG_CHANNEL,                5, "NCSI_MSG_CHANNEL") 

DEFINE_ENUM(ncsi_msg_id_t, NCSI_MSG_ID)
#undef NCSI_MSG_ID

#define NCSI_MSG_OPER(ENTRY)                                            \
    ENTRY(NCSI_MSG_OPER_CREATE,     0, "NCSI_MSG_OPER_CREATE")          \
    ENTRY(NCSI_MSG_OPER_UPDATE,     1, "NCSI_MSG_OPER_UPDATE")          \
    ENTRY(NCSI_MSG_OPER_DELETE,     2, "NCSI_MSG_OPER_DELETE")          \
    ENTRY(NCSI_MSG_OPER_GET,        3, "NCSI_MSG_OPER_GET")         

DEFINE_ENUM(ncsi_msg_oper_t, NCSI_MSG_OPER)
#undef NCSI_MSG_OPER

#define NCSI_VLAN_MODE(ENTRY)                                                        \
    ENTRY(NCSI_VLAN_MODE0_RSVD,             0, "NCSI_VLAN_MODE0_RSVD")               \
    ENTRY(NCSI_VLAN_MODE1_VLAN,             1, "NCSI_VLAN_MODE1_VLAN")               \
    ENTRY(NCSI_VLAN_MODE2_VLAN_NATIVE,      2, "NCSI_VLAN_MODE2_VLAN_NATIVE")        \
    ENTRY(NCSI_VLAN_MODE3_ANY_VLAN_NATIVE,  3, "NCSI_VLAN_MODE3_ANY_VLAN_NATIVE")    \
    ENTRY(NCSI_VLAN_MODE4_RSVD,             4, "NCSI_VLAN_MODE4_RSVD")  
DEFINE_ENUM(ncsi_vlan_mode_t, NCSI_VLAN_MODE)
#undef NCSI_VLAN_MODE

typedef struct vlan_filter_s {
    uint32_t id;
    uint32_t vlan_id;
    uint32_t channel;
} __PACK__ vlan_filter_t;

typedef struct mac_filter_s {
    uint32_t id;
    uint64_t mac_addr;
    uint32_t channel;
} __PACK__ mac_filter_t;

typedef struct vlan_mode_s {
    bool enable;
    uint32_t mode;
    uint32_t channel;
} __PACK__ vlan_mode_t;

typedef struct channel_state_s {
    bool tx_enable;
    bool rx_enable;
    bool reset;
    uint32_t channel;
} __PACK__ channel_state_t;

typedef struct bcast_filter_s {
    bool enable_arp;
    bool enable_dhcp_client;
    bool enable_dhcp_server;
    bool enable_netbios;
    uint32_t channel;
} __PACK__ bcast_filter_t;

typedef struct mcast_filter_s {
    bool enable_ipv6_neigh_adv;
    bool enable_ipv6_router_adv;
    bool enable_dhcpv6_relay;
    bool enable_dhcpv6_mcast;
    bool enable_ipv6_mld;
    bool enable_ipv6_neigh_sol;
    uint32_t channel;
} __PACK__ mcast_filter_t;


typedef struct ncsi_ipc_msg_s {
    ncsi_msg_id_t   msg_id;
    ncsi_msg_oper_t oper;
    Message         *msg;
    Message         *rsp;
    union {
        vlan_filter_t   vlan_filter;
        mac_filter_t    mac_filter;
        vlan_mode_t     vlan_mode;
        channel_state_t channel_state;
        bcast_filter_t  bcast_filter;
        mcast_filter_t  mcast_filter;
    };
    sdk_ret_t       rsp_ret;    // Return code from nicmgr
} ncsi_ipc_msg_t;


}    // namespace hal

#endif    // __NCSI_IPC_HPP__

