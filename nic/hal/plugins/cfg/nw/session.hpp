//-----------------------------------------------------------------------------

// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include "nic/include/base.hpp"
#include "lib/list/list.hpp"
#include "lib/ht/ht.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "gen/proto/session.pb.h"
#include "nic/include/pd.hpp"
#include "nic/hal/plugins/cfg/aclqos/qos.hpp"
#include "gen/proto/system.pb.h"
#include "nic/p4/common/defines.h"

#define HAL_MAX_INACTIVTY_TIMEOUT    0xFFFFFFFF

using sdk::lib::ht_ctxt_t;
using sdk::lib::dllist_ctxt_t;

using session::FlowKeyL2;
using session::FlowKeyV4;
using session::FlowKeyV6;
using session::FlowKey;
using session::FlowData;
using session::FlowInfo;
using session::FlowSpec;
using session::FlowStatus;
using session::ConnTrackInfo;
using session::SessionSpec;
using session::SessionStatus;
using session::SessionFilter;
using session::SessionResponse;
using session::SessionRequestMsg;
using session::SessionResponseMsg;
using session::SessionDeleteRequestMsg;
using session::SessionDeleteResponseMsg;
using session::SessionGetRequest;
using session::SessionGetRequestMsg;
using session::SessionGetResponse;
using session::SessionGetResponseMsg;
using session::SessionDeleteRequest;
using session::SessionDeleteResponse;
using session::FlowTCPState;
using types::IPProtocol;
using sys::SystemResponse;

