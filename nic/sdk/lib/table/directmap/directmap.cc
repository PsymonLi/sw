//------------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#include <cstring>
#include "directmap.hpp"
#include "lib/p4pd/p4pd_api.hpp"
#include "include/sdk/mem.hpp"

using sdk::table::directmap_entry_t;

namespace sdk {
namespace table {

//----------------------------------------------------------------------------
// directmap instance initialization
//----------------------------------------------------------------------------
bool
directmap::init(char *name, uint32_t id, uint32_t capacity,
                uint32_t swdata_len, bool sharing_en, bool entry_trace_en,
                table_health_monitor_func_t health_monitor_func)
{
    uint32_t hwkey_len = 0, hwkeymask_len = 0;

    strncpy(name_, name, SDK_MAX_NAME_LEN);
    id_                  = id;
    capacity_            = capacity;
    swdata_len_          = swdata_len;
    sharing_en_          = sharing_en;
    entry_trace_en_      = entry_trace_en;
    health_monitor_func_ = health_monitor_func;
    memset(stats_, 0, sizeof(stats_));
    indexer_ = indexer::factory(capacity, false, false);

    // initialize hash table if sharing is enabled
    if (sharing_en_) {
        entry_ht_ = ht::factory(capacity,
                                dm_entry_get_key_func,
                                dm_entry_compute_hash_func,
                                dm_entry_compare_key_func);
    }

    hwdata_len_ = 0;
    p4pd_global_hwentry_query(id_, &hwkey_len, &hwkeymask_len, &hwdata_len_);
    // rounding off to nearest byte
    hwdata_len_ = (hwdata_len_ >> 3) + ((hwdata_len_ & 0x7) ? 1 : 0);

    return true;
}

//----------------------------------------------------------------------------
// factory method to instantiate the class
//----------------------------------------------------------------------------
directmap *
directmap::factory(char *name, uint32_t id, uint32_t capacity,
                   uint32_t swdata_len, bool sharing_en, bool entry_trace_en,
                    table_health_monitor_func_t health_monitor_func)
{
    void      *mem = NULL;
    directmap *dm  = NULL;

    mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_DIRECTMAP, sizeof(directmap));
    if (!mem) {
        return NULL;
    }

    dm = new (mem) directmap();
    if (dm->init(name, id, capacity, swdata_len, sharing_en,
                 entry_trace_en, health_monitor_func) == false) {
        SDK_TRACE_ERR("Failed to initialize directmap %s, idd %u",
                      name, id);
        dm->~directmap();
        SDK_FREE(SDK_MEM_ALLOC_LIB_DIRECTMAP, mem);
        return NULL;
    }

#if 0
    SDK_TRACE_DEBUG("%-30s: tableid: %-4d swdata_len: %-4d "
                    "hwdata_len_: %-4d sharing_en:%d", dm->name_, dm->id_,
                    dm->swdata_len_, dm->hwdata_len_,
                    dm->sharing_en_);
#endif

