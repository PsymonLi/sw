//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// internal service lif handling in apollo pipelines
///
//----------------------------------------------------------------------------


#include <cmath>
#include <cstdio>
#include "nic/sdk/ipsec/ipsec.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/platform/utils/lif_manager_base.hpp"
#include "nic/sdk/platform/utils/qstate_mgr.hpp"
#include "nic/sdk/asic/common/asic_qstate.hpp"
#include "nic/sdk/p4/loader/loader.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/core.hpp"
#include "nic/apollo/api/impl/apollo/apollo_impl.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/p4/include/defines.h"
#include "nic/apollo/api/impl/devapi_impl.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/impl/svc_lif_impl_pstate.hpp"
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/apollo/api/upgrade_state.hpp"

using sdk::platform::utils::program_info;
using namespace sdk::asic::pd;

#define JLIF2QSTATE_MAP_NAME        "lif2qstate_map"
#define JIPSEC_LIF2QSTATE_MAP_NAME  "ipsec_lif2qstate_map"
#define JRXDMA_TO_TXDMA_BUF_NAME    "rxdma_to_txdma_buf"
#define JRXDMA_TO_TXDMA_DESC_NAME   "rxdma_to_txdma_desc"

typedef struct __attribute__((__packed__)) lifqstate_  {
    uint64_t pc : 8;
    uint64_t rsvd : 8;
    uint64_t cos_a : 4;
    uint64_t coa_b : 4;
    uint64_t cos_sel : 8;
    uint64_t eval_last : 8;
    uint64_t host_rings : 4;
    uint64_t total_rings : 4;
    uint64_t pid : 16;
    uint64_t pindex : 16;
    uint64_t cindex : 16;

    uint16_t sw_pindex;
    uint16_t sw_cindex;
    uint64_t ring0_base : 64;
    uint64_t ring1_base : 64;
    uint64_t ring_size : 16;
    uint64_t rxdma_cindex_addr : 64;

    uint8_t  pad[(512-336)/8];
} lifqstate_t;

void inline
init_lif_info (svc_lif_pstate_t *lif_pstate, sdk::platform::lif_info_t *lif_info)
{
    memset(lif_info, 0, sizeof(sdk::platform::lif_info_t));
    strncpy(lif_info->name, "Apollo Service LIF", sizeof(lif_info->name));
    lif_info->type = sdk::platform::LIF_TYPE_SERVICE;
    lif_info->lif_id = lif_pstate->lif_id;
    lif_info->tx_sched_table_offset = lif_pstate->tx_sched_table_offset;
    lif_info->tx_sched_num_table_entries = lif_pstate->tx_sched_num_table_entries;
    lif_info->queue_info[0].type_num = 0;
    lif_info->queue_info[0].size = 1; // 1 unit = 64B
    lif_info->queue_info[0].entries = 2; // 4 queues
}

