// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "nic/sdk/asic/cmn/asic_hbm.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#include "ipseccb.hpp"
#include "ipseccb_internal.hpp"
#include "gen/p4gen/esp_ipv4_tunnel_h2n_rxdma/include/esp_ipv4_tunnel_h2n_rxdma_p4plus_ingress.h"

namespace utils {
namespace ipseccb {

static sdk_ret_t
get_ipsec_rx_stage0_prog_addr(uint64_t *offset)
{
    char progname[] = "rxdma_stage0.bin";
    char labelname[]= "ipsec_rx_stage0";

    sdk_ret_t ret = sdk::p4::p4_program_label_to_offset("p4plus",
                                                        progname,
                                                        labelname,
                                                        offset);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    *offset >>= CACHE_LINE_SIZE_SHIFT;
    return ret;
}

static sdk_ret_t
add_ipsec_rx_stage0_entry(ipsec_sa_t *sa)
{
    common_p4plus_stage0_app_header_table_d data = { 0 };
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_ENC_QSTATE_0_OFFSET;
    sdk_ret_t ret;
    uint64_t pc_offset = 0;
    uint64_t ipsec_cb_ring_addr;
    uint64_t ipsec_barco_ring_addr;

    ret = get_ipsec_rx_stage0_prog_addr(&pc_offset);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    data.action_id = pc_offset;
    data.u.ipsec_encap_rxdma_initial_table_d.total = 2;
    data.u.ipsec_encap_rxdma_initial_table_d.iv = sa->iv;
    data.u.ipsec_encap_rxdma_initial_table_d.iv_salt = sa->iv_salt;
    data.u.ipsec_encap_rxdma_initial_table_d.iv_size = sa->iv_size;
    data.u.ipsec_encap_rxdma_initial_table_d.icv_size = sa->icv_size;
    data.u.ipsec_encap_rxdma_initial_table_d.barco_enc_cmd = sa->barco_enc_cmd;
    data.u.ipsec_encap_rxdma_initial_table_d.esn_lo = htonl(sa->esn_lo);
    data.u.ipsec_encap_rxdma_initial_table_d.spi = htonl(sa->spi);
    data.u.ipsec_encap_rxdma_initial_table_d.ipsec_cb_index = htons(sa->sa_id);

    SDK_TRACE_DEBUG("iv 0x%lx salt 0x%x iv_size %d icv_size %d barco_cmd 0x%x esn_lo %d spi %d",
            sa->iv, sa->iv_salt, sa->iv_size, sa->icv_size, sa->barco_enc_cmd,
            sa->esn_lo, sa->spi);

    data.u.ipsec_encap_rxdma_initial_table_d.key_index = htons(sa->key_index);
    SDK_TRACE_DEBUG("key_index = %d", sa->key_index);

    ipsec_cb_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB) +
                         sa->sa_id * IPSEC_DEFAULT_RING_SIZE * IPSEC_PER_CB_RING_SIZE;
    SDK_TRACE_DEBUG("CB Ring Addr 0x%lx", ipsec_cb_ring_addr);

    data.u.ipsec_encap_rxdma_initial_table_d.cb_ring_base_addr = htonl((uint32_t)(ipsec_cb_ring_addr & 0xFFFFFFFF));
    data.u.ipsec_encap_rxdma_initial_table_d.cb_cindex = 0;
    data.u.ipsec_encap_rxdma_initial_table_d.cb_pindex = 0;

    ipsec_barco_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_BARCO) +
                            sa->sa_id * IPSEC_BARCO_SLOT_ELEM_SIZE *
                            IPSEC_BARCO_RING_SIZE;
    SDK_TRACE_DEBUG("Barco Ring Addr 0x%lx", ipsec_barco_ring_addr);

    data.u.ipsec_encap_rxdma_initial_table_d.barco_ring_base_addr = htonl((uint32_t) (ipsec_barco_ring_addr & 0xFFFFFFFF));
    data.u.ipsec_encap_rxdma_initial_table_d.barco_cindex = 0;
    data.u.ipsec_encap_rxdma_initial_table_d.barco_pindex = 0;

    if (sa->is_v6) {
        data.u.ipsec_encap_rxdma_initial_table_d.flags |= 1;
    } else {
        data.u.ipsec_encap_rxdma_initial_table_d.flags &= 0xFE;
    }

    if (sa->is_nat_t) {
        data.u.ipsec_encap_rxdma_initial_table_d.flags |= 2;
    } else {
        data.u.ipsec_encap_rxdma_initial_table_d.flags &= 0xFD;
    }

