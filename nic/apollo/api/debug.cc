/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    debug.cc
 *
 * @brief   This file has helper routines for troubleshooting the system
 */

#include "nic/sdk/linkmgr/port_mac.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/api/debug.hpp"

namespace debug {

/**
 * @brief        set clock frequency
 * @param[in]    freq clock frequency to be set
 * @return       #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_clock_frequency_update (pds_clock_freq_t freq)
{
    return impl_base::asic_impl()->set_frequency(freq);
}

/**
 * @brief        set arm clock frequency
 * @param[in]    freq clock frequency to be set
 * @return       #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_arm_clock_frequency_update (pds_clock_freq_t freq)
{
    return impl_base::asic_impl()->set_arm_frequency(freq);
}

/**
 * @brief        get system temperature
 * @param[out]   Temperate to be read
 * @return       #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_get_system_temperature (pds_system_temperature_t *temp)
{
    return impl_base::asic_impl()->get_system_temperature(temp);
}

/**
 * @brief        get system power
 * @param[out]   Power to be read
 * @return       #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_get_system_power (pds_system_power_t *pow)
{
    return impl_base::asic_impl()->get_system_power(pow);
}

/**
 * @brief       Gets API stats for different tables
 * @param[in]   CB to fill API stats & ctxt
 * @return      #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_table_stats_get (debug::table_stats_get_cb_t cb, void *ctxt)
{
    return impl_base::pipeline_impl()->table_stats(cb, ctxt);
}

/**
 * @brief       Setup LLC
 * @param[in]   llc_counters_t with type set
 * @return      #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_llc_setup (llc_counters_t *llc_args)
{
    return impl_base::asic_impl()->llc_setup(llc_args);
}

/**
 * @brief       LLC Stats Get
 * @param[out]  llc_counters_t to be filled
 * @return      #SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
pds_llc_get (llc_counters_t *llc_args)
{
    return impl_base::asic_impl()->llc_get(llc_args);
}

sdk_ret_t
pds_pb_stats_get (debug::pb_stats_get_cb_t cb, void *ctxt)
{
    return impl_base::asic_impl()->pb_stats(cb, ctxt);
}

sdk_ret_t
pds_meter_stats_get (debug::meter_stats_get_cb_t cb, uint32_t lowidx,
                     uint32_t highidx, void *ctxt)
{
    return impl_base::pipeline_impl()->meter_stats(cb, lowidx, highidx, ctxt);
}

sdk_ret_t
pds_session_stats_get (debug::session_stats_get_cb_t cb, uint32_t lowidx,
                       uint32_t highidx, void *ctxt)
{
    return impl_base::pipeline_impl()->session_stats(cb, lowidx, highidx, ctxt);
}

sdk_ret_t
pds_fte_api_stats_get (void)
{
    return SDK_RET_OK;
}

sdk_ret_t
pds_fte_table_stats_get (void)
{
    return SDK_RET_OK;
}

sdk_ret_t
pds_fte_api_stats_clear (void)
{
    return SDK_RET_OK;
}

sdk_ret_t
pds_fte_table_stats_clear (void)
{
    return SDK_RET_OK;
}

sdk_ret_t
pds_session_get (debug::session_get_cb_t cb, void *ctxt)
{
    return impl_base::pipeline_impl()->session(cb, ctxt);
}

sdk_ret_t
pds_flow_get (debug::flow_get_cb_t cb, void *ctxt)
{
    return impl_base::pipeline_impl()->flow(cb, ctxt);
}

sdk_ret_t
pds_session_clear (uint32_t idx)
{
    return impl_base::pipeline_impl()->session_clear(idx);
}

sdk_ret_t
pds_flow_clear (uint32_t idx)
{
    return impl_base::pipeline_impl()->flow_clear(idx);
}

}    // namespace debug