    return dm;
}

//----------------------------------------------------------------------------
// method to free & delete the object
//----------------------------------------------------------------------------
void
directmap::destroy(directmap *dm)
{
    if (dm) {
        indexer::destroy(dm->indexer_);
        dm->~directmap();
        SDK_FREE(SDK_MEM_ALLOC_LIB_DIRECTMAP, dm);
    }
}

void
directmap::trigger_health_monitor(void)
{
    if (health_monitor_func_) {
        health_monitor_func_(id_, name_, health_state_, capacity_,
                             num_entries_in_use(), &health_state_);
    }
}

//-----------------------------------------------------------------------------
// get key function for directmap entry
//-----------------------------------------------------------------------------
void *
directmap::dm_entry_get_key_func(void *entry)
{
    return entry;
}

//-----------------------------------------------------------------------------
// compute hash function
//-----------------------------------------------------------------------------
uint32_t
directmap::dm_entry_compute_hash_func(void *key, uint32_t ht_size)
{
    directmap_entry_t           *ht_entry;
    void                        *ht_key;

    SDK_ASSERT(key != NULL);
    ht_entry = (directmap_entry_t *)key;

    ht_key = ht_entry->data;
    SDK_ASSERT(ht_key != NULL);

    return sdk::lib::hash_algo::fnv_hash(ht_key, ht_entry->len) % ht_size;
}

//-----------------------------------------------------------------------------
// compare function
//-----------------------------------------------------------------------------
bool
directmap::dm_entry_compare_key_func(void *key1, void *key2)
{
    directmap_entry_t           *ht_entry1, *ht_entry2;
    void                        *ht_key1, *ht_key2;

    SDK_ASSERT((key1 != NULL) && (key2 != NULL));
    ht_entry1 = (directmap_entry_t *)key1;
    ht_entry2 = (directmap_entry_t *)key2;

    ht_key1 = ht_entry1->data;
    ht_key2 = ht_entry2->data;

    if (!memcmp(ht_key1, ht_key2, ht_entry1->len)) {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// insert entry in HW
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::insert(void *data, uint32_t *index, void *data_mask)
{
    sdk_ret_t           rs     = SDK_RET_OK;
    p4pd_error_t        pd_err = P4PD_SUCCESS;
    directmap_entry_t   dme    = { 0 }, *dme_elem = NULL;

    // if sharing is enabled, check if entry already exists
    if (sharing_en_) {
        dme.data = data;
        dme.len = swdata_len_;
        dme_elem = find_directmap_entry(&dme);
        if (dme_elem) {
            // increment ref count
            dme_elem->ref_cnt++;
            *index = dme_elem->index;
            SDK_TRACE_DEBUG("sharing enabled, reusing index : %u ref cnt: %d\n",
                             dme_elem->index, dme_elem->ref_cnt);
            goto end;
        }
    }

    rs = alloc_index_(index);
    if (rs != SDK_RET_OK) {
        goto end;
    }

    // print entry
    if (entry_trace_en_) {
        entry_trace_(data, *index);
    }

    // program P4
    pd_err = p4pd_global_entry_write_with_datamask(id_, *index,
                                                   NULL, NULL,
                                                   data, data_mask);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

    if (sharing_en_) {
        // allocate entry to insert into hash table
        dme_elem = directmap_entry_alloc_init();
        dme_elem->data = (void *)SDK_MALLOC(SDK_MEM_ALLOC_LIB_DIRECT_MAP_DATA,
                                            swdata_len_);
        memcpy(dme_elem->data, data, swdata_len_);
        dme_elem->len = swdata_len_;
        dme_elem->index = *index;
        dme_elem->ref_cnt = 1;

        SDK_TRACE_DEBUG("allocated index : %u ref_cnt : %d",
                        dme_elem->index, dme_elem->ref_cnt);

        // insert into hash table
        rs = add_directmap_entry_to_db(dme_elem);
        SDK_ASSERT(rs == SDK_RET_OK);
    }

end:

    stats_update(INSERT, rs);
    trigger_health_monitor();
    return rs;
}

//-----------------------------------------------------------------------------
// insert entry in HW at index
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::insert_withid(void *data, uint32_t index, void* data_mask)
{
    sdk_ret_t rs = SDK_RET_OK;
    p4pd_error_t pd_err = P4PD_SUCCESS;
    directmap_entry_t   dme = { 0 }, *dme_elem = NULL;

    if (sharing_en_) {
        dme.data = data;
        dme.len = swdata_len_;
        dme_elem = find_directmap_entry(&dme);
        if (dme_elem) {
            // increment ref count
            dme_elem->ref_cnt++;

            SDK_TRACE_DEBUG("sharing enabled, reusing index : %u ref_cnt : %d\n",
                            dme_elem->index, dme_elem->ref_cnt);
            // You cant do insert_withid() for already existing entry at a
            // different index
            SDK_ASSERT(index == dme_elem->index);
            goto end;
        }
    }

    rs = alloc_index_withid_(index);
    if (rs != SDK_RET_OK) {
        goto end;
    }

    // print entry
    if (entry_trace_en_) {
        entry_trace_(data, index);
    }

    // program P4
    pd_err = p4pd_global_entry_write_with_datamask(id_, index, NULL, NULL,
                                                   data, data_mask);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

    if (sharing_en_) {
        dme_elem = directmap_entry_alloc_init();
        dme_elem->data = (void *)SDK_MALLOC(SDK_MEM_ALLOC_LIB_DIRECT_MAP_DATA,
                                            swdata_len_);
        memcpy(dme_elem->data, data, swdata_len_);
        dme_elem->len = swdata_len_;
        dme_elem->index = index;
        dme_elem->ref_cnt = 1;

        SDK_TRACE_DEBUG("sharing enabled, index : %d ref_cnt : %d\n",
                        dme_elem->index, dme_elem->ref_cnt);

        // insert into hash table
        rs = add_directmap_entry_to_db(dme_elem);
        SDK_ASSERT(rs == SDK_RET_OK);
    }

end:

    stats_update(INSERT_WITHID, rs);
    trigger_health_monitor();
    return rs;
}

//-----------------------------------------------------------------------------
// update entry in HW
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::update(uint32_t index, void *data, void *data_mask)
{
    sdk_ret_t           rs     = SDK_RET_OK;
    p4pd_error_t        pd_err = P4PD_SUCCESS;
    directmap_entry_t   dme    = { 0 }, *dme_elem = NULL;

    if (index > capacity_) {
        rs = SDK_RET_INVALID_ARG;
        goto end;
    }
    if (!indexer_->is_index_allocated(index)) {
        rs = SDK_RET_ENTRY_NOT_FOUND;
        goto end;
    }

    if (sharing_en_) {

        // Bharat: Not sure what it means. If you update to already existing
        //         data, you may have two entries in hash table with same
        //         data. So not supporting update for now.
        SDK_ASSERT(0);

        // Update HW
        pd_err = p4pd_global_entry_write_with_datamask(id_, index,
                                                       NULL, NULL,
                                                       data, data_mask);
        if (pd_err != P4PD_SUCCESS) {
            rs = SDK_RET_HW_PROGRAM_ERR;
            SDK_ASSERT(0);
        }

        // Update SW
        dme.data = data;
        dme.len = swdata_len_;
        dme_elem = find_directmap_entry(&dme);
        SDK_ASSERT(dme_elem != NULL);
        memcpy(dme_elem->data, data, swdata_len_);
    }

    // print entry
    if (entry_trace_en_) {
        entry_trace_(data, index);
    }

    // update p4 table entry
    pd_err = p4pd_global_entry_write_with_datamask(id_, index, NULL, NULL,
                                                   data, data_mask);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

end:

    stats_update(UPDATE, rs);
    return rs;
}

//-----------------------------------------------------------------------------
// remove entry from HW
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::remove(uint32_t index, void *data)
{
    sdk_ret_t           rs        = SDK_RET_OK;
    p4pd_error_t        pd_err    = P4PD_SUCCESS;
    void                *tmp_data = NULL;
    directmap_entry_t   dme       = { 0 }, *dme_elem = NULL;

    if (index > capacity_) {
        rs = SDK_RET_INVALID_ARG;
        goto end;
    }

    if (!indexer_->is_index_allocated(index)) {
        rs = SDK_RET_ENTRY_NOT_FOUND;
        goto end;
    }

    if (sharing_en_) {
        // if sharing is enabled, remove is supported only with data
        SDK_ASSERT(data != NULL);

        // find SW entry
        dme.data = data;
        dme.len  = swdata_len_;
        dme_elem = find_directmap_entry(&dme);
        SDK_ASSERT(dme_elem != NULL);
        index = dme_elem->index;

        // decrement ref count
        dme_elem->ref_cnt--;
        SDK_TRACE_DEBUG("index : %u ref_cnt : %d", index, dme_elem->ref_cnt);
        if (dme_elem->ref_cnt != 0) {
            goto end;
        }
    }

    // update P4 table by zeroing out
    tmp_data = SDK_CALLOC(SDK_MEM_ALLOC_LIB_DIRECT_MAP_SW_DATA, swdata_len_);
    pd_err = p4pd_global_entry_write(id_, index, NULL, NULL, tmp_data);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

    rs = free_index_(index);
    if (rs != SDK_RET_OK) {
       goto end;
    }

    if (sharing_en_) {
        // remove from hash table
        dme_elem = (directmap_entry_t *)del_directmap_entry_from_db(dme_elem);

        // free data
        SDK_FREE(SDK_MEM_ALLOC_LIB_DIRECT_MAP_DATA, dme_elem->data);

        // free DM entry
        rs = directmap_entry_free(dme_elem);
        SDK_ASSERT(rs == SDK_RET_OK);
    }

end:

    if (tmp_data) {
        SDK_FREE(SDK_MEM_ALLOC_LIB_DIRECT_MAP_SW_DATA, tmp_data);
    }
    stats_update(REMOVE, rs);
    trigger_health_monitor();
    return rs;
}

//-----------------------------------------------------------------------------
// retrieve entry from HW
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::retrieve(uint32_t index, void *data)
{
    sdk_ret_t       rs = SDK_RET_OK;
    p4pd_error_t    pd_err = P4PD_SUCCESS;

    if (index > capacity_) {
        rs = SDK_RET_INVALID_ARG;
        goto end;
    }

    if (!indexer_->is_index_allocated(index)) {
        rs = SDK_RET_ENTRY_NOT_FOUND;
        goto end;
    }

    // read the entry
    pd_err = p4pd_global_entry_read(id_, index, NULL, NULL, data);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

end:

    stats_update(RETRIEVE, rs);
    return rs;
}

//-----------------------------------------------------------------------------
// iterate every entry and gives a call back
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::iterate(direct_map_iterate_func_t iterate_func, const void *cb_data)
{
    p4pd_error_t    pd_err;
    sdk_ret_t       rs = SDK_RET_OK;
    void            *tmp_data;

    tmp_data = SDK_CALLOC(SDK_MEM_ALLOC_LIB_DIRECT_MAP_HW_DATA, swdata_len_);
    for (uint32_t i = 0; i < capacity_; i++) {
        if (indexer_->is_index_allocated(i)) {
            pd_err = p4pd_global_entry_read(id_, i, NULL, NULL, tmp_data);
            SDK_ASSERT_GOTO((pd_err == P4PD_SUCCESS), end);
            iterate_func(i, tmp_data, cb_data);
        }
    }

end:

    SDK_FREE(SDK_MEM_ALLOC_LIB_DIRECT_MAP_HW_DATA, tmp_data);
    if (pd_err != P4PD_SUCCESS) {
        rs = SDK_RET_HW_PROGRAM_ERR;
        SDK_ASSERT(0);
    }

    stats_update(ITERATE, rs);
    return rs;
}

//-----------------------------------------------------------------------------
// return stats pointer
//-----------------------------------------------------------------------------
void
directmap::fetch_stats(const uint64_t **stats)
{
    *stats = stats_;
}

//-----------------------------------------------------------------------------
// allocate an index
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::alloc_index_(uint32_t *idx)
{
    // allocate an index
    indexer::status irs = indexer_->alloc(idx);
    if (irs != indexer::SUCCESS) {
        return SDK_RET_NO_RESOURCE;
    }

    return SDK_RET_OK;
}

//-----------------------------------------------------------------------------
// allocate an index with id
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::alloc_index_withid_(uint32_t idx)
{
    sdk_ret_t   rs = SDK_RET_OK;

    // allocate an index
    indexer::status irs = indexer_->alloc_withid(idx);
    if (irs != indexer::SUCCESS) {
        rs = (irs == indexer::DUPLICATE_ALLOC) ? SDK_RET_DUPLICATE_INS : SDK_RET_OOB;
    }

    return rs;
}

//-----------------------------------------------------------------------------
// free an index
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::free_index_(uint32_t idx)
{
    sdk_ret_t   rs = SDK_RET_OK;

    indexer::status irs = indexer_->free((uint32_t)idx);
    if (irs != indexer::SUCCESS) {
        rs = (irs == indexer::DUPLICATE_FREE) ? SDK_RET_DUPLICATE_FREE : SDK_RET_OOB;
    }

    return rs;
}

//-----------------------------------------------------------------------------
// increment stats
//-----------------------------------------------------------------------------
void
directmap::stats_incr(stats stat)
{
    SDK_ASSERT_RETURN_VOID((stat < STATS_MAX));
    SDK_ATOMIC_INC_UINT32(&stats_[stat], 1);
}

//-----------------------------------------------------------------------------
// decrement stats
//-----------------------------------------------------------------------------
void
directmap::stats_decr(stats stat)
{
    SDK_ASSERT_RETURN_VOID((stat < STATS_MAX));
    SDK_ATOMIC_DEC_UINT32(&stats_[stat], 1);
}

//-----------------------------------------------------------------------------
// update stats
//-----------------------------------------------------------------------------
void
directmap::stats_update(directmap::api ap, sdk_ret_t rs)
{
    switch (ap) {
    case INSERT:
        if (rs == SDK_RET_OK) stats_incr(STATS_INS_SUCCESS);
        else if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_INS_FAIL_HW);
        else if (rs == SDK_RET_NO_RESOURCE) stats_incr(STATS_INS_FAIL_NO_RES);
        else SDK_ASSERT(0);
        break;
    case INSERT_WITHID:
        if (rs == SDK_RET_OK) stats_incr(STATS_INS_WITHID_SUCCESS);
        else if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_INS_WITHID_FAIL_HW);
        else if (rs == SDK_RET_DUPLICATE_INS) stats_incr(STATS_INS_WITHID_FAIL_DUP_INS);
        else if (rs == SDK_RET_OOB) stats_incr(STATS_INS_WITHID_FAIL_OOB);
        else SDK_ASSERT(0);
        break;
    case UPDATE:
        if (rs == SDK_RET_OK) stats_incr(STATS_UPD_SUCCESS);
        else if (rs == SDK_RET_ENTRY_NOT_FOUND)
            stats_incr(STATS_UPD_FAIL_ENTRY_NOT_FOUND);
        else if (rs == SDK_RET_INVALID_ARG) stats_incr(STATS_UPD_FAIL_INV_ARG);
        else if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_UPD_FAIL_HW);
        else SDK_ASSERT(0);
        break;
    case REMOVE:
        if (rs == SDK_RET_OK) stats_incr(STATS_REM_SUCCESS);
        else if (rs == SDK_RET_ENTRY_NOT_FOUND)
            stats_incr(STATS_REM_FAIL_ENTRY_NOT_FOUND);
        else if (rs == SDK_RET_INVALID_ARG) stats_incr(STATS_REM_FAIL_INV_ARG);
        else if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_REM_FAIL_HW);
        else SDK_ASSERT(0);
        break;
    case RETRIEVE:
        if (rs == SDK_RET_OK) stats_incr(STATS_RETR_SUCCESS);
        else if (rs == SDK_RET_ENTRY_NOT_FOUND)
            stats_incr(STATS_RETR_FAIL_ENTRY_NOT_FOUND);
        else if (rs == SDK_RET_INVALID_ARG) stats_incr(STATS_RETR_FAIL_INV_ARG);
        else if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_RETR_FAIL_HW);
        else SDK_ASSERT(0);
        break;
    case ITERATE:
        if (rs == SDK_RET_HW_PROGRAM_ERR) stats_incr(STATS_ITER_FAIL_HW);
        else if (rs == SDK_RET_OK) stats_incr(STATS_ITER_SUCCESS);
        else SDK_ASSERT(0);
        break;
    default:
        SDK_ASSERT(0);
    }
}

