#ifndef __FTL_DEV_HPP__
#define __FTL_DEV_HPP__

#include <map>
#include <ev++.h>

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/shmstore/shmstore.hpp"
#include "nic/sdk/platform/evutils/include/evutils.h"
#include "nic/sdk/platform/devapi/devapi.hpp"
#include "nic/sdk/platform/misc/include/misc.h"

#include "device.hpp"
#include "pd_client.hpp"
#include "nic/include/ftl_dev_if.hpp"

#define FTL_DEV_NAME                        "ftl"
#define FTL_DEV_IFNAMSIZ                    16
#define FTL_DEV_PAGE_SIZE                   4096
#define FTL_DEV_PAGE_MASK                   (FTL_DEV_PAGE_SIZE - 1)

#define FTL_DEV_ADDR_ALIGN(addr, sz)        \
    (((addr) + ((uint64_t)(sz) - 1)) & ~((uint64_t)(sz) - 1))

#define FTL_DEV_PAGE_ALIGN(addr)            \
    FTL_DEV_ADDR_ALIGN(addr, FTL_DEV_PAGE_SIZE)

/*
 * Limit each single HBM read/write to this many bytes
 */
#define FTL_DEV_HBM_RW_LARGE_BYTES_MAX      8192

using namespace ftl_dev_if;

/**
 * Ftl Device Spec
 */
typedef struct ftl_devspec {
    std::string             name;
    // RES
    uint32_t                lif_count;
    uint32_t                session_hw_scanners;
    uint32_t                session_burst_size;
    uint32_t                session_burst_resched_time_us;
    uint32_t                conntrack_hw_scanners;
    uint32_t                conntrack_burst_size;
    uint32_t                conntrack_burst_resched_time_us;
    uint32_t                sw_pollers;
    uint32_t                sw_poller_qdepth;
} ftl_devspec_t;

/**
 * FTL Device persistent state
 */
typedef struct ftldev_pstate_v1_s {

    ftldev_pstate_v1_s(const ftl_devspec_t *dev_spec = nullptr) {
        version_magic = 1;
        base_lif_id_ = 0;
        strncpy0(name, FTL_DEV_NAME, sizeof(name));
        if (dev_spec) {
            lif_count = dev_spec->lif_count;
            session_hw_scanners = dev_spec->session_hw_scanners;
            session_burst_size = dev_spec->session_burst_size;
            session_burst_resched_time_us = dev_spec->session_burst_resched_time_us;
            conntrack_hw_scanners = dev_spec->conntrack_hw_scanners;
            conntrack_burst_size = dev_spec->conntrack_burst_size;
            conntrack_burst_resched_time_us = dev_spec->conntrack_burst_resched_time_us;
            sw_pollers = dev_spec->sw_pollers;
            sw_poller_qdepth = dev_spec->sw_poller_qdepth;
        } else {
            lif_count = 0;
            session_hw_scanners = 0;
            session_burst_size = 0;
            session_burst_resched_time_us = 0;
            conntrack_hw_scanners = 0;
            conntrack_burst_size = 0;
            conntrack_burst_resched_time_us = 0;
            sw_pollers = 0;
            sw_poller_qdepth = 0;
        }
    }

    uint64_t            version_magic;
    char                name[FTL_DEV_IFNAMSIZ];
    uint32_t            base_lif_id_;
    uint32_t            lif_count;
    uint32_t            session_hw_scanners;
    uint32_t            session_burst_size;
    uint32_t            session_burst_resched_time_us;
    uint32_t            conntrack_hw_scanners;
    uint32_t            conntrack_burst_size;
    uint32_t            conntrack_burst_resched_time_us;
    uint32_t            sw_pollers;
    uint32_t            sw_poller_qdepth;

    void base_lif_id_set(uint32_t base_lif_id)
    {
        SDK_ATOMIC_STORE_UINT32(&base_lif_id_, &base_lif_id);
    }
    uint32_t base_lif_id(void)
    {
        uint32_t base_id;
        SDK_ATOMIC_LOAD_UINT32(&base_lif_id_, &base_id);
        return base_id;
    }

} __PACK__ ftldev_pstate_v1_t;

/**
 * typdefs for persistent state;
 * these typedefs should always be latest version so no need to changes inside.
 */
typedef ftldev_pstate_v1_t ftldev_pstate_t;

/* forward declaration */
class FtlLif;

typedef int ftl_status_code_t;

const char *ftl_dev_opcode_str(uint32_t opcode);
const char *ftl_dev_status_str(ftl_status_code_t status);

/**
 * Flow Table Device
 */
class FtlDev : public Device {
public:
    FtlDev(devapi *dev_api,
            void *dev_spec,
            PdClient *pd_client,
            uint32_t lif_base,
            EV_P);
    ~FtlDev();

    static FtlDev *SoftFtlDev(devapi *dapi,
                              PdClient *pd_client,
                              EV_P);
    void HalEventHandler(bool status);
    virtual void DelphiMountEventHandler(bool mounted);
    void SetHalClient(devapi *dapi);

    ftl_status_code_t CmdHandler(ftl_devcmd_t *req,
                                 void *req_data,
                                 ftl_devcmd_cpl_t *rsp,
                                 void *rsp_data);
    std::string GetName() { return spec->name; }
    const std::string& DevNameGet(void) { return spec->name; }
    const ftl_devspec_t *DevSpecGet(void) { return spec; }
    PdClient *PdClientGet(void) { return pd; }
    devapi *DevApiGet(void) { return dev_api; }
    uint32_t NumLifsGet(void) { return lif_vec.size(); }
    FtlLif *LifFind(uint32_t lif_index);

    static struct ftl_devspec *
    ParseConfig(boost::property_tree::ptree::value_type node);

private:
    const ftl_devspec_t         *spec;
    ftldev_pstate_t             *dev_pstate;

    // PD Info
    PdClient                    *pd;
    // HAL Info
    devapi                      *dev_api;
    // Resources
    sdk::lib::shmstore          *shm_mem;
    std::vector<FtlLif *>       lif_vec;
    uint32_t                    lif_base;
    bool                        dtor_lif_free;
    bool                        delphi_mounted;

    ftl_status_code_t devcmd_reset(dev_reset_cmd_t *cmd,
                                   void *req_data,
                                   dev_reset_cpl_t *cpl,
                                   void *rsp_data);
    ftl_status_code_t devcmd_identify(dev_identify_cmd_t *cmd,
                                      void *req_data,
                                      dev_identify_cpl_t *cpl,
                                      void *rsp_data);
    // Tasks
    EV_P;
};

#endif
