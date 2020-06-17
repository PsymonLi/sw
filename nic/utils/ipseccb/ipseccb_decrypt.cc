
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include "nic/sdk/asic/common/asic_hbm.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#include "ipseccb.hpp"
#include "ipseccb_internal.hpp"
#include "gen/p4gen/esp_v4_tunnel_n2h_rxdma/include/esp_v4_tunnel_n2h_rxdma_p4plus_ingress.h"
#include "gen/p4gen/esp_v4_tunnel_n2h_txdma1/include/esp_v4_tunnel_n2h_txdma1_p4plus_ingress.h"

namespace utils {
namespace ipseccb {

static sdk_ret_t
get_ipsec_decrypt_rx_stage0_prog_addr(uint64_t *offset)
{
    char progname[] = "rxdma_stage0.bin";
    char labelname[]= "ipsec_rx_n2h_stage0";

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
add_ipsec_decrypt_rx_stage0_entry(ipsec_sa_t *sa)
{
    common_p4plus_stage0_app_header_table_d data = {0};
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_DEC_QSTATE_0_OFFSET;
    sdk_ret_t ret;
    uint64_t pc_offset = 0;
    uint64_t ipsec_cb_ring_addr;
    uint64_t ipsec_barco_ring_addr;

    ret = get_ipsec_decrypt_rx_stage0_prog_addr(&pc_offset);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    data.action_id = pc_offset;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.total = 2;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.iv_size = sa->iv_size;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.icv_size = sa->icv_size;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.ipsec_cb_index = htons(sa->sa_id);

    // for now aes-decrypt-encoding hard-coded.
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_enc_cmd = 0x30100000;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.expected_seq_no = sa->esn_lo;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.key_index = htons(sa->key_index);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.new_key_index = htons(sa->new_key_index);

    SDK_TRACE_DEBUG("key_index %d, new_key_index %d", sa->key_index,
                    sa->new_key_index);
    // the below may have to use a different range for the reverse direction

    ipsec_cb_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_DECRYPT);
    SDK_TRACE_DEBUG("CB Ring Addr 0x%lx", ipsec_cb_ring_addr);

    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_ring_base_addr =
        htonl((uint32_t)ipsec_cb_ring_addr & 0xFFFFFFFF);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_cindex = 0;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_pindex = 0;

    ipsec_barco_ring_addr =
        asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_BARCO_DECRYPT);
    SDK_TRACE_DEBUG("Barco Ring Addr 0x%lx", ipsec_barco_ring_addr);

    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_ring_base_addr = htonll(ipsec_barco_ring_addr);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_cindex = 0;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_pindex = 0;

    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.vrf_vlan = htons(sa->vrf_vlan);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.is_v6 = sa->is_v6;
    SDK_TRACE_DEBUG("Programming Decrypt stage0 at hw-id: 0x%lx", addr);

    ret = ipseccb_write_one(addr, (uint8_t *)&data, sizeof(data));

    return ret;
}

static sdk_ret_t
add_ipsec_decrypt_part2(ipsec_sa_t *sa)
{
    tx_table_s1_t2_esp_v4_tunnel_n2h_load_part2_d decrypt_part2;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_DEC_QSTATE_1_OFFSET;
    sdk_ret_t ret;

    decrypt_part2.spi = htonl(sa->spi);
    decrypt_part2.new_spi = htonl(sa->new_spi);
    ret = ipseccb_write_one(addr, (uint8_t *)&decrypt_part2,
                            sizeof(decrypt_part2));
    return ret;
}
 
static sdk_ret_t
add_ipsec_decrypt_entry(ipsec_sa_t *sa)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(add_ipsec_decrypt_rx_stage0_entry(sa));
    CALL_AND_CHECK_FN(add_ipsec_decrypt_part2(sa));

    return ret;
}

