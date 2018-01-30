// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "linkmgr_rw.hpp"

namespace sdk {
namespace linkmgr {

uint32_t
read_reg_base (uint32_t chip, uint64_t addr)
{
    uint32_t data = 0x0;

    if (sdk::lib::pal_reg_read(addr, &data) != sdk::lib::PAL_RET_OK) {
        SDK_TRACE_ERR("%s read failed\n", __FUNCTION__);
    }

    return data;
}

void write_reg_base(uint32_t chip, uint64_t addr, uint32_t  data)
{
    if (sdk::lib::pal_reg_write(addr, &data) != sdk::lib::PAL_RET_OK) {
        SDK_TRACE_ERR("%s write failed\n", __FUNCTION__);
    }

    uint32_t read_data = read_reg_base(chip, addr);
    SDK_TRACE_DEBUG("%s read_data after write 0x%x\n", __FUNCTION__, read_data);
}

}    // namespace linkmgr
}    // namespace sdk
