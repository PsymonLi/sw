//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines callbacks and metrics update for system monitoring
///
//----------------------------------------------------------------------------

#include "nic/sdk/platform/sensor/sensor.hpp"
#include "nic/operd/alerts/alerts.hpp"

namespace api {
#include <sys/time.h>

/// \brief     event callback for system power
/// \param[in] power system power info
static inline void
power_event_cb (system_power_t *power)
{
    PDS_HMON_TRACE_VERBOSE("Power of pin is %dW, pout1 is %dW, "
                           "pout2 is %dW", power->pin,
                           power->pout1, power->pout2);
    sdk::metrics::metrics_update(g_pds_state.power_metrics_handle(),
                                 *(sdk::metrics::key_t *)uuid_from_objid(0).id,
                                 (uint64_t *)power);
}

/// \brief      populate the array based on asic temperature fields
/// \param[in]  temperature temperature info
/// \param[out] arr array to be populated
static inline void
populate_asic_temperature (system_temperature_t *temperature,
                           uint64_t *arr)
{
    arr[asic_temp_metrics_type_t::ASIC_TEMP_METRICS_TYPE_LOCAL] =
            temperature->localtemp;
    arr[asic_temp_metrics_type_t::ASIC_TEMP_METRICS_TYPE_DIE] =
            temperature->dietemp;
    arr[asic_temp_metrics_type_t::ASIC_TEMP_METRICS_TYPE_HBM] =
            temperature->hbmtemp;
}

/// \brief      populate the array based on port temperature fields
/// \param[in]  temperature temperature info
/// \parma[in]  phy_port physical port
/// \param[out] arr array to be populated
static inline void
populate_port_temperature (system_temperature_t *temperature,
                           uint32_t phy_port, uint64_t *arr)
{
    arr[port_temp_metrics_type_t::PORT_TEMP_METRICS_TYPE_PORT] =
            temperature->xcvrtemp[phy_port].temperature;
    arr[port_temp_metrics_type_t::PORT_TEMP_METRICS_TYPE_WARN] =
            temperature->xcvrtemp[phy_port].warning_temperature;
    arr[port_temp_metrics_type_t::PORT_TEMP_METRICS_TYPE_ALARM] =
            temperature->xcvrtemp[phy_port].alarm_temperature;
}

/// \brief     update metrics for port temperature
/// \param[in] temperature temperature info
static inline void
port_temperature_metrics_update (system_temperature_t *temperature)
{
    uint64_t port_temp_metrics[
        port_temp_metrics_type_t::PORT_TEMP_METRICS_TYPE_MAX] = { 0 };
    uint32_t num_phy_ports;
    if_index_t ifindex;

    num_phy_ports = g_pds_state.catalogue()->num_fp_ports();
    for (uint32_t phy_port = 1; phy_port <= num_phy_ports; phy_port++) {
        populate_port_temperature(temperature, phy_port,
                                  port_temp_metrics);
        ifindex = ETH_IFINDEX(g_pds_state.catalogue()->slot(),
                              phy_port, ETH_IF_DEFAULT_CHILD_PORT);
        sdk::metrics::metrics_update(
            g_pds_state.port_temperature_metrics_handle(),
            *(sdk::metrics::key_t *)uuid_from_objid(ifindex).id,
            port_temp_metrics);
    }
}

/// \brief     update metrics for asic temperature
/// \param[in] temperature temperature info
static inline void
asic_temperature_metrics_update (system_temperature_t *temperature)
{
    uint64_t asic_temp_metrics[
        asic_temp_metrics_type_t::ASIC_TEMP_METRICS_TYPE_MAX] = { 0 };

    populate_asic_temperature(temperature, asic_temp_metrics);
    sdk::metrics::metrics_update(g_pds_state.asic_temperature_metrics_handle(),
                                 *(sdk::metrics::key_t *)uuid_from_objid(0).id,
                                 asic_temp_metrics);
}

/// \brief     event callback for asic and port temperature
/// \param[in] temperature temperature info
/// \param[in] hbm_event events for hbm temperature threshold
static inline void
temperature_event_cb (system_temperature_t *temperature,
                      sysmon_hbm_threshold_event_t hbm_event)
{
    PDS_HMON_TRACE_VERBOSE("Die temperature is %dC, local temperature is "
                           "%dC, HBM temperature is %dC",
                           temperature->dietemp,
                           temperature->localtemp, temperature->hbmtemp);
    switch (hbm_event) {
    case sysmon_hbm_threshold_event_t::SYSMON_HBM_TEMP_ABOVE_THRESHOLD:
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_MEM_TEMP_ABOVE_THRESHOLD,
            "Memory temperature above threshold");
        break;
    case sysmon_hbm_threshold_event_t::SYSMON_HBM_TEMP_BELOW_THRESHOLD:
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_MEM_TEMP_BELOW_THRESHOLD,
            "Memory temperature below threshold");
        break;
    default:
        break;
    }
    asic_temperature_metrics_update(temperature);
    port_temperature_metrics_update(temperature);
}

