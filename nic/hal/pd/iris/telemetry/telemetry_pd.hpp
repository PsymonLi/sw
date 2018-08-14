// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __HAL_TELEMETRY_PD_HPP__
#define __HAL_TELEMETRY_PD_HPP__

#include "nic/include/base.hpp"
#include "nic/include/pd.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/plugins/cfg/telemetry/telemetry.hpp"
#include "nic/hal/pd/cpupkt_headers.hpp"

#define TELEMETRY_EXPORT_BUFF_SIZE      2047
#define TELEMETRY_IPFIX_BUFSIZE         2048
#define TELEMETRY_IPFIX_HBM_MEMSIZE     (64 * 1024)
#define TELEMETRY_NUM_EXPORT_DEST       (TELEMETRY_IPFIX_HBM_MEMSIZE/TELEMETRY_IPFIX_BUFSIZE)

namespace hal {
namespace pd {

typedef struct telemetry_pd_export_buf_header_s telemetry_pd_export_buf_header_t;
struct telemetry_pd_export_buf_header_s {
    uint16_t packet_start;
    uint16_t payload_length;
    uint16_t payload_start;
    uint16_t ip_hdr_start;
} __attribute__ ((__packed__));

typedef struct telemetry_pd_ipfix_header_s telemetry_pd_ipfix_header_t;
struct telemetry_pd_ipfix_header_s {
    vlan_header_t vlan;
    ipv4_header_t iphdr;
    udp_header_t udphdr;
} __attribute__ ((__packed__));

class telemetry_export_dest {
public:
    hal_ret_t set_src_mac(mac_addr_t in);
    hal_ret_t set_dst_mac(mac_addr_t in);
    hal_ret_t set_vlan(uint16_t in);
    hal_ret_t set_dscp(uint8_t in);
    hal_ret_t set_ttl(uint8_t in);
    hal_ret_t set_src_ip(ip_addr_t in);
    hal_ret_t set_dst_ip(ip_addr_t in);
    hal_ret_t set_sport(uint16_t in);
    hal_ret_t set_dport(uint16_t in);
    uint16_t get_exporter_id();
    hal_ret_t commit();
    hal_ret_t init(uint16_t id);

private:
    telemetry_pd_export_buf_header_t buf_hdr_;
    telemetry_pd_ipfix_header_t ipfix_hdr_;
    uint16_t id_;
    uint64_t base_addr_;
    void * buffer_;
    // base addr
    // IPFix Header defined and maintained
};

}    // namespace pd
}    // namespace hal

#endif    //__HAL_TELEMETRY_PD_HPP__