    if (sa->is_random) {
        data.u.ipsec_encap_rxdma_initial_table_d.flags |= 4;
    } else {
        data.u.ipsec_encap_rxdma_initial_table_d.flags &= 0xFB;
    }
    if (sa->extra_pad) {
        data.u.ipsec_encap_rxdma_initial_table_d.flags |= 8;
    } else {
        data.u.ipsec_encap_rxdma_initial_table_d.flags &= 0xF7;
    }
    data.u.ipsec_encap_rxdma_initial_table_d.flags |= 16;

    SDK_TRACE_DEBUG("flags 0x%lx is_v6 %d is_nat_t %d is_random %d extra_pad %d",
            data.u.ipsec_encap_rxdma_initial_table_d.flags,
            sa->is_v6, sa->is_nat_t, sa->is_random, sa->extra_pad);
    SDK_TRACE_DEBUG("Programming ipsec stage0 at address: 0x%lx", addr);

    ret = ipseccb_write_one(addr, (uint8_t *)&data, sizeof(data));
    return ret;
}

static sdk_ret_t
add_ipsec_ip_header_entry(ipsec_sa_t *sa)
{
    pd_ipsec_eth_ip4_hdr_t eth_ip_hdr = { 0 };
    pd_ipsec_eth_ip6_hdr_t eth_ip6_hdr = { 0 };
    pd_ipsec_udp_nat_t_hdr_t nat_t_udp_hdr = { 0 };
    uint8_t data[CACHE_LINE_SIZE];
    sdk_ret_t ret = SDK_RET_OK;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_ENC_ETH_IP_HDR_OFFSET;

    if (sa->is_v6 == 0) {
        memcpy(eth_ip_hdr.smac, sa->smac, ETH_ADDR_LEN);
        memcpy(eth_ip_hdr.dmac, sa->dmac, ETH_ADDR_LEN);
        eth_ip_hdr.dot1q_ethertype = htons(0x8100);
        eth_ip_hdr.vlan = htons(sa->vrf_vlan);
        SDK_TRACE_DEBUG("vrf vlan : %d", sa->vrf_vlan);
        eth_ip_hdr.ethertype = htons(0x800);
        eth_ip_hdr.version_ihl = 0x45;
        eth_ip_hdr.tos = 0;
        //p4 will update/correct this part - fixed for now.
        eth_ip_hdr.tot_len = htons(64);
        eth_ip_hdr.id = 0;
        eth_ip_hdr.frag_off = 0;
        eth_ip_hdr.ttl = 255;
        if (sa->is_nat_t == 1) {
            eth_ip_hdr.protocol = 17; // UDP - will hash define it.
        } else {
            eth_ip_hdr.protocol = 50; // ESP - will hash define it.
        }
        eth_ip_hdr.check = 0; // P4 to fill the right checksum
        eth_ip_hdr.saddr = htonl(sa->tunnel_sip4.addr.v4_addr);
        eth_ip_hdr.daddr = htonl(sa->tunnel_dip4.addr.v4_addr);
        SDK_TRACE_DEBUG("Tunnel SIP 0x%x Tunnel DIP 0x%x", eth_ip_hdr.saddr,
                        eth_ip_hdr.daddr);
    } else {
        memcpy(eth_ip6_hdr.smac, sa->smac, ETH_ADDR_LEN);
        memcpy(eth_ip6_hdr.dmac, sa->dmac, ETH_ADDR_LEN);
        eth_ip6_hdr.ethertype = htons(0x86dd);
        eth_ip6_hdr.ver_tc_flowlabel = htonl(0x60000000);
        eth_ip6_hdr.payload_length = 128;
        eth_ip6_hdr.next_hdr = 50;
        eth_ip6_hdr.hop_limit = 255;
        //memcpy(eth_ip6_hdr.src, sa->sip6.addr.v6_addr.addr8, IP6_ADDR8_LEN);
        //memcpy(eth_ip6_hdr.dst, sa->dip6.addr.v6_addr.addr8, IP6_ADDR8_LEN);
        SDK_TRACE_DEBUG("Adding IPV6 header");
    }

    SDK_TRACE_DEBUG("Programming at addr: 0x%lx", addr);
    if (sa->is_v6 == 0) {
        memcpy(data, (uint8_t *)&eth_ip_hdr, sizeof(eth_ip_hdr));
        if (sa->is_nat_t == 1) {
            nat_t_udp_hdr.sport = htons(UDP_PORT_NAT_T);
            nat_t_udp_hdr.dport = htons(UDP_PORT_NAT_T);
            memcpy(data+34,  (uint8_t *)&nat_t_udp_hdr, sizeof(nat_t_udp_hdr));
        }
    } else {
        memcpy(data,  (uint8_t *)&eth_ip6_hdr, sizeof(eth_ip6_hdr));
        if (sa->is_nat_t == 1) {
            nat_t_udp_hdr.sport = htons(UDP_PORT_NAT_T);
            nat_t_udp_hdr.dport = htons(UDP_PORT_NAT_T);
            memcpy(data+54,  (uint8_t *)&nat_t_udp_hdr, sizeof(nat_t_udp_hdr));
        }
    }
    ret = ipseccb_write_one(addr, (uint8_t *)&data, sizeof(data));
    return ret;
}

