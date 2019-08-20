#include "nic/hal/pd/utils/met/repl_table_entry.hpp"
#include "nic/hal/pd/utils/met/repl_entry.hpp"
#include "nic/hal/pd/utils/met/repl_list.hpp"
#include "nic/hal/pd/utils/met/repl_entry.hpp"
#include "nic/hal/pd/utils/met/met.hpp"
#include "nic/hal/hal_trace.hpp"

using hal::pd::utils::ReplTableEntry;
using hal::pd::utils::ReplEntryHw;

//---------------------------------------------------------------------------
// Factory method to instantiate the class
//---------------------------------------------------------------------------
ReplTableEntry *
ReplTableEntry::factory(uint32_t repl_table_index, ReplList *repl_list,
                        uint32_t mtrack_id)
{
    void            *mem = NULL;
    ReplTableEntry  *rte = NULL;

    mem = HAL_CALLOC(mtrack_id, sizeof(ReplTableEntry));
    if (!mem) {
        return NULL;
    }

    rte = new (mem) ReplTableEntry(repl_table_index, repl_list);
    return rte;
}

//---------------------------------------------------------------------------
// Method to free & delete the object
//---------------------------------------------------------------------------
void
ReplTableEntry::destroy(ReplTableEntry *re, uint32_t mtrack_id)
{
    if (re) {
        re->~ReplTableEntry();
        HAL_FREE(mtrack_id, re);
    }
}

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
ReplTableEntry::ReplTableEntry(uint32_t repl_table_index, ReplList *repl_list)
{

    repl_table_index_ = repl_table_index;
    repl_list_        = repl_list;
    num_repl_entries_ = 0;
    first_repl_entry_ = NULL;
    prev_             = NULL;
    next_             = NULL;

}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
ReplTableEntry::~ReplTableEntry() {}


// ----------------------------------------------------------------------------
// Add Replication entry
// ----------------------------------------------------------------------------
hal_ret_t
ReplTableEntry::add_replication(ReplEntry *re)
{
    hal_ret_t rs = HAL_RET_OK;
    SDK_ASSERT_GOTO(num_repl_entries_ <
            repl_list_->get_met()->get_max_num_repls_per_entry(), end);

    HAL_TRACE_VERBOSE("Adding replication entry to repl_table_entry: {}",
                        repl_table_index_);

    // Add replication entry
    if (first_repl_entry_ == NULL) {
        first_repl_entry_ = re;
        last_repl_entry_ = re;
    } else {
        re->set_prev(last_repl_entry_);
        last_repl_entry_->set_next(re);
        last_repl_entry_ = re;
    }

    num_repl_entries_++;
    program_table();
end:
    return rs;
}

// ----------------------------------------------------------------------------
// Del Replication entry
// ----------------------------------------------------------------------------
hal_ret_t
ReplTableEntry::del_replication(void *data)
{
    hal_ret_t       rs = HAL_RET_ENTRY_NOT_FOUND;
    ReplEntry       *re = NULL;

    SDK_ASSERT_GOTO(num_repl_entries_ != 0, end);

    HAL_TRACE_DEBUG("Deleting replication entry from repl_table_entry: {}",
                        repl_table_index_);
    re = first_repl_entry_;
    while(re) {

        if (!memcmp(data, re->get_data(), re->get_data_len())) {
            HAL_TRACE_DEBUG("Match");
            // match
            if (re->get_prev() == NULL) {
                first_repl_entry_ = re->get_next();
                if (re->get_next()) {
                    re->get_next()->set_prev(NULL);
                }
            } else if (re->get_next() == NULL) {
                last_repl_entry_ = re->get_prev();
                if (re->get_prev()) {
                    re->get_prev()->set_next(NULL);
                }
            } else {
                re->get_prev()->set_next(re->get_next());
                re->get_next()->set_prev(re->get_prev());
            }

            num_repl_entries_--;

            if (num_repl_entries_) {
                program_table();
            }
            //else the table entry is going away.
            //de-programming will be done there.

            // delete re;
            ReplEntry::destroy(re);
            rs = HAL_RET_OK;
            goto end;
        }

        re = re->get_next();
    }

end:
    return rs;
}


// ----------------------------------------------------------------------------
// Programs Replication table
// ----------------------------------------------------------------------------
hal_ret_t
ReplTableEntry::program_table()
{
    int i = 0;
    ReplEntry *repl_entry;
    ReplEntryHw hw_entry;

    for (repl_entry = get_first_repl_entry(); repl_entry;
         repl_entry = repl_entry->get_next()) {
        hw_entry.set_token(repl_entry->get_data(), i++, repl_entry->get_data_len());
    }

    hw_entry.set_num_tokens(get_num_repl_entries());

    if (get_next()) {
        hw_entry.set_next_ptr(get_next()->get_repl_table_index());
        hw_entry.set_last_entry(0);
    } else {
        if (get_repl_list()->get_attached_list_index() != 0) {
            hw_entry.set_next_ptr(get_repl_list()->get_attached_list_index());
            hw_entry.set_last_entry(0);
        } else {
            hw_entry.set_next_ptr(0);
            hw_entry.set_last_entry(1);
        }
    }

    HAL_TRACE_DEBUG("Program Replication Table Entry: Index:{} is_last:{}, "
                    "num_elems:{}, next_ptr:{}",
                    get_repl_table_index(),
                    hw_entry.get_last_entry(),
                    hw_entry.get_num_tokens(),
                    hw_entry.get_next_ptr());
    SDK_ASSERT(!(hw_entry.get_last_entry() == 0 &&
               get_repl_table_index() == hw_entry.get_next_ptr()));
    return hw_entry.write(get_repl_table_index());
}

// ----------------------------------------------------------------------------
// DePrograms Replication table
// ----------------------------------------------------------------------------
hal_ret_t
ReplTableEntry::deprogram_table()
{
    hal_ret_t rs = HAL_RET_OK;

    // TODO: zerout the entry at repl_table_index_
    return rs;
}

hal_ret_t
ReplTableEntry::entry_to_str(char **buff, uint32_t *buff_size)
{
    hal_ret_t       rs = HAL_RET_ENTRY_NOT_FOUND;
    ReplEntry       *re = NULL;
    uint32_t        b = 0;

    b = snprintf(*buff, *buff_size, "\n\tRTE: Idx: %d, Next_idx: %d, #Repls: %d => ",
                 repl_table_index_, next_ ? next_->get_repl_table_index() : -1,
                 num_repl_entries_);

    *buff += b;
    *buff_size -= b;

    re = first_repl_entry_;
    while (re) {
        re->entry_to_str(repl_list_->get_met()->get_repl_entry_to_str_func(),
                         buff, buff_size);
        re = re->get_next();
    }

    return rs;
}

// ----------------------------------------------------------------------------
// Trace Replication Table Entry
// ----------------------------------------------------------------------------
hal_ret_t
ReplTableEntry::trace_repl_tbl_entry()
{
    hal_ret_t       rs = HAL_RET_ENTRY_NOT_FOUND;
    ReplEntry       *re = NULL;

    HAL_TRACE_DEBUG("Repl_Tbl_Entry: {} Num_of_Repl_Entries: {}",
                    repl_table_index_, num_repl_entries_);
    HAL_TRACE_DEBUG("Next_RTE: {}",
                    next_ ? next_->get_repl_table_index() : -1);

    re = first_repl_entry_;
    while (re) {
        re->trace_repl_entry();
        re = re->get_next();
    }

    return rs;
}


