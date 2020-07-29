#include "nic/hal/pd/utils/met/met.hpp"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/lib/utils/path_utils.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/platform/utils/mpartition.hpp"
#include <gtest/gtest.h>
#include <stdio.h>

using hal::pd::utils::Met;

class met_test : public ::testing::Test {
protected:
    met_test() {
  }

  virtual ~met_test() {
  }

  // will be called immediately after the constructor before each test
  virtual void SetUp() {
  }

  // will be called immediately after each test before the destructor
  virtual void TearDown() {
  }

   // Will be called at the beginning of all test cases in this class
   static void SetUpTestCase() {
	   sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_SIM);
   }

};

typedef struct __attribute__((__packed__)) __p4_replication_data_t {
    uint64_t rewrite_index:12;
    uint64_t is_tunnel:1;
    uint64_t is_qid:1;
    uint64_t repl_type:2;
    uint64_t qid_or_vnid:24;
    uint64_t tunnel_rewrite_index:10;
    uint64_t lport:11;
    uint64_t qtype:3;
} p4_replication_data_t;

typedef struct __attribute__((__packed__)) hal_pd_replication_tbl_mbr_entry_s {
    uint64_t lif : 11;
    uint64_t encap_id : 24;
    uint64_t tunnel_rewrite_idx : 11;
    uint64_t l3_rewrite_index : 12;
    uint64_t pad : 6;
} hal_pd_replication_tbl_mbr_entry_t;

/* ---------------------------------------------------------------------------
 *
 * Test Case 1:
 *      - Test Case to verify the update
 *   - Create Met
 *   - Create Repl list
 *   - Add repl
 *   - Rem repl
 *   - Delete Repl list
 */
TEST_F(met_test, test1) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx = 0;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(hal_pd_replication_tbl_mbr_entry_t));

    memset(&entry, 0, sizeof(entry));

    HAL_TRACE_DEBUG("Creating Repl list.");
    rs = test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lif = 10;
	entry.encap_id = 100;
    HAL_TRACE_DEBUG("Adding Repl entry.");
    rs = test_met->add_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    HAL_TRACE_DEBUG("Deleting Repl entry.");
    rs = test_met->del_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    HAL_TRACE_DEBUG("Deleting Repl list.");
	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);
}

/* ---------------------------------------------------------------------------
 *
 * Test Case 2:
 *      - Test Case to verify -ve create
 *   - Create Met
 */
TEST_F(met_test, test2) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx[100], tmp_idx = 0;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 100, 6,
                       sizeof(entry));

    for (int i = 1; i < 100; i++) {
        rs = test_met->create_repl_list(&repl_list_idx[i]);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

    rs = test_met->create_repl_list(&tmp_idx);
    ASSERT_TRUE(rs == HAL_RET_NO_RESOURCE);

    for (int i = 1; i < 100; i++) {
        rs = test_met->delete_repl_list(repl_list_idx[i]);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

    for (int i = 1; i < 100; i++) {
        rs = test_met->create_repl_list(&repl_list_idx[i]);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

    for (int i = 1; i < 100; i++) {
        rs = test_met->delete_repl_list(repl_list_idx[i]);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

}

/* ---------------------------------------------------------------------------
 *
 * Test Case 3:
 *      - Test Case to verify -ve delete
 *   - Create Met
 */
TEST_F(met_test, test3) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t tmp_idx = 0;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 100, 6,
                       sizeof(entry));

    rs = test_met->create_repl_list(&tmp_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->delete_repl_list(tmp_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->delete_repl_list(tmp_idx);
    ASSERT_TRUE(rs == HAL_RET_ENTRY_NOT_FOUND);
}

/* ---------------------------------------------------------------------------
 *
 * Test Case 4:
 *      - Test Case to verify -ve add repl
 *   - Create Met
 */
TEST_F(met_test, test4) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx = 0;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(hal_pd_replication_tbl_mbr_entry_t));

    test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lif = 10;
	entry.encap_id = 100;
    rs = test_met->add_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->add_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->del_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->del_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->add_replication(repl_list_idx + 1, &entry);
    ASSERT_TRUE(rs == HAL_RET_ENTRY_NOT_FOUND);

	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);
}

