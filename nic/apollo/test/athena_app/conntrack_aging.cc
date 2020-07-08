//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all conntrack aging test cases for athena
///
//----------------------------------------------------------------------------
#include "conntrack_aging.hpp"
#include "session_aging.hpp"
#include "athena_test.hpp"
#include "nic/apollo/api/include/athena/pds_conntrack.h"
#include "nic/apollo/api/include/athena/pds_flow_age.h"
#include "nic/apollo/api/impl/athena/ftl_pollers_client.hpp"
#include "nic/apollo/api/impl/athena/pds_conntrack_ctx.hpp"
#include "nic/apollo/p4/include/athena_defines.h"
#include "nic/apollo/core/trace.hpp"
#include "fte_athena.hpp"

namespace test {
namespace athena_app {

#define CONNTRACK_RET_VALIDATE(ret)         \
   ((ret) == PDS_RET_OK)

#define CONNTRACK_CREATE_RET_VALIDATE(ret)  \
   (((ret) == PDS_RET_OK) || ((ret) == PDS_RET_ENTRY_EXISTS))

#define CONNTRACK_DELETE_RET_VALIDATE(ret)  \
   (((ret) == PDS_RET_OK) || ((ret) == PDS_RET_ENTRY_NOT_FOUND))

static uint32_t             pollers_qcount;
static pds_flow_expiry_fn_t aging_expiry_dflt_fn;

/*
 * Global tolerance for use across multiple linked tests
 */
static aging_tolerance_t    ct_tolerance(EXPIRY_TYPE_CONNTRACK);
static aging_metrics_t      conntrack_metrics(ftl_dev_if::FTL_QTYPE_SCANNER_CONNTRACK);
static ftl_table_metrics_t  session_ftl_metrics;

const aging_tolerance_t&
ct_tolerance_get(void)
{
    return ct_tolerance;
}

static uint32_t
conntrack_table_depth(void)
{
    static uint32_t table_depth;

    if (!table_depth) {
        table_depth = std::min(ftl_pollers_client::conntrack_table_depth_get(),
                                           (uint32_t)PDS_CONNTRACK_ID_MAX);
    }
    return table_depth;
}

static inline void
conntrack_spec_init(pds_conntrack_spec_t *spec)
{
    memset(spec, 0, sizeof(*spec));
}

bool
conntrack_table_clear_full(test_vparam_ref_t vparam)
{
    pds_conntrack_key_t key;
    uint32_t    depth;
    pds_ret_t   ret = PDS_RET_OK;

    depth = vparam.expected_num(conntrack_table_depth()) + 1;
    depth = std::min(depth, conntrack_table_depth());

    flow_conntrack_key_init(&key);
    for (key.conntrack_id = 1;
         key.conntrack_id < depth;
         key.conntrack_id++) {

        ret = pds_conntrack_state_delete(&key);
        if (!CONNTRACK_DELETE_RET_VALIDATE(ret)) {
            break;
        }
        pds_conntrack_ctx_clr(key.conntrack_id);
    }
    TEST_LOG_INFO("Cleared %u conntrack entries: ret %d\n",
                  key.conntrack_id - 1, ret);
    return CONNTRACK_DELETE_RET_VALIDATE(ret);
}

bool
conntrack_aging_tolerance_secs_set(test_vparam_ref_t vparam)
{
    uint32_t secs = vparam.expected_num();
    ct_tolerance.tolerance_secs_set(secs);
    return true;
}

bool
conntrack_populate_simple(test_vparam_ref_t vparam)
{
    pds_conntrack_spec_t    spec;
    tuple_eval_t            tuple_eval;
    pds_ret_t               ret = PDS_RET_OK;

    conntrack_metrics.baseline();
    conntrack_spec_init(&spec);

    ct_tolerance.reset(vparam.size());
    for (uint32_t i = 0; i < vparam.size(); i++) {

        /*
         * Here we expect tuple of the form {id flowtype flowstate}
         */
        tuple_eval.reset(vparam, i);
        spec.key.conntrack_id = tuple_eval.num(0);
        spec.data.flow_type = tuple_eval.flowtype(1);
        spec.data.flow_state = tuple_eval.flowstate(2);
        if (!tuple_eval.zero_failures()) {
            break;
        }

        ret = pds_conntrack_state_create(&spec);
        if (!CONNTRACK_CREATE_RET_VALIDATE(ret)) {
            break;
        }
        ct_tolerance.create_id_map_insert(spec.key.conntrack_id);
    }

    TEST_LOG_INFO("Conntrack entries created: %d, ret %d\n",
                  ct_tolerance.create_id_map_size(), ret);
    return CONNTRACK_CREATE_RET_VALIDATE(ret) && tuple_eval.zero_failures();
}

bool
conntrack_populate_random(test_vparam_ref_t vparam)
{
    pds_conntrack_spec_t    spec;
    tuple_eval_t            tuple_eval;
    uint32_t                start_idx = 0;
    uint32_t                count = 0;
    bool                    randomize_typestate;
    pds_ret_t               ret = PDS_RET_OK;

    conntrack_metrics.baseline();

    /*
     * 3 possible different sets of parameters:
     *
     * - no params: generate random {start count} and random {flowtype flowstate}
     * - 1 tuple  : use specified {start count} and generate random {flowtype flowstate}
     * - 2 tuples : use specified {start count} and specified {flowtype flowstate}
     */
    conntrack_spec_init(&spec);
    randomize_typestate = true;
    switch (vparam.size()) {

    case 0:
        start_idx = randomize_max(conntrack_table_depth() - 1);
        count = randomize_max(conntrack_table_depth() - start_idx);
        break;

    case 1:
    case 2:
        tuple_eval.reset(vparam, 0);
        start_idx = min(tuple_eval.num(0), conntrack_table_depth() - 1);
        count = min(tuple_eval.num(1), conntrack_table_depth() - start_idx);

        if (!tuple_eval.zero_failures() || (vparam.size() < 2)) {
            break;
        }

        randomize_typestate = false;
        tuple_eval.reset(vparam, 1);
        spec.data.flow_type = tuple_eval.flowtype(0);
        spec.data.flow_state = tuple_eval.flowstate(1);
        break;

    default:
        TEST_LOG_ERR("Too many tuples specified, only a max of 2 needed\n");
        ret = PDS_RET_INVALID_ARG;
        break;
    }

    ct_tolerance.reset(count);
    if (tuple_eval.zero_failures() && CONNTRACK_RET_VALIDATE(ret)) {
        TEST_LOG_INFO("start_idx: %u count: %u\n", start_idx, count);
        for (uint32_t i = 0; i < count; i++) {
            spec.key.conntrack_id = start_idx++;

            /*
             * Randomize flowtype and flowstate if needed
             */
            if (randomize_typestate) {
                pds_flow_state_t flow_state_max;

                spec.data.flow_type = (pds_flow_type_t)
                     randomize_max((uint32_t)PDS_FLOW_TYPE_OTHERS, true);
                /*
                 * Exclude REMOVED state as it would be aged too rapidly
                 */
                flow_state_max = spec.data.flow_type == PDS_FLOW_TYPE_TCP ?
                                 RST_CLOSE : OPEN_CONN_RECV;
                spec.data.flow_state = (pds_flow_state_t)
                     randomize_max((uint32_t)flow_state_max, true,
                                   (uint32_t)REMOVED);
            }

            ret = pds_conntrack_state_create(&spec);
            if (!CONNTRACK_CREATE_RET_VALIDATE(ret)) {
                break;
            }
            ct_tolerance.create_id_map_insert(spec.key.conntrack_id);
        }
    }

    TEST_LOG_INFO("Conntrack entries created: %u, ret %d\n",
                  ct_tolerance.create_id_map_size(), ret);
    return CONNTRACK_CREATE_RET_VALIDATE(ret) && tuple_eval.zero_failures();
}

bool
conntrack_populate_full(test_vparam_ref_t vparam)
{
    pds_conntrack_spec_t    spec;
    tuple_eval_t            tuple_eval;
    uint32_t                depth;
    pds_ret_t               ret = PDS_RET_OK;

    conntrack_metrics.baseline();
    conntrack_spec_init(&spec);

    /*
     * Here we expect tuple of the form {flowtype flowstate depth}
     */
    tuple_eval.reset(vparam, 0);
    spec.data.flow_type = tuple_eval.flowtype(0);
    spec.data.flow_state = tuple_eval.flowstate(1);
    depth = tuple_eval.num(2) + 1;

    ct_tolerance.reset(depth);
    if (tuple_eval.zero_failures()) {
        depth = std::min(depth, conntrack_table_depth());

        for (spec.key.conntrack_id = 1;
             spec.key.conntrack_id < depth;
             spec.key.conntrack_id++) {

            ret = pds_conntrack_state_create(&spec);
            if (!CONNTRACK_CREATE_RET_VALIDATE(ret)) {
                break;
            }
            ct_tolerance.create_id_map_insert(spec.key.conntrack_id);
        }
    }

    TEST_LOG_INFO("Conntrack entries created: %u, ret %d\n",
                  ct_tolerance.create_id_map_size(), ret);
    return CONNTRACK_CREATE_RET_VALIDATE(ret) && tuple_eval.zero_failures();
}

/*
 * Populate flow cache entries with data directly as conntrack indices
 * without any intervening sessions.
 */
bool
conntrack_and_cache_populate(test_vparam_ref_t vparam)
{
    pds_conntrack_spec_t    spec;
    tuple_eval_t            tuple_eval;
    std::string             field_type;
    uint64_t                ids_max;
    flow_key_field_t        vnic;
    flow_key_field_t        sip;
    flow_key_field_t        dip;
    flow_key_field_t        dport;
    flow_key_field_t        sport;
    uint32_t                value;
    uint32_t                count;
    uint32_t                proto = IPPROTO_NONE;
    pds_ret_t               ret = PDS_RET_OK;
    pds_ret_t               cache_ret = PDS_RET_OK;

    conntrack_metrics.baseline();
    conntrack_spec_init(&spec);

    if (vparam.size() == 0) {
        TEST_LOG_ERR("A protocol type (UDP/TCP/ICMP) is required\n");
        return false;
    }
    for (uint32_t i = 0; i < vparam.size(); i++) {

        if (i == 0) {
            ret = vparam.proto(0, &proto);
            if (ret != PDS_RET_OK) {
                return false;
            }
            continue;
        }

        /*
         * Here we expect tuples of the form {type value [count]},
         * e.g., {sip 192.168.1.1 1000}. Note: count is optional (default 1)
         */
        tuple_eval.reset(vparam, i);
        field_type = tuple_eval.str(0);
        value = tuple_eval.num(1);
        count = tuple_eval.size() > 2 ? tuple_eval.num(2) : 0;
        if (field_type == "vnic") {
            vnic.reset(value, count);
        } else if (field_type == "sip") {
            sip.reset(value, count);
        } else if (field_type == "dip") {
            dip.reset(value, count);
        } else if (field_type == "sport") {
            sport.reset(value, count);
        } else if (field_type == "dport") {
            dport.reset(value, count);
        } else {
            TEST_LOG_ERR("Unknown tuple type %s\n", field_type.c_str());
            return false;
        }
    }

    ids_max = (uint64_t)vnic.count() * (uint64_t)sip.count()   *
              (uint64_t)dip.count()  * (uint64_t)sport.count() *
              (uint64_t)dport.count();
    ct_tolerance.reset(ids_max > UINT32_MAX ? UINT32_MAX : ids_max);

    spec.key.conntrack_id = 1;
    spec.data.flow_state = ESTABLISHED;
    switch (proto) { 
    case IPPROTO_ICMP:
        spec.data.flow_type = PDS_FLOW_TYPE_ICMP;
        break;
    case IPPROTO_UDP:
        spec.data.flow_type = PDS_FLOW_TYPE_UDP;
        break;
    case IPPROTO_TCP:
        spec.data.flow_type = PDS_FLOW_TYPE_TCP;
        break;
    default:
        spec.data.flow_type = PDS_FLOW_TYPE_OTHERS;
        break;
    }

    for (vnic.restart(); vnic.count(); vnic.next_value()) {
        for (sip.restart(); sip.count(); sip.next_value()) {
            for (dip.restart(); dip.count(); dip.next_value()) {
                for (sport.restart(); sport.count(); sport.next_value()) {
                    for (dport.restart(); dport.count(); dport.next_value()) {

                        /*
                         * Create a conntrack entry for 1-to-1 mapping to flow
                         * cache but only do so if this iteration is not a recirc
                         * due to an earlier "cache entry already exists".
                         */
                        if (cache_ret != PDS_RET_ENTRY_EXISTS) {
                            if (proto == IPPROTO_TCP) {
                                spec.data.flow_state = (pds_flow_state_t)
                                     randomize_max((uint32_t)RST_CLOSE, true);
                            }
                            ret = pds_conntrack_state_create(&spec);
                            if (!CONNTRACK_CREATE_RET_VALIDATE(ret)) {
                                goto done;
                            }
                            ct_tolerance.create_id_map_insert(spec.key.conntrack_id);
                        }

                        if (proto == IPPROTO_ICMP) {
                            cache_ret = (pds_ret_t)
                                        fte_ath::fte_flow_create_icmp(vnic.value(),
                                               sip.value(), dip.value(),
                                               proto, sport.value(), dport.value(),
                                               spec.key.conntrack_id,
                                               PDS_FLOW_SPEC_INDEX_CONNTRACK,
                                               spec.key.conntrack_id);
                        } else {
                            cache_ret = (pds_ret_t)
                                        fte_ath::fte_flow_create(vnic.value(),
                                               sip.value(), dip.value(),
                                               proto, sport.value(), dport.value(),
                                               PDS_FLOW_SPEC_INDEX_CONNTRACK,
                                               spec.key.conntrack_id);
                        }

                        switch (cache_ret) {

                        case PDS_RET_OK:
                            if (++spec.key.conntrack_id >= PDS_CONNTRACK_ID_MAX) {
                                goto done;
                            }
                            break;

                        case PDS_RET_NO_RESOURCE:
                            TEST_LOG_INFO("flow cache table full at %u entries\n",
                                          ct_tolerance.create_id_map_size());
                            cache_ret = PDS_RET_OK;
                            goto done;

                        case PDS_RET_ENTRY_EXISTS:

                            /*
                             * Continue cache entry creation even on key hash
                             * collision but use the last conntrack ID.
                             */
                            break;

                        default:
                            TEST_LOG_ERR("failed cache create - vnic:%u "
                                 "sip:0x%x dip:0x%x sport:%u dport:%u proto:%u "
                                 "at count:%u error:%d\n", vnic.value(),
                                 sip.value(), dip.value(), sport.value(),
                                 dport.value(), proto,
                                 ct_tolerance.create_id_map_size(), cache_ret);
                            goto done;
                        }
                    }
                }
            }
        }
    }

done:
    TEST_LOG_INFO("Flow entries created: %d, ret %d cache_ret %d\n",
                  ct_tolerance.create_id_map_size(), ret, cache_ret);

    /*
     * Flow cache entry creations were best effort due to hash outcomes
     * so any non-zero count would be considered a success.
     */
    return ct_tolerance.create_id_map_size() &&
           CONNTRACK_CREATE_RET_VALIDATE(ret) &&
           CONNTRACK_CREATE_RET_VALIDATE(cache_ret);
}

bool
conntrack_aging_traffic_chkpt_start(test_vparam_ref_t vparam)
{
    conntrack_metrics.baseline();
    session_ftl_metrics.baseline();
    ct_tolerance.reset(UINT32_MAX);
    ct_tolerance.using_fte_indices(true);

    TEST_LOG_INFO("Current active FTL entries: %" PRIu64 "\n",
                  session_ftl_metrics.num_entries());
    return true;
}

bool
conntrack_aging_traffic_chkpt_end(test_vparam_ref_t vparam)
{
    uint64_t    actual = session_ftl_metrics.delta_num_entries();
    uint32_t    expected = vparam.expected_num();

    if (expected) {
        TEST_LOG_INFO("Test param expected active FTL entries: %u vs actual: %"
                      PRIu64 "\n", expected, actual);
    } else {
        TEST_LOG_INFO("Delta active FTL entries: %" PRIu64 "\n", actual);
    }
    ct_tolerance.create_id_map_override(actual);
    return true;
}

pds_ret_t
conntrack_aging_expiry_fn(uint32_t expiry_id,
                          pds_flow_age_expiry_type_t expiry_type,
                          void *user_ctx,
                          uint32_t *ret_handle)
{
    uint32_t    handle = 0;
    pds_ret_t   ret = PDS_RET_OK;

    switch (expiry_type) {

    case EXPIRY_TYPE_CONNTRACK:
        if (aging_expiry_dflt_fn) {
            ct_tolerance.conntrack_tmo_tolerance_check(expiry_id);
            ret = (*aging_expiry_dflt_fn)(expiry_id, expiry_type,
                                          user_ctx, &handle);
            if (ret != PDS_RET_RETRY) {
                ct_tolerance.expiry_count_inc();
                ct_tolerance.create_id_map_find_erase(expiry_id);
                if (handle) {
                    inter_poll_params_t inter_poll;
                    inter_poll.skip_expiry_fn = true;
                    ret = session_aging_expiry_fn(handle, EXPIRY_TYPE_SESSION,
                                                  &inter_poll, ret_handle);
                }
            }
        }
        break;

    case EXPIRY_TYPE_SESSION:
        ret = session_aging_expiry_fn(expiry_id, expiry_type,
                                      user_ctx, ret_handle);
        break;

    default:
        ret = PDS_RET_INVALID_ARG;
        break;
    }

    if ((ret != PDS_RET_OK) && (ret != PDS_RET_RETRY)) {
        if (ct_tolerance.delete_errors() == 0) {
            TEST_LOG_ERR("failed flow deletion on conntrack_id %u: "
                         "ret %d\n", expiry_id, ret);
        }
        ct_tolerance.delete_errors_inc();
    }
    return ret;
}
                             
bool
conntrack_aging_init(test_vparam_ref_t vparam)
{
    pds_ret_t   ret = PDS_RET_OK;

    // On SIM platform, the following needs to be set early
    // before scanners are started to prevent lockup in scanners
    // due to the lack of true LIF timers in SIM.
    if (!hw()) {
        ret = ftl_pollers_client::force_conntrack_expired_ts_set(true);
    }
    if (CONNTRACK_RET_VALIDATE(ret)) {
        ret = pds_flow_age_sw_pollers_qcount(&pollers_qcount);
    }
    if (CONNTRACK_RET_VALIDATE(ret)) {
        ret = pds_flow_age_sw_pollers_expiry_fn_dflt(&aging_expiry_dflt_fn);
    }
    if (CONNTRACK_RET_VALIDATE(ret)) {

        // On HW go with FTE polling threads; 
        // otherwise do self polling in this module.

        ret = pds_flow_age_sw_pollers_poll_control(!hw(),
                                      conntrack_aging_expiry_fn);
    }
    if (!hw() && CONNTRACK_RET_VALIDATE(ret)) {
        ret = pds_flow_age_hw_scanners_start();
    }
    return CONNTRACK_RET_VALIDATE(ret) && pollers_qcount && 
           conntrack_table_depth() && aging_expiry_dflt_fn;
}

bool
conntrack_aging_expiry_log_set(test_vparam_ref_t vparam)
{
    ftl_pollers_client::expiry_log_set(vparam.expected_bool());
    return true;
}

bool
conntrack_aging_force_expired_ts(test_vparam_ref_t vparam)
{
    pds_ret_t   ret;

    ret = ftl_pollers_client::force_conntrack_expired_ts_set(vparam.expected_bool());
    return CONNTRACK_RET_VALIDATE(ret);
}

bool
conntrack_aging_fini(test_vparam_ref_t vparam)
{
    test_vparam_t   sim_vparam;
    pds_ret_t       ret;

    if (hw()) {

        // Restore FTE aging expiry handler
        ret = pds_flow_age_sw_pollers_poll_control(false,
                                      fte_ath::fte_flows_aging_expiry_fn);
    } else {
        ret = pds_flow_age_hw_scanners_stop(true);
        if (CONNTRACK_RET_VALIDATE(ret)) {
            ret = pds_flow_age_sw_pollers_poll_control(false, NULL);
        }
    }

    sim_vparam.push_back(test_param_t((uint32_t)false));
    conntrack_aging_force_expired_ts(sim_vparam);
    return CONNTRACK_RET_VALIDATE(ret);
}


static void
conntrack_aging_pollers_poll(void *user_ctx)
{
    for (uint32_t qid = 0; qid < pollers_qcount; qid++) {
        pds_flow_age_sw_pollers_poll(qid, user_ctx);
    }
}

static bool
conntrack_aging_expiry_count_check(void *user_ctx)
{
    if (!hw()) {
        conntrack_aging_pollers_poll(user_ctx);
    }
    return ct_tolerance.expiry_count() >= ct_tolerance.create_count();
}

bool
conntrack_4combined_expiry_count_check(bool poll_needed)
{
    if (!hw() && poll_needed) {
        conntrack_aging_pollers_poll((void *)&ct_tolerance);
    }
    return ct_tolerance.expiry_count() >= ct_tolerance.create_count();
}

bool
conntrack_4combined_result_check(void)
{
    TEST_LOG_INFO("Conntrack entries aged out: %u, over_age_min: %u, "
                  "over_age_max: %u\n", ct_tolerance.expiry_count(),
                  ct_tolerance.over_age_min(), ct_tolerance.over_age_max());
    ct_tolerance.create_id_map_empty_check();
    conntrack_metrics.expiry_count_check(ct_tolerance.expiry_count());
    return conntrack_4combined_expiry_count_check() &&
           ct_tolerance.zero_failures();
}

bool
conntrack_aging_test(test_vparam_ref_t vparam)
{
    test_timestamp_t    ts;

    ts.time_expiry_set(APP_TIME_LIMIT_EXEC_SECS(ct_tolerance.curr_max_tmo() +
                                                APP_TIME_LIMIT_EXEC_GRACE));
    ts.time_limit_exec(conntrack_aging_expiry_count_check, nullptr);
    return conntrack_4combined_result_check();
}

bool
conntrack_aging_normal_tmo_set(test_vparam_ref_t vparam)
{
    tuple_eval_t        tuple_eval;
    pds_flow_type_t     flowtype;
    pds_flow_state_t    flowstate;
    uint32_t            tmo_val;
    pds_ret_t           ret = PDS_RET_OK;

    ct_tolerance.normal_tmo.reset();

    /*
     * Here we expect tuples of the form {flowtype flowstate tmo_val}
     */
    for (uint32_t i = 0; i < vparam.size(); i++) {

        /*
         * Here we expect tuple of the form {flowtype flowstate tmo_val}
         */
        tuple_eval.reset(vparam, i);
        flowtype = tuple_eval.flowtype(0);
        flowstate = tuple_eval.flowstate(1);
        tmo_val = tuple_eval.num(2);
        if (!tuple_eval.zero_failures()) {
            break;
        }
        ct_tolerance.normal_tmo.conntrack_tmo_set(flowtype, flowstate, tmo_val);
    }
    return CONNTRACK_RET_VALIDATE(ret) && tuple_eval.zero_failures() &&
           ct_tolerance.zero_failures();
}

bool
conntrack_aging_accel_tmo_set(test_vparam_ref_t vparam)
{
    tuple_eval_t        tuple_eval;
    pds_flow_type_t     flowtype;
    pds_flow_state_t    flowstate;
    uint32_t            tmo_val;
    pds_ret_t           ret = PDS_RET_OK;

    ct_tolerance.accel_tmo.reset();

    /*
     * Here we expect tuples of the form {flowtype flowstate tmo_val}
     */
    for (uint32_t i = 0; i < vparam.size(); i++) {

        /*
         * Here we expect tuple of the form {flowtype flowstate tmo_val}
         */
        tuple_eval.reset(vparam, i);
        flowtype = tuple_eval.flowtype(0);
        flowstate = tuple_eval.flowstate(1);
        tmo_val = tuple_eval.num(2);
        if (!tuple_eval.zero_failures()) {
            break;
        }
        ct_tolerance.accel_tmo.conntrack_tmo_set(flowtype, flowstate, tmo_val);
    }
    return CONNTRACK_RET_VALIDATE(ret) && tuple_eval.zero_failures() &&
           ct_tolerance.zero_failures();
}

bool
conntrack_aging_tmo_factory_dflt_set(test_vparam_ref_t vparam)
{
    ct_tolerance.normal_tmo.failures_clear();
    ct_tolerance.accel_tmo.failures_clear();
    ct_tolerance.normal_tmo.tmo_factory_dflt_set();
    ct_tolerance.accel_tmo.tmo_factory_dflt_set();
    return ct_tolerance.normal_tmo.zero_failures() &&
           ct_tolerance.accel_tmo.zero_failures();
}

bool
conntrack_aging_tmo_artificial_long_set(test_vparam_ref_t vparam)
{
    ct_tolerance.normal_tmo.failures_clear();
    ct_tolerance.accel_tmo.failures_clear();
    ct_tolerance.normal_tmo.tmo_artificial_long_set();
    ct_tolerance.accel_tmo.tmo_artificial_long_set();
    return ct_tolerance.normal_tmo.zero_failures() &&
           ct_tolerance.accel_tmo.zero_failures();
}

bool
conntrack_aging_accel_control(test_vparam_ref_t vparam)
{
    ct_tolerance.failures_clear();
    ct_tolerance.age_accel_control(vparam.expected_bool());
    return ct_tolerance.zero_failures();
}

bool
conntrack_aging_metrics_show(test_vparam_ref_t vparam)
{
    aging_metrics_t scanner_metrics(ftl_dev_if::FTL_QTYPE_SCANNER_CONNTRACK);
    aging_metrics_t poller_metrics(ftl_dev_if::FTL_QTYPE_POLLER);
    aging_metrics_t timestamp_metrics(ftl_dev_if::FTL_QTYPE_MPU_TIMESTAMP);

    scanner_metrics.show();
    poller_metrics.show();
    timestamp_metrics.show();
    return true;
}


}    // namespace athena_app
}    // namespace test
