//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec cb encrypt routines
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ipsec.hpp"
#include "nic/sdk/ipsec/ipsec.hpp"
#include "nic/sdk/asic/common/asic_mem.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#include "nic/sdk/asic/common/asic_qstate.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/core/trace.hpp"
#include "gen/p4gen/esp_ipv4_tunnel_h2n_rxdma/include/esp_ipv4_tunnel_h2n_rxdma_p4plus_ingress.h"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb_internal.hpp"

namespace api {
namespace impl {

static sdk_ret_t
get_ipsec_rx_stage0_prog_addr (uint8_t *offset)
{
    char progname[] = "rxdma_stage0.bin";
    char labelname[]= "ipsec_rx_stage0";
    program_info *prog_info = api::g_pds_state.prog_info();
    int ret;

    ret = sdk::asic::get_pc_offset(prog_info, progname, labelname, offset);
    if (ret != 0) {
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
add_ipsec_rx_stage0_entry (ipseccb_ctxt_t *ctxt)
{
    common_p4plus_stage0_app_header_table_d data = { 0 };
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_ENC_QSTATE_0_OFFSET;
    sdk_ret_t ret;
    uint64_t ipsec_cb_ring_addr;
    uint64_t ipsec_barco_ring_addr;
    uint8_t pc_offset = 0;

    ret = get_ipsec_rx_stage0_prog_addr(&pc_offset);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Could not get ipsec stage 0 addr, ret = %d", ret);
        return ret;
    }
    data.action_id = pc_offset;
    data.u.ipsec_encap_rxdma_initial_table_d.total = 2;
    data.u.ipsec_encap_rxdma_initial_table_d.iv = ctxt->encrypt_spec->iv;
    data.u.ipsec_encap_rxdma_initial_table_d.iv_salt = ctxt->encrypt_spec->salt;
    data.u.ipsec_encap_rxdma_initial_table_d.iv_size = IPSEC_DEF_IV_SIZE;
    data.u.ipsec_encap_rxdma_initial_table_d.icv_size = IPSEC_AES_GCM_DEF_ICV_SIZE;
    data.u.ipsec_encap_rxdma_initial_table_d.esn_lo = 0;
    data.u.ipsec_encap_rxdma_initial_table_d.spi = htonl(ctxt->encrypt_spec->spi);
    data.u.ipsec_encap_rxdma_initial_table_d.ipsec_cb_index = htons(ctxt->hw_id);

    PDS_TRACE_DEBUG("iv 0x%lx salt 0x%x iv_size %d icv_size %d esn_lo %d spi %d",
            ctxt->encrypt_spec->iv, ctxt->encrypt_spec->salt, IPSEC_DEF_IV_SIZE, IPSEC_AES_GCM_DEF_ICV_SIZE,
            0, ctxt->encrypt_spec->spi);

    data.u.ipsec_encap_rxdma_initial_table_d.key_index = htons(ctxt->key_index);
    PDS_TRACE_DEBUG("key_index = %d", ctxt->key_index);

    ipsec_cb_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB) +
                         ctxt->hw_id * IPSEC_DEFAULT_RING_SIZE * IPSEC_PER_CB_RING_SIZE;
    PDS_TRACE_DEBUG("CB ring addr 0x%lx", ipsec_cb_ring_addr);

    data.u.ipsec_encap_rxdma_initial_table_d.cb_ring_base_addr = htonl((uint32_t)(ipsec_cb_ring_addr & 0xFFFFFFFF));
    data.u.ipsec_encap_rxdma_initial_table_d.cb_cindex = 0;
    data.u.ipsec_encap_rxdma_initial_table_d.cb_pindex = 0;

    ipsec_barco_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_BARCO) +
                            ctxt->hw_id * IPSEC_BARCO_SLOT_ELEM_SIZE *
                            IPSEC_BARCO_RING_SIZE;
    PDS_TRACE_DEBUG("Barco ring addr 0x%lx", ipsec_barco_ring_addr);

    data.u.ipsec_encap_rxdma_initial_table_d.barco_ring_base_addr = htonl((uint32_t) (ipsec_barco_ring_addr & 0xFFFFFFFF));
    data.u.ipsec_encap_rxdma_initial_table_d.barco_cindex = 0;
    data.u.ipsec_encap_rxdma_initial_table_d.barco_pindex = 0;