bool met_print_iterate(uint32_t repl_list_idx, Met *met, const void *cb_data)
{
    char buff[8192];

    met->repl_list_to_str(repl_list_idx, buff, 8192);

    HAL_TRACE_DEBUG("Repl List: \n{}", buff);

    return false;
}

const char *
repl_type_to_str(uint32_t type)
{
    switch(type) {
        case TM_REPL_TYPE_DEFAULT: return "DEFAULT";
        case TM_REPL_TYPE_TO_CPU_REL_COPY: return "CPU_REL_COPY";
        case TM_REPL_TYPE_HONOR_INGRESS: return "HON_ING";
        default: return "INVALID";
    }
}

uint32_t repl_entry_data_to_str(void *repl_entry_data,
                                char *buff, uint32_t buff_size)
{
    uint32_t b = 0;
    p4_replication_data_t *data = (p4_replication_data_t *)repl_entry_data;

    b = snprintf(buff, buff_size, "\n\t\tType: %s, lport: %lu,"
                 "(is_qid: %lu, qid/vnid: %lu), rw_idx: %lu,"
                 "is_tunnel: %lu, tnnl_rw_index: %lu,"
                 "qtype: %lu",
                 repl_type_to_str(data->repl_type),
                 data->lport,
                 data->is_qid,
                 data->qid_or_vnid,
                 data->rewrite_index,
                 data->is_tunnel,
                 data->tunnel_rewrite_index,
                 data->qtype);

    return b;
}


 /* ---------------------------------------------------------------------------
  *
  * Test Case 5:
  *      - Test Case to verify more than 6 replications
  *   - Create Met
  */
 TEST_F(met_test, test5) {

     hal_ret_t rs = HAL_RET_OK;
     uint32_t repl_list_idx;
     std::string table_name = "Test_Table";
     hal_pd_replication_tbl_mbr_entry_t entry;

     memset(&entry, 0, sizeof(entry));
     Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                        sizeof(hal_pd_replication_tbl_mbr_entry_t));

     test_met->create_repl_list(&repl_list_idx);
     ASSERT_TRUE(rs == HAL_RET_OK);

     entry.lif = 10;
     entry.encap_id = 100;
     for (int i = 0; i < 100; i++) {
         HAL_TRACE_DEBUG("Adding Repl entry: {}", i);
         rs = test_met->add_replication(repl_list_idx, &entry);
         ASSERT_TRUE(rs == HAL_RET_OK);
     }

     for (int i = 0; i < 100; i++) {
         HAL_TRACE_DEBUG("Deleting Repl entry: {}", i);
         rs = test_met->del_replication(repl_list_idx, &entry);
         ASSERT_TRUE(rs == HAL_RET_OK);
     }

     rs = test_met->delete_repl_list(repl_list_idx);
     ASSERT_TRUE(rs == HAL_RET_OK);
 }


/* ---------------------------------------------------------------------------
 *
 * Test Case 6:
 *      - Test Case to verify -ve del repl
 *   - Create Met
 */
TEST_F(met_test, test6) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx = 0;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(hal_pd_replication_tbl_mbr_entry_t));

    test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lif = 10;
	entry.encap_id = 100;
    rs = test_met->add_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->add_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->del_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->del_replication(repl_list_idx, &entry);
    ASSERT_TRUE(rs == HAL_RET_OK);

    rs = test_met->del_replication(repl_list_idx + 1, &entry);
    ASSERT_TRUE(rs == HAL_RET_ENTRY_NOT_FOUND);

	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);
}

/* ---------------------------------------------------------------------------
 *
 * Test Case 7:
 *      - Test Case to verify deletion of middle replications
 */
TEST_F(met_test, test7) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(hal_pd_replication_tbl_mbr_entry_t));

    test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lif = 10;
	entry.encap_id = 100;
    for (int i = 0; i < 100; i++) {
        HAL_TRACE_DEBUG("Adding Repl entry: {}", i);
        rs = test_met->add_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
        entry.encap_id++;
    }

    rs = test_met->del_replication(repl_list_idx, &entry);
    HAL_TRACE_DEBUG("ret: {}", rs);
    ASSERT_TRUE(rs == HAL_RET_ENTRY_NOT_FOUND);

    entry.encap_id--;
    for (int i = 0; i < 100; i++) {
        HAL_TRACE_DEBUG("Deleting Repl entry: {}", i);
        rs = test_met->del_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
        entry.encap_id--;
    }

	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);
}

