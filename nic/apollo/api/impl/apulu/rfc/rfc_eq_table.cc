/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    rfc_eq_table.cc
 *
 * @brief   RFC library equivalence table handling
 */

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/apollo/framework/pipeline_impl_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/impl/rfc/rfc.hpp"
#include "nic/apollo/api/impl/rfc/rfc_tree.hpp"
#include "nic/apollo/api/impl/rfc/rte_bitmap_utils.hpp"
#include "nic/apollo/api/impl/apulu/rfc/rfc_utils.hpp"
#include "gen/p4gen/p4plus_txdma/include/p4plus_txdma_p4pd.h"
#include "nic/apollo/p4/include/apulu_sacl_defines.h"

namespace rfc {

typedef void (*rfc_compute_cl_addr_entry_cb_t)(mem_addr_t base_address,
                                               uint32_t table_idx,
                                               mem_addr_t *next_cl_addr,
                                               uint16_t *next_entry_num);
typedef sdk_ret_t (*rfc_action_data_flush_cb_t)(mem_addr_t addr,
                                                void *action_data);
typedef sdk_ret_t (*rfc_table_entry_pack_cb_t)(uint32_t table_idx,
                                               void *action_data,
                                               uint32_t entry_num,
                                               uint16_t entry_val);
typedef uint16_t rfc_compute_entry_val_cb_t(rfc_ctxt_t *rfc_ctxt,
                                            rfc_table_t *rfc_table,
                                            rte_bitmap *cbm, uint32_t cbm_size,
                                            void *ctxt);
/**
 * @brief    given a classid & entry id, fill the corresponding portion of the
 *           RFC phase 1 table entry action data
 * @param[in] table_idx     index of the entry (for debugging)
 * @param[in] actiondata    pointer to the action data
 * @param[in] entry_num     entry idx (0 to 50, inclusive), we can fit 51
 *                          entries, each 10 bits wide
 * @param[in] cid           RFC class id
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p1_table_entry_pack (uint32_t table_idx, void *actiondata,
                         uint32_t entry_num, uint16_t cid)
{
    rfc_p1_actiondata_t    *action_data;

    PDS_TRACE_VERBOSE("P1 table[0x%x] <- class id %u", table_idx, cid);
    action_data = (rfc_p1_actiondata_t *)actiondata;
    switch (entry_num) {
    case 0:
        action_data->action_u.rfc_p1_rfc_action_p1.id00 = cid;
        break;
    case 1:
        action_data->action_u.rfc_p1_rfc_action_p1.id01 = cid;
        break;
    case 2:
        action_data->action_u.rfc_p1_rfc_action_p1.id02 = cid;
        break;
    case 3:
        action_data->action_u.rfc_p1_rfc_action_p1.id03 = cid;
        break;
    case 4:
        action_data->action_u.rfc_p1_rfc_action_p1.id04 = cid;
        break;
    case 5:
        action_data->action_u.rfc_p1_rfc_action_p1.id05 = cid;
        break;
    case 6:
        action_data->action_u.rfc_p1_rfc_action_p1.id06 = cid;
        break;
    case 7:
        action_data->action_u.rfc_p1_rfc_action_p1.id07 = cid;
        break;
    case 8:
        action_data->action_u.rfc_p1_rfc_action_p1.id08 = cid;
        break;
    case 9:
        action_data->action_u.rfc_p1_rfc_action_p1.id09 = cid;
        break;
    case 10:
        action_data->action_u.rfc_p1_rfc_action_p1.id10 = cid;
        break;
    case 11:
        action_data->action_u.rfc_p1_rfc_action_p1.id11 = cid;
        break;
    case 12:
        action_data->action_u.rfc_p1_rfc_action_p1.id12 = cid;
        break;
    case 13:
        action_data->action_u.rfc_p1_rfc_action_p1.id13 = cid;
        break;
    case 14:
        action_data->action_u.rfc_p1_rfc_action_p1.id14 = cid;
        break;
    case 15:
        action_data->action_u.rfc_p1_rfc_action_p1.id15 = cid;
        break;
    case 16:
        action_data->action_u.rfc_p1_rfc_action_p1.id16 = cid;
        break;
    case 17:
        action_data->action_u.rfc_p1_rfc_action_p1.id17 = cid;
        break;
    case 18:
        action_data->action_u.rfc_p1_rfc_action_p1.id18 = cid;
        break;
    case 19:
        action_data->action_u.rfc_p1_rfc_action_p1.id19 = cid;
        break;
    case 20:
        action_data->action_u.rfc_p1_rfc_action_p1.id20 = cid;
        break;
    case 21:
        action_data->action_u.rfc_p1_rfc_action_p1.id21 = cid;
        break;
    case 22:
        action_data->action_u.rfc_p1_rfc_action_p1.id22 = cid;
        break;
    case 23:
        action_data->action_u.rfc_p1_rfc_action_p1.id23 = cid;
        break;
    case 24:
        action_data->action_u.rfc_p1_rfc_action_p1.id24 = cid;
        break;
    case 25:
        action_data->action_u.rfc_p1_rfc_action_p1.id25 = cid;
        break;
    case 26:
        action_data->action_u.rfc_p1_rfc_action_p1.id26 = cid;
        break;
    case 27:
        action_data->action_u.rfc_p1_rfc_action_p1.id27 = cid;
        break;
    case 28:
        action_data->action_u.rfc_p1_rfc_action_p1.id28 = cid;
        break;
    case 29:
        action_data->action_u.rfc_p1_rfc_action_p1.id29 = cid;
        break;
    case 30:
        action_data->action_u.rfc_p1_rfc_action_p1.id30 = cid;
        break;
    case 31:
        action_data->action_u.rfc_p1_rfc_action_p1.id31 = cid;
        break;
    case 32:
        action_data->action_u.rfc_p1_rfc_action_p1.id32 = cid;
        break;
    case 33:
        action_data->action_u.rfc_p1_rfc_action_p1.id33 = cid;
        break;
    case 34:
        action_data->action_u.rfc_p1_rfc_action_p1.id34 = cid;
        break;
    case 35:
        action_data->action_u.rfc_p1_rfc_action_p1.id35 = cid;
        break;
    case 36:
        action_data->action_u.rfc_p1_rfc_action_p1.id36 = cid;
        break;
    case 37:
        action_data->action_u.rfc_p1_rfc_action_p1.id37 = cid;
        break;
    case 38:
        action_data->action_u.rfc_p1_rfc_action_p1.id38 = cid;
        break;
    case 39:
        action_data->action_u.rfc_p1_rfc_action_p1.id39 = cid;
        break;
    case 40:
        action_data->action_u.rfc_p1_rfc_action_p1.id40 = cid;
        break;
    case 41:
        action_data->action_u.rfc_p1_rfc_action_p1.id41 = cid;
        break;
    case 42:
        action_data->action_u.rfc_p1_rfc_action_p1.id42 = cid;
        break;
    case 43:
        action_data->action_u.rfc_p1_rfc_action_p1.id43 = cid;
        break;
    case 44:
        action_data->action_u.rfc_p1_rfc_action_p1.id44 = cid;
        break;
    case 45:
        action_data->action_u.rfc_p1_rfc_action_p1.id45 = cid;
        break;
    case 46:
        action_data->action_u.rfc_p1_rfc_action_p1.id46 = cid;
        break;
    case 47:
        action_data->action_u.rfc_p1_rfc_action_p1.id47 = cid;
        break;
    case 48:
        action_data->action_u.rfc_p1_rfc_action_p1.id48 = cid;
        break;
    case 49:
        action_data->action_u.rfc_p1_rfc_action_p1.id49 = cid;
        break;
    case 50:
        action_data->action_u.rfc_p1_rfc_action_p1.id50 = cid;
        break;
    default:
        PDS_TRACE_ERR("Invalid entry number %u while packing RFC P1 table",
                      entry_num);
        break;
    }

    return SDK_RET_OK;
}

/**
 * @brief    given a classid & entry id, fill the corresponding portion of the
 *           RFC phase 2 table entry action data
 * @param[in] table_idx     index of the entry (for debugging)
 * @param[in] actiondata    pointer to the action data
 * @param[in] entry_num     entry idx (0 to 50, inclusive), we can fit 51
 *                          entries, each 10 bits wide
 * @param[in] cid           RFC class id
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p2_table_entry_pack (uint32_t table_idx, void *actiondata,
                         uint32_t entry_num, uint16_t cid)
{
    rfc_p2_actiondata_t    *action_data;

    PDS_TRACE_VERBOSE("P2 table[0x%x] <- class id %u", table_idx, cid);
    action_data = (rfc_p2_actiondata_t *)actiondata;
    switch (entry_num) {
        case 0:
            action_data->action_u.rfc_p2_rfc_action_p2.id00 = cid;
            break;
        case 1:
            action_data->action_u.rfc_p2_rfc_action_p2.id01 = cid;
            break;
        case 2:
            action_data->action_u.rfc_p2_rfc_action_p2.id02 = cid;
            break;
        case 3:
            action_data->action_u.rfc_p2_rfc_action_p2.id03 = cid;
            break;
        case 4:
            action_data->action_u.rfc_p2_rfc_action_p2.id04 = cid;
            break;
        case 5:
            action_data->action_u.rfc_p2_rfc_action_p2.id05 = cid;
            break;
        case 6:
            action_data->action_u.rfc_p2_rfc_action_p2.id06 = cid;
            break;
        case 7:
            action_data->action_u.rfc_p2_rfc_action_p2.id07 = cid;
            break;
        case 8:
            action_data->action_u.rfc_p2_rfc_action_p2.id08 = cid;
            break;
        case 9:
            action_data->action_u.rfc_p2_rfc_action_p2.id09 = cid;
            break;
        case 10:
            action_data->action_u.rfc_p2_rfc_action_p2.id10 = cid;
            break;
        case 11:
            action_data->action_u.rfc_p2_rfc_action_p2.id11 = cid;
            break;
        case 12:
            action_data->action_u.rfc_p2_rfc_action_p2.id12 = cid;
            break;
        case 13:
            action_data->action_u.rfc_p2_rfc_action_p2.id13 = cid;
            break;
        case 14:
            action_data->action_u.rfc_p2_rfc_action_p2.id14 = cid;
            break;
        case 15:
            action_data->action_u.rfc_p2_rfc_action_p2.id15 = cid;
            break;
        case 16:
            action_data->action_u.rfc_p2_rfc_action_p2.id16 = cid;
            break;
        case 17:
            action_data->action_u.rfc_p2_rfc_action_p2.id17 = cid;
            break;
        case 18:
            action_data->action_u.rfc_p2_rfc_action_p2.id18 = cid;
            break;
        case 19:
            action_data->action_u.rfc_p2_rfc_action_p2.id19 = cid;
            break;
        case 20:
            action_data->action_u.rfc_p2_rfc_action_p2.id20 = cid;
            break;
        case 21:
            action_data->action_u.rfc_p2_rfc_action_p2.id21 = cid;
            break;
        case 22:
            action_data->action_u.rfc_p2_rfc_action_p2.id22 = cid;
            break;
        case 23:
            action_data->action_u.rfc_p2_rfc_action_p2.id23 = cid;
            break;
        case 24:
            action_data->action_u.rfc_p2_rfc_action_p2.id24 = cid;
            break;
        case 25:
            action_data->action_u.rfc_p2_rfc_action_p2.id25 = cid;
            break;
        case 26:
            action_data->action_u.rfc_p2_rfc_action_p2.id26 = cid;
            break;
        case 27:
            action_data->action_u.rfc_p2_rfc_action_p2.id27 = cid;
            break;
        case 28:
            action_data->action_u.rfc_p2_rfc_action_p2.id28 = cid;
            break;
        case 29:
            action_data->action_u.rfc_p2_rfc_action_p2.id29 = cid;
            break;
        case 30:
            action_data->action_u.rfc_p2_rfc_action_p2.id30 = cid;
            break;
        case 31:
            action_data->action_u.rfc_p2_rfc_action_p2.id31 = cid;
            break;
        case 32:
            action_data->action_u.rfc_p2_rfc_action_p2.id32 = cid;
            break;
        case 33:
            action_data->action_u.rfc_p2_rfc_action_p2.id33 = cid;
            break;
        case 34:
            action_data->action_u.rfc_p2_rfc_action_p2.id34 = cid;
            break;
        case 35:
            action_data->action_u.rfc_p2_rfc_action_p2.id35 = cid;
            break;
        case 36:
            action_data->action_u.rfc_p2_rfc_action_p2.id36 = cid;
            break;
        case 37:
            action_data->action_u.rfc_p2_rfc_action_p2.id37 = cid;
            break;
        case 38:
            action_data->action_u.rfc_p2_rfc_action_p2.id38 = cid;
            break;
        case 39:
            action_data->action_u.rfc_p2_rfc_action_p2.id39 = cid;
            break;
        case 40:
            action_data->action_u.rfc_p2_rfc_action_p2.id40 = cid;
            break;
        case 41:
            action_data->action_u.rfc_p2_rfc_action_p2.id41 = cid;
            break;
        case 42:
            action_data->action_u.rfc_p2_rfc_action_p2.id42 = cid;
            break;
        case 43:
            action_data->action_u.rfc_p2_rfc_action_p2.id43 = cid;
            break;
        case 44:
            action_data->action_u.rfc_p2_rfc_action_p2.id44 = cid;
            break;
        case 45:
            action_data->action_u.rfc_p2_rfc_action_p2.id45 = cid;
            break;
        case 46:
            action_data->action_u.rfc_p2_rfc_action_p2.id46 = cid;
            break;
        case 47:
            action_data->action_u.rfc_p2_rfc_action_p2.id47 = cid;
            break;
        case 48:
            action_data->action_u.rfc_p2_rfc_action_p2.id48 = cid;
            break;
        case 49:
            action_data->action_u.rfc_p2_rfc_action_p2.id49 = cid;
            break;
        case 50:
            action_data->action_u.rfc_p2_rfc_action_p2.id50 = cid;
            break;
        default:
        PDS_TRACE_ERR("Invalid entry number %u while packing RFC P2 table",
                      entry_num);
            break;
    }

    return SDK_RET_OK;
}

/**
 * @brief    write the current contents of RFC P1 action data buffer to memory
 * @param[in] addr        address to write the action data to
 * @param[in] actiondata    action data buffer
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p1_action_data_flush (mem_addr_t addr, void *actiondata)
{
    sdk_ret_t                        ret;
    rfc_p1_actiondata_t    *action_data;

    PDS_TRACE_VERBOSE("Flushing action data to 0x%lx", addr);
    action_data = (rfc_p1_actiondata_t *)actiondata;
    ret = impl_base::pipeline_impl()->write_to_txdma_table(addr,
                                   P4_P4PLUS_TXDMA_TBL_ID_RFC_P1,
                                   RFC_P1_RFC_ACTION_P1_ID,
                                   action_data);
    // reset the action data after flushing it
    memset(action_data, 0, sizeof(*action_data));
    return ret;
}

/**
 * @brief    write the current contents of RFC P2 action data buffer to memory
 * @param[in] addr        address to write the action data to
 * @param[in] actiondata    action data buffer
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p2_action_data_flush (mem_addr_t addr, void *actiondata)
{
    sdk_ret_t                        ret;
    rfc_p2_actiondata_t    *action_data;

    PDS_TRACE_VERBOSE("Flushing action data to 0x%lx", addr);
    action_data = (rfc_p2_actiondata_t *)actiondata;
    ret = impl_base::pipeline_impl()->write_to_txdma_table(addr,
              P4_P4PLUS_TXDMA_TBL_ID_RFC_P2,
              RFC_P2_RFC_ACTION_P2_ID,
              action_data);
    // reset the action data after flushing it
    memset(action_data, 0, sizeof(*action_data));
    return ret;
}

/**
 * @brief    dump the contents of P1 equivalence table(s)
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 */
static inline void
rfc_p1_eq_class_tables_dump (rfc_ctxt_t *rfc_ctxt)
{
    rfc_table_t    *rfc_table = &rfc_ctxt->p1_table;

    PDS_TRACE_VERBOSE("RFC P1 equivalence class table dump : ");
    rfc_eq_class_table_dump(rfc_table);
}

/**
 * @brief    dump the contents of P1 equivalence table(s)
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 */
static inline void
rfc_p2_eq_class_tables_dump (rfc_ctxt_t *rfc_ctxt)
{
    rfc_table_t    *rfc_table = &rfc_ctxt->p2_table;

    PDS_TRACE_VERBOSE("RFC P2 equivalence class table dump : ");
    rfc_eq_class_table_dump(rfc_table);
}

/**
 * @brief    compute the cache line address where the given class block
 *           starts from and the entry index where the 1st entry should
 *           be written to
 * @param[in] base_address            RFC phase 1 table's base address
 * @param[in] table_idx               index into the P1 table
 * @param[out] next_cl_addr           next cache line address computed
 * @param[out] next_entry_num         next entry number computed
 */
static inline void
rfc_compute_p1_next_cl_addr_entry_num (mem_addr_t base_address,
                                       uint32_t table_idx,
                                       mem_addr_t *next_cl_addr,
                                       uint16_t *next_entry_num)
{
    uint32_t cache_line_idx = table_idx / SACL_P1_ENTRIES_PER_CACHE_LINE;
    *next_cl_addr = base_address + (cache_line_idx << CACHE_LINE_SIZE_SHIFT);
    *next_entry_num = table_idx % SACL_P1_ENTRIES_PER_CACHE_LINE;
}

/**
 * @brief    compute the cache line address where the given class block
 *           starts from and the entry index where the 1st entry should
 *           be written to
 * @param[in] base_address            RFC phase 2 table's base address
 * @param[in] table_idx               index into the P2 table
 * @param[out] next_cl_addr           next cache line address computed
 * @param[out] next_entry_num         next entry number computed
 */
static inline void
rfc_compute_p2_next_cl_addr_entry_num (mem_addr_t base_address,
                                       uint32_t table_idx,
                                       mem_addr_t *next_cl_addr,
                                       uint16_t *next_entry_num)
{
    uint32_t cache_line_idx = table_idx / SACL_P2_ENTRIES_PER_CACHE_LINE;
    *next_cl_addr = base_address + (cache_line_idx << CACHE_LINE_SIZE_SHIFT);
    *next_entry_num = table_idx % SACL_P2_ENTRIES_PER_CACHE_LINE;
}

/**
 * @brief    given the entry num, fill corresponding portion of the RFC phase 3
 *           table entry action data.
 * @param[in] table_idx      index of the entry (for debugging)
 * @param[in] action_data    pointer to the action data
 * @param[in] entry_num      entry idx (0 to 46, inclusive), we can fit 46
 *                           entries, each 11 bits wide
 * @param[in] rule_id        rule id
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p3_table_entry_pack (uint32_t table_idx, void *actiondata,
                         uint32_t entry_num, uint16_t rule_id)
{
    rfc_p3_actiondata_t    *action_data;

    action_data = (rfc_p3_actiondata_t *)actiondata;
    rule_id &= (uint16_t)0x3FF; // Only 10 bits are valid
    PDS_TRACE_VERBOSE("P3 table[0x%x] <- rule id %u", table_idx, rule_id);
    switch (entry_num) {
    case 0:
        action_data->action_u.rfc_p3_rfc_action_p3.id00 = rule_id;
        break;
    case 1:
        action_data->action_u.rfc_p3_rfc_action_p3.id01 = rule_id;
        break;
    case 2:
        action_data->action_u.rfc_p3_rfc_action_p3.id02 = rule_id;
        break;
    case 3:
        action_data->action_u.rfc_p3_rfc_action_p3.id03 = rule_id;
        break;
    case 4:
        action_data->action_u.rfc_p3_rfc_action_p3.id04 = rule_id;
        break;
    case 5:
        action_data->action_u.rfc_p3_rfc_action_p3.id05 = rule_id;
        break;
    case 6:
        action_data->action_u.rfc_p3_rfc_action_p3.id06 = rule_id;
        break;
    case 7:
        action_data->action_u.rfc_p3_rfc_action_p3.id07 = rule_id;
        break;
    case 8:
        action_data->action_u.rfc_p3_rfc_action_p3.id08 = rule_id;
        break;
    case 9:
        action_data->action_u.rfc_p3_rfc_action_p3.id09 = rule_id;
        break;
    case 10:
        action_data->action_u.rfc_p3_rfc_action_p3.id10 = rule_id;
        break;
    case 11:
        action_data->action_u.rfc_p3_rfc_action_p3.id11 = rule_id;
        break;
    case 12:
        action_data->action_u.rfc_p3_rfc_action_p3.id12 = rule_id;
        break;
    case 13:
        action_data->action_u.rfc_p3_rfc_action_p3.id13 = rule_id;
        break;
    case 14:
        action_data->action_u.rfc_p3_rfc_action_p3.id14 = rule_id;
        break;
    case 15:
        action_data->action_u.rfc_p3_rfc_action_p3.id15 = rule_id;
        break;
    case 16:
        action_data->action_u.rfc_p3_rfc_action_p3.id16 = rule_id;
        break;
    case 17:
        action_data->action_u.rfc_p3_rfc_action_p3.id17 = rule_id;
        break;
    case 18:
        action_data->action_u.rfc_p3_rfc_action_p3.id18 = rule_id;
        break;
    case 19:
        action_data->action_u.rfc_p3_rfc_action_p3.id19 = rule_id;
        break;
    case 20:
        action_data->action_u.rfc_p3_rfc_action_p3.id20 = rule_id;
        break;
    case 21:
        action_data->action_u.rfc_p3_rfc_action_p3.id21 = rule_id;
        break;
    case 22:
        action_data->action_u.rfc_p3_rfc_action_p3.id22 = rule_id;
        break;
    case 23:
        action_data->action_u.rfc_p3_rfc_action_p3.id23 = rule_id;
        break;
    case 24:
        action_data->action_u.rfc_p3_rfc_action_p3.id24 = rule_id;
        break;
    case 25:
        action_data->action_u.rfc_p3_rfc_action_p3.id25 = rule_id;
        break;
    case 26:
        action_data->action_u.rfc_p3_rfc_action_p3.id26 = rule_id;
        break;
    case 27:
        action_data->action_u.rfc_p3_rfc_action_p3.id27 = rule_id;
        break;
    case 28:
        action_data->action_u.rfc_p3_rfc_action_p3.id28 = rule_id;
        break;
    case 29:
        action_data->action_u.rfc_p3_rfc_action_p3.id29 = rule_id;
        break;
    case 30:
        action_data->action_u.rfc_p3_rfc_action_p3.id30 = rule_id;
        break;
    case 31:
        action_data->action_u.rfc_p3_rfc_action_p3.id31 = rule_id;
        break;
    case 32:
        action_data->action_u.rfc_p3_rfc_action_p3.id32 = rule_id;
        break;
    case 33:
        action_data->action_u.rfc_p3_rfc_action_p3.id33 = rule_id;
        break;
    case 34:
        action_data->action_u.rfc_p3_rfc_action_p3.id34 = rule_id;
        break;
    case 35:
        action_data->action_u.rfc_p3_rfc_action_p3.id35 = rule_id;
        break;
    case 36:
        action_data->action_u.rfc_p3_rfc_action_p3.id36 = rule_id;
        break;
    case 37:
        action_data->action_u.rfc_p3_rfc_action_p3.id37 = rule_id;
        break;
    case 38:
        action_data->action_u.rfc_p3_rfc_action_p3.id38 = rule_id;
        break;
    case 39:
        action_data->action_u.rfc_p3_rfc_action_p3.id39 = rule_id;
        break;
    case 40:
        action_data->action_u.rfc_p3_rfc_action_p3.id40 = rule_id;
        break;
    case 41:
        action_data->action_u.rfc_p3_rfc_action_p3.id41 = rule_id;
        break;
    case 42:
        action_data->action_u.rfc_p3_rfc_action_p3.id42 = rule_id;
        break;
    case 43:
        action_data->action_u.rfc_p3_rfc_action_p3.id43 = rule_id;
        break;
    case 44:
        action_data->action_u.rfc_p3_rfc_action_p3.id44 = rule_id;
        break;
    case 45:
        action_data->action_u.rfc_p3_rfc_action_p3.id45 = rule_id;
        break;
    case 46:
        action_data->action_u.rfc_p3_rfc_action_p3.id46 = rule_id;
        break;
    case 47:
        action_data->action_u.rfc_p3_rfc_action_p3.id47 = rule_id;
        break;
    case 48:
        action_data->action_u.rfc_p3_rfc_action_p3.id48 = rule_id;
        break;
    case 49:
        action_data->action_u.rfc_p3_rfc_action_p3.id49 = rule_id;
        break;
    case 50:
        action_data->action_u.rfc_p3_rfc_action_p3.id50 = rule_id;
        break;
    default:
        break;
    }

    return SDK_RET_OK;
}

/**
 * @brief    write the current contents of RFC P2 action data buffer to memory
 * @param[in] addr        address to write the action data to
 * @param[in] actiondata action data buffer
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_p3_action_data_flush (mem_addr_t addr, void *actiondata)
{
    sdk_ret_t               ret;
    rfc_p3_actiondata_t    *action_data;

    PDS_TRACE_VERBOSE("Flushing action data to 0x%lx", addr);
    action_data = (rfc_p3_actiondata_t *)actiondata;
    ret = impl_base::pipeline_impl()->write_to_txdma_table(addr,
              P4_P4PLUS_TXDMA_TBL_ID_RFC_P3,
              RFC_P3_RFC_ACTION_P3_ID, action_data);
    // reset the action data after flushing it
    memset(action_data, 0, sizeof(*action_data));
    return ret;
}

/**
 * @brief    compute the cache line address where the given class block
 *           starts from and the entry index where the 1st entry should
 *           be written to
 * @param[in] base_address            RFC phase 3 table's base address
 * @param[in] table_idx               index into the P3 table
 * @param[out] next_cl_addr           next cache line address computed
 * @param[out] next_entry_num         next entry number computed
 */
static inline void
rfc_compute_p3_next_cl_addr_entry_num (mem_addr_t base_address,
                                       uint32_t table_idx,
                                       mem_addr_t *next_cl_addr,
                                       uint16_t *next_entry_num)
{
    uint32_t cache_line_idx = table_idx / SACL_P3_ENTRIES_PER_CACHE_LINE;
    *next_cl_addr = base_address + (cache_line_idx << CACHE_LINE_SIZE_SHIFT);
    *next_entry_num = table_idx % SACL_P3_ENTRIES_PER_CACHE_LINE;
}

#define fw_act    action_data.fw_action.action
/**
 * @brief    given two equivalence class tables, compute the new equivalence
 *           class table or the result table by doing cross product of class
 *           bitmaps of the two tables
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @param[in] rfc_table   result RFC table
 * @param[in] cbm         class bitmap from which is result is computed from
 * @param[in] cbm_size    class bitmap size
 * @return    rule idx for P3 table corresponding to the class bitmap provided
 */
uint16_t
rfc_compute_p3_result (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table,
                       rte_bitmap *cbm, uint32_t cbm_size, void *ctxt)
{
    int         rv;
    uint16_t    priority = SACL_PRIORITY_INVALID;
    uint16_t    final_rule_id = SACL_RULE_ID_DEFAULT;
    uint32_t    ruleidx, posn, start_posn = 0, new_posn = 0;
    uint64_t    slab = 0;

    // TODO: remove
    std::stringstream    a1ss, a2ss;
    rte_bitmap2str(cbm, a1ss, a2ss);
    PDS_TRACE_VERBOSE("a1ss %s\nbitmap %s",
                      a1ss.str().c_str(), a2ss.str().c_str());

    // remember the start position of the scan
    rv = rte_bitmap_scan(cbm, &start_posn, &slab);
    if (rv == 0) {
        // no bit is set in the bitmap
        PDS_TRACE_VERBOSE("No bits set in bitmap, setting rule id %u",
                final_rule_id);
    } else {
        do {
            posn = RTE_BITMAP_START_SLAB_SCAN_POS;
            while (rte_bitmap_slab_scan(slab, posn, &new_posn) != 0) {
                ruleidx = rte_bitmap_get_global_bit_pos(cbm->index2-1, new_posn);
                if (priority > rfc_ctxt->policy->rules[ruleidx].attrs.priority) {
                    priority = rfc_ctxt->policy->rules[ruleidx].attrs.priority;
                    final_rule_id = ruleidx;
                    PDS_TRACE_VERBOSE("Picked high priority rule %u", ruleidx);
                } else {
                    PDS_TRACE_VERBOSE("rule %u priority %u < current %u"
                        ", skipping", ruleidx,
                        rfc_ctxt->policy->rules[ruleidx].attrs.priority,
                        priority);
                }
                posn = new_posn;
            }
            rte_bitmap_scan(cbm, &posn, &slab);
        } while (posn != start_posn);
    }
    return final_rule_id;
}

/**
 * @brief    given two equivalence class tables, compute the new equivalence
 *           class table or the result table by doing cross product of class
 *           bitmaps of the two tables
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @param[in] rfc_table1                RFC equivalance class table 1
 * @param[in] rfc_table2                RFC equivalence class table 2
 * @param[in] result_table              pointer to the result table
 * @param[in] result_table_base_addr    base address of the result table
 * @param[in] action_data               pointer to action data where entries are
 *                                      computed and packed
 * @param[in] entries_per_cl            number of entries that can fit in each
 *                                      cache line
 * @param[in] cl_addr_entry_cb          callback function that computes next
 *                                      cacheline address and entry number in
 *                                      the cache line to write to
 * @param[in] compute_entry_val_cb      callback function to compute the entry
 *                                      value
 * @param[in] entry_pack_cb             callback function to pack each entry
 *                                      value with in the action data at the
 *                                      given entry number
 * @param[in] action_data_flush_cb      callback function to flush the action
 *                                      data contents
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_compute_eq_class_tables (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table1,
                             rfc_table_t *rfc_table2, rfc_table_t *result_table,
                             mem_addr_t result_table_base_addr,
                             void *action_data, uint32_t entries_per_cl,
                             rfc_compute_cl_addr_entry_cb_t cl_addr_entry_cb,
                             rfc_compute_entry_val_cb_t compute_entry_val_cb,
                             rfc_table_entry_pack_cb_t entry_pack_cb,
                             rfc_action_data_flush_cb_t action_data_flush_cb)
{
    uint16_t      entry_val, entry_num = 0, next_entry_num;
    uint32_t      table_idx;
    mem_addr_t    cl_addr = 0, next_cl_addr;

    // do cross product of bitmaps, compute the entries (e.g., class ids or
    // final table result etc.), pack them into appropriate cache lines and
    // flush them
    for (uint32_t i = 0; i < rfc_table1->num_classes; i++) {
        for (uint32_t j = 0; j < rfc_table2->num_classes; j++) {
            table_idx = (rfc_table1->cbm_table[i].class_id *
                         rfc_table2->max_classes) +
                         rfc_table2->cbm_table[j].class_id;
            cl_addr_entry_cb(result_table_base_addr, table_idx,
                             &next_cl_addr, &next_entry_num);

            if (cl_addr != next_cl_addr) {
                // New cache line
                if (cl_addr) {
                    // flush the current partially filled cache line
                    action_data_flush_cb(cl_addr, action_data);
                }

                cl_addr  = next_cl_addr;
                entry_num = next_entry_num;
            } else {
                // cache line didn't change, so no need to flush the cache line
                // but we need to set the entry_num to right value
                entry_num = next_entry_num;
            }

            rte_bitmap_reset(rfc_ctxt->cbm);
            rte_bitmap_and(rfc_table1->cbm_table[i].cbm,
                           rfc_table2->cbm_table[j].cbm,
                           rfc_ctxt->cbm);
            entry_val = compute_entry_val_cb(rfc_ctxt, result_table,
                                             rfc_ctxt->cbm,
                                             rfc_ctxt->cbm_size, NULL);
            entry_pack_cb(table_idx, action_data, entry_num, entry_val);
        }
    }
    if (cl_addr) {
        // flush the partially filled last cache line as well
        action_data_flush_cb(cl_addr, action_data);
    }
    return SDK_RET_OK;
}

/**
 * @brief    given the class bitmap tables of phase0, compute class
 *           bitmap tables of RFC phase 1
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @param[in] table1      Phase0 bitmap table 1
 * @param[in] table2      Phase0 bitmap table 2
 * @param[in] addr_offset Offset of the RFC table wrt base addr
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_compute_p1_tables (rfc_ctxt_t *rfc_ctxt,
                       rfc_table_t *table1,
                       rfc_table_t *table2,
                       uint64_t addr_offset)
{
    rfc_p1_actiondata_t action_data = { 0 };

    rfc_ctxt->p1_table.num_classes = 0;
    rfc_compute_eq_class_tables(rfc_ctxt,
                                table1,
                                table2,
                                &rfc_ctxt->p1_table,
                                rfc_ctxt->base_addr + addr_offset,
                                &action_data, SACL_P1_ENTRIES_PER_CACHE_LINE,
                                rfc_compute_p1_next_cl_addr_entry_num,
                                rfc_compute_class_id_cb,
                                rfc_p1_table_entry_pack,
                                rfc_p1_action_data_flush);
    return SDK_RET_OK;
}

/**
 * @brief    given the class bitmap tables of phase0, compute class
 *           bitmap table(s) of RFC phase 2, and set the results bits
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @param[in] table3      Phase0 bitmap table 3
 * @param[in] table4      Phase0 bitmap table 4
 * @param[in] addr_offset Offset of the RFC table wrt base addr
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_compute_p2_tables (rfc_ctxt_t *rfc_ctxt,
                       rfc_table_t *table3,
                       rfc_table_t *table4,
                       uint64_t addr_offset)
{
    rfc_p2_actiondata_t     action_data = { 0 };

    rfc_ctxt->p2_table.num_classes = 0;
    rfc_compute_eq_class_tables(rfc_ctxt,
                                table3,
                                table4,
                                &rfc_ctxt->p2_table,
                                rfc_ctxt->base_addr + addr_offset,
                                &action_data, SACL_P2_ENTRIES_PER_CACHE_LINE,
                                rfc_compute_p2_next_cl_addr_entry_num,
                                rfc_compute_class_id_cb,
                                rfc_p2_table_entry_pack,
                                rfc_p2_action_data_flush);
    return SDK_RET_OK;
}

/**
 * @brief    given the class bitmap tables of phase1 and Phase2 compute class
 *           bitmap table(s) of RFC phase 3, and set the results bits
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @param[in] addr_offset Offset of the RFC table wrt base addr
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
rfc_compute_p3_tables (rfc_ctxt_t *rfc_ctxt, uint64_t addr_offset)
{
    rfc_p3_actiondata_t     action_data = { 0 };

    rfc_compute_eq_class_tables(rfc_ctxt,
                                &rfc_ctxt->p1_table,
                                &rfc_ctxt->p2_table,
                                NULL,
                                rfc_ctxt->base_addr + addr_offset,
                                &action_data, SACL_P3_ENTRIES_PER_CACHE_LINE,
                                rfc_compute_p3_next_cl_addr_entry_num,
                                rfc_compute_p3_result, rfc_p3_table_entry_pack,
                                rfc_p3_action_data_flush);
    return SDK_RET_OK;
}

/**
 * @brief    given the class bitmap tables of phase0 & phase1, compute class
 *           bitmap table(s) of RFC phase 2, and set the results bits
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
rfc_build_eqtables (rfc_ctxt_t *rfc_ctxt)
{
    PDS_TRACE_DEBUG("Computing P1 table, SIP|STAG:SPORT (%ub:%ub), "
                    "num classes %u:%u", SACL_SIP_STAG_CLASSID_WIDTH,
                    SACL_SPORT_CLASSID_WIDTH,
                    rfc_ctxt->sip_stag_tbl.num_classes,
                    rfc_ctxt->sport_tbl.num_classes);
    rfc_compute_p1_tables(rfc_ctxt,
                          &rfc_ctxt->sip_stag_tbl,
                          &rfc_ctxt->sport_tbl,
                          SACL_P1_TABLE_OFFSET);
    rfc_p1_eq_class_tables_dump(rfc_ctxt);
    PDS_TRACE_DEBUG("Computing P2 table, DIP|DTAG:PROTO_DPORT (%ub:%ub), "
                    "num classes %u:%u", SACL_DIP_DTAG_CLASSID_WIDTH,
                    SACL_PROTO_DPORT_CLASSID_WIDTH,
                    rfc_ctxt->dip_dtag_tbl.num_classes,
                    rfc_ctxt->proto_dport_tbl.num_classes);
    rfc_compute_p2_tables(rfc_ctxt,
                          &rfc_ctxt->dip_dtag_tbl,
                          &rfc_ctxt->proto_dport_tbl,
                          SACL_P2_TABLE_OFFSET);
    rfc_p2_eq_class_tables_dump(rfc_ctxt);
    PDS_TRACE_DEBUG("Computing P3 table, P1:P2 (%ub:%ub), num classes %u:%u",
                    SACL_P1_CLASSID_WIDTH, SACL_P2_CLASSID_WIDTH,
                    rfc_ctxt->p1_table.num_classes,
                    rfc_ctxt->p2_table.num_classes);
    rfc_compute_p3_tables(rfc_ctxt, SACL_P3_TABLE_OFFSET);
    rfc_table_destroy(&rfc_ctxt->p1_table);
    rfc_table_destroy(&rfc_ctxt->p2_table);

    return SDK_RET_OK;
}

/**
 * @brief    given the entry num, fill corresponding portion of the sacl result
 *           table entry action data.
 * @param[in] table_idx      index of the entry (for debugging)
 * @param[in] action_data    pointer to the action data
 * @param[in] entry_num      entry idx (0 to 31, inclusive), we can fit 32
 *                           entries in a single cache line
 * @param[in] alg_type       algo type
 * @param[in] stateful       stateful
 * @param[in] priority       priority
 * @param[in] action         sacl action
 *
 * @return    SDK_RET_OK on success, failure status code on error
 */

sdk_ret_t
sacl_result_table_entry_pack (uint32_t table_idx, uint32_t entry_num,
                              sacl_result_actiondata_t *entry,
                              uint8_t alg_type, uint8_t stateful,
                              uint16_t priority, uint8_t action)
{
    PDS_TRACE_VERBOSE("RFC result table[0x%x] <- alg %u, priority %u, "
                      "action %u", table_idx, alg_type, priority, action);
    switch (entry_num) {
    case 0:
        entry->action_u.sacl_result_sacl_action_result.alg_type00 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful00 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority00 = priority;
        entry->action_u.sacl_result_sacl_action_result.action00 = action;
        break;
    case 1:
        entry->action_u.sacl_result_sacl_action_result.alg_type01 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful01 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority01 = priority;
        entry->action_u.sacl_result_sacl_action_result.action01 = action;
        break;
    case 2:
        entry->action_u.sacl_result_sacl_action_result.alg_type02 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful02 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority02 = priority;
        entry->action_u.sacl_result_sacl_action_result.action02 = action;
        break;
    case 3:
        entry->action_u.sacl_result_sacl_action_result.alg_type03 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful03 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority03 = priority;
        entry->action_u.sacl_result_sacl_action_result.action03 = action;
        break;
    case 4:
        entry->action_u.sacl_result_sacl_action_result.alg_type04 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful04 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority04 = priority;
        entry->action_u.sacl_result_sacl_action_result.action04 = action;
        break;
    case 5:
        entry->action_u.sacl_result_sacl_action_result.alg_type05 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful05 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority05 = priority;
        entry->action_u.sacl_result_sacl_action_result.action05 = action;
        break;
    case 6:
        entry->action_u.sacl_result_sacl_action_result.alg_type06 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful06 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority06 = priority;
        entry->action_u.sacl_result_sacl_action_result.action06 = action;
        break;
    case 7:
        entry->action_u.sacl_result_sacl_action_result.alg_type07 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful07 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority07 = priority;
        entry->action_u.sacl_result_sacl_action_result.action07 = action;
        break;
    case 8:
        entry->action_u.sacl_result_sacl_action_result.alg_type08 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful08 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority08 = priority;
        entry->action_u.sacl_result_sacl_action_result.action08 = action;
        break;
    case 9:
        entry->action_u.sacl_result_sacl_action_result.alg_type09 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful09 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority09 = priority;
        entry->action_u.sacl_result_sacl_action_result.action09 = action;
        break;
    case 10:
        entry->action_u.sacl_result_sacl_action_result.alg_type10 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful10 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority10 = priority;
        entry->action_u.sacl_result_sacl_action_result.action10 = action;
        break;
    case 11:
        entry->action_u.sacl_result_sacl_action_result.alg_type11 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful11 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority11 = priority;
        entry->action_u.sacl_result_sacl_action_result.action11 = action;
        break;
    case 12:
        entry->action_u.sacl_result_sacl_action_result.alg_type12 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful12 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority12 = priority;
        entry->action_u.sacl_result_sacl_action_result.action12 = action;
        break;
    case 13:
        entry->action_u.sacl_result_sacl_action_result.alg_type13 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful13 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority13 = priority;
        entry->action_u.sacl_result_sacl_action_result.action13 = action;
        break;
    case 14:
        entry->action_u.sacl_result_sacl_action_result.alg_type14 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful14 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority14 = priority;
        entry->action_u.sacl_result_sacl_action_result.action14 = action;
        break;
    case 15:
        entry->action_u.sacl_result_sacl_action_result.alg_type15 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful15 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority15 = priority;
        entry->action_u.sacl_result_sacl_action_result.action15 = action;
        break;
    case 16:
        entry->action_u.sacl_result_sacl_action_result.alg_type16 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful16 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority16 = priority;
        entry->action_u.sacl_result_sacl_action_result.action16 = action;
        break;
    case 17:
        entry->action_u.sacl_result_sacl_action_result.alg_type17 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful17 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority17 = priority;
        entry->action_u.sacl_result_sacl_action_result.action17 = action;
        break;
    case 18:
        entry->action_u.sacl_result_sacl_action_result.alg_type18 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful18 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority18 = priority;
        entry->action_u.sacl_result_sacl_action_result.action18 = action;
        break;
    case 19:
        entry->action_u.sacl_result_sacl_action_result.alg_type19 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful19 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority19 = priority;
        entry->action_u.sacl_result_sacl_action_result.action19 = action;
        break;
    case 20:
        entry->action_u.sacl_result_sacl_action_result.alg_type20 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful20 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority20 = priority;
        entry->action_u.sacl_result_sacl_action_result.action20 = action;
        break;
    case 21:
        entry->action_u.sacl_result_sacl_action_result.alg_type21 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful21 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority21 = priority;
        entry->action_u.sacl_result_sacl_action_result.action21 = action;
        break;
    case 22:
        entry->action_u.sacl_result_sacl_action_result.alg_type22 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful22 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority22 = priority;
        entry->action_u.sacl_result_sacl_action_result.action22 = action;
        break;
    case 23:
        entry->action_u.sacl_result_sacl_action_result.alg_type23 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful23 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority23 = priority;
        entry->action_u.sacl_result_sacl_action_result.action23 = action;
        break;
    case 24:
        entry->action_u.sacl_result_sacl_action_result.alg_type24 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful24 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority24 = priority;
        entry->action_u.sacl_result_sacl_action_result.action24 = action;
        break;
    case 25:
        entry->action_u.sacl_result_sacl_action_result.alg_type25 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful25 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority25 = priority;
        entry->action_u.sacl_result_sacl_action_result.action25 = action;
        break;
    case 26:
        entry->action_u.sacl_result_sacl_action_result.alg_type26 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful26 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority26 = priority;
        entry->action_u.sacl_result_sacl_action_result.action26 = action;
        break;
    case 27:
        entry->action_u.sacl_result_sacl_action_result.alg_type27 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful27 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority27 = priority;
        entry->action_u.sacl_result_sacl_action_result.action27 = action;
        break;
    case 28:
        entry->action_u.sacl_result_sacl_action_result.alg_type28 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful28 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority28 = priority;
        entry->action_u.sacl_result_sacl_action_result.action28 = action;
        break;
    case 29:
        entry->action_u.sacl_result_sacl_action_result.alg_type29 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful29 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority29 = priority;
        entry->action_u.sacl_result_sacl_action_result.action29 = action;
        break;
    case 30:
        entry->action_u.sacl_result_sacl_action_result.alg_type30 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful30 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority30 = priority;
        entry->action_u.sacl_result_sacl_action_result.action30 = action;
        break;
    case 31:
        entry->action_u.sacl_result_sacl_action_result.alg_type31 = alg_type;
        entry->action_u.sacl_result_sacl_action_result.stateful31 = stateful;
        entry->action_u.sacl_result_sacl_action_result.priority31 = priority;
        entry->action_u.sacl_result_sacl_action_result.action31 = action;
        break;
    default:
    PDS_TRACE_ERR("Invalid entry number %u while packing RFC result table",
                  entry_num);
        break;
    }

    return SDK_RET_OK;
}

/**
 * @brief    write the current contents of sacl_result action data buffer to
 *           memory
 * @param[in] addr        address to write the action data to
 * @param[in] actiondata    action data buffer
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
sacl_result_action_data_flush (mem_addr_t addr, void *actiondata)
{
    sdk_ret_t                   ret;
    sacl_result_actiondata_t    *action_data;

    PDS_TRACE_DEBUG("Flushing action data to 0x%lx", addr);
    action_data = (sacl_result_actiondata_t *)actiondata;
    ret = impl_base::pipeline_impl()->write_to_txdma_table(addr,
                                                           P4_P4PLUS_TXDMA_TBL_ID_SACL_RESULT,
                                                           SACL_RESULT_SACL_ACTION_RESULT_ID,
                                                           action_data);
    // reset the action data after flushing it
    memset(action_data, 0, sizeof(*action_data));
    return ret;
}

/**
 * @brief     program the sacl_result table
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @return    SDK_RET_OK on success, failure status code on error
 */

sdk_ret_t
program_sacl_result (rfc_ctxt_t *rfc_ctxt)
{
    uint32_t action;
    bool flush = false;
    sacl_result_actiondata_t data = { 0 };
    mem_addr_t addr = rfc_ctxt->base_addr + SACL_RSLT_TABLE_OFFSET;

    // Iterate through the rules
    for (uint32_t rule_num = 0;
         rule_num < SACL_NUM_RULES;
         rule_num += SACL_RSLT_ENTRIES_PER_CACHE_LINE) {
        for (uint32_t entry_num = 0;
             entry_num < SACL_RSLT_ENTRIES_PER_CACHE_LINE;
             entry_num++) {
            uint32_t rule_idx = rule_num + entry_num;
            if ((rule_idx < rfc_ctxt->policy->num_rules) && (rule_idx !=
                    SACL_RULE_ID_DEFAULT)) {
                // Get the result fields from the rule
                rule_t *rule = &rfc_ctxt->policy->rules[rule_idx];
                action = (rule->attrs.action_data.fw_action.action ==
                                SECURITY_RULE_ACTION_ALLOW) ?
                                SACL_RSLT_ENTRY_ACTION_ALLOW :
                                SACL_RSLT_ENTRY_ACTION_DENY;
                sacl_result_table_entry_pack(rule_idx, entry_num, &data,
                                             rule->attrs.alg_type,
                                             rule->attrs.stateful,
                                             rule->attrs.priority,
                                             action);
                flush = true;
            } else if (rule_idx == SACL_RULE_ID_DEFAULT) {
                // Set the default entry
                action = (rfc_ctxt->policy->default_action.fw_action.action ==
                                SECURITY_RULE_ACTION_ALLOW) ?
                                SACL_RSLT_ENTRY_ACTION_ALLOW :
                                SACL_RSLT_ENTRY_ACTION_DENY;
                sacl_result_table_entry_pack(rule_idx, entry_num, &data, 0, 0,
                                             SACL_PRIORITY_LOWEST, action);
                flush = true;
            }
        }

        if (flush) {
            sacl_result_action_data_flush(addr, (void*)&data);
            flush = false;
        }

        addr += SACL_CACHE_LINE_SIZE;
    }

    return SDK_RET_OK;
}

}    // namespace rfc
