//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/table.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/platform/marvell/marvell.hpp"
#include "nic/apollo/api/include/pds_debug.hpp"
#include "nic/apollo/api/debug.hpp"
#include "nic/apollo/api/internal/debug.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/svc/debug_svc.hpp"
#include "nic/apollo/agent/svc/meter_svc.hpp"
#include "nic/apollo/agent/trace.hpp"
#include <malloc.h>

Status
DebugSvcImpl::ClockFrequencyUpdate(ServerContext *context,
                                   const pds::ClockFrequencyRequest *proto_req,
                                   pds::ClockFrequencyResponse *proto_rsp) {
    uint32_t freq;
    pds_clock_freq_t clock_freq;
    sdk_ret_t ret = SDK_RET_INVALID_ARG;

    if (proto_req == NULL) {
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
    }

    freq = proto_req->clockfrequency();
    if (freq) {
        switch (freq) {
        case 833:
            clock_freq = PDS_CLOCK_FREQUENCY_833;
            break;
        case 900:
            clock_freq = PDS_CLOCK_FREQUENCY_900;
            break;
        case 957:
            clock_freq = PDS_CLOCK_FREQUENCY_957;
            break;
        case 1033:
            clock_freq = PDS_CLOCK_FREQUENCY_1033;
            break;
        case 1100:
            clock_freq = PDS_CLOCK_FREQUENCY_1100;
            break;
        default:
            PDS_TRACE_ERR("Clock-frequency update failed, accepted frequencies are 833, 900, 957, 1033 and 1100");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
            return Status::OK;
        }
        ret = debug::pds_clock_frequency_update(clock_freq);
    }

    freq = proto_req->armclockfrequency();
    if (freq) {
        switch (freq) {
        case 1667:
            clock_freq = PDS_CLOCK_FREQUENCY_1666;
            break;
        case 2200:
            clock_freq = PDS_CLOCK_FREQUENCY_2200;
            break;
        default:
            PDS_TRACE_ERR("Clock-frequency update failed, accepted frequencies are 1667 and 2200");
            proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
            return Status::OK;
        }
        ret = debug::pds_arm_clock_frequency_update(clock_freq);
    }

    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
DebugSvcImpl::SystemTemperatureGet(ServerContext *context, const Empty *proto_req,
                                   pds::SystemTemperatureGetResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_system_temperature_t temp = {0};

    ret = debug::pds_get_system_temperature(&temp);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_dietemp(temp.dietemp);
    proto_rsp->set_localtemp(temp.localtemp);
    proto_rsp->set_hbmtemp(temp.hbmtemp);
    return Status::OK;
}

Status
DebugSvcImpl::SystemPowerGet(ServerContext *context, const Empty *proto_req,
                             pds::SystemPowerGetResponse *proto_rsp) {
    sdk_ret_t ret;
    pds_system_power_t pow = {0};

    ret = debug::pds_get_system_power(&pow);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    proto_rsp->set_pin(pow.pin);
    proto_rsp->set_pout1(pow.pout1);
    proto_rsp->set_pout2(pow.pout2);
    return Status::OK;
}

Status
DebugSvcImpl::TraceUpdate(ServerContext *context, const pds::TraceRequest *proto_req,
                          pds::TraceResponse *proto_rsp) {
    utils::trace_level_e trace_level;

    switch (proto_req->trace_level()) {
    case pds::TRACE_LEVEL_NONE:
        trace_level = utils::trace_none;
        break;
    case pds::TRACE_LEVEL_DEBUG:
        trace_level = utils::trace_debug;
        break;
    case pds::TRACE_LEVEL_ERROR:
        trace_level = utils::trace_err;
        break;
    case pds::TRACE_LEVEL_WARN:
        trace_level = utils::trace_warn;
        break;
    case pds::TRACE_LEVEL_INFO:
        trace_level = utils::trace_info;
        break;
    case pds::TRACE_LEVEL_VERBOSE:
        trace_level = utils::trace_verbose;
        break;
    default:
        proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_INVALID_ARG);
        return Status::OK;
        break;
    }

    core::trace_update(trace_level);
    proto_rsp->set_apistatus(types::ApiStatus::API_STATUS_OK);
    proto_rsp->set_tracelevel(proto_req->trace_level());
    return Status::OK;
}