namespace hal {

// forward declarations
typedef struct flow_s flow_t;
typedef struct session_s session_t;

// from p4pd.h
typedef enum tunnel_rewrite_actions_enum {
    TUNNEL_REWRITE_NOP_ID = 0,
    TUNNEL_REWRITE_ENCAP_VXLAN_ID = 1,
    TUNNEL_REWRITE_ENCAP_ERSPAN_ID = 2,
    TUNNEL_REWRITE_ENCAP_VLAN_ID = 3,
    TUNNEL_REWRITE_ENCAP_MPLS_UDP_ID = 4,
    TUNNEL_REWRITE_MAX_ID = 5
} tunnel_rewrite_actions_en;

// from p4pd.h
typedef enum rewrite_actions_enum {
    REWRITE_NOP_ID = 0,
    REWRITE_REWRITE_ID = 1,
    REWRITE_IPV4_NAT_SRC_REWRITE_ID = 2,
    REWRITE_IPV4_NAT_DST_REWRITE_ID = 3,
    REWRITE_IPV4_NAT_SRC_UDP_REWRITE_ID = 4,
    REWRITE_IPV4_NAT_DST_UDP_REWRITE_ID = 5,
    REWRITE_IPV4_NAT_SRC_TCP_REWRITE_ID = 6,
    REWRITE_IPV4_NAT_DST_TCP_REWRITE_ID = 7,
    REWRITE_IPV4_TWICE_NAT_REWRITE_ID = 8,
    REWRITE_IPV4_TWICE_NAT_UDP_REWRITE_ID = 9,
    REWRITE_IPV4_TWICE_NAT_TCP_REWRITE_ID = 10,
    REWRITE_IPV6_NAT_SRC_REWRITE_ID = 11,
    REWRITE_IPV6_NAT_DST_REWRITE_ID = 12,
    REWRITE_IPV6_NAT_SRC_UDP_REWRITE_ID = 13,
    REWRITE_IPV6_NAT_DST_UDP_REWRITE_ID = 14,
    REWRITE_IPV6_NAT_SRC_TCP_REWRITE_ID = 15,
    REWRITE_IPV6_NAT_DST_TCP_REWRITE_ID = 16,
    REWRITE_IPV6_TWICE_NAT_REWRITE_ID = 17,
    REWRITE_IPV6_TWICE_NAT_UDP_REWRITE_ID = 18,
    REWRITE_IPV6_TWICE_NAT_TCP_REWRITE_ID = 19,
    REWRITE_MAX_ID = 20
} rewrite_actions_en;

// flow type depends on the type of packets flow is for
enum flow_type_t {
    FLOW_TYPE_L2   = 0,    // non-IP flow
    FLOW_TYPE_V4   = 1,    // IPv4 flow
    FLOW_TYPE_V6   = 2,    // IPv6 flow
};

// flow direction tells whether flow is from or to the workload
enum flow_direction_t {
    FLOW_DIR_FROM_DMA    = 0,
    FLOW_DIR_FROM_UPLINK = 1,
};

enum {
    FLOW_KEY_LOOKUP_TYPE_NONE           = 0,
    FLOW_KEY_LOOKUP_TYPE_MAC            = 1,
    FLOW_KEY_LOOKUP_TYPE_IPV4           = 2,
    FLOW_KEY_LOOKUP_TYPE_IPV6           = 3,
    FLOW_KEY_LOOKUP_TYPE_FROM_VM_BOUNCE = 4,
    FLOW_KEY_LOOKUP_TYPE_TO_VM_BOUNCE   = 5
};

#define CPU_TO_P4PLUS_FLAGS_UPD_VLAN        0x0001
#define CPU_TO_P4PLUS_FLAGS_ADD_TX_QS_TRLR  0x0002

#define FLOW_ROLES(ENTRY)                                           \
    ENTRY(FLOW_ROLE_INITIATOR,      0, "iflow")                     \
    ENTRY(FLOW_ROLE_RESPONDER,      1, "rflow")                     \
    ENTRY(FLOW_ROLE_NONE,           2, "none")

DEFINE_ENUM(flow_role_t, FLOW_ROLES)
#undef FLOW_ROLES

// NAT types
enum nat_type_t {
    NAT_TYPE_NONE         = 0,
    NAT_TYPE_SNAT         = 1,    // source NAT
    NAT_TYPE_DNAT         = 2,    // destination NAT
    NAT_TYPE_TWICE_NAT    = 3,    // twice NAT
};

// exceptions seen on flows
// TODO: define more !!
#define FLOW_EXPN_TCP_SEGMENTS_OUT_OF_ORDER          0x1
#define FLOW_EXPN_TCP_OVERLAPPING_SEGMENTS           0x2

// alternate flow key structure
typedef struct flow_key_s {
    uint8_t              flow_type:4;    // one of the flow types defined above
    uint8_t              dir:1;
    vrf_id_t             svrf_id;    // src vrf id
    vrf_id_t             dvrf_id;    // dst vrf id

    union {
        // L2 flow key information
        struct {
            l2seg_id_t    l2seg_id;          // L2 segment
            mac_addr_t    smac;              // source MAC address of flow
            mac_addr_t    dmac;              // destination MAC address of flow
            uint16_t      ether_type;        // ether type
        } __PACK__;

        // IPv4/IPv6 flow key information
        struct {
            ipvx_addr_t            sip;          // source IP address
            ipvx_addr_t            dip;          // destination IP address
            IPProtocol             proto;        // IP protocol
            union {
                struct {
                    uint8_t        icmp_type;    // ICMP type
                    uint8_t        icmp_code;    // ICMP code
                    uint16_t       icmp_id;      // ICMP identifier
                } __PACK__;
                struct {
                    uint16_t       sport;        // TCP/UDP source port
                    uint16_t       dport;        // TCP/UDP destination port
                } __PACK__;
                struct {
                    uint32_t        spi;         // ESP header's SPI
                } __PACK__;
            } __PACK__;
        } __PACK__;

    } __PACK__;
} __PACK__ flow_key_t;

// Flow's L2 key info
typedef struct flow_l2key_s {
    l2seg_id_t    l2seg_id;          // L2 segment
    mac_addr_t    smac;              // source MAC address of flow
    mac_addr_t    dmac;              // destination MAC address of flow
} __PACK__ flow_l2key_t;

// spdlog formatter for flow_key_t
const char *flowkey2str(const flow_key_t& key);
inline std::ostream& operator<<(std::ostream& os, const flow_key_t& key)
{
    return os << flowkey2str(key);
}

// flow config
typedef struct flow_cfg_s {
    flow_key_t                key;                 // flow's key
    uint16_t                  action:3;            // flow action(s)
    uint16_t                  role:1;              // flow role (initiator or responder)
    uint16_t                  nat_type:3;          // type of NAT
    uint16_t                  is_ing_proxy_mirror:1; // Is ing mirror a proxy
    uint16_t                  is_eg_proxy_mirror:1; // Is eg mirror a proxy
    ip_addr_t                 nat_sip;             // source NAT IP, if any
    ip_addr_t                 nat_dip;             // destination NAT IP, if any
    uint16_t                  nat_sport;           // NAT source port
    uint16_t                  nat_dport;           // NAT destination port
    uint8_t                   eg_mirror_session;   // Mirror sessions in egress direction
    uint8_t                   ing_mirror_session;  // Mirror sessions in ingress direction
    uint32_t                  idle_timeout;        // Idle timeout for session
    flow_l2key_t              l2_info;             // Flow's l2 info
} __PACK__ flow_cfg_t;

typedef struct flow_pgm_attrs_s {
    uint64_t                  role:1;              // flow role (initiator or responder)
    uint64_t                  drop:1;              // drop for this flow
    uint64_t                  mac_sa_rewrite:1;    // rewrite src mac
    uint64_t                  mac_da_rewrite:1;    // rewrite dst mac
    uint64_t                  ttl_dec:1;           // decrement ttl
    uint64_t                  mcast_en:1;          // mcast enable
    uint64_t                  tunnel_orig:1;       // tunnel originate
    uint64_t                  lport:11;            // dest lport
    uint64_t                  qid_en:1;            // qid enabled
    uint64_t                  qtype:3;             // Qtype
    uint64_t                  qid:24;              // Qid
    uint64_t                  lkp_inst:1;          // Flow instance
    uint64_t                  expected_src_lif_en:1;  // src lif check for host pinning
    uint64_t                  qos_class_en: 1;        // Qos class enable
    uint64_t                  is_proxy_mcast: 1;      // Is mcast ptr proxy ?
    uint64_t                  is_proxy_en: 1;         // Is this flow prxy enabled?
    uint64_t                  use_vrf:1;              // Flow lookup object is vrf
    uint64_t                  export_en:4;            // Export enable: 4 exporters

    uint32_t                  vrf_hwid;            // source l2seg vrf hwid (lkp_vrf)
    rewrite_actions_en        rw_act;              // rewrite action
    uint32_t                  rw_idx;              // rewrite index
    tunnel_rewrite_actions_en tnnl_rw_act;         // tunnel rewrite action
    uint32_t                  tnnl_rw_idx;         // tunnel rewrite index
    uint32_t                  tnnl_vnid;           // tunnel vnid / encap vlan
    ip_addr_t                 nat_ip;              // NAT IP, (dip for dnat/twice_nat, sip for snat)
    uint16_t                  nat_l4_port;         // NAT L4 port
    uint32_t                  twice_nat_idx;       // Twice nat index
    uint16_t                  mcast_ptr;           // Mcast Pointer
    uint32_t                  expected_src_lif;    // src lif for host pinning check
    uint32_t                  qos_class_id;        // Qos class id
    uint8_t                   export_id1;          // Export Id1
    uint8_t                   export_id2;          // Export Id2
    uint8_t                   export_id3;          // Export Id3
    uint8_t                   export_id4;          // Export Id4
} __PACK__ flow_pgm_attrs_t;

typedef struct flow_stats_s {
    uint8_t              num_tcp_tickles_sent;   // Number of TCP tickles sent for this
    uint8_t              num_tcp_rst_sent;       // Number of TCP reset sent as a result of aging
} __PACK__ flow_stats_t;

// flow state
struct flow_s {
    sdk_spinlock_t        slock;               // lock to protect this structure
    bool                  is_aug_flow;         // is an augment flow

