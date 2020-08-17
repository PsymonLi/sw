/*
 * Copyright (c) 2019, Pensando Systems Inc.
*/

#include "asic/pd/pd.hpp"
#include "sysmon_internal.hpp"
#include "platform/sensor/sensor.hpp"
#include "platform/drivers/xcvr_qsfp.hpp"

using namespace sdk::asic::pd;
using namespace sdk::platform::sensor;

static pd_adjust_perf_index_t perf_id = PD_PERF_ID0;
int startingfrequency_1100 = 0;

#define FREQUENCY_FILE "/sysconfig/config0/frequency.json"
#define FREQUENCY_KEY "frequency"

#define HBM_TEMP_LOWER_LIMIT 85
#define HBM_TEMP_UPPER_LIMIT 95

// Hystersis to control auxilary fan based on HBM temperature.
#define AUX_FAN_TEMP_HYSTERESIS 10

static void
cpld_temp_update (system_temperature_t *temperature,
                  sdk::platform::qsfp_temperature_t *xcvrtemp)
{
    static uint32_t aux_fan_state;
    uint32_t num_phy_ports;

    pal_write_core_temp(temperature->dietemp);
    pal_write_board_temp(temperature->localtemp);
    pal_write_hbm_temp(temperature->hbmtemp);
    pal_write_hbmwarning_temp(temperature->hbmwarningtemp);
    pal_write_hbmcritical_temp(temperature->hbmcriticaltemp);
    pal_write_hbmfatal_temp(g_sysmon_cfg.catalog->hbmtemperature_threshold());

    // walk fp ports
    num_phy_ports = g_sysmon_cfg.catalog->num_fp_ports();
    for (uint32_t port = 1; port <= num_phy_ports; port++) {
        if (g_sysmon_cfg.catalog->port_type_fp(port)
            != port_type_t::PORT_TYPE_MGMT) {
            // adding place holders for updating qsfp temperature to cpld
            pal_write_qsfp_temp(xcvrtemp[port-1].temperature, port);
            pal_write_qsfp_alarm_temp(xcvrtemp[port-1].alarm_temperature, port);
            pal_write_qsfp_warning_temp(xcvrtemp[port-1].warning_temperature,
                                        port);
        }
    }

    if (g_sysmon_cfg.catalog->aux_fan()) {
        if (aux_fan_state == 0 &&
            temperature->hbmtemp > g_sysmon_cfg.catalog->aux_fan_threshold()) {
            aux_fan_state = 1;
            pal_enable_auxiliary_fan();
        } else if ((aux_fan_state == 1) &&
                   (temperature->hbmtemp <
                   (g_sysmon_cfg.catalog->aux_fan_threshold()
                   - AUX_FAN_TEMP_HYSTERESIS))) {
            aux_fan_state = 0;
            pal_disable_auxiliary_fan();
        }
    }
    return;
}

static void
changefrequency (uint64_t hbmtemperature)
{
    sdk_ret_t ret;
    int chip_id = 0;
    int inst_id = 0;

    if (hbmtemperature <= HBM_TEMP_LOWER_LIMIT) {
        ret = asicpd_adjust_perf(chip_id, inst_id, perf_id, PD_PERF_UP);
        if (ret == SDK_RET_OK) {
            SDK_HMON_TRACE_INFO("Increased the frequency.");
        } else {
            if (perf_id != PD_PERF_ID4) {
                SDK_HMON_TRACE_ERR("Unable to change the frequency failed, "
                                   "perf_id is %u", perf_id);
            }
        }
    } else if (hbmtemperature >= HBM_TEMP_UPPER_LIMIT) {
        ret = asicpd_adjust_perf(chip_id, inst_id, perf_id, PD_PERF_DOWN);
        if (ret == SDK_RET_OK) {
            SDK_HMON_TRACE_INFO("Decreased the frequency.");
        } else {
            if (perf_id != PD_PERF_ID0) {
                SDK_HMON_TRACE_ERR("Unable to change the frequency failed, "
                                   "perf_id is %u", perf_id);
            }
        }
    } else {
        return;
    }
}