static sdk_ret_t
add_ipsec_encrypt_entry(ipsec_sa_t *sa)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(add_ipsec_rx_stage0_entry(sa));
    CALL_AND_CHECK_FN(add_ipsec_ip_header_entry(sa));

    return ret;
}

static sdk_ret_t
get_ipsec_rx_stage0_entry(ipsec_sa_t *sa)
{
    common_p4plus_stage0_app_header_table_d data;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_ENC_QSTATE_0_OFFSET;
    uint64_t ipsec_cb_ring_addr, ipsec_barco_ring_addr;

    ipseccb_read_one(addr, (uint8_t *)&data, sizeof(data));
    sa->iv = ntohll(data.u.ipsec_encap_rxdma_initial_table_d.iv);
    sa->iv_salt = data.u.ipsec_encap_rxdma_initial_table_d.iv_salt;
    sa->iv_size = data.u.ipsec_encap_rxdma_initial_table_d.iv_size;
    sa->icv_size = data.u.ipsec_encap_rxdma_initial_table_d.icv_size;
    sa->barco_enc_cmd = data.u.ipsec_encap_rxdma_initial_table_d.barco_enc_cmd;
    sa->esn_lo = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.esn_lo);
    sa->spi = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.spi);
    sa->key_index = ntohs(data.u.ipsec_encap_rxdma_initial_table_d.key_index);
    ipsec_cb_ring_addr = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.cb_ring_base_addr);

    ipsec_barco_ring_addr  = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.barco_ring_base_addr);

    sa->is_v6 = data.u.ipsec_encap_rxdma_initial_table_d.flags & 0x1;
    sa->is_nat_t = data.u.ipsec_encap_rxdma_initial_table_d.flags & 0x2;
    sa->is_random = data.u.ipsec_encap_rxdma_initial_table_d.flags & 0x4;
    SDK_TRACE_DEBUG("CB Ring Addr 0x%lx Barco Ring Addr 0x%lx Pindex %ld CIndex %ld",
                    ipsec_cb_ring_addr, ipsec_barco_ring_addr,
                    data.u.ipsec_encap_rxdma_initial_table_d.barco_pindex,
                    data.u.ipsec_encap_rxdma_initial_table_d.barco_cindex);
    SDK_TRACE_DEBUG("Flags : is_v6 : %d is_nat_t : %d is_random : %d", sa->is_v6,
                    sa->is_nat_t, sa->is_random);
    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_ip_header_entry(ipsec_sa_t *sa)
{
    pd_ipsec_qstate_addr_part2_t data;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_ENC_ETH_IP_HDR_OFFSET;
    
    ipseccb_read_one(addr, (uint8_t *)&data, sizeof(data));

    sa->tunnel_sip4.addr.v4_addr = ntohl(data.u.eth_ip4_hdr.saddr);
    sa->tunnel_dip4.addr.v4_addr = ntohl(data.u.eth_ip4_hdr.daddr);

    SDK_TRACE_DEBUG("SIP 0x%x DIP 0x%x", sa->tunnel_sip4.addr.v4_addr,
                    sa->tunnel_dip4.addr.v4_addr);

    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_encrypt_entry(ipsec_sa_t *sa)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(get_ipsec_rx_stage0_entry(sa));
    CALL_AND_CHECK_FN(get_ipsec_ip_header_entry(sa));

    return ret;
}

sdk_ret_t
ipseccb_encrypt_create(ipsec_sa_t *sa)
{
    sdk_ret_t ret;

    ret = capri_barco_sym_alloc_key(&sa->key_index);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to create key index");
        return ret;
    }

    ret = capri_barco_setup_key(sa->key_index, sa->key_type,
                                sa->key, sa->key_size);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to write key at index %u", sa->key_index);
        goto cleanup;
    }

    ret = add_ipsec_encrypt_entry(sa);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to ipsec cb at index %u", sa->sa_id);
        goto cleanup;
    }

    return ret;

cleanup:
    capri_barco_sym_free_key(sa->key_index);
    return ret;
}

sdk_ret_t
ipseccb_encrypt_get(ipsec_sa_t *sa)
{
    sdk_ret_t ret;

    ret = get_ipsec_encrypt_entry(sa);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to get ipsec cb at index %u", sa->sa_id);
    }

    return ret;
}

} // namespace ipseccb
} // namespace utils
