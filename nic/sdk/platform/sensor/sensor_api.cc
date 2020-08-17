//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains C API wrappers for sensor module
///
//----------------------------------------------------------------------------

#include "platform/sensor/sensor.hpp"
#include "platform/sensor/sensor_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// API to read all system temperatures
int
dsc_sys_temperatures_read (system_temperature_t *temperature)
{
    return sdk::platform::sensor::read_temperatures(temperature);
}

// API to read all system power values
int
dsc_sys_powers_read (system_power_t *power)
{
    return sdk::platform::sensor::read_powers(power);
}

// API to read all system voltage values
int
dsc_sys_voltages_read (system_voltage_t *voltage)
{
    return sdk::platform::sensor::read_voltages(voltage);
}

#ifdef __cplusplus
}
#endif
