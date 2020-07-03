//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains utilities to get path for well known files.
///
//----------------------------------------------------------------------------

#ifndef __SDK_PATH_UTILS_HPP__
#define __SDK_PATH_UTILS_HPP__

#include "platform/pal/include/pal.h"
#include "lib/device/device.hpp"
#include "lib/catalog/catalog.hpp"

namespace sdk {
namespace lib {

/// \brief     gets path to the device configuration files
/// \return    return path to device configuration file, if
///            successful, else empty string
static inline __attribute__((always_inline)) std::string
get_device_conf_path (void)
{
    return std::string("/sysconfig/config0/device.conf");
}

/// \brief     gets path to the configuration files
/// \param[in] plaform_type type of platform
/// \return    return path to configuration file, if
///            successful, else empty string
static inline __attribute__((always_inline)) std::string
get_config_path (platform_type_t platform_type)
{
    char *config_path = std::getenv("CONFIG_PATH");

    if (config_path) {
        return config_path;
    } else if (likely(platform_type == platform_type_t::PLATFORM_TYPE_HW)) {
        return "/nic/conf";
    } else {
        return "./conf";
    }
}

/// \brief     gets the path to pipeline file
/// \param[in] cfg_path system configuration file path
/// \return    return path to pipeline file, if
///            successful, else empty string
static inline __attribute__((always_inline)) std::string
get_pipeline_file_path (std::string cfg_path)
{
    return cfg_path + "/pipeline.json";
}

/// \brief     gets the path to memory partition file
/// \param[in] cfg_path system configuration file path
/// \param[in] pipeline name of the pipeline
/// \param[in] catalog pointer to catalog object
/// \param[in] profile feature or memory profile
/// \param[in] plaform_type type of platform 
/// \return    return absolute path to memory partition file, if
///            successful, else empty string
static inline __attribute__((always_inline)) std::string
get_mpart_file_path (std::string cfg_path, std::string pipeline,
                     sdk::lib::catalog *catalog,
                     std::string profile, platform_type_t platform_type)
{
    uint64_t datapath_mem;
    std::string mem_str;
    std::string hbm_file;

    if (unlikely(catalog == NULL)) {
        return std::string("");
    }
    // on Vomero, uboot gives Linux 6G on boot up, so only 2G is left
    // for datapath, load 4G HBM profile on systems with 2G data path memory
    mem_str = catalog->memory_capacity_str();
    if (likely((platform_type == platform_type_t::PLATFORM_TYPE_HW))) {
        datapath_mem = pal_mem_get_phys_totalsize();
        if (datapath_mem == 0x80000000) { //2G
            mem_str = "4g";
        }
    }    
    hbm_file = profile.length() ? "hbm_mem_" + profile + ".json" : "hbm_mem.json";
    if (likely((platform_type == platform_type_t::PLATFORM_TYPE_HW))) {
        return cfg_path + "/" + pipeline + "/" + mem_str + "/" + hbm_file;
    } else {
        return cfg_path + "/" + pipeline + "/" + catalog->asic_type_str(0) + "/" + mem_str + "/" + hbm_file;
    }
}

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_PATH_UTILS_HPP__
