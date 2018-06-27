// ============================================================================
//
// Flow Table Managment
//
//
//  Hash(Flow_key) =
//  +------+---------+
//  |  11  |    21   |
//  +------+---------+
//              |        Flow Hash Table
//              |           +-----+
//              |           |     |
//              |           +-----+     +-----+----+----+----+------+----+
//              |-------->  |     | --> | 320 | 20 | 11 | 14 |  ... | 15 |
//                          +-----+     +-----+----+----+----+------+----+
//                          |     |                        |
//                          +-----+                        | Flow Hash Coll Tbl
//                          |     |                        |      +----+
//                          +-----+                        |      |    |
//                          |     |                        |      +----+
//                          +-----+                        -----> |    |
//                                                                +----+
//                                                                |    |
//                                                                +----+
//
//
//
// < ----------     Spine Entry     ---------->
// +-----+----+----+----+------+----+----+----+
// |  Anchor  |Hint Grp | ...  |Hint Grp | NS |
// +-----+----+----+----+------+----+----+----+
//
// Spine Entry: Entry which either goes into Flow Table or Flow Hash Collision
//              Table and has Hint Groups.
// Anchor: Initial Few bits of spine entry which has the key and data of
//         a Flow Entry.
// Hint Group: Flow entries which has the same 21 bits and 11 bits.
//             6 HGs per spine.
// NS: Next Spine Pointer.
//
// ============================================================================


#ifndef __FLOW_HPP__
#define __FLOW_HPP__

#include "nic/include/base.hpp"
#include <string>
#include <map>
#include <queue>
#include "sdk/indexer.hpp"
#include <boost/crc.hpp>
#include "nic/include/hal_mem.hpp"

using namespace std;
using sdk::lib::indexer;

namespace hal {
namespace pd {
namespace utils {

/* forward declarations */
class FlowTableEntry;
class FlowEntry;
class FlowHintGroup;


typedef std::map<uint32_t, FlowTableEntry*> FlowTableEntryMap;
typedef std::map<uint32_t, FlowEntry*> FlowEntryMap;


typedef bool (*flow_iterate_func_t)(uint32_t gl_flow_index,
                                    const void *cb_data);

/** ---------------------------------------------------------------------------
  *
  * class Flow
  *
  *     - Flow Management
  *
  * ---------------------------------------------------------------------------
*/
#if 0
typedef struct hg_root_ {
    uint16_t hash;
    uint16_t hint;
} hg_root_t;
#endif

class Flow {
public:
    enum HashPoly {
        // CRC_32,
        HASH_POLY0,
        HASH_POLY1,
        HASH_POLY2,
        HASH_POLY3
    };

    enum stats {
        STATS_INS_SUCCESS,
        STATS_INS_FLOW_COLL, // STATS_INS_SUCCESS will not be incr.
        STATS_INS_FAIL_DUP_INS,
        STATS_INS_FAIL_NO_RES,
        STATS_INS_FAIL_HW,
        STATS_UPD_SUCCESS,
        STATS_UPD_FAIL_ENTRY_NOT_FOUND,
        STATS_REM_SUCCESS,
        STATS_REM_FAIL_ENTRY_NOT_FOUND,
        STATS_REM_FAIL_HW,
        STATS_MAX
    };


private:

    // Private Data
    std::string         table_name_;                // table name
    uint32_t            table_id_;                  // table id
    uint32_t            oflow_table_id_;            // oflow table id
    uint32_t            flow_hash_capacity_;        // size of flow table
    uint32_t            flow_coll_capacity_;        // size of coll. table
    uint32_t            key_len_;                   // key len
    uint32_t            data_len_;                  // data len
    uint32_t            entire_data_len_;           // entire data len
    HashPoly            hash_poly_;                 // hash polynomial


    uint32_t            hash_tbl_key_len_;          // hash table key len (21)
    uint32_t            hash_coll_tbl_key_len_;     // coll table key len (14)
    uint32_t            hint_len_;                  // hint len (11)
    uint32_t            hint_mem_len_B_;            // sw index into coll table
    uint32_t            num_hints_per_flow_entry_;  // HGs per Flow Entry (6)

    uint32_t            hwkey_len_;          // Key len for Flow Hash Table
    uint32_t            hwdata_len_;         // Data Len for Flow Hash Table

    bool                enable_delayed_del_;        // enable delayed del
    bool                entry_trace_en_;            // enable entry tracing

    // Hash Value(21 bits) => Flow Table Entry
    FlowTableEntryMap   flow_table_;

    // indexer for Flow Coll. Table
    indexer             *flow_coll_indexer_;

    // indexer and Map to store Flow Entries
    indexer             *flow_entry_indexer_;
    FlowEntryMap        flow_entry_map_;

    // Delayed Delete Queue
    std::queue<FlowEntry *> flow_entry_del_q_;
    std::queue<FlowHintGroup *> flow_hg_del_q_;

    uint64_t        *stats_;                // Statistics

