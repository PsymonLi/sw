// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

#ifndef __XCVR_HPP__
#define __XCVR_HPP__

#include <cstring>
#include "include/sdk/types.hpp"
#include "linkmgr/linkmgr.hpp"
#include "lib/pal/pal.hpp"
#include "xcvr_utils.hpp"

namespace sdk {
namespace platform {

#define XCVR_SPROM_CACHE_SIZE   512
#define XCVR_MAX_PORTS          2
#define XCVR_SPROM_READ_MAX     10
#define XCVR_SPROM_READ_SIZE    128

#define AN_USER_CAP_10GBKR     0x4
#define AN_USER_CAP_40GBKR4    0x8
#define AN_USER_CAP_40GBCR4    0x10
#define AN_USER_CAP_100GBKP4   0x40
#define AN_USER_CAP_100GBKR4   0x80
#define AN_USER_CAP_100GBCR4   0x100
#define AN_USER_CAP_25GBKRCR_S 0x200
#define AN_USER_CAP_25GBKRCR   0x400

#define AN_FEC_REQ_25GB_RSFEC 0x2
#define AN_FEC_REQ_25GB_FCFEC 0x4

#define XCVR_SPROM_LEN_SMFKM_OFFSET           14
#define XCVR_SPROM_LEN_OM3_QSFP_OFFSET        15
#define XCVR_SPROM_LEN_OM2_OFFSET             16
#define XCVR_SPROM_LEN_OM1_OFFSET             17
#define XCVR_SPROM_LEN_DAC_OFFSET             18
#define XCVR_SPROM_LEN_OM3_SFP_OFFSET         19
#define XCVR_SPROM_VENDOR_NAME_START_OFFSET   20
#define XCVR_SPROM_VENDOR_NAME_END_OFFSET     35
#define XCVR_SPROM_OUI_OFFSET1                37
#define XCVR_SPROM_OUI_OFFSET2                38
#define XCVR_SPROM_OUI_OFFSET3                39
#define XCVR_SPROM_VENDOR_PART_START_OFFSET   40
#define XCVR_SPROM_VENDOR_PART_END_OFFSET     55
#define XCVR_SPROM_VENDOR_REV_START_OFFSET    56
#define XCVR_SPROM_VENDOR_REV_QSFP_END_OFFSET 57
#define XCVR_SPROM_VENDOR_REV_SFP_END_OFFSET  59
#define XCVR_SPROM_VENDOR_SERIAL_START_OFFSET 68
#define XCVR_SPROM_VENDOR_SERIAL_END_OFFSET   83

#define ASCII_VALID_START_VALUE 32     // SPACE character
#define ASCII_VALID_END_VALUE   126    // ~ character

using sdk::linkmgr::xcvr_event_notify_t;
using sdk::linkmgr::port_args_t;
using sdk::types::xcvr_type_t;
using sdk::types::xcvr_pid_t;
using sdk::types::cable_type_t;
using sdk::types::xcvr_state_t;

extern bool xcvr_valid_enable;

// QSFP/QSFP28 sprom data
typedef struct qsfp_sprom_data_s {
    uint8_t  id;               //  0 Type of transceiver
    uint8_t  ext_id;           //  1 Extended identifier of type of transceiver
    uint8_t  connector;        //  2 Code for connector type
    uint8_t  compliance[8];    //  3 Code for electronic or optical compatibility
    uint8_t  encoding;         // 11 Code for high speed serial encoding algorithm
    uint8_t  br_nominal1;      // 12 Norminal signalling rate, units of 100MBd
    uint8_t  ext_rate_select;  // 13 Extended rate select compliance
    uint8_t  length_smf_km;    // 14 Link length supported for single mode fiber, units of km
    uint8_t  length_om3;       // 15 Link length supported for 50/125um   OM3 fiber, units of 2m
    uint8_t  length_om2;       // 16 Link length supported for 50/125um   OM2 fiber, units of 1m
    uint8_t  length_om1;       // 17 Link length supported for 62.5/125um OM1 fiber, units of 1m
    uint8_t  length_dac;       // 18 Link length supported for copper or direct attach cable, units of m
    uint8_t  device_tech;      // 19 Device technology
    uint8_t  vendor_name[16];  // 20 SFP vendor name
    uint8_t  ext_module;       // 36 Extended module codes for Infiniband
    uint8_t  vendor_oui[3];    // 37 SFP vendor IEEE company ID
    uint8_t  vendor_pn[16];    // 40 Part number provided by vendor
    uint8_t  vendor_rev[2];    // 56 Revision number for part number provided by vendor
    uint8_t  wavelength1[2];   // 58 Nominal laser wavelength OR copper cable attenuation
    uint8_t  wavelength2[2];   // 60 Guaranteed range of laser wavelength from nominal wavelength or copper cable attenuation
    uint8_t  max_case_temp;    // 62
    uint8_t  cc_base;          // 63 Check code for Base ID Fields

    // Extended ID Fields
    uint8_t  link_codes;       // 64 extended specification compliance codes
    uint8_t  options[3];       // 65 Indicates which optional transceiver signals are implemented
    uint8_t  vendor_sn[16];    // 68 Serial number provided by vendor
    uint8_t  date_code[8];     // 84 Vendor's manufacturing data code
    uint8_t  diag_mon_type;    // 92 Indicates which type of diagnostic monitoring is implemented
    uint8_t  enhanced_options; // 93 Indicates which optional enhanced features are implemented
    uint8_t  br_nominal2;      // 94 Nominal bit rate per channel
    uint8_t  cc_ext;           // 95 Check code for Extended ID Fields

    // Vendor Specific Fields
    uint8_t  vendor_specific[32]; // 96 Vendor specific EEPROM
} __attribute__((__packed__)) qsfp_sprom_data_t;

// SFP/SFP+ sprom data
typedef struct sfp_sprom_data_s {
    uint8_t  id;               //  0 Type of transceiver
    uint8_t  ext_id;           //  1 Extended identifier of type of transceiver
    uint8_t  connector;        //  2 Code for connector type
    uint8_t  compliance[8];    //  3 Code for electronic or optical compatibility
    uint8_t  encoding;         // 11 Code for high speed serial encoding algorithm
    uint8_t  br_nominal;       // 12 Norminal signalling rate, units of 100MBd
    uint8_t  rate_identifier;  // 13 Type of rate select functionality
    uint8_t  length_smf_km;    // 14 Link length supported for single mode fiber, units of km
    uint8_t  length_smf;       // 15 Link length supported for single mode fiber, units of 100m
    uint8_t  length_om2;       // 16 Link length supported for 50um   OM2  fiber, units of 10m
    uint8_t  length_om1;       // 17 Link length supported for 62.5um OM1  fiber, units of 10m
    uint8_t  length_dac;       // 18 Link length supported for copper or direct attach cable, units of m
    uint8_t  length_om3;       // 19 Link length supported for 50um   OM3  fiber, units of 10m
    uint8_t  vendor_name[16];  // 20 SFP vendor name
    uint8_t  transceiver;      // 36 Code for electronic or optical compatibility
    uint8_t  vendor_oui[3];    // 37 SFP vendor IEEE company ID
    uint8_t  vendor_pn[16];    // 40 Part number provided by vendor
    uint8_t  vendor_rev[4];    // 56 Revision number for part number provided by vendor
    uint8_t  wavelength[2];    // 60 Laser wavelength
    uint8_t  unallocated;      // 62
    uint8_t  cc_base;          // 63 Check code for Base ID Fields

    // Extended ID Fields
    uint8_t  options[2];       // 64 Indicates which optional transceiver signals are implemented
    uint8_t  br_max;           // 66 Upper bit rate margin, units of %
    uint8_t  br_min;           // 67 Lower bit rate margin, units of %
    uint8_t  vendor_sn[16];    // 68 Serial number provided by vendor
    uint8_t  date_code[8];     // 84 Vendor's manufacturing data code
    uint8_t  diag_mon_type;    // 92 Indicates which type of diagnostic monitoring is implemented
    uint8_t  enhanced_options; // 93 Indicates which optional enhanced features are implemented
    uint8_t  sff_compliance;   // 94 Indicates which revision of SFF 8472 transceiver complies with
    uint8_t  cc_ext;           // 95 Check code for Extended ID Fields

    // Vendor Specific Fields
    uint8_t  vendor_specific[32]; // 96 Vendor specific EEPROM
} __attribute__((__packed__)) sfp_sprom_data_t;

typedef struct xcvr_s {
    xcvr_type_t    type;         // QSFP28, QSFP, SFP
    cable_type_t   cable_type;   // CU, Fiber
    xcvr_state_t   state;
    xcvr_pid_t     pid;
    uint8_t        sprom_read_count;
    bool           debounce;
    port_an_args_t port_an_args;
    port_speed_t   cable_speed;     // speed of inserted xcvr
    port_fec_type_t fec_type;       // default fec type for xcvr
    uint32_t length_om3;
    uint32_t length_smfkm;
    uint32_t length_om2;
    uint32_t length_om1;
    uint32_t length_dac;

    // Transceiver data
    uint8_t        cache[XCVR_SPROM_CACHE_SIZE];
} __PACK__ xcvr_t;

typedef struct upg_obj_meta_s {
    uint64_t num_entries;       ///< number of stored structs
    uint64_t entry_size;        ///< size of each struct
    uint64_t reserved1;         ///< reserved field
    uint64_t reserved2;         ///< reserved field
    uint64_t reserved3;         ///< reserved field
    uint8_t  obj[0];            ///< start offset of structs
} upg_obj_meta_t;

extern xcvr_t *g_xcvr[XCVR_MAX_PORTS];

inline void
xcvr_reset (int port) {
    memset(g_xcvr[port], 0, sizeof(xcvr_t));
}

inline void
xcvr_cache_reset (int port) {
    memset(g_xcvr[port]->cache, 0, sizeof(uint8_t) * XCVR_SPROM_CACHE_SIZE);
}

inline xcvr_state_t
xcvr_state (int port) {
    return g_xcvr[port]->state;
}

inline void
xcvr_set_state (int port, xcvr_state_t state) {
    g_xcvr[port]->state = state;
}

inline uint8_t *
xcvr_cache (int port) {
    return g_xcvr[port]->cache;
}

inline void
xcvr_set_cache (int port, uint8_t *data, int len) {
    memcpy(g_xcvr[port]->cache, data, len);
}

inline void
xcvr_set_pid (int port, xcvr_pid_t pid) {
    g_xcvr[port]->pid = pid;
}

inline xcvr_pid_t
xcvr_pid (int port) {
    return g_xcvr[port]->pid;
}

inline xcvr_type_t
xcvr_type (int port) {
    return g_xcvr[port]->type;
}

inline bool
xcvr_get_debounce (int port) {
    return g_xcvr[port]->debounce;
}

inline void
xcvr_set_debounce (int port) {
    g_xcvr[port]->debounce = true;
}

inline void
xcvr_reset_debounce (int port) {
    g_xcvr[port]->debounce = false;
}

inline void
set_cable_type (int port, cable_type_t cable_type)
{
    g_xcvr[port]->cable_type = cable_type;
}

inline cable_type_t
cable_type (int port)
{
    return g_xcvr[port]->cable_type;
}

inline port_an_args_t*
xcvr_get_an_args (int port)
{
    return &g_xcvr[port]->port_an_args;
}

inline void
xcvr_set_an_args (int port, uint32_t user_cap,
                  bool fec_ability, uint32_t fec_request)
{
    port_an_args_t *port_an_args = xcvr_get_an_args(port);
    port_an_args->user_cap    = user_cap;
    port_an_args->fec_ability = fec_ability;
    port_an_args->fec_request = fec_request;
}

inline void
xcvr_set_type (int port, xcvr_type_t type) {
    g_xcvr[port]->type = type;
}

inline bool
xcvr_sprom_read_count_inc (int port) {
    g_xcvr[port]->sprom_read_count ++;
    if (g_xcvr[port]->sprom_read_count == XCVR_SPROM_READ_MAX) {
        g_xcvr[port]->sprom_read_count = 0;
        return false;
    }
    return true;
}

void xcvr_poll_timer(void);
bool xcvr_valid(int port);
sdk_ret_t xcvr_get(int port, xcvr_event_info_t *xcvr_event_info);
sdk_ret_t xcvr_enable(int port, bool enable, uint8_t mask);

/// \brief     initialize the transceiver info callback and memory
/// \param[in] xcvr_notify_cb callback to be invoked for transceiver events
/// \param[in] mem pointer to memory
/// \param[in] backup_mem pointer to backup memory
/// \return    SDK_RET_OK on success, failure status code on error
sdk_ret_t xcvr_init(xcvr_event_notify_t xcvr_notify_cb, void *mem,
                    void *backup_mem);

static inline bool
xcvr_valid_check_enabled (void)
{
    return xcvr_valid_enable;
}

static inline void
xcvr_set_valid_check (bool enable)
{
    xcvr_valid_enable = enable;
}

static inline port_speed_t
cable_speed (int port)
{
    return g_xcvr[port]->cable_speed;
}

static inline void
xcvr_set_cable_speed (int port, port_speed_t speed)
{
    g_xcvr[port]->cable_speed = speed;
}

static inline void
xcvr_set_fec_type (int port, port_fec_type_t fec_type)
{
    g_xcvr[port]->fec_type = fec_type;
}

static inline port_fec_type_t
xcvr_fec_type (int port)
{
    return g_xcvr[port]->fec_type;
}

static inline uint32_t
xcvr_length_dac (int port)
{
    return g_xcvr[port]->length_dac;
}

static inline void
xcvr_parse_length (int port, uint8_t *data)
{
    xcvr_t *xcvr = g_xcvr[port];

    xcvr->length_om3 = 0;
    xcvr->length_smfkm = data[XCVR_SPROM_LEN_SMFKM_OFFSET];
    xcvr->length_om2 = data[XCVR_SPROM_LEN_OM2_OFFSET];
    xcvr->length_om1 = data[XCVR_SPROM_LEN_OM1_OFFSET];
    xcvr->length_dac = data[XCVR_SPROM_LEN_DAC_OFFSET];

    if (xcvr_type(port) == xcvr_type_t::XCVR_TYPE_SFP) {
        xcvr->length_om3 = data[XCVR_SPROM_LEN_OM3_SFP_OFFSET];

		// convert from units of 10m to meters
        xcvr->length_om1 *= 10;
        xcvr->length_om2 *= 10;
        xcvr->length_om3 *= 10;
    } else {
		// convert from units of 2m to meters
        xcvr->length_om3 = data[XCVR_SPROM_LEN_OM3_QSFP_OFFSET] * 2;
    }
}

static inline string
xcvr_extract_ascii (const uint8_t *xcvr_sprom, uint32_t first, uint32_t last)
{
    std::string str;
    uint32_t val, reg;

    while (first <= last && xcvr_sprom[last] == ' ')
        last--;
    for (reg = first; reg <= last; reg++) {
        val = xcvr_sprom[reg];
        str.append(1u, putchar(((val >= ASCII_VALID_START_VALUE) &&
                   (val <= ASCII_VALID_END_VALUE)) ? val : '_'));
    }
    return str;
}

static inline string
xcvr_vendor_name (const uint8_t *xcvr_sprom)
{
    return xcvr_extract_ascii(xcvr_sprom, XCVR_SPROM_VENDOR_NAME_START_OFFSET,
                              XCVR_SPROM_VENDOR_NAME_END_OFFSET);
}

static inline string
xcvr_vendor_oui (const uint8_t *xcvr_sprom)
{
    char oui[32];
    string oui_str;

    std::snprintf(oui, 32, "%02x:%02x:%02x", xcvr_sprom[XCVR_SPROM_OUI_OFFSET1],
                  xcvr_sprom[XCVR_SPROM_OUI_OFFSET2],
                  xcvr_sprom[XCVR_SPROM_OUI_OFFSET3]);
    oui_str = oui;
    return oui_str;
}

static inline string
xcvr_vendor_serial_number (const uint8_t *xcvr_sprom)
{
    return xcvr_extract_ascii(xcvr_sprom,
                              XCVR_SPROM_VENDOR_SERIAL_START_OFFSET,
                              XCVR_SPROM_VENDOR_SERIAL_END_OFFSET);
}

static inline string
xcvr_vendor_part_number (const uint8_t *xcvr_sprom)
{
    return xcvr_extract_ascii(xcvr_sprom, XCVR_SPROM_VENDOR_PART_START_OFFSET,
                              XCVR_SPROM_VENDOR_PART_END_OFFSET);
}

static inline string
xcvr_vendor_revision (uint32_t phy_port, const uint8_t *xcvr_sprom)
{
    sdk::types::xcvr_type_t xtype;

    xtype = xcvr_type(phy_port - 1);
    if (xtype == xcvr_type_t::XCVR_TYPE_QSFP ||
        xtype == xcvr_type_t::XCVR_TYPE_QSFP28) {
        return xcvr_extract_ascii(xcvr_sprom,
                                  XCVR_SPROM_VENDOR_REV_START_OFFSET,
                                  XCVR_SPROM_VENDOR_REV_QSFP_END_OFFSET);
    } else {
        return xcvr_extract_ascii(xcvr_sprom,
                                  XCVR_SPROM_VENDOR_REV_START_OFFSET,
                                  XCVR_SPROM_VENDOR_REV_SFP_END_OFFSET);
    }
}

/// \brief  returns the size to be allocate for all transceiver ports including
///         meta info
/// \return size of the transceiver info for all transceiver ports including
///         meta info
uint64_t xcvr_mem_size(void);

/// \brief      reads the DOM info for SFP/QSFP
///             For SFP, reads 256 bytes from 0xA2(0x51) I2C address
///             For qsfp reads 128 bytes from lower page 0x00 and
///             128 bytes from upper page 0x03
/// \param[in]  port transceiver port
/// \param[out] data pointer to buffer to fill DOM info
/// \return     SDK_RET_OK on success, failure status code on error
sdk_ret_t xcvr_read_dom(int port, uint8_t *data);

} // namespace platform
} // namespace sdk

#endif
