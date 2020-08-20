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
using boost::optional;

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
    /// \param[in]  data    key value
    /// \return     #SDK_RET_OK on success, failure status code on error
    template <typename T>
    sdk_ret_t set(const std::string &key, const T &data);

    /// \brief      adds the given ptree as a child for the key
    /// \param[in]  key '.' qualified json key
    /// \param[in]  data ptree to be added
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t set_child(const std::string &key, ptree &data);

    /// \brief      deletes the all the children of the key
    /// \param[in]  key '.' qualified json key
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t erase(const std::string &key);

    /// \brief      returns value of the '.' qualified JSON key that is present
    ///             in the associated configuration file
    /// \param[in]  key '.' qualified json key
    /// \return     key value
    template <typename T>
    T get(const std::string &key);

    /// \brief      returns value of the '.' qualified JSON key that is present
    ///             in the associated configuration file, else the default value is
    ///             returned
    /// \param[in]  key     '.' qualified json key
    /// \param[in]  default_val default value to be returned if key not found
    /// \return     key value
    template <typename T>
    T get(const std::string &key, T &default_val);

    /// \brief      gets the child ptree indexed by given key
    /// \param[in]  key '.' qualified json key
    /// \return     child ptree reference or boost::null if not found
    optional<ptree &> get_child_optional(const std::string &key);
private:
    config(std::string config_file);
    ~config(void);
    sdk_ret_t get_ptree_(std::string &config_file);
    bool update_ptree_(void);

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

#include "config_impl.hpp"
/// @}

#endif    //__SDK_CONFIG_HPP__
