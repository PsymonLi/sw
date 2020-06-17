//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_FLOW_PDSA_HDLR_H__
#define __VPP_FLOW_PDSA_HDLR_H__

#ifdef __cplusplus
extern "C" {
#endif

#define foreach_flow_type_counter                                   \
        _(TCPV4, "TCP sessions over IPv4")                          \
        _(UDPV4, "UDP sessions over IPv4")                          \
        _(ICMPV4, "ICMP sessions over IPv4")                        \
        _(OTHERV4, "Other sessions over IPv4")                      \
        _(TCPV6, "TCP sessions over IPv6")                          \
        _(UDPV6, "UDP sessions over IPv6")                          \
        _(ICMPV6, "ICMP sessions over IPv6")                        \
        _(OTHERV6, "Other sessions over IPv6")                      \
        _(L2, "L2 sessions")                                        \
        _(ERROR, "Session create errors")

#define FLOW_STATS_SCHEMA_NAME  "FlowStatsSummary"
#define FLOW_STATS_KEY          1

typedef enum {
    PDS_FLOW_PROTO_START,
    PDS_FLOW_PROTO_TCP = PDS_FLOW_PROTO_START,
    PDS_FLOW_PROTO_UDP,
    PDS_FLOW_PROTO_ICMP,
    PDS_FLOW_PROTO_OTHER,
    PDS_FLOW_PROTO_END,
} pds_flow_protocol;

typedef enum {
    PDS_FLOW_STATE_CONN_INIT,
    PDS_FLOW_STATE_CONN_SETUP,
    PDS_FLOW_STATE_ESTABLISHED,
    PDS_FLOW_STATE_KEEPALIVE_SENT,
    PDS_FLOW_STATE_HALF_CLOSE_IFLOW,
    PDS_FLOW_STATE_HALF_CLOSE_RFLOW,
    PDS_FLOW_STATE_CLOSE,
} pds_flow_state;

// packet types - Any new addition should be handled in
// pds_packet_type_flags_build ()
typedef enum {
    PDS_FLOW_L2L_INTRA_SUBNET = 0,
    PDS_FLOW_L2L_INTER_SUBNET,
    PDS_FLOW_L2R_INTRA_SUBNET,
    PDS_FLOW_L2R_INTER_SUBNET,
    PDS_FLOW_L2N_ASYMMETRIC_ROUTE,
    PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAPT,
    PDS_FLOW_L2N_ASYMMETRIC_ROUTE_NAT,
    PDS_FLOW_L2N_SYMMETRIC_ROUTE,
    PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAPT,
    PDS_FLOW_L2N_SYMMETRIC_ROUTE_NAT,
    PDS_FLOW_L2N_SYMMETRIC_ROUTE_TWICE_NAT,
    PDS_FLOW_L2N_INTRA_VCN_ROUTE,
    PDS_FLOW_R2L_INTRA_SUBNET,
    PDS_FLOW_R2L_INTER_SUBNET,
    PDS_FLOW_N2L_ASYMMETRIC_ROUTE,
    PDS_FLOW_N2L_ASYMMETRIC_ROUTE_NAT,
    PDS_FLOW_N2L_SYMMETRIC_ROUTE,
    PDS_FLOW_N2L_SYMMETRIC_ROUTE_NAT,
    PDS_FLOW_N2L_ASYMMETRIC_ROUTE_SVC_NAT,
    PDS_FLOW_N2L_SYMMETRIC_ROUTE_SVC_NAT,
    PDS_FLOW_N2L_INTRA_VCN_ROUTE,
    PDS_FLOW_PKT_TYPE_MAX,
} pds_flow_pkt_type;

// function prototypes

// pdsa_hdlr.cc
void pdsa_flow_hdlr_init(void);
void *pdsa_flow_stats_init(void);
void pdsa_flow_stats_publish(void *, uint64_t *);

// pdsa_vpp_hdlr.c
void pds_flow_cfg_set(uint8_t con_track_en,
                      uint32_t tcp_syn_timeout,
                      uint32_t tcp_half_close_timeout,
                      uint32_t tcp_close_timeout,
                      const uint32_t *flow_idle_timeout,
                      const uint32_t *flow_drop_timeout);

// cli_helper.c
int clear_all_flow_entries();
uint16_t get_vlib_thread_index();
void flow_stats_summary_get (void *ctxt_v4, void *ctxt_v6);
int flow_vnic_active_ses_count_get(uint16_t vnic_id, uint32_t *active_sessions);
uint32_t pds_encode_flow_state(pds_flow_state flow_state);
bool pds_decode_flow_state(uint32_t encoded_flow_state,
                           pds_flow_state *flow_state);
uint32_t pds_encode_flow_pkt_type(pds_flow_pkt_type encoded_flow_type);
bool pds_decode_flow_pkt_type(uint32_t encoded_flow_type,
                              pds_flow_pkt_type *flow_type);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_FLOW_PDSA_HDLR_H__
