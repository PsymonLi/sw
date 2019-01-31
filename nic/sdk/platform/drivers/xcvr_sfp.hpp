// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __SDK_XCVR_SFP_HPP__
#define __SDK_XCVR_SFP_HPP__

namespace sdk {
namespace platform {

#define SFP_OFFSET_LENGTH_CU                 18
#define SFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES 36

inline sdk_ret_t
sfp_sprom_parse (int port, uint8_t *data)
{
    sdk_ret_t sdk_ret = SDK_RET_OK;

    // SFF 8472

    if (data[SFP_OFFSET_LENGTH_CU] != 0) {
        set_cable_type(port, cable_type_t::CABLE_TYPE_CU);
    } else {
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
    }

    // TODO reset when removed?
    xcvr_set_an_args(port, 0, false, 0);

    if (data[3] != 0) {
        if (data[3] & (1 << 4)) {
            // 10G Base SR
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_SR);
            set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        } else if (data[3] & ( 1 << 5)) {
            // 10G Base LR
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_LR);
            set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        } else if (data[3] & ( 1 << 6)) {
            // 10G Base LRM
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_LRM);
            set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        } else if (data[3] & ( 1 << 7)) {
            // 10G Base ER
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_ER);
            set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        }

        return sdk_ret;
    }

    switch (data[SFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES]) {
    case 0x1:
        // 25GAUI C2M AOC - BER 5x10^(-5)
        break;

    case 0x2:
        // 25GBASE-SR
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_SR);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0x3:
        // 25GBASE-LR
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_LR);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0x4:
        // 25GBASE-ER
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_ER);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0x8:
        // 25GAUI C2M ACC - BER 5x10^(-5)
        break;

    case 0xb:
        // 25GBASE-CR CA-L
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_L);
        xcvr_set_an_args(port,
                         AN_USER_CAP_25GBKRCR_S | AN_USER_CAP_25GBKRCR,
                         true,
                         AN_FEC_REQ_25GB_RSFEC | AN_FEC_REQ_25GB_FCFEC);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0xc:
        // 25GBASE-CR CA-S
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_S);
        xcvr_set_an_args(port,
                         AN_USER_CAP_25GBKRCR_S | AN_USER_CAP_25GBKRCR,
                         true,
                         AN_FEC_REQ_25GB_RSFEC | AN_FEC_REQ_25GB_FCFEC);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0xd:
        // 25GBASE-CR CA-N
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_N);
        xcvr_set_an_args(port,
                         AN_USER_CAP_25GBKRCR_S | AN_USER_CAP_25GBKRCR,
                         true,
                         AN_FEC_REQ_25GB_RSFEC | AN_FEC_REQ_25GB_FCFEC);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        break;

    case 0x16:
        // 10GBASE-T
        break;

    case 0x18:
        // 25GAUI C2M AOC - BER 10^(-12)
        break;

    case 0x19:
        // 25GAUI C2M ACC - BER 10^(-12)
        break;

    default:
        break;
    }

    return SDK_RET_OK;
}

} // namespace sdk
} // namespace platform

#endif  // __SDK_XCVR_SFP_HPP__