/// \brief     event callback for asic interrupts
/// \param[in] reg interrupt register info
/// \param[in] field interrupt field info
static inline void
interrupt_event_cb (const intr_reg_t *reg, const intr_field_t *field)
{
    switch (field->severity) {
    case INTR_SEV_TYPE_HW_RMA:
    case INTR_SEV_TYPE_FATAL:
    case INTR_SEV_TYPE_ERR:
        if (field->count == 1) {
            // log to onetime interrupt error
            PDS_INTR_TRACE_ERR("name %s_%s, count %lu, severity %s, desc %s",
                reg->name, field->name, field->count,
                get_severity_str(field->severity).c_str(), field->desc);
        }
        // log in hmon error
        PDS_HMON_TRACE_ERR("name %s_%s, count %lu, severity %s, desc %s",
            reg->name, field->name, field->count,
            get_severity_str(field->severity).c_str(), field->desc);
        break;
    case INTR_SEV_TYPE_INFO:
    default:
        break;
    }

    // post processing of interrupts
    impl_base::asic_impl()->process_interrupts(reg, field);
}

void
cattrip_event_cb (void)
{
    operd::alerts::alert_recorder::get()->alert(
        operd::alerts::DSC_CATTRIP_INTERRUPT, "DSC temperature crossed the "
        "fatal threshold and will reset");
}

void
memory_event_cb (system_memory_t *system_memory)
{
    sdk::metrics::metrics_update(g_pds_state.memory_metrics_handle(),
                                 *(sdk::metrics::key_t *)uuid_from_objid(0).id,
                                 (uint64_t *)system_memory);
}

void
panic_event_cb (void)
{
    operd::alerts::alert_recorder::get()->alert(operd::alerts::DSC_PANIC_EVENT,
                                                "Panic occurred on DSC "
                                                "on the previous boot");
}

void
postdiag_event_cb (void)
{
    operd::alerts::alert_recorder::get()->alert(
        operd::alerts::DSC_POST_DIAG_FAILURE_EVENT,
        "DSC post diag failed in this boot");
}

static void
cpld_port_status_update (void *entry, void *ctxt)
{
    uint32_t phy_port;
    port_oper_status_t port_status;
    pds_if_info_t *info = (pds_if_info_t *)entry;

    port_status = info->status.port_status.link_status.oper_state;
    phy_port = g_pds_state.catalogue()->ifindex_to_phy_port(info->status.ifindex);
    if (info->spec.port_info.type == port_type_t::PORT_TYPE_ETH) {
        pal_cpld_set_port_link_status(phy_port, (uint8_t)port_status);
    }
}

void
liveness_event_cb (void)
{
    // update port status
    port_get_all(cpld_port_status_update, NULL);
    return;
}

void
pciehealth_event_cb (sysmon_pciehealth_severity_t sev, const char *reason)
{
    if (sev == SYSMON_PCIEHEALTH_INFO) {
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_INFO_PCIEHEALTH_EVENT, reason);
    } else if (sev == SYSMON_PCIEHEALTH_WARN){
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_WARN_PCIEHEALTH_EVENT, reason);
    } else if (sev == SYSMON_PCIEHEALTH_ERROR){
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_ERR_PCIEHEALTH_EVENT, reason);
    }
}

void
filesystem_threshold_event_cb (sysmon_filesystem_threshold_event_t event,
                               const char *path, uint32_t threshold_percent)
{
    char event_log[512];

    if (event == SYSMON_FILESYSTEM_USAGE_ABOVE_THRESHOLD) {
        snprintf(event_log, sizeof(event_log),
                 "%s is above filesystem usage threshold of %u percent", path,
                 threshold_percent);
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_FILESYSTEM_USAGE_ABOVE_THRESHOLD, event_log);
    } else if (event == SYSMON_FILESYSTEM_USAGE_BELOW_THRESHOLD) {
        snprintf(event_log, sizeof(event_log),
                 "%s is below filesystem usage threshold of %u percent", path,
                 threshold_percent);
        operd::alerts::alert_recorder::get()->alert(
            operd::alerts::DSC_FILESYSTEM_USAGE_BELOW_THRESHOLD, event_log);
    }
}

}    // namespace api