    // Private Methods
    uint32_t get_num_bits_from_size_(uint32_t size);
    void pre_process_sizes_(uint32_t num_flow_hash_entries,
                            uint32_t num_flow_hash_coll_entries);
    uint32_t generate_hash_(void *key, uint32_t key_len, bool log = true);
    void stats_incr(stats stat);
    void stats_decr(stats stat);
    enum api {
        INSERT,
        UPDATE,
        REMOVE,
        RETRIEVE,
        RETRIEVE_FROM_HW,
        ITERATE
    };
    void stats_update(api ap, hal_ret_t rs);
    // Public Methods
    Flow(std::string table_name, uint32_t table_id, uint32_t oflow_table_id,
            uint32_t flow_hash_capacity,
            uint32_t oflow_capacity,
            uint32_t flow_key_len,                  // 320
            uint32_t flow_data_len,                 // 20
            // uint32_t flow_table_entry_len,          // 512
            uint32_t num_hints_per_flow_entry = 6,
            Flow::HashPoly hash_poly = HASH_POLY0,
            bool entry_trace_en = false);
    ~Flow();

public:
    static Flow *factory(std::string table_name, uint32_t table_id,
                         uint32_t oflow_table_id, uint32_t flow_hash_capacity,
                         uint32_t oflow_capacity, uint32_t flow_key_len,
                         uint32_t flow_data_len, uint32_t num_hints_per_flow_entry = 6,
                         Flow::HashPoly hash_poly = HASH_POLY0,
                         uint32_t mtrack_id = HAL_MEM_ALLOC_FLOW,
                         bool entry_trace_en = false);
    static void destroy(Flow *flow,
                        uint32_t mtrack_id = HAL_MEM_ALLOC_FLOW);

    // Debug Info
    uint32_t table_id(void) { return table_id_; }
    const char *table_name(void) { return table_name_.c_str(); }
    uint32_t table_capacity(void) { return flow_hash_capacity_; }
    uint32_t oflow_table_capacity(void) { return flow_coll_capacity_; }
    uint32_t table_num_entries_in_use(void);
    uint32_t oflow_table_num_entries_in_use(void);
    uint32_t table_num_inserts(void);
    uint32_t table_num_insert_errors(void);
    uint32_t table_num_updates(void);
    uint32_t table_num_update_errors(void);
    uint32_t table_num_deletes(void);
    uint32_t table_num_delete_errors(void);

    hal_ret_t insert(void *key, void *data, uint32_t *index);
    // calc_hash_ is a test only method used to generate hash collissions
    uint32_t calc_hash_(void *key, void *data);
    hal_ret_t update(uint32_t index, void *data);
    hal_ret_t remove(uint32_t index);


    /*
    Hash::ReturnStatus retrieve(uint32_t index, void **key, uint32_t *key_len,
                                void **data, uint32_t *data_len);
    Hash::ReturnStatus iterate(
            boost::function<Hash::ReturnStatus (const void *key, uint32_t key_len,
                                                const void *data, uint32_t data_len,
                                                uint32_t hash_idx, const void *cb_data)> cb,
                                                const void *cb_data,
                                                Hash::EntryType type);
    */
    hal_ret_t iterate(flow_iterate_func_t func, const void *cb_data);
    hal_ret_t alloc_flow_entry_index_(uint32_t *idx);
    hal_ret_t free_flow_entry_index_(uint32_t idx);

    uint32_t fetch_flow_table_bits_(uint32_t hash_val);
    uint32_t fetch_hint_bits_(uint32_t hash_val);
    hal_ret_t alloc_fhct_index(uint32_t *index);
    hal_ret_t free_fhct_index(uint32_t index);
    void add_flow_entry_global_map(FlowEntry *fe, uint32_t index);
    hal_ret_t print_flow();
    hal_ret_t entry_to_str(uint32_t gl_index, char *buff, uint32_t buff_size);

    // Getters & Setters
    bool get_delayed_del_en();
    uint32_t get_table_id() { return table_id_; }
    uint32_t get_oflow_table_id() { return oflow_table_id_; }
    uint32_t get_flow_data_len() { return data_len_; }
    uint32_t get_flow_entire_data_len() { return entire_data_len_; }
    uint32_t get_key_len() { return key_len_; }
    uint32_t get_hwkey_len() { return hwkey_len_; }
    uint32_t get_hwdata_len() { return hwdata_len_; }
    uint32_t get_hint_mem_len_B(void) { return hint_mem_len_B_; }
    bool get_entry_trace_en() { return entry_trace_en_; }


    void set_delayed_del_en(bool en);

    uint32_t get_num_hints_per_flow_entry();
    indexer *get_flow_coll_indexer();
    void push_fe_delete_q(FlowEntry *fe);
    void push_fhg_delete_q(FlowHintGroup *fhg);
    hal_ret_t flow_action_data_offsets(void *action_data,
                                       uint8_t **action_id,
                                       uint8_t **entry_valid,
                                       void **data,
                                       void **first_hash_hint,
                                       uint8_t **more_hashs,
                                       void **more_hints);

};

}   // namespace utils
}   // namespace pd
}   // namespace hal

using hal::pd::utils::Flow;

#endif // __FLOW_HPP__
