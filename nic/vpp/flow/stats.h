//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_FLOW_STATS_H__
#define __VPP_FLOW_STATS_H__

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

#define foreach_datapath_assist_stats_counter                              \
        _(TOTALPKTSRX, "Total Pkts Rx")                             \
        _(TOTALDROPS, "Total Drops")                                \
        _(DHCPPKTSRX, "DHCP Pkts Rx")                               \
        _(DHCPPKTSTXTOPROXYSERVER, "DHCP Pkts Tx to Proxy Server")  \
        _(DHCPPKTSTXTORELAYSERVER, "DHCP Pkts Tx to Relay Server")  \
        _(DHCPPKTSTXTORELAYCLIENT, "DHCP Pkts Tx to Relay Client")  \
        _(DHCPDROPS, "DHCP Drops")                                  \
        _(ARPPKTSRX, "ARP Pkts Rx")                                 \
        _(ARPREPLIESSENT, "ARP Replies Sent")                       \
        _(ARPDROPS, "ARP Drops")                                    \
        _(TOTALSESSIONSLEARNED, "Total Sessions Learned")           \
        _(TOTALSESSIONSAGED, "Total Sessions Aged")

#define DATAPATH_ASSIST_STATS_SCHEMA_NAME  "DataPathAssistStats"

#define MAX_ERROR_IDX_ACCUMULATED    8

#define foreach_accumulate_node                      \
    _(DPDK_INPUT, "dpdk-input")                      \
    _(DROP, "drop")                                  \
    _(ARP_PROXY, "pds-arp-proxy")                    \
    _(DHCP_HOST, "pds-dhcp-relay-host-classify")     \
    _(DHCP_UPLINK, "pds-dhcp-relay-uplink-classify") \
    _(SESSION_PROG, "pds-session-program")           \
    _(AGE_PROCESS, "pds-flow-age-process")

typedef enum
{
#define _(n,s) FLOW_TYPE_COUNTER_##n,
    foreach_flow_type_counter
#undef _
    FLOW_TYPE_COUNTER_MAX,
} flow_type_counter_t;

typedef enum {
#define _(s,n) PDS_DATAPATH_ASSIST_STAT_##s,
    foreach_datapath_assist_stats_counter
#undef _
    PDS_DATAPATH_ASSIST_STAT_MAX
} pds_flow_datapath_assist_stats_e;

typedef enum {
#define _(n,s)   PDS_FLOW_ACCUMULATE_NODE_##n,
    foreach_accumulate_node
#undef _
    PDS_FLOW_ACCUMULATE_NODE_LAST
} pds_flow_accumulate_node_e;

typedef struct {
    int stats_idx;
    int node_idx;
    bool use_vectors;
    int  error_idx[MAX_ERROR_IDX_ACCUMULATED];
} datapath_assist_stats_accumulate_t;

// stats.cc
void *pdsa_flow_stats_init(void);
void pdsa_flow_stats_publish(void *, uint64_t *);
void *pdsa_datapath_assist_stats_init(void);
void pdsa_datapath_assist_stats_publish(void *, uint64_t *);

// stats_collect.c
void pds_flow_monitor_accumulate_stats (void *vm);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_FLOW_STATS_H__
