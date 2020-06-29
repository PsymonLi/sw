//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec cb decrypt routines
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ipsec.hpp"
#include "nic/sdk/ipsec/ipsec.hpp"
#include "nic/sdk/asic/common/asic_mem.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#include "nic/sdk/asic/common/asic_qstate.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/core/trace.hpp"
#include "gen/p4gen/esp_v4_tunnel_n2h_rxdma/include/esp_v4_tunnel_n2h_rxdma_p4plus_ingress.h"
#include "gen/p4gen/esp_v4_tunnel_n2h_txdma1/include/esp_v4_tunnel_n2h_txdma1_p4plus_ingress.h"
#include "nic/apollo/api/impl/ipsec/ipseccb.hpp"
#include "nic/apollo/api/impl/ipsec/ipseccb_internal.hpp"

namespace api {
namespace impl {

static sdk_ret_t
get_ipsec_decrypt_rx_stage0_prog_addr (uint8_t *offset)
{
    char progname[] = "rxdma_stage0.bin";
    char labelname[]= "ipsec_rx_n2h_stage0";
    program_info *prog_info = api::g_pds_state.prog_info();
    int ret;

    ret = sdk::asic::get_pc_offset(prog_info, progname, labelname, offset);
    if (ret != 0) {
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

static sdk_ret_t
add_ipsec_decrypt_rx_stage0_entry (ipseccb_ctxt_t *ctxt)
{
    common_p4plus_stage0_app_header_table_d data = { 0 };
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_DEC_QSTATE_0_OFFSET;
    sdk_ret_t ret;
    uint64_t ipsec_cb_ring_addr;
    uint64_t ipsec_barco_ring_addr;
    uint8_t pc_offset = 0;

    ret = get_ipsec_decrypt_rx_stage0_prog_addr(&pc_offset);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    data.action_id = pc_offset;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.total = 2;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.iv_size = IPSEC_DEF_IV_SIZE;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.icv_size = IPSEC_AES_GCM_DEF_ICV_SIZE;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.ipsec_cb_index = htons(ctxt->hw_id);

    // for now aes-decrypt-encoding hard-coded.
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_enc_cmd = IPSEC_BARCO_DECRYPT_AES_GCM_256;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.key_index = htons(ctxt->key_index);

    PDS_TRACE_DEBUG("key_index %d", ctxt->key_index);
    // the below may have to use a different range for the reverse direction

    ipsec_cb_ring_addr = asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_DECRYPT);
    PDS_TRACE_DEBUG("CB ring addr 0x%lx", ipsec_cb_ring_addr);

    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_ring_base_addr =
        htonl((uint32_t)ipsec_cb_ring_addr & 0xFFFFFFFF);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_cindex = 0;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_pindex = 0;

    ipsec_barco_ring_addr =
        asicpd_get_mem_addr(ASIC_HBM_REG_IPSECCB_BARCO_DECRYPT);
    PDS_TRACE_DEBUG("Barco ring addr 0x%lx", ipsec_barco_ring_addr);

    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_ring_base_addr = htonll(ipsec_barco_ring_addr);
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_cindex = 0;
    data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_pindex = 0;

    PDS_TRACE_DEBUG("Programming decrypt stage0 at addr 0x%lx", addr);

    ret = impl_base::pipeline_impl()->p4plus_write(0, addr, (uint8_t *)&data,
                                                   sizeof(data),
                                                   P4PLUS_CACHE_ACTION_NONE);

    return ret;
}

static sdk_ret_t
add_ipsec_decrypt_part2 (ipseccb_ctxt_t *ctxt)
{
    tx_table_s1_t2_esp_v4_tunnel_n2h_load_part2_d decrypt_part2;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_DEC_QSTATE_1_OFFSET;
    sdk_ret_t ret;

    decrypt_part2.spi = htonl(ctxt->decrypt_spec->spi);
    ret = impl_base::pipeline_impl()->p4plus_write(0, addr,
                                                   (uint8_t *)&decrypt_part2,
                                                   sizeof(decrypt_part2),
                                                   P4PLUS_CACHE_ACTION_NONE);
    return ret;
}

static sdk_ret_t
add_ipsec_decrypt_entry (ipseccb_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(add_ipsec_decrypt_rx_stage0_entry(ctxt));
    CALL_AND_CHECK_FN(add_ipsec_decrypt_part2(ctxt));

    return ret;
}

static sdk_ret_t
get_ipsec_decrypt_rx_stage0_entry (ipseccb_ctxt_t *ctxt)
{
    common_p4plus_stage0_app_header_table_d data;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_DEC_QSTATE_0_OFFSET;
    uint64_t ipsec_cb_ring_addr, ipsec_barco_ring_addr;
    uint16_t cb_cindex, cb_pindex;
    uint16_t barco_cindex, barco_pindex;

    impl_base::pipeline_impl()->p4plus_read(0, addr, (uint8_t *)&data,
                                            sizeof(data));

    ctxt->decrypt_info->status.key_index = ntohs(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.key_index);

    ipsec_cb_ring_addr = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_ring_base_addr);
    cb_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_cindex;
    cb_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.cb_pindex;
    PDS_TRACE_DEBUG("CB ring addr 0x%lx pindex %d cindex %d",
                    ipsec_cb_ring_addr, cb_pindex, cb_cindex);

    ipsec_barco_ring_addr = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_ring_base_addr);
    barco_cindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_cindex;
    barco_pindex = data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.barco_pindex;
    PDS_TRACE_DEBUG("Barco ring addr 0x%lx pindex %d cindex %d",
                    ipsec_barco_ring_addr, barco_pindex, barco_cindex);

