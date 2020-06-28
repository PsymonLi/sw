// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __ETH_HPP__
#define __ETH_HPP__

#include <stdio.h>
#include <cinttypes>
#include <ostream>
#include "include/sdk/base.hpp"

#define ETH_MIN_FRAME_LEN                              64

#define L2_MIN_HDR_LEN                                 14
#define L2_ETH_HDR_LEN                                 14
#define L2_DOT1Q_HDR_LEN                               18

#define ARPOP_REQUEST                                  1
#define ARPOP_REPLY                                    2
#define ARPOP_REVREQUEST                               3
#define ARPOP_REVREPLY                                 4

#define ARP_HARDWARE_TYPE                              1    // Ethernet

#define ETH_TYPE_IPV4                                  0x0800
#define ETH_TYPE_ARP                                   0x0806
#define ETH_TYPE_RARP                                  0x8035
#define ETH_TYPE_DOT1Q                                 0x8100
#define ETH_TYPE_IPV6                                  0x86DD
#define ETH_TYPE_NCSI                                  0x88F8
#define ETH_TYPE_LLDP                                  0x88CC

// MAC address
#define ETH_ADDR_LEN                                 6
typedef uint8_t    mac_addr_t[ETH_ADDR_LEN];

// Ethernet header
typedef struct eth_hdr_s {
    mac_addr_t dmac;
    mac_addr_t smac;
    uint16_t eth_type;
} __PACK__ eth_hdr_t;

#define MAC_TO_UINT64(mac_addr)                                         \
    (((mac_addr)[5] & 0xFF)                      |                      \
     (((mac_addr)[4] & 0xFF) << 8)               |                      \
     (((mac_addr)[3] & 0xFF) << 16)              |                      \
     ((uint64_t)((mac_addr)[2] & 0xFF) << 24)    |                      \
     ((uint64_t)((mac_addr)[1] & 0xFF) << 32ul)  |                      \
     ((uint64_t)((mac_addr)[0] & 0xFF) << 40ul))

// Mac: 0xaabbccddeeff
//    mac_uint64: 0xaabbccddeeff
//    mac_addr_t: [0]:aa, [1]:bb, [2]:cc, [3]:dd, [4]:ee, [5]:ff
#define MAC_UINT64_TO_ADDR(mac_addr, mac_uint64)                          \
{                                                                         \
     (mac_addr)[5] = mac_uint64 & 0xFF;                                   \
     (mac_addr)[4] = (mac_uint64 >> 8) & 0xFF;                            \
     (mac_addr)[3] = (mac_uint64 >> 16) & 0xFF;                           \
     (mac_addr)[2] = (mac_uint64 >> 24) & 0xFF;                           \
     (mac_addr)[1] = (mac_uint64 >> 32) & 0xFF;                           \
     (mac_addr)[0] = (mac_uint64 >> 40) & 0xFF;                           \
}

#define MAC_ADDR_COPY(mac_addr_dst, mac_addr_src)                         \
{                                                                         \
     ((uint16_t *)mac_addr_dst)[0] = ((uint16_t *)mac_addr_src)[0];       \
     ((uint16_t *)mac_addr_dst)[1] = ((uint16_t *)mac_addr_src)[1];       \
     ((uint16_t *)mac_addr_dst)[2] = ((uint16_t *)mac_addr_src)[2];       \
}

#define IS_MCAST_MAC_ADDR(mac_addr)            ((mac_addr)[0] & 0x1)

static inline bool
is_multicast(uint64_t mac) {
    return ((mac & 0x010000000000) == 0x010000000000);
}

static inline void
mac_str_to_addr (const char *str, mac_addr_t mac_addr)
{
    unsigned char* mac = (unsigned char*) mac_addr;
    sscanf(str, "%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%"
           SCNx8, &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

static inline bool
is_mac_set (mac_addr_t mac_addr)
{
    for (uint32_t i = 0; i < ETH_ADDR_LEN; i++) {
        if ((mac_addr)[i] != 0x0) {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// thread safe helper to stringify MAC address
//------------------------------------------------------------------------------
static inline char *
macaddr2str (const mac_addr_t mac_addr, bool dotted_notation=false)
{
    static thread_local char       macaddr_str[4][20];
    static thread_local uint8_t    macaddr_str_next = 0;
    char                           *buf;

    buf = macaddr_str[macaddr_str_next++ & 0x3];
    if (dotted_notation) {
        snprintf(buf, 20, "%02x%02x.%02x%02x.%02x%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        snprintf(buf, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
    }
    return buf;
}

static inline char *
mac2str (uint64_t mac)
{
    mac_addr_t mac_addr;

    MAC_UINT64_TO_ADDR(mac_addr, mac);
    return macaddr2str(mac_addr);
}

static inline char *
ethtype2str (uint32_t eth_type)
{
    static thread_local char       str[4][6];
    static thread_local uint8_t    str_next = 0;
    char                           *buf;

    buf = str[str_next++ & 0x3];
    switch (eth_type) {
    case ETH_TYPE_ARP:
        snprintf(buf, 6, "%-6s", "ARP");
        break;
    case ETH_TYPE_RARP:
        snprintf(buf, 6, "%-6s", "RARP");
        break;
    case ETH_TYPE_DOT1Q:
        snprintf(buf, 6, "%-6s", "DOT1Q");
        break;
    case ETH_TYPE_NCSI:
        snprintf(buf, 6, "%-6s", "NCSI");
        break;
    case ETH_TYPE_IPV4:
        snprintf(buf, 6, "%-6s", "IPV4");
        break;
    case ETH_TYPE_IPV6:
        snprintf(buf, 6, "%-6s", "IPV6");
        break;
    default:
        snprintf(buf, 6, "%-6u", eth_type);
        break;
    }
    return buf; 
}

// spdlog formatter for mac_addr_t
inline std::ostream& operator<<(std::ostream& os, mac_addr_t mac) {
    return os << macaddr2str(mac);
}

// NOTE: this is temporary until we have reserved block
#define PENSANDO_NIC_MAC        0x022222111111ull
#define PENSANDO_NIC_MAC_STR    "02:22:22:11:11:11"

#endif    // __ETH_HPP__