//-----------------------------------------------------------------------------
// adds directmap entry to hash table
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::add_directmap_entry_to_db(directmap_entry_t *dme)
{
    return entry_ht_->insert(dme, &dme->ht_ctxt);
}

//-----------------------------------------------------------------------------
// removes directmap entry from hash table
//-----------------------------------------------------------------------------
void *
directmap::del_directmap_entry_from_db(directmap_entry_t *dme)
{
    return entry_ht_->remove(dme);
}

//-----------------------------------------------------------------------------
// find directmap entry in hash table
//-----------------------------------------------------------------------------
directmap_entry_t *
directmap::find_directmap_entry(directmap_entry_t *key)
{
    return (directmap_entry_t *)entry_ht_->lookup(key);
}

//-----------------------------------------------------------------------------
// number of entries in use.
//-----------------------------------------------------------------------------
uint32_t
directmap::num_entries_in_use(void) const
{
    return indexer_->num_indices_allocated();
}

//-----------------------------------------------------------------------------
// number of insert operations attempted
//-----------------------------------------------------------------------------
uint32_t
directmap::num_inserts(void) const
{
    return stats_[STATS_INS_SUCCESS] + stats_[STATS_INS_FAIL_HW] +
               stats_[STATS_INS_FAIL_NO_RES] +
               stats_[STATS_INS_WITHID_SUCCESS] +
               stats_[STATS_INS_WITHID_FAIL_DUP_INS] +
               stats_[STATS_INS_WITHID_FAIL_OOB] +
               stats_[STATS_INS_WITHID_FAIL_HW];
}

