/*
* Copyright (c) 2018, Pensando Systems Inc.
*/

#ifndef __DEV_HPP__
#define __DEV_HPP__

#include <string>
#include <map>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "nic/include/globals.hpp"
#include "nic/include/adminq.h"
#include "nic/include/notify.hpp"

#include "nic/sdk/lib/pal/pal.hpp"

#include "nic/sdk/platform/evutils/include/evutils.h"
#include "platform/src/lib/hal_api/include/hal_common_client.hpp"
#include "platform/src/lib/hal_api/include/hal_grpc_client.hpp"

#include "hal_client.hpp"

#ifdef __x86_64__

#define READ_MEM        sdk::lib::pal_mem_read
#define WRITE_MEM       sdk::lib::pal_mem_write
#define MEM_SET(pa, val, sz, flags) { \
    uint8_t v = val; \
    for (size_t i = 0; i < sz; i += sizeof(v)) { \
        sdk::lib::pal_mem_write(pa + i, &v, sizeof(v)); \
    } \
}

#define READ_REG        sdk::lib::pal_reg_read
#define WRITE_REG       sdk::lib::pal_reg_write
static inline uint32_t READ_REG32(uint64_t addr)
{
    uint32_t val;
    sdk::lib::pal_reg_write(addr, &val);
    return val;
}
#define WRITE_REG32(addr, val) { \
    uint32_t v = val; \
    sdk::lib::pal_reg_write(addr, &v); \
}
static inline uint64_t READ_REG64(uint64_t addr)
{
    uint64_t val;
    sdk::lib::pal_reg_read(addr, (uint32_t *)&val, 2);
    return val;
}
#define WRITE_REG64(addr, val) { \
    uint64_t v = val; \
    sdk::lib::pal_reg_write(addr, (uint32_t *)&v, 2); \
}

#define WRITE_DB64      sdk::lib::pal_ring_db64

#else
#include "nic/sdk/platform/pal/include/pal.h"
#define READ_MEM        pal_mem_rd
#define WRITE_MEM       pal_mem_wr
#define MEM_SET         pal_memset

#define READ_REG        pal_reg_rd32w
#define WRITE_REG       pal_reg_wr32w
#define READ_REG32      pal_reg_rd32
#define WRITE_REG32     pal_reg_wr32
#define READ_REG64      pal_reg_rd64
#define WRITE_REG64     pal_reg_wr64

#define WRITE_DB64      pal_reg_wr64
#endif

#ifdef __aarch64__
#include "nic/sdk/platform/pciemgr/include/pciemgr.h"
#endif
#include "nic/sdk/platform/pciemgrutils/include/pciemgrutils.h"
#include "nic/sdk/platform/pciehdevices/include/pciehdevices.h"


/**
 * ADMINQ
 */
#define NICMGR_QTYPE_REQ        0
#define NICMGR_QTYPE_RESP       1

enum {
    NICMGR_THREAD_ID_MIN           = 0,
    NICMGR_THREAD_ID_DELPHI_CLIENT = 1,
    NICMGR_THREAD_ID_MAX           = 2,
};

enum {
    NICMGR_TIMER_ID_NONE                     = 0,
    NICMGR_TIMER_ID_MIN                      = 1,
    NICMGR_TIMER_ID_HEARTBEAT                = NICMGR_TIMER_ID_MIN,
    NICMGR_TIMER_ID_MAX                      = 2,
};

/**
 * Device Types
 */
enum DeviceType
{
    INVALID,
    MNIC,
    DEBUG,
    ETH,
    ACCEL,
    NVME,
    VIRTIO,
};

/**
 * OPROM
 */
typedef enum OpromType_s {
    OPROM_UNKNOWN,
    OPROM_LEGACY,
    OPROM_UEFI,
    OPROM_UNIFIED
} OpromType;

const char *oprom_type_to_str(OpromType_s);

/**
 * ETH Device type
 */
typedef enum EthDevType_s {
    ETH_UNKNOWN,
    ETH_HOST,
    ETH_HOST_MGMT,
    ETH_MNIC_OOB_MGMT,
    ETH_MNIC_INTERNAL_MGMT,
    ETH_MNIC_INBAND_MGMT,
} EthDevType;

const char *eth_dev_type_to_str(EthDevType type);

/**
 * Eth Device Spec
 */
struct eth_devspec {
    // Delphi
    uint64_t dev_uuid;
    // Device
    EthDevType eth_type;
    std::string name;
    OpromType oprom;
    uint8_t pcie_port;
    bool host_dev;
    // Network
    uint32_t uplink_port_num;
    std::string qos_group;
    // RES
    uint32_t lif_count;
    uint32_t rxq_count;
    uint32_t txq_count;
    uint32_t eq_count;
    uint32_t adminq_count;
    uint32_t intr_count;
    uint64_t mac_addr;
    // RDMA
    bool enable_rdma;
    uint32_t pte_count;
    uint32_t key_count;
    uint32_t ah_count;
    uint32_t rdma_sq_count;
    uint32_t rdma_rq_count;
    uint32_t rdma_cq_count;
    uint32_t rdma_eq_count;
    uint32_t rdma_adminq_count;
    uint32_t rdma_pid_count;
    uint32_t barmap_size;    // in 8MB units
};

typedef struct dev_cmd_db_s {
    uint32_t    v;
} dev_cmd_db_t;

/**
 * Utils
 */
void invalidate_rxdma_cacheline(uint64_t addr);
void invalidate_txdma_cacheline(uint64_t addr);

class PdClient;

/**
 * Base Class for devices
 */
class Device {
public:
    virtual void ThreadsWaitJoin(void) {}
    enum DeviceType GetType() { return type; }
    void SetType(enum DeviceType type) { this->type = type;}
private:
    enum DeviceType type;
};

/**
 * Device Manager
 */
class DeviceManager {
public:
    DeviceManager(std::string config_file, enum ForwardingMode fwd_mode,
        platform_t platform);

    int LoadConfig(std::string path);
    Device *AddDevice(enum DeviceType type, void *dev_spec);
    static DeviceManager *GetInstance() { return instance; }

    void HalEventHandler(bool is_up);
    void LinkEventHandler(port_status_t *evd);

    static void AdminQPoll(void *obj);

    Device *GetDevice(uint64_t id);
    void CreateUplinkVRFs();
    void SetHalClient(HalClient *hal_client, HalCommonClient *hal_cmn_client);
    int GenerateQstateInfoJson(std::string qstate_info_file);
    void ThreadsWaitJoin(void);
    static string ParseDeviceConf(string input_arg);

private:
    static DeviceManager *instance;

    boost::property_tree::ptree spec;
    std::map<uint64_t, Device*> lif_map; // lif -> device
    std::vector<Device*> devices;

    // Service Lif Info
    hal_lif_info_t hal_lif_info_;
    static struct queue_info qinfo[NUM_QUEUE_TYPES];
    int DumpQstateInfo(boost::property_tree::ptree &lifs);

    // HAL Info
    HalClient *hal;
    HalCommonClient *hal_common_client;
    PdClient *pd;

    bool init_done;
    std::string config_file;

    // Tasks
    evutil_timer adminq_timer;
    evutil_check adminq_check;
    evutil_prepare adminq_prepare;

    // AdminQ
    uint64_t req_ring_base;
    uint64_t resp_ring_base;
    uint16_t ring_size;
    uint16_t req_head;
    uint16_t req_tail;
    uint16_t resp_head;
    uint16_t resp_tail;
    ForwardingMode fwd_mode;
};

#endif /* __DEV_HPP__ */