    flow_cfg_t            config;              // flow config
    flow_pgm_attrs_t      pgm_attrs;           // table program attributes
    flow_t               *reverse_flow;       // reverse flow data
    flow_t               *assoc_flow;         // valid only if flow has an associated flow
    session_t            *session;            // session this flow belongs to, if any
    FlowTCPState          state;              // run-time state of the flow
    flow_stats_t          stats;              // Flow level stats

    // PD state
    pd::pd_flow_t     *pd;                 // all PD specific state

    // meta data maintained for flow
    ht_ctxt_t         flow_key_ht_ctxt;    // flow key based hash table context
} __PACK__;

typedef struct flow_state_s {
    uint64_t        create_ts;           // flow create timestamp
    uint64_t        last_pkt_ts;         // last packet timestamp
    uint64_t        packets;             // packet count on this flow
    uint64_t        bytes;               // byte count on this flow
    uint64_t        drop_packets;        // packets dropped for this flow
    uint64_t        drop_bytes;          // bytes dropped for this flow
    uint32_t        exception_bmap;      // exceptions seen on this flow
    FlowTCPState    state;               // flow state

    uint32_t        tcp_seq_num;
    uint32_t        tcp_ack_num;
    uint32_t        tcp_win_sz;
    int32_t         syn_ack_delta;       // ACK delta of iflow
    uint16_t        tcp_mss;
    uint8_t         tcp_win_scale;
    uint8_t         tcp_ws_option_sent : 1;
    uint8_t         tcp_ts_option_sent : 1;
    uint8_t         tcp_sack_perm_option_sent : 1;
} __PACK__ flow_state_t;
inline std::ostream& operator<<(std::ostream& os, const flow_state_t& val)
{
    os << "{state=" << val.state;
    if (val.syn_ack_delta) {
        os << ", syn_ack_delta=" << val.syn_ack_delta;
    }
    if (val.tcp_seq_num) {
        os << ", tcp_seq_num=" << val.tcp_seq_num;
    }
    if (val.tcp_ack_num) {
        os << ", tcp_ack_num=" << val.tcp_ack_num;
    }
    return os << "}";
}

enum {
    SESSION_MATCH_SVRF   = 1 << 0,
    SESSION_MATCH_DVRF   = 1 << 1,
    SESSION_MATCH_V4_SIP = 1 << 2,
    SESSION_MATCH_V4_DIP = 1 << 3,
    SESSION_MATCH_V6_SIP = 1 << 4,
    SESSION_MATCH_V6_DIP = 1 << 5,
    SESSION_MATCH_SMAC   = 1 << 6,
    SESSION_MATCH_DMAC   = 1 << 7,
    SESSION_MATCH_V4_FLOW = 1 << 8
        /* Add more match cases here, if needed */
};
typedef struct session_match_s {
    uint16_t         match_fields; 
    flow_key_t      key;
    dllist_ctxt_t   *session_list;   
} session_match_t;

typedef struct session_state_s {
    uint8_t             tcp_ts_option:1;
    uint8_t             tcp_sack_perm_option:1;
    flow_state_t        iflow_state;
    flow_state_t        rflow_state;
    flow_state_t        iflow_aug_state;
    flow_state_t        rflow_aug_state;
} __PACK__ session_state_t;

typedef struct session_cfg_s {
    uint8_t             tcp_ts_option:1;
    uint8_t             tcp_sack_perm_option:1;
    uint8_t             conn_track_en:1;          // enable connection tracking

    session_id_t        session_id;               // unique session id
    uint32_t            idle_timeout;             // Session idle timeout
} __PACK__ session_cfg_t;

static const uint8_t MAX_SESSION_FLOWS = 2;
typedef struct session_args_s {
    session_cfg_t      *session;                          // session config
    flow_cfg_t         *iflow[MAX_SESSION_FLOWS];         // initiator flow
    flow_cfg_t         *rflow[MAX_SESSION_FLOWS];         // responder flow
    flow_pgm_attrs_t   *iflow_attrs[MAX_SESSION_FLOWS];   // initiator flow attrs
    flow_pgm_attrs_t   *rflow_attrs[MAX_SESSION_FLOWS];   // responder flow attrs
    session_state_t    *session_state;                    // connection tracking state
    bool               valid_rflow;                       // Rflow valid ?
    vrf_t              *vrf;                              // src vrf
    ep_t               *sep;                              // spurce ep
    ep_t               *dep;                              // dest ep
    if_t               *sif;                              // source interface
    if_t               *dif;                              // dest interface
    l2seg_t            *sl2seg;                           // source l2seg
    l2seg_t            *dl2seg;                           // dest l2seg
    SessionSpec        *spec;                             // session spec
    SessionResponse    *rsp;                              // session response
    uint32_t           flow_hash;                         // flow hash
} __PACK__ session_args_t;

//------------------------------------------------------------------------------
// A session consists of a initiator flow and responder flow
//   - initiator flow is the flow on which the first packet arrived
//   - responder flow is the reverse flow for the initiator flow
// In some cases, it is possible to have uni-directional flows and in such cases
// session consists of only single flow
//------------------------------------------------------------------------------
struct session_s {
    sdk_spinlock_t      slock;                    // lock to protect this structure
    uint8_t             fte_id:4;                 // FTE that created this session
    uint8_t             conn_track_en:1;          // enable connection tracking
    flow_t              *iflow;                   // initiator flow
    flow_t              *rflow;                   // responder flow, if any
    hal_handle_t        vrf_handle;               // src vrf handle
    void                *tcp_cxntrack_timer;      // Timer to check connection establishment
                                                  // and connection close
    uint32_t            idle_timeout;             // idle timeout configured for a session

