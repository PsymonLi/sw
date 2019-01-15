// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __HAL_CFG_HPP__
#define __HAL_CFG_HPP__

#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/lib/catalog/catalog.hpp"
#include "nic/sdk/platform/utils/mpartition.hpp"

namespace hal {

enum {
    HAL_THREAD_ID_MIN           = 0,
    HAL_THREAD_ID_CFG           = HAL_THREAD_ID_MIN,
    HAL_THREAD_ID_PERIODIC      = 1,
    HAL_THREAD_ID_FTE_MIN       = 2,
    HAL_THREAD_ID_FTE_MAX       = 4,
    HAL_THREAD_ID_ASIC_RW       = 5,
    HAL_THREAD_ID_DELPHI_CLIENT = 6,
    HAL_THREAD_ID_LINKMGR_CTRL  = 7,
    HAL_THREAD_ID_MAX           = 8,
};

#define HAL_MAX_NAME_STR  16
const uint16_t MAX_FTE_THREADS =
               HAL_THREAD_ID_FTE_MAX - HAL_THREAD_ID_FTE_MIN + 1;

typedef enum hal_feature_set_s {
    HAL_FEATURE_SET_NONE,
    HAL_FEATURE_SET_IRIS,
    HAL_FEATURE_SET_GFT,
    HAL_FEATURE_SET_APOLLO,
} hal_feature_set_t;

#define UPLINK_FLOOD_MODES(ENTRY)                                                                   \
     ENTRY(HAL_UPLINK_FLOOD_MODE_NONE,   0, "HAL_UPLINK_FLOOD_MODE_NONE")                           \
     ENTRY(HAL_UPLINK_FLOOD_MODE_RPF,    1, "HAL_UPLINK_FLOOD_MODE_RPF")                            \
     ENTRY(HAL_UPLINK_FLOOD_MODE_PINNED, 2, "HAL_UPLINK_FLOOD_MODE_PINNED")

 DEFINE_ENUM(hal_uplink_flood_mode_t, UPLINK_FLOOD_MODES)
 #undef UPLINK_FLOOD_MODES

#define FORWARDING_MODES(ENTRY)                                                                     \
     ENTRY(HAL_FORWARDING_MODE_NONE,    0, "HAL_FORWARDING_MODE_NONE")                              \
     ENTRY(HAL_FORWARDING_MODE_SMART_SWITCH,    1, "HAL_FORWARDING_MODE_SMART_SWITCH")              \
     ENTRY(HAL_FORWARDING_MODE_SMART_HOST_PINNED,    2, "HAL_FORWARDING_MODE_SMART_HOST_PINNED")    \
     ENTRY(HAL_FORWARDING_MODE_CLASSIC,    3, "HAL_FORWARDING_MODE_CLASSIC")

 DEFINE_ENUM(hal_forwarding_mode_t, FORWARDING_MODES)
 #undef FORWARDING_MODES

typedef struct hal_cfg_s {
    void                     *server_builder;    // grpc server builder
    platform_type_t          platform;
    char                     asic_name[HAL_MAX_NAME_STR];
    std::string              grpc_port;
    std::string              loader_info_file;
    char                     feature_set[HAL_MAX_NAME_STR];
    hal_feature_set_t        features;
    hal_forwarding_mode_t    forwarding_mode;
    bool                     sync_mode_logging;
    std::string              cfg_path;                  // full HAL config path
    std::string              catalog_file;              // full path of catalog file
    sdk::lib::catalog        *catalog;
    mpartition               *mempartition;
    std::string              default_config_dir;        // TODO: remove this !!
    uint16_t                 num_control_cores;
    uint16_t                 num_data_cores;
    uint64_t                 control_cores_mask;
    uint64_t                 data_cores_mask;
    bool                     shm_mode;
    bool                     qemu;                      // true if this is running on QEMU
} hal_cfg_t;

}    // namespace hal

#endif    // __HAL_CFG_HPP__