//-----------------------------------------------------------------------------
// number of failed insert operations
//-----------------------------------------------------------------------------
uint32_t
directmap::num_insert_errors(void) const
{
    return stats_[STATS_INS_FAIL_HW] + stats_[STATS_INS_FAIL_NO_RES] +
               stats_[STATS_INS_WITHID_FAIL_DUP_INS] +
               stats_[STATS_INS_WITHID_FAIL_OOB] +
               stats_[STATS_INS_WITHID_FAIL_HW];
}

//-----------------------------------------------------------------------------
// number of update operations attempted
//-----------------------------------------------------------------------------
uint32_t
directmap::num_updates(void) const
{
    return stats_[STATS_UPD_SUCCESS] + stats_[STATS_UPD_FAIL_INV_ARG] +
               stats_[STATS_UPD_FAIL_ENTRY_NOT_FOUND] +
               stats_[STATS_UPD_FAIL_HW];
}

//-----------------------------------------------------------------------------
// number of failed update operations
//-----------------------------------------------------------------------------
uint32_t
directmap::num_update_errors(void) const
{
    return stats_[STATS_UPD_FAIL_INV_ARG] +
               stats_[STATS_UPD_FAIL_ENTRY_NOT_FOUND] +
               stats_[STATS_UPD_FAIL_HW];
}

