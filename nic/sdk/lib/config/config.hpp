//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// API for generic get/set functions to JSON files.
///
//----------------------------------------------------------------------------

#ifndef __SDK_CONFIG_HPP__
#define __SDK_CONFIG_HPP__

#include <list>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sys/file.h>
#include "include/sdk/base.hpp"
#include "include/sdk/types.hpp"

/// \defgroup DEVICE_LIB API for config files reading
/// @{

namespace sdk {
namespace lib {

using boost::property_tree::ptree;

/// \brief Class definition to manage read, write operations to configuration
///        file
class config {
public:
    /// \brief      creates configuration object for a specific file
    /// \param[in]  name of file to be written in to or read from
    /// \return     pointer to the object on successful initialization or
    ///             'nullptr' on failure.
    /// \remark     successive create() calls from a process on same file will
    ///             return same object pointer. Each create() call must have
    ///             a corresponding destroy() call, when the file access
    ///             is done, for the last one to release the object at the end.
    static config *create(const std::string &config_file);

    /// \brief      does the clean up of context, frees resources (close the
    ///             file etc), and destroys the object itself.
    /// \param[in]  object pointer returned by create() method
    /// \remark     to be called when the application is done reading/writing to
    ///             the associated configuration file in question.
    static void destroy(config *cfg);

    /// \brief      writes a key, value pair into JSON configuration file
    /// \param[in]  key     '.' qualified json key
    /// \param[in]  data    key value string
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t set(const std::string &key, const std::string &data);

    /// \brief      returns value of the '.' qualified JSON key that is peresent
    ///             in the associated configuration file
    /// \return     key value if the key is present or empty string ("") if not
    ///             present
    std::string get(const std::string &key);
private:
    config(std::string config_file);
    ~config(void);
    sdk_ret_t get_ptree_(std::string &config_file);
    bool update_ptree_(void);
private:
    // list to filter out duplicate config::create() requests on a given file
    // and shared the same context across threads.
    static std::list<config *> config_list_;
    static std::mutex config_list_mtx_;
    std::string conf_file_;
    int conf_fd_;
    ptree prop_tree_;
    struct timespec ts_;
    unsigned int ref_count_;
    std::mutex conf_mtx_;
};

}    // namespace lib
}    // namespace sdk

/// @}

#endif    //__SDK_CONFIG_HPP__