void
checktemperature(void)
{
    int ret;
    int chip_id = 0;
    int inst_id = 0;
    static int max_die_temp;
    static int max_local_temp;
    static int max_hbm_temp;
    uint32_t num_phy_ports;
    static sysmon_hbm_threshold_event_t prev_hbmtemp_event;
    sysmon_hbm_threshold_event_t hbmtemp_event;
    system_temperature_t temperature;
    sdk::platform::qsfp_temperature_t xcvrtemp[MAX_FP_PORTS];

    ret = read_temperatures(&temperature);
    if (!ret) {
        if (max_die_temp < temperature.dietemp) {
            SDK_HMON_TRACE_INFO("%s is : %uC",
                                "Die temperature", temperature.dietemp);
            max_die_temp = temperature.dietemp;
        }

        if (max_local_temp < temperature.localtemp) {
            SDK_HMON_TRACE_INFO("%s is : %uC",
                                "Local temperature", temperature.localtemp);
            max_local_temp = temperature.localtemp;
        }

        if (max_hbm_temp < temperature.hbmtemp) {
            SDK_HMON_TRACE_INFO("HBM temperature is : %uC",
                                temperature.hbmtemp);
            max_hbm_temp = temperature.hbmtemp;
        }

        num_phy_ports = g_sysmon_cfg.catalog->num_fp_ports();
        for (uint32_t phy_port = 1; phy_port <= num_phy_ports; phy_port++) {
            if (g_sysmon_cfg.catalog->port_type_fp(phy_port)
                != port_type_t::PORT_TYPE_MGMT) {
                sdk::platform::read_qsfp_temperature(phy_port - 1,
                                                     &xcvrtemp[phy_port-1]);
            }
        }

        if (startingfrequency_1100 == 1) {
            changefrequency(temperature.hbmtemp);
        }
        if ((temperature.hbmtemp >=
                 g_sysmon_cfg.catalog->hbmtemperature_threshold()) &&
            (prev_hbmtemp_event != SYSMON_HBM_TEMP_ABOVE_THRESHOLD)) {
            SDK_HMON_TRACE_ERR(
                "HBM temperature is : %uC ***, threshold is %u",
                temperature.hbmtemp,
                g_sysmon_cfg.catalog->hbmtemperature_threshold());
            SDK_HMON_TRACE_INFO(
                "HBM temperature is : %uC ***, threshold is %u",
                temperature.hbmtemp,
                g_sysmon_cfg.catalog->hbmtemperature_threshold());
            hbmtemp_event = SYSMON_HBM_TEMP_ABOVE_THRESHOLD;
            prev_hbmtemp_event = SYSMON_HBM_TEMP_ABOVE_THRESHOLD;
        } else if ((prev_hbmtemp_event == SYSMON_HBM_TEMP_ABOVE_THRESHOLD) &&
                   (temperature.hbmtemp <
                        g_sysmon_cfg.catalog->hbmtemperature_threshold())) {
            hbmtemp_event = SYSMON_HBM_TEMP_BELOW_THRESHOLD;
            prev_hbmtemp_event = SYSMON_HBM_TEMP_BELOW_THRESHOLD;
        } else {
            hbmtemp_event = SYSMON_HBM_TEMP_NONE;
        }
        // update temperature in cpld
        cpld_temp_update(&temperature, xcvrtemp);
        if (g_sysmon_cfg.temp_event_cb) {
            g_sysmon_cfg.temp_event_cb(&temperature, xcvrtemp, hbmtemp_event);
        }
    } else {
        SDK_HMON_TRACE_ERR("Reading temperature failed");
    }

    return;
}

// MONFUNC(checktemperature);

int configurefrequency() {
    boost::property_tree::ptree input;
    sdk_ret_t ret;
    int chip_id = 0;
    int inst_id = 0;
    string frequency;

    try {
        boost::property_tree::read_json(FREQUENCY_FILE, input);
    }
    catch (std::exception const &ex) {
        SDK_HMON_TRACE_ERR("%s", ex.what());
        return -1;
    }

    if (input.empty()) {
        return -1;
    }

    if (input.get_optional<std::string>(FREQUENCY_KEY)) {
        try {
            frequency = input.get<std::string>(FREQUENCY_KEY);
            if (frequency.compare("833") == 0) {
                perf_id = PD_PERF_ID0;
            } else if (frequency.compare("900") == 0) {
                perf_id = PD_PERF_ID1;
            } else if (frequency.compare("957") == 0) {
                perf_id = PD_PERF_ID2;
            } else if (frequency.compare("1033") == 0) {
                perf_id = PD_PERF_ID3;
            } else if (frequency.compare("1100") == 0) {
                startingfrequency_1100 = 1;
                perf_id = PD_PERF_ID4;
            } else {
                return -1;
            }
            ret = asicpd_adjust_perf(chip_id, inst_id, perf_id, PD_PERF_SET);
        } catch (std::exception const &ex) {
            SDK_HMON_TRACE_ERR("%s", ex.what());
            return -1;
        }
    }

    return (ret == SDK_RET_OK) ? 0 : -1;
}
