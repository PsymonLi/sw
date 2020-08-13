//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Test cases to verify sdk::lib::config class API
///
//----------------------------------------------------------------------------

#include <list>
#include <string>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include "lib/config/config.hpp"

using namespace sdk::lib;

#define K_BUF_LEN 256
typedef std::map<std::string, std::string>  kvmap;
kvmap g_data_set = {
          {"FirstLevelKey1", "FirstLevelKey1Value"},
          {"FirstLevelKey2", "FirstLevelKey2Value"},
          {"FirstLevelKey3", "FirstLevelKey3Value"},
          {"SecondLevelKey1.Key1", "SecondLevelKey1.Key1Value"},
          {"SecondLevelKey1.Key2", "SecondLevelKey1.Key2Value"},
          {"SecondLevelKey1.Key3", "SecondLevelKey1.Key3Value"},
          {"SecondLevelKey2.Key1", "SecondLevelKey2.Key1Value"},
      };

class config_lib_test : public ::testing::Test {
public:
    config *cfg_;
    std::string exec_path_;
    std::string test_file_;
protected:
    config_lib_test() {
    }

    virtual ~config_lib_test() {
    }

    // will be called immediately after the constructor before each test
    virtual void SetUp() {
        boost::property_tree::ptree pt;
        char exec_path[K_BUF_LEN];
        size_t path_len;
        size_t pos;

        for (auto const &it : g_data_set) {
             pt.put(it.first, it.second);
        }
        path_len = readlink("/proc/self/exe", exec_path, K_BUF_LEN);
        ASSERT_GT(path_len, 0) << "Error figuring path for test data file.";
        exec_path[path_len] = '\0';
        exec_path_ = exec_path;
        pos = exec_path_.rfind("/");
        ASSERT_NE(pos, std::string::npos) << "Error parsing the exec path name";
        exec_path_ = exec_path_.substr(0, pos);
        test_file_ = exec_path_ + "/config_test.json";
        printf("test file %s\n", test_file_.c_str());

        boost::property_tree::write_json(test_file_, pt);
        std::string cmd("cat ");
        cmd += test_file_;
        printf("Test data:\n");
        system(cmd.c_str());
    }

    // will be called immediately after each test before the destructor
    virtual void TearDown() {
        std::string cmd("rm ");
        cmd += test_file_;
        printf("Removing test data file %s\n", cmd.c_str());
        system(cmd.c_str());
    }
};

void
check_data_set (config *cfg)
{
    std::string key_val;

    for (auto const &it : g_data_set) {
         key_val = cfg->get(it.first);
         ASSERT_EQ(key_val, it.second) << "Read key value not matching data";
    }
}

TEST_F(config_lib_test, basic_read)
{
    std::string key_val;

    this->cfg_ = config::create(this->test_file_);
    ASSERT_NE(this->cfg_, nullptr) << "Config object creation failed.";

    check_data_set(this->cfg_);
    config::destroy(this->cfg_);
    this->cfg_ = nullptr;
}

kvmap g_write_test_data = {
          {"FirstLevelKey4", "FirstLevelKey4Value"},
          {"FirstLevelKey5", "FirstLevelKey5Value"},
          {"SecondLevelKey3.key1", "SecondLevelKey3.key1Value"},
          {"SecondLevelKey3.key2", "SecondLevelKey3.key2Value"},
      };

void write_additional_keys (config *cfg)
{
    sdk_ret_t ret;

    printf("Data being added on (key,value):\n");
    for (auto const &it : g_write_test_data) {
         printf("%s:%s:\n", it.first.c_str(), it.second.c_str());
         ret = cfg->set(it.first, it.second);
         ASSERT_EQ(ret, SDK_RET_OK) << "Failed to write key\n";
    }
}

void
check_additional_keys (config *cfg)
{
    std::string key_val;

    for (auto const &it : g_write_test_data) {
         key_val = cfg->get(it.first);
         ASSERT_EQ(key_val, it.second) << "Read key value not matching data";
    }
}

void
check_write_read (std::string test_file)
{
    config *cfg;

    cfg = config::create(test_file);
    ASSERT_NE(cfg, nullptr) << "Config object creation failed.";
    write_additional_keys(cfg);
    config::destroy(cfg);

    // check if file contains the keys added in above.
    cfg = config::create(test_file);
    ASSERT_NE(cfg, nullptr) << "Config object creation failed.";
    check_additional_keys(cfg);
    config::destroy(cfg);
}

TEST_F(config_lib_test, basic_write)
{

    this->cfg_ = config::create(this->test_file_);
    ASSERT_NE(this->cfg_, nullptr) << "Config object creation failed.";

    printf("Writing additioanl keys in.\n");
    write_additional_keys(this->cfg_);

    // check original data set is still intact in the file.
    printf("Checking original keys are intact.\n");
    check_data_set(this->cfg_);
    // check if the addtional written data set is present too.
    printf("Checking additional keys are added in.\n");
    check_additional_keys(this->cfg_);
    printf("Post write, data file:\n");
    std::string cmd("cat ");
    cmd += this->test_file_;
    system(cmd.c_str());

    config::destroy(this->cfg_);
    this->cfg_ = nullptr;
}

TEST_F(config_lib_test, get_on_new_file)
{
    config *cfg;
    std::string test_file;
    std::string cmd, key_val;

    // make sure the .json file question is not present.
    test_file = exec_path_ + "/misc_test_file.json";
    cmd = "rm -rf " + test_file;
    system(cmd.c_str());

    cmd = "ls -l " + test_file;
    system(cmd.c_str());
    cfg = config::create(test_file);
    ASSERT_NE(cfg, nullptr) << "Config object creation failed.";
    key_val = cfg->get("abc");
    ASSERT_EQ(key_val, "") << "Key val is not null on new file (no data) read";
    config::destroy(cfg);
}

TEST_F(config_lib_test, misc_file_access)
{
    std::string test_file;
    std::string cmd;

    // set and gets on new file.
    test_file = exec_path_ + "/misc_test_file.json";
    cmd = "rm -rf " + test_file;
    system(cmd.c_str());

    cmd = "ls -l " + test_file;
    system(cmd.c_str());

    check_write_read(test_file);

    // check if the written data set is present.
    printf("Post write, data file:\n");
    cmd = "cat " + test_file;
    system(cmd.c_str());

    cmd = "rm -rf " + test_file;
    system(cmd.c_str());

    // check set()/get() on an exiting empty file.
    cmd = "touch " + test_file;
    system(cmd.c_str());

    cmd = "ls -l " + test_file;
    system(cmd.c_str());

    check_write_read(test_file);
    printf("Post write to new data file:\n");
    cmd = "cat " + test_file;
    system(cmd.c_str());

    cmd = "rm -rf " + test_file;
}

int main (int argc, char **argv) {
      ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
}
