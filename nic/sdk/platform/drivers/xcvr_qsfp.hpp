// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __XCVR_QSFP_HPP__
#define __XCVR_QSFP_HPP__

#include "lib/pal/pal.hpp"

namespace sdk {
namespace platform {

#define QSFP_MODULE_TEMPERATURE               22    ///< low page 00
#define QSFP_MODULE_TEMP_HIGH_ALARM           128   ///< upper page 03
#define QSFP_MODULE_TEMP_HIGH_WARNING         132   ///< upper page 03
#define QSFP_OFFSET_TX_DISABLE                0x56
#define QSFP_OFFSET_LENGTH_CU                 146   ///< 0x92
#define QSFP_OFFSET_ETH_COMPLIANCE_CODES      131
#define QSFP_OFFSET_EXT_SPEC_COMPLIANCE_CODES 192

using sdk::lib::qsfp_page_t;

sdk_ret_t qsfp_read_page(int port, qsfp_page_t pgno, int offset,
                         int num_bytes, uint8_t *data);
sdk_ret_t qsfp_enable(int port, bool enable, uint8_t mask);
sdk_ret_t qsfp_sprom_parse(int port, uint8_t *data);

/// \brief      parses the qsfp dom info
/// \param[in]  sprom qsfp dom info
///             (128 bytes lower page 00 + 128 bytes upper page 03)
/// \param[out] xcvr_temp pointer to transceiver temperature info
/// \return     SDK_RET_OK on success, failure status code on error
sdk_ret_t parse_qsfp_temperature(uint8_t *sprom,
                                 xcvr_temperature_t *xcvr_temp);

/// \brief      reads the DOM info for QSFP
///             reads 128 bytes from lower page 0x00 and
///             128 bytes from upper page 0x03
/// \param[in]  port transceiver port
/// \param[out] data pointer to buffer to fill DOM info
/// \return     SDK_RET_OK on success, failure status code on error
sdk_ret_t qsfp_read_dom(int port, uint8_t *data);

} // namespace platform
} // namespace sdk

#endif
