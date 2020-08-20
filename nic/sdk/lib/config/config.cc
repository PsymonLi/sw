//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// API for generic get/set functions to JSON files.
///
//----------------------------------------------------------------------------

#include <iostream>
#include <libgen.h>
#include <sys/stat.h>
#include "lib/config/config.hpp"
#include "include/sdk/mem.hpp"
#include "lib/utils/utils.hpp"

namespace sdk {
namespace lib {

// list to maintain objects across all threads of a process.
// config::create("foo") from different threads of a process will share same
// object.
std::list<config *> config::config_list_;
std::mutex config::config_list_mtx_;

sdk_ret_t
config::get_ptree_(std::string &config_file) {
    struct stat sb;
    int ret;
    bool threw;

    conf_fd_ = open(config_file.c_str(), (O_RDWR | O_CREAT),
                    (S_IRGRP | S_IWGRP));
    if (conf_fd_ < 0) {
        SDK_TRACE_ERR("Failed to open file %s, err %s",
                      config_file.c_str(), strerror(errno));
        return SDK_RET_ERR;
    }

    if (flock(conf_fd_, LOCK_SH) < 0) {
        SDK_TRACE_ERR("Failed to read lock file %s, err %s",
                      config_file.c_str(), strerror(errno));
        close(conf_fd_);
        return SDK_RET_ERR;
    }

    threw = false;
    if (file_is_empty(config_file)) {
        SDK_TRACE_DEBUG("Config file %s is empty. ", config_file.c_str());
    } else {
        try {
            boost::property_tree::read_json(config_file, prop_tree_);
        } catch (std::exception const &ex) {
            threw = true;
            SDK_TRACE_ERR("Error parsing file %s, err %s",
                          config_file.c_str(), ex.what());
        }
    }

    if (fstat(conf_fd_, &sb) == 0) {
        // record the time stamp of the file. on subsequent get()
        // calls this stamp is used to ascertain if the file got modified after
        // this point and hence has to be reread to 'prop_tree_' and then
        // service get() request.
        this->ts_.tv_sec = sb.st_mtim.tv_sec;
        this->ts_.tv_nsec = sb.st_mtim.tv_nsec;
    } else {
        SDK_TRACE_ERR("Failed to stat file %s, err %s",
                      config_file.c_str(), strerror(errno));
        this->ts_.tv_sec = 0;
        this->ts_.tv_nsec = 0;
    }

    if ((ret = flock(conf_fd_, LOCK_UN)) < 0) {
        SDK_TRACE_ERR("Read lock release failure on %s, err %s",
                      config_file.c_str(), strerror(errno));
    }
    if ((ret < 0) || threw) {
        close(conf_fd_);
        conf_fd_ = -1;
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

config::config(std::string config_file) {
    sdk_ret_t  ret;

    ret = get_ptree_(config_file);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to init. config object for %s",
                      config_file.c_str());
    } else {
        this->conf_file_ = config_file;
    }
}

config *
config::create(const std::string &config_file) {
    void    *mem;
    config  *new_config;
    bool    destroy_new_config;

    SDK_ASSERT_RETURN((!config_file.empty()), nullptr);
    new_config = nullptr;

    // multiple threads of an app. (say in case of different libs linked
    // from the app) could access same file. share the context across
    // threads. check if one exists alredy for the file in question and use it.
    config_list_mtx_.lock();
    for (auto const &cfg : config_list_) {
        if (cfg->conf_file_ == config_file) {
            SDK_TRACE_DEBUG("Found a matched entry for %s",
                            config_file.c_str());
            new_config = cfg;
            new_config->ref_count_++;
            break;
        }
    }

    destroy_new_config = false;
    if (new_config == nullptr) {
        mem = SDK_CALLOC(sdk::SDK_MEM_ALLOC_LIB_CONFIG, sizeof(config));
        if (!mem) {
            SDK_TRACE_ERR("Failed to allocate memory for lib config");
        } else {
            new_config = new (mem) config(config_file);
            // 'new_config->conf_file_' is set by constructor only on successful
            // init of the object.
            if (new_config->conf_file_ == config_file) {
                config_list_.push_front(new_config);
                new_config->ref_count_ = 1;
            } else {
                // failure in config object creation. mark for clean up.
                destroy_new_config = true;
                new_config->ref_count_ = 0;
            }
        }
    }
    config_list_mtx_.unlock();

    if (destroy_new_config) {
        new_config->destroy(new_config);
        new_config = nullptr;
        SDK_TRACE_ERR("Error creating config instance for %s",
                      config_file.c_str());
    }
    return new_config;
}

config::~config(void) {
}

void
config::destroy(config *cfg) {
    bool destroy_object;

    if (!cfg) {
        return;
    }

    SDK_TRACE_DEBUG("Destroy on config object %p %s, ref_count_ %u, "
                    "list count %lu", cfg,
                    ((cfg->conf_file_ != "") ? cfg->conf_file_.c_str() : ""),
                    cfg->ref_count_, config_list_.size());

    destroy_object = false;
    config_list_mtx_.lock();
    if (cfg->ref_count_ == 0) {
        // a case of destroy() getting called from ::constructor() itself.
        destroy_object = true;
    } else {
        for (auto const &it_cfg : config_list_) {
            if (it_cfg->conf_file_ == cfg->conf_file_) {
                it_cfg->ref_count_--;
                if ((it_cfg)->ref_count_ == 0) {
                    config_list_.remove(it_cfg);
                    destroy_object = true;
                    break;
                }
            }
        }
    }

    cfg->conf_mtx_.lock();
    if (destroy_object &&
        (cfg->conf_fd_ >= 0)) {
        // shouldn't really be having flock if we got 'conf_mtx_' above,
        // but seems no harm in trying to release anyway.
        if (flock(cfg->conf_fd_, LOCK_UN) < 0) {
            SDK_TRACE_ERR("Lock release failure on %s, err %s",
                          cfg->conf_file_.c_str(), strerror(errno));
        }
        close(cfg->conf_fd_);
    }
    cfg->conf_mtx_.unlock();
    if (destroy_object) {
        cfg->~config();
        SDK_FREE(sdk::SDK_MEM_ALLOC_LIB_CONFIG, cfg);
    }
    config_list_mtx_.unlock();
    return;
}

// reread associated json file if file got updated since our last read.
// returns 'true' if no need to read the file (per time stamps) or on successful
// read. returns 'false' on any error.
bool
config::update_ptree_(void) {
    struct stat sb;
    std::string data;
    int stat_ret;
    bool threw;

    if ((stat_ret = fstat(this->conf_fd_, &sb)) != 0) {
        SDK_TRACE_ERR("Failed to stat file %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }

    threw = false;
    if ((stat_ret != 0) ||
        ((this->ts_.tv_sec != sb.st_mtim.tv_sec) ||
         (this->ts_.tv_nsec != sb.st_mtim.tv_nsec))) {
        SDK_TRACE_DEBUG("Rereading file %s, rc %d", this->conf_file_.c_str(),
                        stat_ret);
        try {
            boost::property_tree::read_json(this->conf_file_, prop_tree_);
            // update time stamp if we have it and we read file successfully.
            if (stat_ret == 0) {
                this->ts_.tv_sec = sb.st_mtim.tv_sec;
                this->ts_.tv_nsec = sb.st_mtim.tv_nsec;
            }
        } catch (std::exception const &ex) {
            threw = true;
            SDK_TRACE_ERR("Error parsing file %s, err %s",
                          this->conf_file_.c_str(), ex.what());
        }
    }
    return (threw ? false : true);
}

optional<ptree &>
config::get_child_optional(const std::string &key) {
    optional<ptree &> data = boost::none;

    this->conf_mtx_.lock();
    // sync across processes.
    if (flock(this->conf_fd_, LOCK_SH) < 0) {
        SDK_TRACE_ERR("Failed to read lock file %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return data;
    }

    if (update_ptree_()) {  // reread file if got updated since last read.
        data = this->prop_tree_.get_child_optional(key);
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release read lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return data;
}

sdk_ret_t
config::set_child(const std::string &key, ptree &data) {
    sdk_ret_t ret;
    struct stat sb;

    this->conf_mtx_.lock();
    // sync. across processes.
    if (flock(this->conf_fd_, LOCK_EX) < 0) {
        SDK_TRACE_ERR("Failed to write lock %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return SDK_RET_ERR;
    }

    ret = SDK_RET_OK;
    if (update_ptree_()) { // reread the file, if updated since last read.
        try {
            prop_tree_.put_child(key, data);
            boost::property_tree::write_json(conf_file_, prop_tree_);
            if (fstat(this->conf_fd_, &sb) == 0) {
                this->ts_.tv_sec = sb.st_mtim.tv_sec;
                this->ts_.tv_nsec = sb.st_mtim.tv_nsec;
            } else {
                SDK_TRACE_ERR("Failed to stat file %s, err %s",
                              this->conf_file_.c_str(), strerror(errno));
            }
        } catch (std::exception const &ex) {
            ret = SDK_RET_ERR;
            SDK_TRACE_ERR("Failed to write %s to %s err %s",
                          key.c_str(), this->conf_file_.c_str(),
                          ex.what());
        }
    } else {
        ret = SDK_RET_ERR;
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release write lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return (ret);
}

sdk_ret_t
config::erase(const std::string &key) {
    sdk_ret_t ret;
    struct stat sb;

    this->conf_mtx_.lock();

    // sync. across processes.
    if (flock(this->conf_fd_, LOCK_EX) < 0) {
        SDK_TRACE_ERR("Failed to write lock %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return SDK_RET_ERR;
    }

    ret = SDK_RET_OK;
    if (update_ptree_()) { // reread the file, if updated since last read.
        try {
            prop_tree_.erase(key);
            boost::property_tree::write_json(conf_file_, prop_tree_);
            if (fstat(this->conf_fd_, &sb) == 0) {
                this->ts_.tv_sec = sb.st_mtim.tv_sec;
                this->ts_.tv_nsec = sb.st_mtim.tv_nsec;
            } else {
                SDK_TRACE_ERR("Failed to stat file %s, err %s",
                              this->conf_file_.c_str(), strerror(errno));
            }
        } catch (std::exception const &ex) {
            ret = SDK_RET_ERR;
            SDK_TRACE_ERR("Failed to write %s to %s err %s",
                          key.c_str(), this->conf_file_.c_str(),
                          ex.what());
        }
    } else {
        ret = SDK_RET_ERR;
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release write lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return (ret);
}

}    // namespace lib
}    // namespace sdk
