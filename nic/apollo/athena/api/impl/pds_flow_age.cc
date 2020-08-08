//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena hardware-assisted flow aging implementation
///
//----------------------------------------------------------------------------

#include "nic/apollo/athena/api/include/pds_flow_age.h"
#include "ftl_dev_impl.hpp"
#include "ftl_pollers_client.hpp"

extern "C" {

pds_ret_t
pds_flow_age_init(pds_cinit_params_t *params)
{
    return ftl_pollers_client::init(params);
}

void
pds_flow_age_fini(void)
{
    ftl_pollers_client::fini();
}

pds_ret_t
pds_flow_age_hw_scanners_start(void)
{
    return ftl_dev_impl::scanners_start();
}

pds_ret_t
pds_flow_age_hw_scanners_stop(bool quiesce_check)
{
    return ftl_dev_impl::scanners_stop(quiesce_check);
}

pds_ret_t
pds_flow_age_sw_pollers_flush(void)
{
    return ftl_dev_impl::pollers_flush();
}

pds_ret_t
pds_flow_age_sw_pollers_qcount(uint32_t *ret_qcount)
{
    return ftl_dev_impl::pollers_qcount_get(ret_qcount);
}

pds_ret_t
pds_flow_age_sw_pollers_expiry_fn_dflt(pds_flow_expiry_fn_t *ret_fn_dflt)
{
    return ftl_pollers_client::expiry_fn_dflt(ret_fn_dflt);
}

pds_ret_t
pds_flow_age_sw_pollers_poll_control(bool user_will_poll,
                                     pds_flow_expiry_fn_t expiry_fn)
{
    return  ftl_pollers_client::poll_control(user_will_poll, expiry_fn);
}

pds_ret_t
pds_flow_age_sw_pollers_poll(uint32_t poller_id,
                             uint32_t cb_limit_multiples,
                             void *user_ctx)
{
    return  ftl_pollers_client::poll(poller_id, cb_limit_multiples, user_ctx);
}

pds_ret_t
pds_flow_age_normal_timeouts_set(const pds_flow_age_timeouts_t *norm_age_timeouts)
{
    return ftl_dev_impl::normal_timeouts_set(norm_age_timeouts);
}

pds_ret_t
pds_flow_age_normal_timeouts_get(pds_flow_age_timeouts_t *norm_age_timeouts)
{
    return ftl_dev_impl::normal_timeouts_get(norm_age_timeouts);
}

pds_ret_t
pds_flow_age_accel_timeouts_set(const pds_flow_age_accel_timeouts_t *accel_age_timeouts)
{
    return ftl_dev_impl::accel_timeouts_set(accel_age_timeouts);
}

pds_ret_t
pds_flow_age_accel_timeouts_get(pds_flow_age_accel_timeouts_t *accel_age_timeouts)
{
    return ftl_dev_impl::accel_timeouts_get(accel_age_timeouts);
}

pds_ret_t
pds_flow_age_accel_control(bool enable_sense)
{
    return ftl_dev_impl::accel_aging_control(enable_sense);
}

}