static sdk_ret_t
get_ipsec_decrypt_rx_stage0_entry(ipsec_sa_t *sa, ipsec_stats_t *stats)
{
    common_p4plus_stage0_app_header_table_d data;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_DEC_QSTATE_0_OFFSET;
    uint64_t ipsec_cb_ring_addr, ipsec_barco_ring_addr;
    uint16_t cb_cindex, cb_pindex;
    uint16_t barco_cindex, barco_pindex;

    ipseccb_read_one(addr, (uint8_t *)&data, sizeof(data));

    sa->iv_size = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.iv_size;
    sa->icv_size = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.icv_size;
    sa->barco_enc_cmd = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_enc_cmd;
    sa->key_index = ntohs(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.key_index);
    sa->new_key_index = ntohs(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.new_key_index);

    ipsec_cb_ring_addr = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_ring_base_addr);
    cb_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_cindex;
    cb_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_pindex;
    SDK_TRACE_DEBUG("CB Ring Addr 0x%lx Pindex %d CIndex %d",
                    ipsec_cb_ring_addr, cb_pindex, cb_cindex);

    ipsec_barco_ring_addr = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_ring_base_addr);
    barco_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_cindex;
    barco_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_pindex;
    SDK_TRACE_DEBUG("Barco Ring Addr 0x%lx Pindex %d CIndex %d",
                    ipsec_barco_ring_addr, barco_pindex, barco_cindex);

    sa->expected_seq_no = ntohl(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.expected_seq_no);
    sa->seq_no_bmp = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.replay_seq_no_bmp);
    sa->vrf_vlan = ntohs(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.vrf_vlan);
    sa->is_v6 = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.is_v6;

    stats->cb_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_cindex;
    stats->cb_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_pindex;
    stats->barco_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_pindex;
    stats->barco_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_cindex;
    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_decrypt_part2(ipsec_sa_t *sa, ipsec_stats_t *stats)
{
    pd_ipsec_decrypt_part2_t decrypt_part2;
    uint64_t addr = sa->cb_base_pa + IPSEC_CB_DEC_QSTATE_1_OFFSET;

    ipseccb_read_one(addr, (uint8_t *)&decrypt_part2, sizeof(decrypt_part2));

    sa->last_replay_seq_no = ntohl(decrypt_part2.last_replay_seq_no);
    sa->iv_salt = decrypt_part2.iv_salt;
    sa->spi = ntohl(decrypt_part2.spi);
    sa->new_spi = ntohl(decrypt_part2.new_spi);
    SDK_TRACE_DEBUG("spi %d new_spi %d", sa->spi, sa->new_spi);
    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_decrypt_entry(ipsec_sa_t *sa, ipsec_stats_t *stats)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(get_ipsec_decrypt_rx_stage0_entry(sa, stats));
    CALL_AND_CHECK_FN(get_ipsec_decrypt_part2(sa, stats));

    return ret;
}


sdk_ret_t
ipseccb_decrypt_create(ipsec_sa_t *sa)
{
    sdk_ret_t ret;
    bool key1_allocated = false, key2_allocated = false;

    ret = capri_barco_sym_alloc_key(&sa->key_index);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to create key index");
        return ret;
    }
    key1_allocated = true;

    ret = capri_barco_setup_key(sa->key_index, sa->key_type,
                                sa->key, sa->key_size);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to write key at index %u", sa->key_index);
        goto cleanup;
    }

    // new key
    ret = capri_barco_sym_alloc_key(&sa->new_key_index);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to create key index");
        goto cleanup;
    }
    key2_allocated = true;

    ret = capri_barco_setup_key(sa->new_key_index, sa->new_key_type,
                                sa->new_key, sa->new_key_size);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to write key at index %u", sa->new_key_index);
        goto cleanup;
    }

    ret = add_ipsec_decrypt_entry(sa);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to ipsec cb at index %u", sa->sa_id);
        goto cleanup;
    }

    return ret;

cleanup:
    if (key1_allocated)
        capri_barco_sym_free_key(sa->key_index);
    if (key2_allocated)
        capri_barco_sym_free_key(sa->new_key_index);
    return ret;
}

sdk_ret_t
ipseccb_decrypt_get(ipsec_sa_t *sa, ipsec_stats_t *stats)
{
    sdk_ret_t ret;

    ret = get_ipsec_decrypt_entry(sa, stats);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to get ipsec cb at index %u", sa->sa_id);
    }

    return ret;
}

} // namespace ipseccb
} // namespace utils
