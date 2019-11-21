//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_UTILS_UTILS_HPP__
#define __TEST_UTILS_UTILS_HPP__

#include <vector>
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/platform/capri/capri_p4.hpp" //UPLINK_0/1
#include "nic/apollo/api/include/pds.hpp"

namespace api_test {

// todo: @kalyanbade delete this once all references are gone
typedef enum op_e {
    OP_NONE,            ///< None
    OP_MANY_CREATE,     ///< Create
    OP_MANY_READ,       ///< Read
    OP_MANY_UPDATE,     ///< Update
    OP_MANY_DELETE,     ///< Delete
    OP_INVALID,         ///< Invalid
} utils_op_t;

static inline int
ip_version (const char *ip)
{
    char buf[16];

    if (inet_pton(AF_INET, ip, buf)) {
        return IP_AF_IPV4;
    } else if (inet_pton(AF_INET6, ip, buf)) {
        return IP_AF_IPV6;
    }
    return -1;
}

static inline void
extract_ipv4_addr (const char *ip, ipv4_addr_t *ip_addr)
{
    int af;
    ip_prefix_t pfx;

    af = ip_version(ip);
    if (af == IP_AF_IPV4) {
        SDK_ASSERT(str2ipv4pfx((char *)ip, &pfx) == 0);
        *ip_addr = pfx.addr.addr.v4_addr;
    } else {
        SDK_ASSERT(0);
    }
}

static inline void
extract_ip_addr (const char *ip, ip_addr_t *ip_addr)
{
    int af;
    ip_prefix_t pfx;
    memset(&pfx, 0x0, sizeof(ip_prefix_t));
    af = ip_version(ip);
    if (af == IP_AF_IPV4) {
        SDK_ASSERT(str2ipv4pfx((char *)ip, &pfx) == 0);
        *ip_addr = pfx.addr;
    } else if (af == IP_AF_IPV6) {
        SDK_ASSERT(str2ipv6pfx((char *)ip, &pfx) == 0);
        *ip_addr = pfx.addr;
    } else {
        SDK_ASSERT(0);
    }
}

static inline void
extract_ip_pfx (const char *str, ip_prefix_t *ip_pfx)
{
    char ip[64];
    char *slash;
    int af;

    // Input may get modified. Copying hence
    strncpy(ip, str, sizeof(ip) - 1);
    slash = strchr(ip, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
    af = ip_version(ip);
    if (slash != NULL) {
        *slash = '/';
    }
    if (af == IP_AF_IPV4) {
        SDK_ASSERT(str2ipv4pfx((char *)ip, ip_pfx) == 0);
    } else if (af == IP_AF_IPV6) {
        SDK_ASSERT(str2ipv6pfx((char *)ip, ip_pfx) == 0);
    } else {
        SDK_ASSERT(0);
    }
}

static inline void
increment_ip_addr (ip_addr_t *ipaddr, int width = 1)
{
    switch (ipaddr->af) {
        case IP_AF_IPV4:
            ipaddr->addr.v4_addr += width;
            break;
        case IP_AF_IPV6:
            if (likely(!((ipaddr->addr.v6_addr.addr64[0] == ((uint64_t)-1)) &&
                         (ipaddr->addr.v6_addr.addr64[1] == ((uint64_t)-1))))) {
                for (uint8_t i = IP6_ADDR8_LEN - 1; i >= 0 ; i--) {
                    // keep adding one until there is no rollover
                    if ((++(ipaddr->addr.v6_addr.addr8[i]))) {
                        break;
                    }
                }
            }
            break;
        default:
            SDK_ASSERT(0);
    }
}

static inline void
increment_mac_addr (mac_addr_t macaddr, int width = 1)
{
    uint64_t mac = MAC_TO_UINT64(macaddr) + width;
    MAC_UINT64_TO_ADDR(macaddr, mac);
}

/// \brief check if given two encaps are same
///
/// \param[in] encap1    encap1 to compare
/// \param[in] encap2    encap2 to compare
/// \returns TRUE if value of encap1 & encap2 are equal, else FALSE
static inline bool
pdsencap_isequal (const pds_encap_t *encap1, const pds_encap_t *encap2)
{
    // compare encap type
    if (encap1->type != encap2->type) {
        return FALSE;
    }
    // compare encap value
    switch (encap1->type) {
    case PDS_ENCAP_TYPE_DOT1Q:
        if (encap1->val.vlan_tag != encap2->val.vlan_tag) {
            return FALSE;
        }
        break;
    case PDS_ENCAP_TYPE_QINQ:
        if ((encap1->val.qinq_tag.c_tag != encap2->val.qinq_tag.c_tag) ||
            (encap1->val.qinq_tag.s_tag != encap2->val.qinq_tag.s_tag)) {
            return FALSE;
        }
        break;
    case PDS_ENCAP_TYPE_MPLSoUDP:
        if (encap1->val.mpls_tag != encap2->val.mpls_tag) {
            return FALSE;
        }
        break;
    case PDS_ENCAP_TYPE_VXLAN:
        if (encap1->val.vnid != encap2->val.vnid) {
            return FALSE;
        }
        break;
    default:
        if (encap1->val.value != encap2->val.value) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static inline void
increment_encap (pds_encap_t *encap, int width)
{
    switch(encap->type) {
    case PDS_ENCAP_TYPE_MPLSoUDP:
        encap->val.mpls_tag += width;
        break;
    case PDS_ENCAP_TYPE_VXLAN:
        encap->val.vnid += width;
        break;
    case PDS_ENCAP_TYPE_DOT1Q:
        encap->val.vlan_tag += width;
        break;
    default:
        encap->val.value += width;
    }
}

/// \brief Packet send function
///
/// \param tx_pkt          Packet to be trasnmited
/// \param tx_pkt_len      Transmit packet size in bytes
/// \param tx_port         Trasmit port TM_PORT_UPLINK_0/TM_PORT_UPLINK_1
/// \param exp_rx_pkt      Expected output on the receive port
/// \param exp_rx_pkt_len  Expected packet size in bytes
/// \param exp_rx_port     Expeted receive port TM_PORT_UPLINK_0/TM_PORT_UPLINK_1
void send_packet(const uint8_t *tx_pkt, uint32_t tx_pkt_len, uint32_t tx_port,
                 const uint8_t *exp_rx_pkt, uint32_t exp_rx_pkt_len,
                 uint32_t exp_rx_port);

/// \brief Packet dump function
///
/// param data Data to be printed
void dump_packet(std::vector<uint8_t> data);

uint32_t pds_get_next_addr16(uint32_t addr);

}    // namespace api_test

#endif    // __TEST_UTILS_UTILS_HPP__
