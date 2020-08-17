//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains test routines for card monitoring API
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <assert.h>
#include "lib/pal/pal.hpp"
#include "nic/sdk/platform/sensor/sensor_api.h"
#include "nic/athena/test/api/platform/utils.hpp"

using namespace pfmapp;

// Call sensor read api's
static void
sensor_read_test (void)
{
    int                     ret;
    system_temperature_t    temperature = { 0 };
    system_voltage_t        voltage = { 0 };
    system_power_t          power = { 0 };

    ret = dsc_sys_temperatures_read(&temperature);
    printf("API: %20s returned %d\n", "dsc_sys_temperatures_read()", ret);
    if (ret == 0) {
        print_temperature(&temperature);
    }

    ret = dsc_sys_voltages_read(&voltage);
    printf("API: %20s returned %d\n", "dsc_sys_voltages_read()", ret);
    if (ret == 0) {
        print_voltage(&voltage);
    }

    ret = dsc_sys_powers_read(&power);
    printf("API: %20s returned %d\n", "dsc_sys_powers_read()", ret);
    if (ret == 0) {
        print_power(&power);
    }

    return;
}

//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------

/// @private
int
main (int argc, char **argv)
{
    assert(sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HAPS) == sdk::lib::PAL_RET_OK);
    sensor_read_test();
    return 0;
}