    ctxt->decrypt_info->status.seq_no_bmp = ntohll(data.u.esp_v4_tunnel_n2h_rxdma_initial_table_d.replay_seq_no_bmp);

    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_decrypt_part2 (ipseccb_ctxt_t *ctxt)
{
    ipsec_decrypt_part2_t decrypt_part2;
    uint64_t addr = ctxt->cb_base_pa + IPSEC_CB_DEC_QSTATE_1_OFFSET;

    impl_base::pipeline_impl()->p4plus_read(0, addr, (uint8_t *)&decrypt_part2,
                                            sizeof(decrypt_part2));

    ctxt->decrypt_info->status.last_replay_seq_no = ntohl(decrypt_part2.last_replay_seq_no);
    ctxt->decrypt_info->spec.salt = decrypt_part2.iv_salt;
    ctxt->decrypt_info->spec.spi = ntohl(decrypt_part2.spi);
    return SDK_RET_OK;
}

static sdk_ret_t
get_ipsec_decrypt_entry (ipseccb_ctxt_t *ctxt)
{
    sdk_ret_t ret;

    CALL_AND_CHECK_FN(get_ipsec_decrypt_rx_stage0_entry(ctxt));
    CALL_AND_CHECK_FN(get_ipsec_decrypt_part2(ctxt));

    return ret;
}


sdk_ret_t
ipseccb_decrypt_create (uint32_t hw_id, uint64_t base_pa,
                        pds_ipsec_sa_decrypt_spec_t *spec)
{
    sdk_ret_t ret;
    ipseccb_ctxt_t ctxt = { 0 };
    bool key1_allocated = false;

    ctxt.hw_id = hw_id;
    ctxt.cb_base_pa = base_pa;
    ctxt.key_type = ipseccb_get_crypto_key_type(spec->decryption_algo);
    ctxt.key_size = ipseccb_get_crypto_key_size(spec->decryption_algo);

    ret = capri_barco_sym_alloc_key(&ctxt.key_index);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to create key index");
        return ret;
    }
    key1_allocated = true;

    ret = capri_barco_setup_key(ctxt.key_index, ctxt.key_type,
                                spec->decrypt_key, ctxt.key_size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to write key at index %u", ctxt.key_index);
        goto cleanup;
    }

    ctxt.decrypt_spec = spec;

    ret = add_ipsec_decrypt_entry(&ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to ipsec cb at index %u", hw_id);
        goto cleanup;
    }

    return ret;

cleanup:
    if (key1_allocated)
        capri_barco_sym_free_key(ctxt.key_index);
    return ret;
}

sdk_ret_t
ipseccb_decrypt_get (uint32_t hw_id, uint64_t base_pa,
                     pds_ipsec_sa_decrypt_info_t *info)
{
    sdk_ret_t ret;
    ipseccb_ctxt_t ctxt = { 0 };

    ctxt.hw_id = hw_id;
    ctxt.cb_base_pa = base_pa;
    ctxt.decrypt_info = info;

    ret = get_ipsec_decrypt_entry(&ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get ipsec cb at index %u", hw_id);
    }

    return ret;
}

}    // namespace impl
}    // namespace api