//-----------------------------------------------------------------------------
// number of delete operations attempted
//-----------------------------------------------------------------------------
uint32_t
directmap::num_deletes(void) const
{
    return stats_[STATS_REM_SUCCESS] + stats_[STATS_REM_FAIL_INV_ARG] +
               stats_[STATS_REM_FAIL_ENTRY_NOT_FOUND] +
               stats_[STATS_REM_FAIL_HW];
}

//-----------------------------------------------------------------------------
// number of failed delete operations
//-----------------------------------------------------------------------------
uint32_t
directmap::num_delete_errors(void) const
{
    return stats_[STATS_REM_FAIL_INV_ARG] +
               stats_[STATS_REM_FAIL_ENTRY_NOT_FOUND] +
               stats_[STATS_REM_FAIL_HW];
}

//-----------------------------------------------------------------------------
// returns string of the entry
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::entry_to_str(void *data, uint32_t index, char *buff,
                        uint32_t buff_size)
{
    p4pd_error_t    p4_err;

    p4_err = p4pd_global_table_ds_decoded_string_get(id_, index, NULL, NULL,
                                                     data, buff, buff_size);
    SDK_ASSERT(p4_err == P4PD_SUCCESS);
    SDK_TRACE_DEBUG("%s : Index: %u\n %s", name_, index, buff);
    return SDK_RET_OK;
}

//-----------------------------------------------------------------------------
// prints the decoded entry
//-----------------------------------------------------------------------------
sdk_ret_t
directmap::entry_trace_(void *data, uint32_t index)
{
    char         buff[4096] = {0};
    sdk_ret_t    ret;

    ret = entry_to_str(data, index, buff, sizeof(buff));
    SDK_ASSERT(ret = SDK_RET_OK);
    SDK_TRACE_DEBUG("%s : Index: %d \n %s", name_, index, buff);
    return SDK_RET_OK;
}

}    // namespace table
}    // namespace sdk