    // PD state
    pd::pd_session_t    *pd;                      // all PD specific state

    // meta data maintained for session
    hal_handle_t        hal_handle;               // hal handle for this session
    ht_ctxt_t           hal_handle_ht_ctxt;       // hal handle based hash table ctxt
    ht_ctxt_t           hal_iflow_ht_ctxt;        // hal iflow based hash table ctxt
    ht_ctxt_t           hal_rflow_ht_ctxt;        // hal rflow based hash table ctxt
    hal_handle_t        sep_handle;               // Source EP Handle
    hal_handle_t        dep_handle;               // Destination EP Handle
    /* TODO: SEP and DEP handles need to be removed */
    dllist_ctxt_t       sif_session_lentry;       // source interface's session list context
    dllist_ctxt_t       dif_session_lentry;       // destination interface's session list context
    dllist_ctxt_t       feature_list_head;        // List of feature specific states
} __PACK__;

typedef struct session_stats_ {
    uint64_t    total_active_sessions;
    uint64_t    l2_sessions;
    uint64_t    tcp_sessions;
    uint64_t    udp_sessions;
    uint64_t    icmp_sessions;
    uint64_t    drop_sessions;
    uint64_t    aged_sessions;
    uint64_t    num_tcp_rst_sent;          // no. of TCP resets found as a result of SFW Reject
    uint64_t    num_icmp_error_sent;       // no. of ICMP errors sent as a result of SFW Reject
    uint64_t    num_cxnsetup_timeout;      // no. of sessions that timed out at connection setup
    uint64_t    num_session_create_err;    // no. of session create errors
} __PACK__ session_stats_t;

// max. number of session supported  (TODO: we can take this from cfg file)
#define HAL_MAX_SESSIONS                             524288
#define HAL_MAX_FLOWS                                (HAL_MAX_SESSIONS << 1)

#define SESSION_AGE_DEBUG                  0

session_t *find_session_by_handle(hal_handle_t handle);
//session_t *find_session_by_id(session_id_t session_id);

extern void *session_get_key_func (void *entry);
extern uint32_t session_compute_hash_func (void *key, uint32_t ht_size);
extern bool session_compare_key_func (void *key1, void *key2);
extern void *session_get_handle_key_func (void *entry);
extern uint32_t session_compute_handle_hash_func (void *key, uint32_t ht_size);
extern bool session_compare_handle_key_func (void *key1, void *key2);
extern void *session_get_iflow_key_func (void *entry);
extern uint32_t session_compute_iflow_hash_func (void *key, uint32_t ht_size);
extern bool session_compare_iflow_key_func (void *key1, void *key2);
extern void *session_get_rflow_key_func (void *entry);
extern uint32_t session_compute_rflow_hash_func (void *key, uint32_t ht_size);
extern bool session_compare_rflow_key_func (void *key1, void *key2);
extern void *flow_get_key_func (void *entry);
extern uint32_t flow_compute_hash_func (void *key, uint32_t ht_size);
extern bool flow_compare_key_func (void *key1, void *key2);
extern hal_ret_t ep_get_from_flow_key (const flow_key_t* key,
                                      ep_t **sep, ep_t **dep);
bool flow_needs_associate_flow (const flow_key_t *flow_key);
extern hal_ret_t session_release (session_t *session);
hal_ret_t extract_flow_key_from_spec (vrf_id_t tid,
                                     flow_key_t *flow_key,
                                     const FlowKey& flow_spec_key);
hal_ret_t session_init (hal_cfg_t *hal_cfg);
hal_ret_t session_create (const session_args_t *args, hal_handle_t *session_handle,
                          session_t **session_p);
hal_ret_t session_update (const session_args_t *args, session_t *session);
hal_ret_t session_delete (const session_args_t *args, session_t *session);
hal::session_t *session_lookup (flow_key_t key, flow_role_t *role);
hal_ret_t session_get (session::SessionGetRequest& spec,
                      session::SessionGetResponseMsg *rsp);
hal_ret_t schedule_tcp_close_timer (session_t *session);
hal_ret_t schedule_tcp_half_closed_timer (session_t *session);
hal_ret_t schedule_tcp_cxnsetup_timer (session_t *session);
void session_set_tcp_state (session_t *session, hal::flow_role_t role,
                           FlowTCPState tcp_state);
hal_ret_t session_get_all (session::SessionGetResponseMsg *rsp);
hal_ret_t session_delete_list (dllist_ctxt_t *session_list, bool async=false);
hal_ret_t session_delete_all (session::SessionDeleteResponseMsg *rsp);
hal_ret_t session_eval_matching_session (session_match_t *match);
hal_ret_t system_fte_stats_get (SystemResponse *rsp);
hal_ret_t system_fte_txrx_stats_get (SystemResponse *rsp);
hal_ret_t system_session_summary_get (SystemResponse *rsp);
hal_ret_t session_delete (session::SessionDeleteRequest& spec,
                      session::SessionDeleteResponseMsg *rsp);
void incr_global_session_tcp_rst_stats (uint8_t fte_id);
void incr_global_session_icmp_error_stats (uint8_t fte_id);
hal_ret_t session_handle_upgrade (void);

}    // namespace hal

#endif    // __SESSION_HPP__

