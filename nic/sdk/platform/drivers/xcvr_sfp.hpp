// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __SDK_XCVR_SFP_HPP__
#define __SDK_XCVR_SFP_HPP__

namespace sdk {
namespace platform {

#define SFP_OFFSET_NOMINAL_BR                12
#define SFP_OFFSET_LENGTH_CU                 18
#define SFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES 36
#define SFP_OFFSET_MAX_BR                    66
#define SFP_OFFSET_MAX_BR_UNITS              250

typedef enum sfp_bitrate_e {
    SFP_BITRATE_10G,
    SFP_BITRATE_25G
} sfp_bitrate_t;

static inline sfp_bitrate_t
sfp_bitrate (uint8_t *data)
{
    switch (data[SFP_OFFSET_NOMINAL_BR]) {
    case 0xFF:
        if ((data[SFP_OFFSET_MAX_BR] * SFP_OFFSET_MAX_BR_UNITS == 25750) ||
                (data[SFP_OFFSET_MAX_BR] * SFP_OFFSET_MAX_BR_UNITS == 25500)) {
            return SFP_BITRATE_25G;
        }
        break;
    default:
        break;
    }
    return SFP_BITRATE_10G;
}

static inline void
sfp_25g_an_params (int port, uint32_t *tech_ability, uint32_t *fec_request)
{
    if (xcvr_length_dac(port) <= 3) {
        *tech_ability = AN_USER_CAP_25GBKRCR_S;
        *fec_request = AN_FEC_REQ_25GB_FCFEC;
    } else {
        *tech_ability = AN_USER_CAP_25GBKRCR_S | AN_USER_CAP_25GBKRCR;
        *fec_request = AN_FEC_REQ_25GB_RSFEC | AN_FEC_REQ_25GB_FCFEC;
    }
}

static inline sdk_ret_t
sfp_sprom_parse (int port, uint8_t *data)
{
    uint32_t tech_ability;
    uint32_t fec_request;

    // SFF 8472

    if (data[SFP_OFFSET_LENGTH_CU] != 0) {
        set_cable_type(port, cable_type_t::CABLE_TYPE_CU);
    } else {
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
    }

    // TODO reset when removed?
    xcvr_set_an_args(port, 0, false, 0);

    // default fec type = none
    xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_NONE);

    // parse the length fields
    xcvr_parse_length(port, data);

    switch (data[7]) {
    case 0:
        if (data[8] == 0x4) {
            // 10G Base CU
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_CU);
            xcvr_set_an_args(port, AN_USER_CAP_10GBKR, false, 0x0);
            set_cable_type(port, cable_type_t::CABLE_TYPE_CU);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        } else if (data[8] == 0x8) {
            // 10G Base AOC
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_AOC);
            set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
        }
        break;

    default:
        break;
    }

    if (data[2] == 0x21) {
        // HPE 10G DAC cables has byte3=0x81, byte8=0x0 and hence were being
        // detected as 10G ER.
        // Add byte2=0x21 check for it.
        // 10G Base CU
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_10GBASE_CU);
        xcvr_set_an_args(port, AN_USER_CAP_10GBKR, false, 0x0);
        set_cable_type(port, cable_type_t::CABLE_TYPE_CU);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
    } else if (data[3] & (1 << 4)) {
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

    switch (data[SFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES]) {
    case 0x0:
        SDK_TRACE_DEBUG("Xcvr port %d SFP ext spec unspecified compliance code "
                        "0x0", port);
        // MOLEX 25G cables has byte2=0x21, byte8=0x4 and hence were being
        // detected as 10G.
        // Add bitrate check for 25G
        if (sfp_bitrate(data) == SFP_BITRATE_25G) {
            // 25GBASE-CR CA-L
            xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_L);
            sfp_25g_an_params(port, &tech_ability, &fec_request);
            xcvr_set_an_args(port, tech_ability, true, fec_request);
            xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        }
        break;
    case 0x1:
        // 25GAUI C2M AOC - BER 5x10^(-5)
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_AOC);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);

        // for 25G AOC, if length >= 3, pick RS-FEC, else pick FC-FEC
        // AOC is active CU. check length_dac
        if (xcvr_length_dac(port) >= 3) {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        } else {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        }
        break;

    case 0x2:
        // 25GBASE-SR
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_SR);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        break;

    case 0x3:
        // 25GBASE-LR
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_LR);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        break;

    case 0x4:
        // 25GBASE-ER
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_ER);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        break;

    case 0x8:
        // 25GAUI C2M ACC - BER 5x10^(-5)
        break;

    case 0xb:
        // 25GBASE-CR CA-L
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_L);
        sfp_25g_an_params(port, &tech_ability, &fec_request);
        xcvr_set_an_args(port, tech_ability, true, fec_request);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        break;

    case 0xc:
        // 25GBASE-CR CA-S
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_S);
        sfp_25g_an_params(port, &tech_ability, &fec_request);
        xcvr_set_an_args(port, tech_ability, true, fec_request);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        break;

    case 0xd:
        // 25GBASE-CR CA-N
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_CR_N);
        sfp_25g_an_params(port, &tech_ability, &fec_request);
        xcvr_set_an_args(port, tech_ability, true, fec_request);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);
        xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        break;

    case 0x16:
        // 10GBASE-T
        break;

    case 0x18:
        // 25GAUI C2M AOC - BER 10^(-12)
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_AOC);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);

        // for 25G AOC, if length >= 3, pick RS-FEC, else pick FC-FEC
        // AOC is active CU. check length_dac
        if (xcvr_length_dac(port) >= 3) {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        } else {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        }
        break;

    case 0x19:
        // 25GAUI C2M ACC - BER 10^(-12)
        xcvr_set_pid(port, xcvr_pid_t::XCVR_PID_SFP_25GBASE_ACC);
        set_cable_type(port, cable_type_t::CABLE_TYPE_FIBER);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_25G);

        // for 25G ACC, if length >= 3, pick RS-FEC, else pick FC-FEC
        if (xcvr_length_dac(port) >= 3) {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_RS);
        } else {
            xcvr_set_fec_type(port, port_fec_type_t::PORT_FEC_TYPE_FC);
        }
        break;

    default:
        SDK_TRACE_DEBUG("Xcvr port %u SFP ext spec code %u not supported yet",
                         port, data[SFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES]);
        break;
    }

    // For unknown SFP, set 10G CU params
    if (xcvr_pid(port) == xcvr_pid_t::XCVR_PID_UNKNOWN) {
        // 10G Base CU
        set_cable_type(port, cable_type_t::CABLE_TYPE_CU);
        xcvr_set_cable_speed(port, port_speed_t::PORT_SPEED_10G);
    }

    return SDK_RET_OK;
}

/// \brief      reads the DOM info for SFP
///             reads 256 bytes from 0xA2(0x51) I2C address
/// \param[in]  port transceiver port
/// \param[out] data pointer to buffer to fill DOM info
/// \return     SDK_RET_OK on success, failure status code on error
static inline sdk_ret_t
sfp_read_dom (int port, uint8_t *data)
{
    pal_ret_t ret;

    ret = sdk::lib::qsfp_dom_read(data, XCVR_SPROM_SIZE, 0x0,
                                  MAX_XCVR_ACCESS_RETRIES,
                                  port + 1);
    if (ret == sdk::lib::PAL_RET_NOK) {
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

} // namespace sdk
} // namespace platform

#endif  // __SDK_XCVR_SFP_HPP__
