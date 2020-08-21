/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <endian.h>
#include <sstream>
#include <sys/time.h>

#include "nic/include/base.hpp"
#include "nic/sdk/platform/misc/include/misc.h"

#include "logger.hpp"
#include "dev.hpp"
#include "ftl_dev.hpp"
#include "ftl_lif.hpp"
#include "pd_client.hpp"

/*
 * Max amount of time to wait for LIF to be fully created by hard init process
 */
#define FTL_DEV_LIF_CREATE_TIME_US  (60 * USEC_PER_SEC)

/*
 * Devapi availability check
 */
#define FTL_DEVAPI_CHECK(ret_val)                                               \
    if (!dev_api) {                                                             \
        NIC_LOG_ERR("{}: Uninitialized devapi", DevNameGet());                  \
        return ret_val;                                                         \
    }

#define FTL_DEVAPI_CHECK_VOID()                                                 \
    if (!dev_api) {                                                             \
        NIC_LOG_ERR("{}: Uninitialized devapi", DevNameGet());                  \
        return;                                                                 \
    }

/*
 * LIFs iterator
 */
#define FOR_EACH_FTL_DEV_LIF(idx, lif)                                          \
    for (idx = 0, lif = LifFind(0); lif; idx++, lif = LifFind(idx))


FtlDev::FtlDev(devapi *dapi,
               void *dev_spec,
               PdClient *pd_client,
               uint32_t lif_base,
               EV_P) :
    spec((ftl_devspec_t *)dev_spec),
    pd(pd_client),
    dev_api(dapi),
    lif_base(lif_base),
    dtor_lif_free(false),
    delphi_mounted(false),
    EV_A(EV_A)
{
    ftl_lif_res_t       lif_res;
    sdk_ret_t           ret = SDK_RET_OK;
    DeviceManager *devmgr = DeviceManager::GetInstance();
    bool is_soft_init = false;

    /*
     * Allocate LIFs if necessary
     */
    if (!lif_base) {
        shm_mem = devmgr->getShmstore();
        dev_pstate = (ftldev_pstate_t *)
            shm_mem->create_segment(FTL_DEV_NAME, sizeof(*dev_pstate));
        if (!dev_pstate) {
            NIC_LOG_ERR("{}: Failed to locate memory for pstate", DevNameGet());
            throw;
        }
        new (dev_pstate) ftldev_pstate_t(spec);
        ret = pd->lm_->alloc_id(&lif_base, spec->lif_count);
        if (ret != SDK_RET_OK) {
            NIC_LOG_ERR("{}: Failed to allocate lifs. ret: {}", DevNameGet(), ret);
            throw;
        }
        dev_pstate->base_lif_id_set(lif_base);
        dtor_lif_free = true;
    } else {
        is_soft_init = true;
    }

    NIC_LOG_DEBUG("{}: lif_base {} lif_count {}",
                  DevNameGet(), lif_base, spec->lif_count);
    /*
     * Create LIF
     */
    lif_res.lif_id = lif_base;
    lif_res.index = 0;
    auto lif = new FtlLif(*this, lif_res, is_soft_init, EV_DEFAULT);
    if (!lif) {
        NIC_LOG_ERR("{}: failed to create FtlLif {}",
                    DevNameGet(), lif_res.lif_id);
        throw;
    }
    lif_vec.push_back(lif);
}