/**
 * @brief    init routine to initialize service LIFs
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
init_service_lif (uint32_t lif_id, const char *cfg_path)
{
    uint8_t pgm_offset = 0;
    std::string prog_info_file;

    prog_info_file = std::string(cfg_path) + std::string("/") +
        std::string(LDD_INFO_FILE_RPATH) +
        std::string("/") + std::string(LDD_INFO_FILE_NAME);

    program_info *pginfo = program_info::factory(prog_info_file.c_str());
    SDK_ASSERT(pginfo != NULL);
    api::g_pds_state.set_prog_info(pginfo);

    sdk::platform::utils::LIFQState qstate = { 0 };
    qstate.lif_id = lif_id;
    qstate.hbm_address =
        api::g_pds_state.mempartition()->start_addr(JLIF2QSTATE_MAP_NAME);
    SDK_ASSERT(qstate.hbm_address != INVALID_MEM_ADDRESS);
    qstate.params_in.type[0].entries = 2; // 4 queues
    qstate.params_in.type[0].size = 1; // 1 unit = 64B
    asicpd_qstate_push(&qstate, 0);

    sdk::asic::get_pc_offset(pginfo, "txdma_stage0.bin",
                             "apollo_read_qstate", &pgm_offset);

    lifqstate_t lif_qstate = { 0 };
    lif_qstate.pc = pgm_offset;
    lif_qstate.ring0_base =
        api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_BUF_NAME);
    SDK_ASSERT(lif_qstate.ring0_base != INVALID_MEM_ADDRESS);
    lif_qstate.ring1_base =
        api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_DESC_NAME);
    SDK_ASSERT(lif_qstate.ring1_base != INVALID_MEM_ADDRESS);
    lif_qstate.ring_size =
        log2((api::g_pds_state.mempartition()->size(JRXDMA_TO_TXDMA_BUF_NAME) >> 10) / 10);
    lif_qstate.total_rings = 1;
    sdk::asic::write_qstate(qstate.hbm_address, (uint8_t *) &lif_qstate,
                            sizeof(lif_qstate));

    lifqstate_t txdma_qstate = { 0 };
    txdma_qstate.pc = pgm_offset;
    txdma_qstate.rxdma_cindex_addr =
        qstate.hbm_address + offsetof(lifqstate_t, sw_cindex);
    txdma_qstate.ring0_base =
        api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_BUF_NAME);
    SDK_ASSERT(txdma_qstate.ring0_base != INVALID_MEM_ADDRESS);
    txdma_qstate.ring1_base =
        api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_DESC_NAME);
    SDK_ASSERT(txdma_qstate.ring1_base != INVALID_MEM_ADDRESS);
    txdma_qstate.ring_size =
        log2((api::g_pds_state.mempartition()->size(JRXDMA_TO_TXDMA_BUF_NAME) >> 10) / 10);
    txdma_qstate.total_rings = 1;
    sdk::asic::write_qstate(qstate.hbm_address + sizeof(lifqstate_t),
                            (uint8_t *) &txdma_qstate, sizeof(txdma_qstate));

    // program the TxDMA scheduler for this LIF.
    sdk::platform::lif_info_t lif_info;
    pds_lif_spec_t lif_spec;
    sdk::lib::shmstore  *backup_store;
    svc_lif_pstate_t *lif_pstate;
    module_version_t  cur_version, prev_version;

    // get backup shmstore and update lif pstate later once scheduler is
    // programmed
    backup_store = api::g_upg_state->backup_shmstore(api::PDS_AGENT_OPER_SHMSTORE_ID);
    SDK_ASSERT(backup_store != NULL);
    std::tie(cur_version, prev_version) = api::g_upg_state->module_version(
                                             PDS_AGENT_MODULE_NAME,
                                             api::MODULE_VERSION_HITLESS);
    // create segment to backup svc lif pstate
    lif_pstate =
        (svc_lif_pstate_t *)backup_store->create_segment(SVC_LIF_PSTATE_NAME,
                                              sizeof(svc_lif_pstate_t));
    if (lif_pstate == NULL) {
        PDS_TRACE_ERR("Failed to create shmstore %s, for svc lif id %u",
                      SVC_LIF_PSTATE_NAME, lif_id);
        return SDK_RET_ERR;
    } else {
        new (lif_pstate) svc_lif_pstate_t();
        memcpy(&lif_pstate->metadata.version, &cur_version, sizeof(cur_version));
    }
    lif_pstate->lif_id = lif_id;

    init_lif_info(lif_pstate, &lif_info);
    api::impl::lif_spec_from_info(&lif_spec, &lif_info);
    api::impl::lif_impl::program_tx_scheduler(&lif_spec);

    // update lif pstate with latest value
    lif_pstate->tx_sched_table_offset = lif_spec.tx_sched_table_offset;
    lif_pstate->tx_sched_num_table_entries =
        lif_spec.tx_sched_num_table_entries;
    return SDK_RET_OK;
}

/**
 * @brief    init routine to initialize ipsec LIF
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
init_ipsec_lif (uint32_t lif_id)
{
    sdk::platform::utils::LIFQState qstate = { 0 };
    qstate.lif_id = lif_id;
    qstate.hbm_address =
        api::g_pds_state.mempartition()->start_addr(JIPSEC_LIF2QSTATE_MAP_NAME);
    SDK_ASSERT(qstate.hbm_address != INVALID_MEM_ADDRESS);
    qstate.params_in.type[0].entries = PDS_MAX_IPSEC_SA_SHIFT;
    qstate.params_in.type[0].size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    qstate.params_in.type[1].entries = PDS_MAX_IPSEC_SA_SHIFT;
    qstate.params_in.type[1].size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    asicpd_qstate_push(&qstate, 0);

    // initialize ipsec lif mgr
    lif_mgr *ipsec_lif_mgr;
    lif_qstate_t *lif_qstate = (lif_qstate_t *)SDK_CALLOC(
                               PDS_MEM_ALLOC_ID_HACK_IMPL_LIF_QSTATE,
                               sizeof(lif_qstate_t));
    lif_qstate->lif_id = lif_id;
    lif_qstate->hbm_address = api::g_pds_state.mempartition()->start_addr(
                              JIPSEC_LIF2QSTATE_MAP_NAME);
    lif_qstate->type[0].qtype_info.size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_qstate->type[0].qtype_info.entries = PDS_MAX_IPSEC_SA_SHIFT;
    lif_qstate->type[0].hbm_offset = 0;
    lif_qstate->type[1].qtype_info.size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_qstate->type[1].qtype_info.entries = PDS_MAX_IPSEC_SA_SHIFT;
    lif_qstate->type[1].hbm_offset = IPSEC_QSTATE_SIZE * PDS_MAX_IPSEC_SA;
    api::g_pds_state.set_ipsec_lif_qstate(lif_qstate);

    //Program the TxDMA scheduler for this LIF.
    sdk::platform::lif_info_t lif_info;
    pds_lif_spec_t lif_spec;

    memset(&lif_info, 0, sizeof(lif_info));
    strncpy(lif_info.name, "Apollo IPSEC LIF", sizeof(lif_info.name));
    lif_info.lif_id = lif_id;
    lif_info.type = sdk::platform::LIF_TYPE_SERVICE;
    lif_info.tx_sched_table_offset = INVALID_INDEXER_INDEX;
    lif_info.tx_sched_num_table_entries = 0;
    // encrypt qtype
    lif_info.queue_info[0].type_num = IPSEC_ENCRYPT_QTYPE;
    lif_info.queue_info[0].size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_info.queue_info[0].entries = PDS_MAX_IPSEC_SA_SHIFT;
    // decrypt qtype
    lif_info.queue_info[1].type_num = IPSEC_DECRYPT_QTYPE;
    lif_info.queue_info[1].size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_info.queue_info[1].entries = PDS_MAX_IPSEC_SA_SHIFT;
    api::impl::lif_spec_from_info(&lif_spec, &lif_info);
    api::impl::lif_impl::program_tx_scheduler(&lif_spec);
    return SDK_RET_OK;
}

/**
 * @brief     routine to validate service LIFs config during upgrade
 *            during A to B upgrade this will be called by B
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
service_lif_upgrade_verify (uint32_t lif_id, const char *cfg_path)
{
    int32_t rv;
    sdk_ret_t ret;
    lifqstate_t lif_qstate;
    uint8_t pgm_offset = 0;
    pds_lif_spec_t lif_spec;
    std::string prog_info_file;
    svc_lif_pstate_t *lif_pstate;
    void *to_pstate, *from_pstate;
    sdk::platform::lif_info_t lif_info;
    module_version_t cur_version, prev_version;
    sdk::platform::utils::LIFQState qstate = { 0 };
    sdk::lib::shmstore *backup_store, *restore_store;

    backup_store = api::g_upg_state->backup_shmstore(api::PDS_AGENT_OPER_SHMSTORE_ID);
    restore_store = api::g_upg_state->restore_shmstore(api::PDS_AGENT_OPER_SHMSTORE_ID);
    std::tie(cur_version, prev_version) = api::g_upg_state->module_version(
                                              PDS_AGENT_MODULE_NAME,
                                              api::MODULE_VERSION_HITLESS);
    //     hitless init:
    //     restore_store != NULL
    //     if version match:
    //         backup_store = NULL
    //     else:
    //         backup_store != NULL
    SDK_ASSERT(restore_store != NULL);
    if (cur_version.minor != prev_version.minor) {
        SDK_ASSERT(backup_store != NULL);
        to_pstate = backup_store->create_segment(SVC_LIF_PSTATE_NAME,
                                                 sizeof(svc_lif_pstate_t));
        SDK_ASSERT(to_pstate != NULL);
        new (to_pstate) svc_lif_pstate_t();
        from_pstate = restore_store->open_segment(SVC_LIF_PSTATE_NAME);
        SDK_ASSERT(from_pstate != NULL);
        // TODO: copy over data from from_pstate to to_pstate
        lif_pstate = (svc_lif_pstate_t *)to_pstate;
        if (lif_pstate->lif_id != lif_id) {
            PDS_TRACE_ERR("Service lif id %u version %u mismatch",
                           lif_id, lif_pstate->lif_id);
            return SDK_RET_ENTRY_NOT_FOUND;
        }
        memcpy(&lif_pstate->metadata.version, &cur_version, sizeof(cur_version));
        PDS_TRACE_DEBUG("svc lif %s, id %u, pstate converted to version %u",
                        SVC_LIF_PSTATE_NAME, lif_id, cur_version.version);
    } else {
        lif_pstate =
            (svc_lif_pstate_t *)restore_store->open_segment(SVC_LIF_PSTATE_NAME);
        if (lif_pstate == NULL || lif_pstate->lif_id != lif_id) {
            PDS_TRACE_ERR("Either shmstore %s open failed or lif id %u, "
                          "did not match with previous id %u",
                          SVC_LIF_PSTATE_NAME, lif_id,
                          (lif_pstate != NULL) ? lif_pstate->lif_id : lif_id);
            return SDK_RET_ENTRY_NOT_FOUND;
        }
    }

    prog_info_file = std::string(cfg_path) + std::string("/") +
        std::string(LDD_INFO_FILE_RPATH) +
        std::string("/") + std::string(LDD_INFO_FILE_NAME);
    program_info *pginfo = program_info::factory(prog_info_file.c_str());
    SDK_ASSERT(pginfo != NULL);
    api::g_pds_state.set_prog_info(pginfo);

    qstate.lif_id = lif_id;
    qstate.hbm_address =
        api::g_pds_state.mempartition()->start_addr(JLIF2QSTATE_MAP_NAME);
    if (qstate.hbm_address == INVALID_MEM_ADDRESS) {
        PDS_TRACE_ERR("LIF map not found");
        return SDK_RET_ERR;
    }
    // get the qstate pc offsets
    rv = sdk::asic::get_pc_offset(pginfo, "txdma_stage0.bin",
                                  "apollo_read_qstate", &pgm_offset);
    if (rv != 0) {
        PDS_TRACE_ERR("TXDMA stage0 pgm not found");
        return SDK_RET_ERR;
    }

    rv = sdk::asic::read_qstate(qstate.hbm_address, (uint8_t *)&lif_qstate,
                                sizeof(lifqstate_t));
    if (rv != 0) {
        PDS_TRACE_ERR("RXDMA qstate read failed");
        return SDK_RET_ERR;
    }
    // compare the ring configuration done by A with B config
    // it should be matching in address and size
    if ((lif_qstate.ring0_base !=
         api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_BUF_NAME)) ||
        (lif_qstate.ring1_base !=
         api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_DESC_NAME)) ||
        (lif_qstate.ring_size !=
         log2((api::g_pds_state.mempartition()->size(JRXDMA_TO_TXDMA_BUF_NAME) >> 10) / 10))) {
        PDS_TRACE_ERR("RXDMA qstate config mismatch found");
        return SDK_RET_ERR;
    }

    PDS_TRACE_DEBUG("Moving qstate addr 0x%lx, pc offset %lu, to offset %u",
                    qstate.hbm_address, lif_qstate.pc, pgm_offset);

    rv = sdk::asic::read_qstate(qstate.hbm_address + sizeof(lifqstate_t),
                                (uint8_t *)&lif_qstate, sizeof(lifqstate_t));
    if (rv != 0) {
        PDS_TRACE_ERR("TXDMA qstate read failed");
        return SDK_RET_ERR;
    }
    // compare the ring configuration done by A with B config
    // it should be matching in address and size
    if ((lif_qstate.ring0_base !=
         api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_BUF_NAME)) ||
        (lif_qstate.ring1_base !=
         api::g_pds_state.mempartition()->start_addr(JRXDMA_TO_TXDMA_DESC_NAME)) ||
        (lif_qstate.ring_size !=
         log2((api::g_pds_state.mempartition()->size(JRXDMA_TO_TXDMA_BUF_NAME) >> 10) / 10))) {
        PDS_TRACE_ERR("TXDMA qstate config mismatch found");
        return SDK_RET_ERR;
    }

    PDS_TRACE_DEBUG("Moving qstate addr 0x%lx, pc offset %lu, to offset %u",
                    qstate.hbm_address + sizeof(lifqstate_t),
                    lif_qstate.pc, pgm_offset);

    // save the qstate hbm address
    // this will be applied during switchover stage from A to B
    if (pgm_offset != lif_qstate.pc) {
        lif_qstate.pc = pgm_offset;
        api::g_upg_state->set_qstate_cfg(qstate.hbm_address,
                                         sizeof(lifqstate_t), pgm_offset);
        api::g_upg_state->set_qstate_cfg(
                 qstate.hbm_address + sizeof(lifqstate_t),
                 sizeof(lifqstate_t), pgm_offset);
    }

    //recover TxDMA scheduler state from shmstore segment
    if (lif_pstate->tx_sched_table_offset == INVALID_INDEXER_INDEX ||
        lif_pstate->tx_sched_num_table_entries == 0) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    init_lif_info(lif_pstate, &lif_info);
    api::impl::lif_spec_from_info(&lif_spec, &lif_info);
    ret = api::impl::lif_impl::reserve_tx_scheduler(&lif_spec);
    SDK_ASSERT(ret == SDK_RET_OK);
    return SDK_RET_OK;
}

/**
 * @brief     routine to validate service LIFs config during upgrade
 *            during A to B upgrade this will be called by B
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
ipsec_lif_upgrade_verify (uint32_t lif_id, const char *cfg_path)
{

   lif_qstate_t *lif_qstate = (lif_qstate_t *)SDK_CALLOC(
                               PDS_MEM_ALLOC_ID_HACK_IMPL_LIF_QSTATE,
                               sizeof(lif_qstate_t));
    lif_qstate->lif_id = lif_id;
    lif_qstate->hbm_address = api::g_pds_state.mempartition()->start_addr(
                              JIPSEC_LIF2QSTATE_MAP_NAME);
    lif_qstate->type[0].qtype_info.size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_qstate->type[0].qtype_info.entries = PDS_MAX_IPSEC_SA_SHIFT;
    lif_qstate->type[0].hbm_offset = 0;
    lif_qstate->type[1].qtype_info.size = (IPSEC_QSTATE_SIZE_SHIFT - 5);
    lif_qstate->type[1].qtype_info.entries = PDS_MAX_IPSEC_SA_SHIFT;
    lif_qstate->type[1].hbm_offset = IPSEC_QSTATE_SIZE * PDS_MAX_IPSEC_SA;
    api::g_pds_state.set_ipsec_lif_qstate(lif_qstate);

    // TODO : further checks in config including pc offsets
    return SDK_RET_OK;
}
