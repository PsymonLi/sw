//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains pfmapp utility functions
///
//----------------------------------------------------------------------------

#include <stdio.h>
#include "nic/sdk/platform/sensor/sensor_api.h"
#include "nic/athena/test/api/platform/utils.hpp"

namespace pfmapp {
void
print_temperature(system_temperature_t *temperature)
{
    printf("Temperature values\n");
    printf("%-25s %u C\n"
           "%-25s %u C\n"
           "%-25s %u C\n"
           "%-25s %u C\n"
           "%-25s %u C\n\n",
           "Die Temperature", temperature->dietemp,
           "Local Temperature", temperature->localtemp,
           "HBM Temperature", temperature->hbmtemp,
           "HBM Warning Temperature", temperature->hbmwarningtemp,
           "HBM Critical Temperature", temperature->hbmcriticaltemp);
    return;
}

void
print_voltage(system_voltage_t *voltage)
{
    printf("Voltage values\n");
    printf("%-25s %u mV\n"
           "%-25s %u mV\n"
           "%-25s %u mV\n\n",
           "Voltage In", voltage->vin,
           "Voltage Out1", voltage->vout1,
           "Voltage Out2", voltage->vout2);
    return;
}

void
print_power(system_power_t *power)
{
    printf("Power values\n");
    printf("%-25s %u mW\n"
           "%-25s %u mW\n"
           "%-25s %u mW\n\n",
           "Power In", power->pin/1000,
           "Power Out1", power->pout1/1000,
           "Power Out2", power->pout2/1000);
    return;
}

}    // namespace pfmapp