FtlDev *
FtlDev::SoftFtlDev(devapi *dapi,
                   PdClient *pd_client,
                   EV_P)
{
    DeviceManager *devmgr = DeviceManager::GetInstance();
    sdk::lib::shmstore  *shm_mem = devmgr->getRestoreShmstore();
    FtlDev              *dev;
    ftldev_pstate_t     *dev_pstate;
    ftl_devspec_t       *spec;
    ftl_timestamp_t     ts;
    uint32_t            lif_base;

    dev_pstate = (ftldev_pstate_t *) shm_mem->open_segment(FTL_DEV_NAME);
    if (!dev_pstate) {
        NIC_LOG_DEBUG("No pstate memory for {} - skipping soft init",
                      FTL_DEV_NAME);
        return nullptr;
    }

    ts.time_expiry_set(FTL_DEV_LIF_CREATE_TIME_US);
    while (!dev_pstate->base_lif_id()) {
        if (ts.time_expiry_check()) {
            NIC_LOG_ERR("Failed to locate lifs for {}", FTL_DEV_NAME);
            throw;
        }

        /*
         * Inside nicmgr init thread so it's ok to sleep
         */
        usleep(1000);
    }
    NIC_LOG_DEBUG("FTL lif created...");

    lif_base = dev_pstate->base_lif_id();
    if (!lif_base) {
        NIC_LOG_ERR("Invalid base_lif_id for {}", FTL_DEV_NAME);
        throw;
    }

    spec = new struct ftl_devspec;
    memset(spec, 0, sizeof(*spec));

    spec->name.assign(dev_pstate->name);
    spec->lif_count = dev_pstate->lif_count;
    spec->session_hw_scanners = dev_pstate->session_hw_scanners;
    spec->session_burst_size = dev_pstate->session_burst_size;
    spec->session_burst_resched_time_us = dev_pstate->session_burst_resched_time_us;
    spec->conntrack_hw_scanners = dev_pstate->conntrack_hw_scanners;
    spec->conntrack_burst_size = dev_pstate->conntrack_burst_size;
    spec->conntrack_burst_resched_time_us = dev_pstate->conntrack_burst_resched_time_us;
    spec->sw_pollers = dev_pstate->sw_pollers;
    spec->sw_poller_qdepth = dev_pstate->sw_poller_qdepth;

    dev = new FtlDev(dapi, spec, pd_client, lif_base, EV_A);
    if (!dev) {
        NIC_LOG_ERR("Failed to create device for {}", FTL_DEV_NAME);
        throw;
    }

    /*
     * HAL can be considered as up at this point for soft init
     * (by virtue of lif_fully_created() above)
     */
    dev->HalEventHandler(true);
    return dev;
}

FtlDev::~FtlDev()
{
    FtlLif      *lif;
    uint32_t    i;

    FOR_EACH_FTL_DEV_LIF(i, lif) {
        delete lif;
    }
    lif_vec.clear();
    if (dtor_lif_free) {
        pd->lm_->free_id(lif_base, spec->lif_count);
    }
}

void
FtlDev::HalEventHandler(bool status)
{
    FtlLif      *lif;
    uint32_t    i;

    FOR_EACH_FTL_DEV_LIF(i, lif) {
        lif->HalEventHandler(status);
    }
}

void
FtlDev::DelphiMountEventHandler(bool mounted)
{
    delphi_mounted = mounted;
}

void
FtlDev::SetHalClient(devapi *dapi)
{
    FtlLif      *lif;
    uint32_t    i;

    dev_api = dapi;
    FOR_EACH_FTL_DEV_LIF(i, lif) {
        lif->SetHalClient(dapi);
    }
}

struct ftl_devspec *
FtlDev::ParseConfig(boost::property_tree::ptree::value_type node)
{
    ftl_devspec_t           *ftl_spec;
    auto                    val = node.second;

    ftl_spec = new struct ftl_devspec;
    memset(ftl_spec, 0, sizeof(*ftl_spec));

    ftl_spec->name = val.get<string>("name");
    ftl_spec->lif_count = 1;
    ftl_spec->session_hw_scanners = val.get<uint32_t>("session_hw_scanners");
    ftl_spec->session_burst_size = val.get<uint32_t>("session_burst_size");
    ftl_spec->session_burst_resched_time_us = val.get<uint32_t>("session_burst_resched_time_us");
    ftl_spec->conntrack_hw_scanners = val.get<uint32_t>("conntrack_hw_scanners");
    ftl_spec->conntrack_burst_size = val.get<uint32_t>("conntrack_burst_size");
    ftl_spec->conntrack_burst_resched_time_us = val.get<uint32_t>("conntrack_burst_resched_time_us");
    ftl_spec->sw_pollers = val.get<uint32_t>("sw_pollers");
    ftl_spec->sw_poller_qdepth = val.get<uint32_t>("sw_poller_qdepth");
    NIC_LOG_DEBUG("Creating FTL device with name: {}", ftl_spec->name);

    return ftl_spec;
}