Status
DebugSvcImpl::TableStatsGet(ServerContext *context, const Empty *proto_req,
                            pds::TableStatsGetResponse *proto_rsp) {
    sdk_ret_t ret;

    if ((ret = debug::pds_table_stats_get(pds_table_stats_entry_to_proto, proto_rsp)) != SDK_RET_OK) {
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    return Status::OK;
}

Status
DebugSvcImpl::LlcSetup(ServerContext *context, const pds::LlcSetupRequest *proto_req,
                       pds::LlcSetupResponse *proto_rsp) {
    sdk_ret_t ret;
    sdk::asic::pd::llc_counters_t llc_args;

    memset (&llc_args, 0, sizeof(sdk::asic::pd::llc_counters_t));
    if (proto_req->type()) {
        llc_args.mask = (1 << (proto_req->type() - 1)); // Req Type starts at 1 so we need to subtract 1
    } else {
        llc_args.mask = 0xffffffff;                     // Clear LLC
    }

    ret = debug::pds_llc_setup(&llc_args);
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));

    return Status::OK;
}

Status
DebugSvcImpl::LlcStatsGet(ServerContext *context, const Empty *proto_req,
                          pds::LlcStatsGetResponse *proto_rsp) {
    sdk_ret_t ret;
    sdk::asic::pd::llc_counters_t llc_args;
    auto stats = proto_rsp->mutable_stats();

    memset (&llc_args, 0, sizeof(llc_counters_t));
    ret = debug::pds_llc_get(&llc_args);
    if (ret == SDK_RET_OK) {
        if (llc_args.mask == 0xffffffff) {
            stats->set_type(pds::LLC_COUNTER_CACHE_NONE);
        } else {
            for (int i = int(pds::LlcCounterType_MIN) + 1; i <= int(pds::LlcCounterType_MAX); i ++) {
                if ((llc_args.mask & (1 << (i - 1))) == 1) {
                    stats->set_type(pds::LlcCounterType(i));
                    break;
                }
            }
        }
        for (int i = 0; i < 16; i ++) {
            stats->add_count(llc_args.data[i]);
        }
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));

    return Status::OK;
}

