/*
 * Copyright (c) 2017-2018, Pensando Systems Inc.
 */

#ifndef __SENSOR_HPP__
#define __SENSOR_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "include/sdk/base.hpp"

namespace sdk {
namespace platform {
namespace sensor {

#define PIN_INPUT_FILE "/sys/class/hwmon/hwmon1/power1_input"
#define POUT1_INPUT_FILE "/sys/class/hwmon/hwmon1/power2_input"
#define POUT2_INPUT_FILE "/sys/class/hwmon/hwmon1/power3_input"
#define LOCAL_TEMP_FILE "/sys/class/hwmon/hwmon0/temp1_input"
#define DIE_TEMP_FILE "/sys/class/hwmon/hwmon0/temp2_input"

//Read temperature functions will fill the value in millidegrees
int read_die_temperature(uint64_t *dietemp);
int read_local_temperature(uint64_t *localtemp);

//Read power functions will fill the value in microwatts
int read_pin(uint64_t *pin);
int read_pout1(uint64_t *pout1);
int read_pout2(uint64_t *pout2);

} // namespace sensor
} // namespace platform
} // namespace sdk

#endif /* __SENSOR_HPP__ */
