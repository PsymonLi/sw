//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// module version db
///
//----------------------------------------------------------------------------

#include <unordered_map>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <nic/sdk/include/sdk/base.hpp>
#include <nic/sdk/include/sdk/types.hpp>
#include <nic/sdk/lib/logger/logger.hpp>

#ifndef __SDK_VERSION_DB_HPP__
#define __SDK_VERSION_DB_HPP__

namespace sdk {

// JSON record keys
#define JSON_KEY_FIRMWARE    "firmware"
#define JSON_KEY_MODULE_LIST "modules"
#define JSON_KEY_MAJOR_VER   "major"
#define JSON_KEY_MINOR_VER   "minor"

//------------------------------------------------------------------------------
// version database, parses a given version json file and
// creates a mapping of module version (major, minor) indexed by the module
// name as the key.
//------------------------------------------------------------------------------
class version_db {
public:
    ~version_db() {
        for (auto iter = mod_ver_db_.begin();
             iter != mod_ver_db_.end(); ++iter) {
            delete iter->second;
        }
    }

    version_db() {
        valid_ = false;
    }

    /// \brief parse version json file and initialize the db
    /// \param[in] version info json file
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t init(const char *cfg) {
        SDK_TRACE_VERBOSE("parsing config file %s\n", cfg);
        boost::property_tree::ptree pt;
        try {
            std::ifstream json_cfg(cfg);
            try {
                boost::property_tree::read_json(json_cfg, pt);
            } catch (boost::exception &e) {
                SDK_TRACE_ERR("config file %s parse error %s\n", cfg,
                              boost::diagnostic_information(e).c_str());
                return SDK_RET_ERR;
            }
        } catch (boost::exception &e) {
            SDK_TRACE_ERR("config file %s open error %s\n", cfg,
                          boost::diagnostic_information(e).c_str());
            return SDK_RET_INVALID_ARG;
        }

        auto fw_info = pt.get_child(JSON_KEY_FIRMWARE);
        fw_ver_.major = fw_info.get<uint16_t>(JSON_KEY_MAJOR_VER);
        fw_ver_.minor = fw_info.get<uint16_t>(JSON_KEY_MINOR_VER);

        BOOST_FOREACH(auto mod_list, pt.get_child(JSON_KEY_MODULE_LIST)) {
            std::string mod_name = mod_list.first;
            auto mod_info = mod_list.second;
            types::module_version_t *mv = new types::module_version_t;
            mv->major = mod_info.get<uint16_t>(JSON_KEY_MAJOR_VER);
            mv->minor = mod_info.get<uint16_t>(JSON_KEY_MINOR_VER);
            mod_ver_db_[mod_name] = mv;
        }
        valid_ = true;
        return SDK_RET_OK;
    }

    /// \brief get named module version info
    /// \param[in] module name
    /// \return pointer to version info, nullptr if not found
    const types::module_version_t* module_version(const char *mod_name) const {
        auto iter = mod_ver_db_.find(mod_name);
        if (iter != mod_ver_db_.end()) {
            return iter->second;
        }
        return nullptr;
    }

    /// \brief get base firmware version info
    /// \return pointer to version info, nullptr if not found
    const types::module_version_t* firmware_version(void) const {
        return &fw_ver_;
    }

    /// \brief print version database into given file
    /// \param[in] opened file pointer
    /// \return none
    void dump(FILE *f) {
        fprintf(f, "firmware major %i minor %i\n", fw_ver_.major,
                fw_ver_.minor);
        for (auto iter = mod_ver_db_.begin(); iter != mod_ver_db_.end();
             ++iter) {
            const types::module_version_t *mv = iter->second;
            fprintf(f, "mod %s major %i minor %i\n",
                    iter->first.c_str(), mv->major, mv->minor);
        }
    }

    /// \brief returns whether the db is valid or not
    bool valid(void) const { return valid_; }

private:
    // firmware version info
    types::module_version_t fw_ver_;
    // module version map with module name as key
    std::unordered_map<std::string, types::module_version_t*> mod_ver_db_;
    // true if version db is initialized, false otherwise
    bool valid_;
};

}    // namespace sdk

#endif    // __SDK_VERSION_DB_HPP__