Status
DebugSvcImpl::PbStatsGet(ServerContext *context, const Empty *proto_req,
                         pds::PbStatsGetResponse *proto_rsp) {
    sdk_ret_t ret;

    if ((ret = debug::pds_pb_stats_get(pds_pb_stats_entry_to_proto, proto_rsp)) != SDK_RET_OK) {
        proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    return Status::OK;
}

Status
DebugSvcImpl::FteStatsGet(ServerContext *context, const Empty *req,
                          pds::FteStatsGetResponse *rsp) {
    sdk_ret_t ret;

    if ((ret = debug::pds_fte_api_stats_get()) != SDK_RET_OK) {
        rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }
    if ((ret = debug::pds_fte_table_stats_get()) != SDK_RET_OK) {
        rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    return Status::OK;
}

Status
DebugSvcImpl::FteStatsClear(ServerContext *context, const pds::FteStatsClearRequest *req,
                          pds::FteStatsClearResponse *rsp) {
    sdk_ret_t ret;

    if (req->apistats()) {
        if ((ret = debug::pds_fte_api_stats_clear()) != SDK_RET_OK) {
            rsp->set_apistatus(sdk_ret_to_api_status(ret));
        }
    }

    if (req->tablestats()) {
        if ((ret = debug::pds_fte_table_stats_clear()) != SDK_RET_OK) {
            rsp->set_apistatus(sdk_ret_to_api_status(ret));
        }
    }

    return Status::OK;
}

Status
DebugSvcImpl::TraceFlush(ServerContext *context, const Empty *req,
                         Empty *rsp) {
    core::flush_logs();
    return Status::OK;
}

Status
DebugSvcImpl::MemoryTrim(ServerContext *context, const Empty *req,
                         Empty *rsp) {
    malloc_trim(0);
    return Status::OK;
}

Status
DebugSvcImpl::HeapGet(ServerContext *context, const pds::HeapGetRequest *req,
                      pds::HeapGetResponse *rsp) {
    pds_svc_heap_get(rsp);
    return Status::OK;
}

Status
DebugSvcImpl::MeterStatsGet(ServerContext *context, const pds::MeterStatsGetRequest *req,
                            pds::MeterStatsGetResponse *rsp) {
    sdk_ret_t ret;

    if ((ret = debug::pds_meter_stats_get(pds_meter_debug_stats_to_proto,
                                          req->statsindexlow(),
                                          req->statsindexhigh(),
                                          rsp)) != SDK_RET_OK) {
        rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    return Status::OK;
}

Status
DebugSvcImpl::SessionStatsGet(ServerContext *context, const pds::SessionStatsGetRequest *req,
                              pds::SessionStatsGetResponse *rsp) {
    sdk_ret_t ret;

    if ((ret = debug::pds_session_stats_get(pds_session_debug_stats_to_proto,
                                            req->statsindexlow(),
                                            req->statsindexhigh(),
                                            rsp)) != SDK_RET_OK) {
        rsp->set_apistatus(sdk_ret_to_api_status(ret));
    }

    return Status::OK;
}

Status
DebugSvcImpl::QueueCreditsGet(ServerContext *context, const Empty *req,
                              pds::QueueCreditsGetResponse *rsp) {
    sdk_ret_t ret;
    ret = sdk::asic::pd::queue_credits_get(pds_queue_credits_to_proto, rsp);
    rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

Status
DebugSvcImpl::StartAacsServer(ServerContext *context,
                              const pds::AacsRequest *proto_req,
                              Empty *proto_rsp) {
    PDS_TRACE_VERBOSE("Received AACS Server Start");
    if (proto_req) {
        debug::start_aacs_server(proto_req->aacsserverport());
    }

    return Status::OK;
}

Status
DebugSvcImpl::StopAacsServer(ServerContext *context,
                             const Empty *proto_req,
                             Empty *proto_rsp) {
    PDS_TRACE_VERBOSE("Received AACS Server Stop");
    debug::stop_aacs_server();
    return Status::OK;
}

Status
DebugSvcImpl::SlabGet(ServerContext *context,
                      const Empty *proto_req,
                      pds::SlabGetResponse *proto_rsp) {
    sdk_ret_t ret;
    ret = core::agent_state::state()->slab_walk(pds_slab_to_proto, proto_rsp);
    if (ret == SDK_RET_OK) {
        ret = debug::pds_slab_get(pds_slab_to_proto, proto_rsp);
    }
    proto_rsp->set_apistatus(sdk_ret_to_api_status(ret));
    return Status::OK;
}

static void
populate_port_info (uint8_t port_num, uint16_t status,
                    sdk::marvell::marvell_port_stats_t *stats,
                    pds::InternalPortResponseMsg *rsp) {
    bool is_up, full_duplex, txpause, fctrl;
    uint8_t speed;

    sdk::marvell::marvell_get_status_updown(status, &is_up);
    sdk::marvell::marvell_get_status_duplex(status, &full_duplex);
    sdk::marvell::marvell_get_status_speed(status, &speed);
    sdk::marvell::marvell_get_status_txpause(status, &txpause);
    sdk::marvell::marvell_get_status_flowctrl(status, &fctrl);

    pds::InternalPortResponse *response = rsp->add_response();
    pds::InternalPortStatus *internal_status = response->mutable_internalstatus();
    response->set_portnumber(port_num + 1);
    response->set_portdescr(sdk::marvell::marvell_get_descr(port_num));
    if (is_up) {
        internal_status->set_portstatus(pds::IF_STATUS_UP);
    } else {
        internal_status->set_portstatus(pds::IF_STATUS_DOWN);
    }
    internal_status->set_portspeed(::types::PortSpeed(speed));
    if (full_duplex) {
        internal_status->set_portmode(::pds::FULL_DUPLEX);
    } else{
        internal_status->set_portmode(::pds::HALF_DUPLEX);
    }
    internal_status->set_porttxpaused(txpause);
    internal_status->set_portflowctrl(fctrl);

    pds::InternalPortStats *internal_stats = response->mutable_stats();

    internal_stats->set_ingoodoctets(stats->in_good_octets);
    internal_stats->set_outoctets(stats->out_octets);
    internal_stats->set_inbadoctets(stats->in_bad_octets);
    internal_stats->set_inunicast(stats->in_unicast);
    internal_stats->set_inbroadcast(stats->in_broadcast);
    internal_stats->set_inmulticast(stats->in_multicast);
    internal_stats->set_inpause(stats->in_pause);
    internal_stats->set_inundersize(stats->in_undersize);
    internal_stats->set_infragments(stats->in_fragments);
    internal_stats->set_inoversize(stats->in_oversize);
    internal_stats->set_injabber(stats->in_jabber);
    internal_stats->set_inrxerr(stats->in_rx_err);
    internal_stats->set_infcserr(stats->in_fcs_err);

    internal_stats->set_outunicast(stats->out_unicast);
    internal_stats->set_outbroadcast(stats->out_broadcast);
    internal_stats->set_outmulticast(stats->out_multicast);
    internal_stats->set_outfcserr(stats->out_fcs_err);
    internal_stats->set_outpause(stats->out_pause);
    internal_stats->set_outcollisions(stats->out_collisions);
    internal_stats->set_outdeferred(stats->out_deferred);
    internal_stats->set_outsingle(stats->out_single);
    internal_stats->set_outmultiple(stats->out_multiple);
    internal_stats->set_outexcessive(stats->out_excessive);
    internal_stats->set_outlate(stats->out_late);
}

static void
marvell_get_port_info (uint8_t port_num, pds::InternalPortResponseMsg *rsp) {
    sdk::marvell::marvell_port_stats_t  stats;
    uint16_t data;

    bzero(&stats, sizeof(stats));
    sdk::marvell::marvell_get_port_status(port_num, &data);
    sdk::marvell::marvell_get_port_stats(port_num, &stats);
    populate_port_info(port_num, data, &stats, rsp);
}

Status
DebugSvcImpl::InternalPortGet(ServerContext *context,
                              const pds::InternalPortRequestMsg *req,
                              pds::InternalPortResponseMsg *rsp) {
    uint32_t i, nreqs = req->request_size();

    if (nreqs == 0) {
        return Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty Request");
    }

    for (i = 0; i < nreqs; i++) {
        auto     request      = req->request(i);
        uint8_t  port_num     = (uint8_t)request.portnumber();
        bool     has_port_num = (port_num != 0);

        port_num = port_num - 1;
        if (has_port_num) {
            if (port_num < MARVELL_NPORTS) {
                marvell_get_port_info(port_num, rsp);
            } else {
                return Status(grpc::StatusCode::INVALID_ARGUMENT,
                              "Internal port number must be between 1-7");
            }
        } else {
            for (port_num = 0; port_num < MARVELL_NPORTS; port_num++) {
                marvell_get_port_info(port_num, rsp);
            }
        }
    }
    return Status::OK;
}

Status
DebugSvcImpl::FlowStatsSummaryGet(ServerContext *context,
                                  const Empty *req,
                                  pds::FlowStatsSummaryResponse *rsp) {
    debug::pds_flow_stats_summary_t flow_stats;
    sdk_ret_t ret;

    ret = debug::pds_flow_summary_get(&flow_stats);
    rsp->set_apistatus(sdk_ret_to_api_status(ret));
    if (ret == sdk::SDK_RET_OK) {
        pds_flow_stats_summary_to_proto(&flow_stats, rsp);
    }
    return Status::OK;
}

Status
DebugSvcImpl::DataPathAssistStatsGet(ServerContext *context,
                                     const Empty *req,
                                     pds::DataPathAssistStatsResponse *rsp) {
    debug::pds_datapath_assist_stats_t datapath_assist_stats;
    sdk_ret_t ret;

    ret = debug::pds_datapath_assist_stats_get(&datapath_assist_stats);
    rsp->set_apistatus(sdk_ret_to_api_status(ret));
    if (ret == sdk::SDK_RET_OK) {
        pds_datapath_assist_stats_to_proto(&datapath_assist_stats, rsp);
    }
    return Status::OK;
}