    data.u.ipsec_encap_rxdma_initial_table_d.flags = 0;

    PDS_TRACE_DEBUG("Programming ipsec stage0 at address 0x%lx", addr);

    ret = impl_base::pipeline_impl()->p4plus_write(0, addr, (uint8_t *)&data,
                                                   sizeof(data),
                                                   P4PLUS_CACHE_ACTION_NONE);
    return ret;
}

static sdk_ret_t
add_ipsec_ip_header_entry (ipseccb_ctxt_t *ctxt)
{
    ipsec_eth_ip4_hdr_t eth_ip_hdr = { 0 };
    uint8_t data[CACHE_LINE_SIZE];
    sdk_ret_t ret = SDK_RET_OK;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_ENC_ETH_IP_HDR_OFFSET;

    // v4 only for now
    // TODO : smac/dmac
    //memcpy(eth_ip_hdr.smac, sa->smac, ETH_ADDR_LEN);
    //memcpy(eth_ip_hdr.dmac, sa->dmac, ETH_ADDR_LEN);
    // no 
    eth_ip_hdr.dot1q_ethertype = htons(0x0800);
    //eth_ip_hdr.vlan = htons(sa->vrf_vlan);
    //PDS_TRACE_DEBUG("vrf vlan : %d", sa->vrf_vlan);
    //eth_ip_hdr.ethertype = htons(0x800);
    eth_ip_hdr.version_ihl = 0x45;
    eth_ip_hdr.tos = 0;
    //p4 will update/correct this part - fixed for now.
    eth_ip_hdr.tot_len = htons(64);
    eth_ip_hdr.id = 0;
    eth_ip_hdr.frag_off = 0;
    eth_ip_hdr.ttl = 255;
    eth_ip_hdr.protocol = 50; // ESP - will hash define it.
    eth_ip_hdr.check = 0; // P4 to fill the right checksum
    eth_ip_hdr.saddr = htonl(ctxt->encrypt_spec->local_gateway_ip.addr.v4_addr);
    eth_ip_hdr.daddr = htonl(ctxt->encrypt_spec->remote_gateway_ip.addr.v4_addr);
    PDS_TRACE_DEBUG("Tunnel SIP 0x%x tunnel DIP 0x%x", eth_ip_hdr.saddr,
                    eth_ip_hdr.daddr);

    PDS_TRACE_DEBUG("Programming at addr 0x%lx", addr);
    memcpy(data, (uint8_t *)&eth_ip_hdr, sizeof(eth_ip_hdr));
    ret = impl_base::pipeline_impl()->p4plus_write(0, addr, (uint8_t *)&data,
                                                   sizeof(data),
                                                   P4PLUS_CACHE_ACTION_NONE);
    return ret;
}

static sdk_ret_t
add_ipsec_encrypt_entry (ipseccb_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(add_ipsec_rx_stage0_entry(ctxt));
    CALL_AND_CHECK_FN(add_ipsec_ip_header_entry(ctxt));

    return ret;
}

static sdk_ret_t
get_ipsec_rx_stage0_entry (ipseccb_ctxt_t *ctxt)
{
    common_p4plus_stage0_app_header_table_d data;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_ENC_QSTATE_0_OFFSET;
    uint64_t ipsec_cb_ring_addr, ipsec_barco_ring_addr;

    impl_base::pipeline_impl()->p4plus_read(0, addr, (uint8_t *)&data,
                                            sizeof(data));
    ctxt->encrypt_info->spec.iv = ntohll(data.u.ipsec_encap_rxdma_initial_table_d.iv);
    ctxt->encrypt_info->spec.salt = data.u.ipsec_encap_rxdma_initial_table_d.iv_salt;
    ctxt->encrypt_info->spec.spi = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.spi);
    ctxt->encrypt_info->status.key_index = ntohs(data.u.ipsec_encap_rxdma_initial_table_d.key_index);
    ipsec_cb_ring_addr = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.cb_ring_base_addr);

    ipsec_barco_ring_addr  = ntohl(data.u.ipsec_encap_rxdma_initial_table_d.barco_ring_base_addr);

    PDS_TRACE_DEBUG("CB ring addr 0x%lx barco ring addr 0x%lx pindex %ld cindex %ld",
                    ipsec_cb_ring_addr, ipsec_barco_ring_addr,
                    data.u.ipsec_encap_rxdma_initial_table_d.barco_pindex,
                    data.u.ipsec_encap_rxdma_initial_table_d.barco_cindex);
    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_ip_header_entry (ipseccb_ctxt_t *ctxt)
{
    ipsec_qstate_addr_part2_t data;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_ENC_ETH_IP_HDR_OFFSET;

    impl_base::pipeline_impl()->p4plus_read(0, addr, (uint8_t *)&data,
                                            sizeof(data));

    ctxt->encrypt_info->spec.local_gateway_ip.addr.v4_addr = ntohl(data.u.eth_ip4_hdr.saddr);
    ctxt->encrypt_info->spec.remote_gateway_ip.addr.v4_addr = ntohl(data.u.eth_ip4_hdr.daddr);

    PDS_TRACE_DEBUG("SIP 0x%x DIP 0x%x", ctxt->encrypt_info->spec.local_gateway_ip.addr.v4_addr,
                    ctxt->encrypt_info->spec.remote_gateway_ip.addr.v4_addr);

    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_encrypt_entry (ipseccb_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(get_ipsec_rx_stage0_entry(ctxt));
    CALL_AND_CHECK_FN(get_ipsec_ip_header_entry(ctxt));

    return ret;
}

sdk_ret_t
ipseccb_encrypt_create (uint32_t hw_id, uint64_t base_pa,
                        pds_ipsec_sa_encrypt_spec_t *spec)
{
    sdk_ret_t ret;
    ipseccb_ctxt_t ctxt = { 0 };

    PDS_TRACE_DEBUG("ipseccb_encrypt_create: hw_id %u, base_pa 0x%lx",
                    hw_id, base_pa);

    ctxt.hw_id = hw_id;
    ctxt.cb_base_pa = base_pa;
    ctxt.key_type = ipseccb_get_crypto_key_type(spec->encryption_algo);
    ctxt.key_size = ipseccb_get_crypto_key_size(spec->encryption_algo);

    ret = capri_barco_sym_alloc_key(&ctxt.key_index);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to create key index");
        return ret;
    }

    ret = capri_barco_setup_key(ctxt.key_index, ctxt.key_type,
                                spec->encrypt_key, ctxt.key_size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to write key at index %u", ctxt.key_index);
        goto cleanup;
    }

    ctxt.encrypt_spec = spec;

    ret = add_ipsec_encrypt_entry(&ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to ipsec cb at index %u", hw_id);
        goto cleanup;
    }

    return ret;

cleanup:
    capri_barco_sym_free_key(ctxt.key_index);
    return ret;
}

