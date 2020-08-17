//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains C API wrappers for sensor module
///
//----------------------------------------------------------------------------

#ifndef __SENSOR_API_H__
#define __SENSOR_API_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "include/sdk/base.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief temperature readings from sensors, in degrees Celsius
typedef struct system_temperature_s {
    int localtemp;                          ///< local temperature
    int dietemp;                            ///< die temperature
    int hbmtemp;                            ///< HBM temperature
    int hbmwarningtemp;                     ///< HBM warning threshold temperature
    int hbmcriticaltemp;                    ///< HBM critical threshold temperature
} __PACK__ system_temperature_t;

/// \brief power I/O readings, in microwatts
typedef struct system_power_s {
    int pin;                        ///< input power
    int pout1;                      ///< output power1
    int pout2;                      ///< output power2
} __PACK__ system_power_t;

/// \brief voltage I/O readings, in millivolts
typedef struct system_voltage_s {
    int vin;                        ///< input voltage in millivolts
    int vout1;                      ///< output voltage1 in millivolts
    int vout2;                      ///< output voltage2 in millivolts
} __PACK__ system_voltage_t;

/// \brief  read system temperatures
/// \param[out] temperature read values
/// \return 0 on success, #EINVAL on error
int dsc_sys_temperatures_read(system_temperature_t *temperature);

/// \brief  read system voltages
/// \param[out] voltage read values
/// \return 0 on success, #EINVAL on error
int dsc_sys_voltages_read(system_voltage_t *voltage);

/// \brief  read system powers
/// \param[out] power read values
/// \return 0 on success, #EINVAL on error
int dsc_sys_powers_read(system_power_t *power);

#ifdef __cplusplus
}
#endif
#endif    // __SENSOR_API_H__