const char *
ftl_dev_opcode_str(uint32_t opcode)
{
    switch(opcode) {
        FTL_DEVCMD_OPCODE_CASE_TABLE
        default: return "FTL_DEVCMD_UNKNOWN";
    }
}

static const char   *status_str_table[] = {
    FTL_RC_STR_TABLE
};

const char *
ftl_dev_status_str(ftl_status_code_t status)
{
    if (status < (ftl_status_code_t)ARRAY_SIZE(status_str_table)) {
        return status_str_table[status];
    }

    return "FTL_RC_UNKNOWN";
}

ftl_status_code_t
FtlDev::CmdHandler(ftl_devcmd_t *req,
                   void *req_data,
                   ftl_devcmd_cpl_t *rsp,
                   void *rsp_data)
{
    ftl_status_code_t   status;

    NIC_LOG_DEBUG("{}: Handling cmd: {}", DevNameGet(),
                  ftl_dev_opcode_str(req->cmd.opcode));

    switch (req->cmd.opcode) {

    case FTL_DEVCMD_OPCODE_NOP:
        status = FTL_RC_SUCCESS;
        break;

    case FTL_DEVCMD_OPCODE_RESET:
        status = devcmd_reset(&req->dev_reset, req_data,
                              &rsp->dev_reset, rsp_data);
        break;

    case FTL_DEVCMD_OPCODE_IDENTIFY:
        status = devcmd_identify(&req->dev_identify, req_data,
                                 &rsp->dev_identify, rsp_data);
        break;

    default:
        auto lif = LifFind(req->cmd.lif_index);
        if (!lif) {
            NIC_LOG_ERR("{}: lif {} unexpectedly missing", DevNameGet(),
                        req->cmd.lif_index);
            return FTL_RC_ENOENT;
        }

        status = lif->CmdHandler(req, req_data, rsp, rsp_data);
        break;
    }

    rsp->status = status;
    NIC_LOG_DEBUG("{}: Done cmd: {}, status: {}", DevNameGet(),
                  ftl_dev_opcode_str(req->cmd.opcode), status);
    return status;
}


ftl_status_code_t
FtlDev::devcmd_reset(dev_reset_cmd_t *cmd,
                     void *req_data,
                     dev_reset_cpl_t *cpl,
                     void *resp_data)
{

    FtlLif              *lif;
    ftl_status_code_t   status = FTL_RC_SUCCESS;
    uint32_t            i;

    NIC_LOG_DEBUG("{}: FTL_DEVCMD_OPCODE_RESET", DevNameGet());

    FOR_EACH_FTL_DEV_LIF(i, lif) {

        /*
         * Reset at the device level will reset and delete the LIF.
         */
        status = lif->reset(true);
        if (status != FTL_RC_SUCCESS) {
            break;
        }
    }

    return status;
}

ftl_status_code_t
FtlDev::devcmd_identify(dev_identify_cmd_t *cmd,
                        void *req_data,
                        dev_identify_cpl_t *cpl,
                        void *rsp_data)
{
    dev_identity_t      *rsp = (dev_identity_t *)rsp_data;

    NIC_LOG_DEBUG("{}: FTL_DEVCMD_OPCODE_IDENTIFY", DevNameGet());

    if (cmd->type >= FTL_DEV_TYPE_MAX) {
        NIC_LOG_ERR("{}: bad device type {}", DevNameGet(), cmd->type);
        return FTL_RC_EINVAL;
    }
    if (cmd->ver != IDENTITY_VERSION_1) {
        NIC_LOG_ERR("{}: unsupported version {}", DevNameGet(), cmd->ver);
        return FTL_RC_EVERSION;
    }
    memset(&rsp->base, 0, sizeof(rsp->base));
    rsp->base.version = IDENTITY_VERSION_1;
    rsp->base.nlifs = spec->lif_count;

    cpl->ver = IDENTITY_VERSION_1;
    return FTL_RC_SUCCESS;
}

FtlLif *
FtlDev::LifFind(uint32_t lif_index)
{
    if (lif_index >= lif_vec.size()) {
        return nullptr;
    }
    return lif_vec.at(lif_index);
}

