//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// athena policer implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/p4/p4_utils.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/athena/api/include/pds_policer.h"
#include "gen/p4gen/athena/include/p4pd.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "nic/athena/p4/p4-16/athena_defines.h"
#include "nic/athena/api/impl/pds_policer_impl.hpp"

using namespace sdk;

extern "C" {

/***********************************
 *  Policer BW1 API's begin
 **********************************/
pds_ret_t
pds_policer_bw1_create(pds_policer_spec_t *spec)
{
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_BPS, spec->data.rate, spec->data.burst };
    sdk::qos::policer_t         *policer = &pol;

    if (!spec) {
        PDS_TRACE_ERR("spec is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = spec->key.policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_bw1,
                              P4TBL_ID_POLICER_BW1,
                              POLICER_BW1_POLICER_BW1_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_policer_bw1_read(pds_policer_key_t *key,
                     pds_policer_info_t *info)
{
    p4pd_error_t                p4pd_ret = P4PD_SUCCESS;
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;

    if (!key || !info) {
        PDS_TRACE_ERR("key/info is null");
        return PDS_RET_INVALID_ARG;
    }

    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    READ_POLICER_TABLE_ENTRY(info,
                             policer_bw1,
                             P4TBL_ID_POLICER_BW1,
                             POLICER_BW1_POLICER_BW1_ID,
                             policer_id,
                             PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_policer_bw1_update(pds_policer_spec_t *spec)
{
    return pds_policer_bw1_create(spec);
}

pds_ret_t
pds_policer_bw1_delete(pds_policer_key_t *key)
{
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_BPS, 0, 0 };
    sdk::qos::policer_t         *policer = &pol;

    if (!key) {
        PDS_TRACE_ERR("key is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_bw1,
                              P4TBL_ID_POLICER_BW1,
                              POLICER_BW1_POLICER_BW1_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}

/***********************************
 *  Policer BW1 API's end
 **********************************/

/***********************************
 *  Policer BW2 API's begin
 **********************************/
pds_ret_t
pds_policer_bw2_create(pds_policer_spec_t *spec)
{
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_BPS, spec->data.rate, spec->data.burst };
    sdk::qos::policer_t         *policer = &pol;

    if (!spec) {
        PDS_TRACE_ERR("spec is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = spec->key.policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_bw2,
                              P4TBL_ID_POLICER_BW2,
                              POLICER_BW2_POLICER_BW2_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_policer_bw2_read(pds_policer_key_t *key,
                     pds_policer_info_t *info)
{
    p4pd_error_t                p4pd_ret = P4PD_SUCCESS;
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;

    if (!key || !info) {
        PDS_TRACE_ERR("key/info is null");
        return PDS_RET_INVALID_ARG;
    }

    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    READ_POLICER_TABLE_ENTRY(info,
                             policer_bw2,
                             P4TBL_ID_POLICER_BW2,
                             POLICER_BW2_POLICER_BW2_ID,
                             policer_id,
                             PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_policer_bw2_update(pds_policer_spec_t *spec)
{
    return pds_policer_bw2_create(spec);
}

pds_ret_t
pds_policer_bw2_delete(pds_policer_key_t *key)
{
    uint16_t                    policer_id = PDS_POLICER_BANDWIDTH_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_BPS, 0, 0 };
    sdk::qos::policer_t         *policer = &pol;

    if (!key) {
        PDS_TRACE_ERR("key is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_POLICER_BANDWIDTH_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_bw2,
                              P4TBL_ID_POLICER_BW2,
                              POLICER_BW2_POLICER_BW2_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}

/*****************************
 *  Policer BW2 API's end
 ****************************/

/*****************************
 *  Policer PPS API's begin
 ****************************/
pds_ret_t
pds_vnic_policer_pps_create(pds_policer_spec_t *spec)
{
    uint16_t                    policer_id = PDS_VNIC_POLICER_PPS_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_PPS, spec->data.rate, spec->data.burst };
    sdk::qos::policer_t         *policer = &pol;

    if (!spec) {
        PDS_TRACE_ERR("spec is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = spec->key.policer_id;
    if (policer_id == 0 || policer_id >= PDS_VNIC_POLICER_PPS_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_pps,
                              P4TBL_ID_POLICER_PPS,
                              POLICER_PPS_POLICER_PPS_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_vnic_policer_pps_read(pds_policer_key_t *key,
                          pds_policer_info_t *info)
{
    p4pd_error_t                p4pd_ret = P4PD_SUCCESS;
    uint16_t                    policer_id = PDS_VNIC_POLICER_PPS_ID_MAX;

    if (!key || !info) {
        PDS_TRACE_ERR("key/info is null");
        return PDS_RET_INVALID_ARG;
    }

    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_VNIC_POLICER_PPS_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    READ_POLICER_TABLE_ENTRY(info,
                             policer_pps,
                             P4TBL_ID_POLICER_PPS,
                             POLICER_PPS_POLICER_PPS_ID,
                             policer_id,
                             PDS_DEFAULT_POLICER_REFRESH_RATE);
}

pds_ret_t
pds_vnic_policer_pps_update(pds_policer_spec_t *spec)
{
    return pds_vnic_policer_pps_create(spec);
}

pds_ret_t
pds_vnic_policer_pps_delete(pds_policer_key_t *key)
{
    uint16_t                    policer_id = PDS_VNIC_POLICER_PPS_ID_MAX;
    sdk::qos::policer_t         pol = { sdk::qos::POLICER_TYPE_PPS, 0, 0 };
    sdk::qos::policer_t         *policer = &pol;

    if (!key) {
        PDS_TRACE_ERR("key is null");
        return PDS_RET_INVALID_ARG;
    }
    policer_id = key->policer_id;
    if (policer_id == 0 || policer_id >= PDS_VNIC_POLICER_PPS_ID_MAX) {
        PDS_TRACE_ERR("policer id %u is beyond range", policer_id);
        return PDS_RET_INVALID_ARG;
    }

    WRITE_POLICER_TABLE_ENTRY(policer,
                              policer_pps,
                              P4TBL_ID_POLICER_PPS,
                              POLICER_PPS_POLICER_PPS_ID,
                              policer_id,
                              PDS_DEFAULT_POLICER_REFRESH_RATE);
}
/**************************
 *  Policer PPS API's end
 *************************/
}
