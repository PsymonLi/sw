//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gen/proto/types.pb.h>
#include <gen/proto/debug.pb.h>
#include <gen/proto/nat.pb.h>
#include <nic/vpp/infra/ipc/uds_internal.h>
#include <nic/vpp/impl/nat.h>
#include "pdsa_uds_hdlr.h"

static void *
nat_uds_cb(void *ctxt, uint32_t vpc_id, nat_flow_key_t *key, uint64_t val)
{
    uint32_t hw_index, tx_xlate_id;
    nat_proto_t nat_proto;
    nat_addr_type_t nat_addr_type;
    uint32_t sip, dip, pvt_ip;
    uint16_t pvt_port;
    int fd = *(int *)ctxt;
    char dip_str[INET_ADDRSTRLEN + 1], sip_str[INET_ADDRSTRLEN + 1];
    char pvt_ip_str[INET_ADDRSTRLEN + 1];

    nat_flow_ht_get_fields(val, &hw_index, &nat_addr_type, &nat_proto);

    pds_snat_tbl_read_ip4(hw_index, &pvt_ip, &pvt_port);

    sip = htonl(key->sip);
    dip = htonl(key->dip);
    pvt_ip = htonl(pvt_ip);

    inet_ntop(AF_INET, &sip, sip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &dip, dip_str, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &pvt_ip, pvt_ip_str, INET_ADDRSTRLEN);

    tx_xlate_id = nat_get_tx_hw_index_pub_ip_port(vpc_id, nat_addr_type,
                                                  nat_proto, key->sip,
                                                  key->sport);

    dprintf(fd, "%-5d%-15s%-15s%-10d%-10d%-7d%-7s%-15s%-10d%-10d%-10d\n",
            vpc_id, sip_str, dip_str, key->sport, key->dport,
            get_proto_from_nat_proto(nat_proto),
            nat_addr_type == NAT_ADDR_TYPE_INTERNET ? "pub" : "infra",
            pvt_ip_str, pvt_port, hw_index, tx_xlate_id);

    return ctxt;
}

// Callback to dump NAT entries via UDS
static void
vpp_uds_nat_flow_dump(int sock_fd, int io_fd, bool summary)
{
    dprintf(io_fd, "-----------------------------------------------------------");
    dprintf(io_fd, "-----------------------------------------------------------\n");
    dprintf(io_fd, "%-5s%-15s%-15s%-10s%-10s%-7s%-7s%-15s%-10s%-10s%-10s\n",
        "VPC", "SIP", "DIP", "Sport", "Dport", "Proto", "Type", "Pvt IP", "Pvt Port",
        "Rx Index", "Tx Index");
    dprintf(io_fd, "-----------------------------------------------------------");
    dprintf(io_fd, "-----------------------------------------------------------\n");

    nat_flow_iterate((void *)&io_fd, &nat_uds_cb);

    close(io_fd);
}

// Callback to NAT global stats via UDS
static void
vpp_uds_global_stats(int sock_fd, int io_fd, bool summary)
{
    nat_global_stats_t stats;

    nat_get_global_stats(&stats);

    dprintf(io_fd, "NAT global statistics\n");
    dprintf(io_fd, "=====================\n");
    dprintf(io_fd, "%-40s: %u\n", "Total HW Index: ", stats.hw_index_total);
    dprintf(io_fd, "%-40s: %u\n", "Allocated HW Index: ", stats.hw_index_alloc);
    dprintf(io_fd, "\n");
    close(io_fd);
}

// initializes callbacks for flow dump
void
pds_nat_uds_init(void)
{
    vpp_uds_register_cb(VPP_UDS_NAT_FLOW_DUMP, vpp_uds_nat_flow_dump);
    vpp_uds_register_cb(VPP_UDS_NAT_GLOBAL_STATS, vpp_uds_global_stats);
}
