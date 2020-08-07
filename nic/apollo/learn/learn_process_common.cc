//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// mode agnostic APIs to process packets received on learn lif
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/include/sdk/l2.hpp"
#include "nic/sdk/include/sdk/l4.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/internal/pds_mapping.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/learn/ep_utils.hpp"
#include "nic/apollo/learn/learn_ctxt.hpp"
#include "nic/apollo/learn/learn_internal.hpp"

namespace learn {

using namespace sdk::types;

void
update_counters (learn_ctxt_t *ctxt, ep_learn_type_t learn_type,
                 pds_mapping_type_t mtype)
{
    int idx = LEARN_TYPE_CTR_IDX(learn_type);
    batch_counters_t *counters = &ctxt->lbctxt->counters;

    if (mtype == PDS_MAPPING_TYPE_L2) {
        counters->mac_learns[idx]++;
    } else {
        counters->ip_learns[idx]++;
    }
}

static void
extract_arp_pkt_type (learn_ctxt_t *ctxt, uint16_t arp_op,
                      arp_data_ipv4_t *arp_data)
{
    bool garp;

    garp = (arp_data->sip == arp_data->tip);
    switch (arp_op) {
    case ARPOP_REQUEST:
        if (garp && MAC_TO_UINT64(arp_data->tmac) == 0) {
            ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_GARP_ANNOUNCE;
        } else if (arp_data->sip == 0) {
            ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_ARP_PROBE;
        } else {
            ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_ARP_REQUEST;
        }
        break;
    case ARPOP_REPLY:
        if (garp) {
            ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_GARP_REPLY;
        } else {
            ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_ARP_REPLY;
        }
        break;
    case ARPOP_REVREQUEST:
        ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_RARP_REQUEST;
        break;
    case ARPOP_REVREPLY:
        ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_RARP_REPLY;
        break;
    default:
        break;
    }
}

static bool
extract_ip_learn_info (char *pkt_data, learn_ctxt_t *ctxt)
{
    ip_addr_t ip = {0};
    impl::learn_info_t *impl = &ctxt->pkt_ctxt.impl_info;
    pds_obj_key_t vpc;

    switch (impl->pkt_type) {
    case PKT_TYPE_DHCP:
    {
        dhcp_header_t *dhcp = (dhcp_header_t *) (pkt_data +
                                                 impl->l3_offset +
                                                 IPV4_MIN_HDR_LEN +
                                                 UDP_HDR_LEN);
        dhcp_option_t *op = dhcp->options;
        bool dhcp_request = false;
        bool ip_found = false;
        while (op->option != DHCP_PACKET_OPTION_END) {
            if (DHCP_PACKET_OPTION_MSG_TYPE == op->option) {
                if (DHCP_PACKET_REQUEST == op->data[0]) {
                    dhcp_request = true;
                    ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_DHCP_REQUEST;
                } else {
                    if (op->data[0] == DHCP_PACKET_DISCOVER) {
                        ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_DHCP_DISCOVER;
                    }
                    // this is not msg of our interest
                    break;
                }
            } else if (DHCP_PACKET_OPTION_REQ_IP_ADDR == op->option) {
                ip.af = IP_AF_IPV4;
                ip.addr.v4_addr = ntohl((*(op->data_as_u32)));
                ip_found = true;
            }
            if (dhcp_request && ip_found) {
                break;
            }
            op = (dhcp_option_t *) (op->data + op->length);
        }
        if (!dhcp_request || !ip_found) {
            return false;
        }
        break;
    }
    case PKT_TYPE_ARP:
    {
        arp_hdr_t *arp_hdr = (arp_hdr_t *) (pkt_data + impl->l3_offset);
        arp_data_ipv4_t *arp_data = (arp_data_ipv4_t *) (arp_hdr + 1);

        // for RARP packets, we are not interested in IP address
        if (ctxt->pkt_ctxt.impl_info.hints & LEARN_HINT_RARP) {
            extract_arp_pkt_type(ctxt, htons(arp_hdr->op), arp_data);
            return false;
        }

        if (ntohs(arp_hdr->ptype) != ETH_TYPE_IPV4) {
            return false;
        }

        extract_arp_pkt_type(ctxt, htons(arp_hdr->op), arp_data);
        // check if packet smac is same as smac in arp header
        if (memcmp(ctxt->mac_key.mac_addr, &arp_data->smac, ETH_ADDR_LEN) != 0) {
            return false;
        }

        ip.af = IP_AF_IPV4;
        ip.addr.v4_addr = ntohl(arp_data->sip);
        break;
    }
    case PKT_TYPE_IPV4:
        ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_IPV4;
        IPV4_HDR_SIP_GET(pkt_data + impl->l3_offset, ip.addr.v4_addr);
        ip.af = IP_AF_IPV4;
        break;
    case PKT_TYPE_NDP:
    case PKT_TYPE_IPV6:
    default:
        return false;
    }

    // check for zeroed out IPv4 address
    if (ip.af == IP_AF_IPV4 && ip.addr.v4_addr == 0) {
        return false;
    }
    if (learn_oper_mode() == PDS_LEARN_MODE_NOTIFY) {
        ctxt->ip_key.make_key(&ip, impl->lif);
    } else {
        vpc = subnet_db()->find(&ctxt->mac_key.subnet)->vpc();
        ctxt->ip_key.make_key(&ip, vpc);
    }
    return true;
}

static sdk_ret_t
extract_learn_info (char *pkt_data, learn_ctxt_t *ctxt)
{
    char *src_mac;
    impl::learn_info_t *impl = &ctxt->pkt_ctxt.impl_info;

    // MAC addr is always available, populate ep->mac_key
    src_mac = pkt_data + impl->l2_offset + ETH_ADDR_LEN;

    if (learn_oper_mode() == PDS_LEARN_MODE_NOTIFY) {
        ctxt->mac_key.make_key(src_mac, impl->lif);
    } else {
        ctxt->mac_key.make_key(src_mac, impl->subnet);
    }
    if (!is_mac_set(ctxt->mac_key.mac_addr)) {
        PDS_TRACE_ERR("Source MAC 00:00:00:00:00:00 is invalid, subnet %s,"
                      " pkt type %s", impl->subnet.str(),
                      sdk::types::pkttype2str(impl->pkt_type).c_str());
        ctxt->pkt_ctxt.pkt_type = LEARN_PKT_TYPE_NONE;
        return SDK_RET_ERR;
    }

    // construct ifindex
    ctxt->ifindex = HOST_IFINDEX(impl->lif);

    // extract IP address if present and populate ep->ip_key
    if (!extract_ip_learn_info(pkt_data, ctxt)) {
        PDS_TRACE_DEBUG("No IP info found, pkt type %s",
                        sdk::types::pkttype2str(impl->pkt_type).c_str());
    }

    return SDK_RET_OK;
}

sdk_ret_t
parse_packet (learn_ctxt_t *ctxt, char *pkt, bool *reinject)
{
    sdk_ret_t ret;

    // get impl specific p4 header results
    ret = impl::extract_info_from_p4_hdr(pkt, &ctxt->pkt_ctxt.impl_info);
    if (unlikely(ret != SDK_RET_OK)) {
        *reinject = false;
        return ret;
    }

    // parse MAC and IP address and extract learn info
    ret = extract_learn_info(pkt, ctxt);
    if (unlikely(ret != SDK_RET_OK)) {
        *reinject = true;
        return ret;
    }

    return SDK_RET_OK;
}

}    // namespace learn
