// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __SDK_ASIC_HPP__
#define __SDK_ASIC_HPP__

#include <string>
#include "include/sdk/base.hpp"
#include "include/sdk/types.hpp"
#include "lib/catalog/catalog.hpp"
#include "platform/utils/mpartition.hpp"
#include "p4/loader/loader.hpp"

namespace sdk {
namespace asic {

#define SDK_ASIC_PGM_CFG_MAX        3
#define SDK_ASIC_ASM_CFG_MAX        3

typedef void (*completion_cb_t)(sdk_status_t status);

typedef struct asic_pgm_cfg_s {
    std::string                 path;
} asic_pgm_cfg_t;

typedef struct asic_asm_cfg_s {
    std::string          name;
    std::string          path;
    std::string          base_addr;
    mpu_pgm_sort_t       sort_func;
    mpu_pgm_symbols_t    symbols_func;
} asic_asm_cfg_t;

typedef struct asic_cfg_s {
    sdk::platform::asic_type_t          asic_type;
    std::string          default_config_dir;    // TODO: vasanth, pls. remove this up eventually
    uint32_t             admin_cos;
    uint32_t             repl_entry_width;
    std::string          cfg_path;
    std::string          pgm_name;
    uint8_t              num_pgm_cfgs;
    uint8_t              num_asm_cfgs;
    asic_pgm_cfg_t       pgm_cfg[SDK_ASIC_PGM_CFG_MAX];
    asic_asm_cfg_t       asm_cfg[SDK_ASIC_ASM_CFG_MAX];
    sdk::lib::catalog    *catalog;
    mpartition           *mempartition;
    platform_type_t      platform;
    completion_cb_t      completion_func;
} asic_cfg_t;

// initialize the asic
sdk_ret_t asic_init(asic_cfg_t *asic_cfg);
// cleanup asic initialization
void asic_cleanup(void);

typedef enum asic_block_e {
    ASIC_BLOCK_PACKET_BUFFER,
    ASIC_BLOCK_TXDMA,
    ASIC_BLOCK_RXDMA,
    ASIC_BLOCK_MS,
    ASIC_BLOCK_PCIE,
    ASIC_BLOCK_MAX
} asic_block_t;

typedef struct asic_bw_s {
    double read;
    double write;
} asic_bw_t;

typedef struct asic_hbm_bw_s {
    asic_block_t type;
    uint64_t clk_diff;
    asic_bw_t max;
    asic_bw_t avg;
} asic_hbm_bw_t;

}    // namespace asic
}    // namespace sdk

using sdk::asic::asic_pgm_cfg_t;
using sdk::asic::asic_asm_cfg_t;
using sdk::asic::asic_cfg_t;
using sdk::asic::asic_block_t;
using sdk::asic::asic_bw_t;
using sdk::asic::asic_hbm_bw_t;

#endif    // __SDK_ASIC_HPP__