/* ---------------------------------------------------------------------------
 *
 * Test Case 8:
 *      - Test Case to verify deletion of middle replications
 */
TEST_F(met_test, test8) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx;
    std::string table_name = "Test_Table";
	hal_pd_replication_tbl_mbr_entry_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(hal_pd_replication_tbl_mbr_entry_t));

    test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lif = 10;
	entry.encap_id = 100;
    for (int i = 0; i < 100; i++) {
        HAL_TRACE_DEBUG("Adding Repl entry: {}", i);
        rs = test_met->add_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
        entry.encap_id++;
    }

    rs = test_met->del_replication(repl_list_idx, &entry);
    HAL_TRACE_DEBUG("ret: {}", rs);
    ASSERT_TRUE(rs == HAL_RET_ENTRY_NOT_FOUND);

    // Delete of 150 - 199
    entry.encap_id = 150;
    for (int i = 0; i < 50; i++) {
        HAL_TRACE_DEBUG("Deleting Repl entry: {}", i);
        rs = test_met->del_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
        entry.encap_id++;
    }

    // Delete of 149 - 100
    entry.encap_id = 149;
    for (int i = 0; i < 50; i++) {
        HAL_TRACE_DEBUG("Deleting Repl entry: {}", i);
        rs = test_met->del_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
        entry.encap_id--;
    }

	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

    HAL_TRACE_DEBUG("tableid:{}, table_name:{}, capacity:{}, num_in_use:{}, "
                    "num_inserts:{}, num_insert_errors:{}, "
                    "num_deletes:{}, num_delete_errors:{}",
                    test_met->table_id(), test_met->table_name(),
                    test_met->table_capacity(), test_met->table_num_entries_in_use(),
                    test_met->table_num_inserts(), test_met->table_num_insert_errors(),
                    test_met->table_num_deletes(), test_met->table_num_delete_errors());

}

/* ---------------------------------------------------------------------------
 *
 * Test Case 9:
 *      - Test Case to verify iterate and to str
 */
TEST_F(met_test, test9) {

    hal_ret_t rs = HAL_RET_OK;
    uint32_t repl_list_idx;
    std::string table_name = "Test_Table";
    p4_replication_data_t entry;

    memset(&entry, 0, sizeof(entry));
    Met *test_met = Met::factory(table_name, (uint32_t)1, 64000, 6,
                       sizeof(p4_replication_data_t),
                       50,
                       NULL,
                       repl_entry_data_to_str);

    test_met->create_repl_list(&repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);

	entry.lport = 10;
	entry.qid_or_vnid = 100;
    for (int i = 0; i < 10; i++) {
        HAL_TRACE_DEBUG("Adding Repl entry: {}", i);
        rs = test_met->add_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

    // Met to String
    test_met->iterate(met_print_iterate, NULL);

    for (int i = 0; i < 10; i++) {
        HAL_TRACE_DEBUG("Deleting Repl entry: {}", i);
        rs = test_met->del_replication(repl_list_idx, &entry);
        ASSERT_TRUE(rs == HAL_RET_OK);
    }

	rs = test_met->delete_repl_list(repl_list_idx);
    ASSERT_TRUE(rs == HAL_RET_OK);
}

int main(int argc, char **argv) {
    std::string logfile;
    std::string cfg_path = std::string(std::getenv("CONFIG_PATH"));
    std::string mpart_json;
    sdk::lib::catalog *catalog;

    catalog = sdk::lib::catalog::factory(cfg_path, "/catalog_4g.json", platform_type_t::PLATFORM_TYPE_SIM);
    mpart_json = sdk::lib::get_mpart_file_path(cfg_path, "iris", catalog, "",
                                               platform_type_t::PLATFORM_TYPE_SIM);
    // Instantiate the singleton class
    sdk::platform::utils::mpartition::factory(mpart_json.c_str());

    ::testing::InitGoogleTest(&argc, argv);

    logfile = std::string("./hal.log");
    hal::utils::trace_init("hal", 0x3, true,
                           logfile.c_str(),
                           NULL,
                           TRACE_FILE_SIZE_DEFAULT,
                           TRACE_NUM_FILES_DEFAULT,
                           sdk::types::trace_debug,
                           sdk::types::trace_none);

    HAL_TRACE_DEBUG("Starting Main ... ");

  return RUN_ALL_TESTS();
}