sdk_ret_t
ipseccb_encrypt_get (uint32_t hw_id, uint64_t base_pa,
                     pds_ipsec_sa_encrypt_info_t *info)
{
    sdk_ret_t ret;
    ipseccb_ctxt_t ctxt = { 0 };

    ctxt.hw_id = hw_id;
    ctxt.cb_base_pa = base_pa;
    ctxt.encrypt_info = info;

    ret = get_ipsec_encrypt_entry(&ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get ipsec cb at index %u", hw_id);
    }

    return ret;
}

sdk_ret_t
ipseccb_encrypt_update_nexthop_id (uint32_t hw_id, uint64_t base_pa,
                                   uint16_t nh_id, uint8_t nh_type)
{
    uint32_t data;
    uint8_t *ptr = (uint8_t *)&data;

    *(uint16_t *)ptr = nh_id;
    ptr += 2;
    *(uint8_t *)ptr = nh_type;
    uint64_t addr = base_pa + IPSEC_CB_ENC_NEXTHOP_ID_OFFSET;
    impl_base::pipeline_impl()->p4plus_write(0, addr, (uint8_t *)&data,
                                             sizeof(uint32_t),
                                             P4PLUS_CACHE_ACTION_NONE);
    return SDK_RET_OK;
}

}    // namespace impl
}    // namespace api
