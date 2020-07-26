/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __ETH_LIF_HPP__
#define __ETH_LIF_HPP__

using namespace std;

#include "nic/sdk/lib/edma/edmaq.h"
#include "nic/sdk/lib/shmstore/shmstore.hpp"
#include "nic/sdk/platform/devapi/devapi.hpp"
#include "pd_client.hpp"
#include "eth_pstate.hpp"
#include "ev.h"
#include <unordered_map>
#include <queue>

namespace pt = boost::property_tree;

class Eth;
class AdminQ;
class EdmaQ;
typedef uint8_t status_code_t;
typedef uint16_t cmd_opcode_t;

/**
 * ETH Qtype Enum
 */
enum eth_hw_qtype {
    ETH_HW_QTYPE_RX = 0,
    ETH_HW_QTYPE_TX = 1,
    ETH_HW_QTYPE_ADMIN = 2,
    ETH_HW_QTYPE_SQ = 3,
    ETH_HW_QTYPE_RQ = 4,
    ETH_HW_QTYPE_CQ = 5,
    ETH_HW_QTYPE_EQ = 6,
    ETH_HW_QTYPE_SVC = 7,
    ETH_HW_QTYPE_NONE = 15,
};


#define BIT_MASK(n) ((1ULL << n) - 1)

#define LG2_LIF_STATS_SIZE 10
#define LIF_STATS_SIZE (1 << LG2_LIF_STATS_SIZE)

#define ETH_NOTIFYQ_QTYPE 7
#define ETH_NOTIFYQ_QID 0
#define LG2_ETH_NOTIFYQ_RING_SIZE 4
#define ETH_NOTIFYQ_RING_SIZE (1 << LG2_ETH_NOTIFYQ_RING_SIZE)

#define ETH_EDMAQ_QTYPE 7
#define ETH_EDMAQ_QID 1
#define LG2_ETH_EDMAQ_RING_SIZE 6
#define ETH_EDMAQ_RING_SIZE (1 << LG2_ETH_EDMAQ_RING_SIZE)

#define ETH_ADMINQ_REQ_QTYPE 7
#define ETH_ADMINQ_REQ_QID 2
#define LG2_ETH_ADMINQ_REQ_RING_SIZE 4
#define ETH_ADMINQ_REQ_RING_SIZE (1 << LG2_ETH_ADMINQ_REQ_RING_SIZE)

#define ETH_ADMINQ_RESP_QTYPE 7
#define ETH_ADMINQ_RESP_QID 3
#define LG2_ETH_ADMINQ_RESP_RING_SIZE 4
#define ETH_ADMINQ_RESP_RING_SIZE (1 << LG2_ETH_ADMINQ_RESP_RING_SIZE)

#define ETH_EDMAQ_ASYNC_QTYPE 7
#define ETH_EDMAQ_ASYNC_QID 4
#define LG2_ETH_EDMAQ_ASYNC_RING_SIZE 6
#define ETH_EDMAQ_ASYNC_RING_SIZE (1 << LG2_ETH_EDMAQ_ASYNC_RING_SIZE)

#define RXDMA_Q_QUIESCE_WAIT_S 0.001  // 1 ms
#define RXDMA_LIF_QUIESCE_WAIT_S 0.01 // 10 ms

/**
 * LIF Resource structure
 */
typedef struct eth_lif_res_s {
    uint64_t lif_index;
    uint64_t lif_id;
    uint64_t intr_base;
    uint64_t rx_eq_base;
    uint64_t tx_eq_base;
    uint64_t cmb_mem_addr;
    uint64_t cmb_mem_size;
} eth_lif_res_t;

class EthLif
{
  public:
    EthLif(Eth *dev, devapi *dev_api, const void *dev_spec, PdClient *pd_client, eth_lif_res_t *res,
           EV_P);

    void Init(void);
    void UpgradeGracefulInit(void);
    void UpgradeHitlessInit(void);

    status_code_t CmdInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t Reset();
    bool EdmaProxy(edma_opcode opcode, uint64_t from, uint64_t to, uint16_t size,
                   struct edmaq_ctx *ctx);
    bool EdmaAsyncProxy(edma_opcode opcode, uint64_t from, uint64_t to, uint16_t size,
                        struct edmaq_ctx *ctx);

    // Command Handlers
    status_code_t CmdProxyHandler(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t CmdHandler(void *req, void *req_data, void *resp, void *resp_data);

    static const char *opcode_to_str(cmd_opcode_t opcode);

    // Event Handlers
    void LinkEventHandler(port_status_t *evd);
    void LifEventHandler(port_status_t *evd);
    void XcvrEventHandler(port_status_t *evd);
    void DelphiMountEventHandler(bool mounted);
    sdk_ret_t SendDeviceReset(void);
    void HalEventHandler(bool status);

    void SetHalClient(devapi *dev_api);

    sdk_ret_t UpgradeSyncHandler(void);
    sdk_ret_t UpdateQStatus(bool enable);
    sdk_ret_t ServiceControl(bool start);

    bool IsLifQuiesced();

    int GenerateQstateInfoJson(pt::ptree &lifs);

    void AddLifMetrics(void);

    EV_P;

    /* Station mac address */
    void GetStationMac(uint8_t macaddr[6]);
    void SetStationMac(uint8_t macaddr[6]);

    /* Default vlan */
    uint16_t GetVlan();
    void SetVlan(uint16_t vlan);
    status_code_t AddVFVlanFilter(uint16_t vlan);

    /* LIF admin state */
    status_code_t SetAdminState(uint8_t admin_state);
    uint8_t GetAdminState() { return lif_pstate->admin_state; }

    /* Proxy LIF admin state of a VF controlled by a PF */
    status_code_t SetProxyAdminState(uint8_t admin_state);
    uint8_t GetProxyAdminState() { return lif_pstate->proxy_admin_state; }

    status_code_t LifToggleTxRxState(bool enable);
    status_code_t LifQuiesce();

    /* LIF overall link state */
    enum eth_lif_state GetLifLinkState() { return lif_pstate->state; }

    /* Address to store VF's stats */
    void SetPfStatsAddr(uint64_t addr);
    uint64_t GetPfStatsAddr();

    /* Rate limiting */
    status_code_t SetMaxTxRate(uint32_t rate_in_mbps);
    status_code_t GetMaxTxRate(uint32_t *rate_in_mbps);

    /* PF VF functions */
    void AddAdminStateSubscriber(EthLif *vf_lif);
    void DelAdminStateSubscriber(EthLif *vf_lif);
    void NotifyAdminStateChange();

    /* Attributes */
    uint64_t GetLifId() { return hal_lif_info_.lif_id; };
    std::string GetName() { return hal_lif_info_.name; };

private:
    Eth *dev;
    std::unordered_map<uint64_t, EthLif *> subscribers_map;
    static sdk::lib::indexer *fltr_allocator;
    bool device_inited;
    // PD Info
    PdClient *pd;
    // HAL Info
    devapi *dev_api;
    lif_info_t hal_lif_info_;
    // LIF Info
    eth_lif_res_t *res;
    uint8_t cosA, cosB, admin_cosA, admin_cosB;
    // Spec
    const struct eth_devspec *spec;
    struct queue_info qinfo[NUM_QUEUE_TYPES];
    // Config
    union ionic_lif_config *lif_config;
    uint64_t lif_config_addr;
    // Status
    struct ionic_lif_status *lif_status;
    uint64_t lif_status_addr;
    // Stats
    uint64_t lif_stats_addr;
    uint64_t pf_lif_stats_addr; //TODO: Revisit pesistent state

    // NotifyQ
    uint16_t notify_ring_head;
    uint64_t notify_ring_base;
    uint8_t notify_enabled;
    // EdmaQ
    uint64_t edma_buf_addr;
    uint8_t *edma_buf;
    // Features
    uint64_t features;
    // Network info
    map<uint64_t, uint64_t> mac_addrs;
    map<uint64_t, uint16_t> vlans;
    map<uint64_t, tuple<uint64_t, uint16_t>> mac_vlans;
    queue<tuple<int, uint64_t, uint16_t>> deferred_filters;
    uint32_t mtu;
    // Tasks
    ev_timer stats_timer = {0};
    ev_timer sched_eval_timer = {0};

    // persistent config
    ethlif_pstate_t *lif_pstate;
    sdk::lib::shmstore *shm_mem;

    // Services
    AdminQ *adminq;
    EdmaQ *edmaq;
    EdmaQ *edmaq_async;

    /* AdminQ Commands */
    static void AdminCmdHandler(void *obj,
        void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdAccessCheck(cmd_opcode_t opcode);

    status_code_t _CmdSetAttr(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t SetFeatures(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t RssConfig(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdGetAttr(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdRxSetMode(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdRxFilterAdd(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdRxFilterDel(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdQIdentify(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t AdminQIdentify(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t NotifyQIdentify(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t TxQIdentify(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t RxQIdentify(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdQInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t AdminQInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t NotifyQInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t EQInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t TxQInit(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t RxQInit(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdQControl(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t AdminQControl(uint32_t qid, bool enable);
    status_code_t EdmaQControl(uint32_t qid, bool enable);
    status_code_t NotifyQControl(uint32_t qid, bool enable);

    status_code_t _CmdRDMAResetLIF(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdRDMACreateEQ(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdRDMACreateCQ(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdRDMACreateAdminQ(void *req, void *req_data, void *resp, void *resp_data);

    status_code_t _CmdFwDownload(void *req, void *req_data, void *resp, void *resp_data);
    status_code_t _CmdFwControl(void *req, void *req_data, void *resp, void *resp_data);

    // Callbacks
    static void StatsUpdate(EV_P_ ev_timer *w, int events);
    static void SchedBulkEvalHandler(EV_P_ ev_timer *w, int events);

    // Helper methods
    bool IsLifTypeCpu(void);
    bool IsLifInitialized();
    bool IsTrustedLif();

    /* Filters */
    status_code_t _AddFilter(int type, uint64_t mac_addr, uint16_t vlan,
                             uint32_t *filter_id);
    status_code_t _AddMacFilter(uint64_t mac_addr, uint32_t *filter_id);
    status_code_t _AddVlanFilter(uint16_t vlan, uint32_t *filter_id);
    status_code_t _AddMacVlanFilter(uint64_t mac_addr, uint16_t vlan,
                                   uint32_t *filter_id);
    status_code_t _CheckRxMacFilterPerm(uint64_t mac);

    /* Rx mode */
    status_code_t _CheckRxModePerm(int mode);

    void _DeferFilter(int filter_type, uint64_t mac_addr, uint16_t vlan);
    void _AddDeferredFilters();
    void FreeUpMacFilters();
    void FreeUpVlanFilters();
    void FreeUpMacVlanFilters();

    status_code_t UpdateQFeatures();

    void Create();

    void LifConfigStatusMem(bool mem_clr);
    status_code_t LifQInit(bool mem_clr);
    void QinfoInit(void);
    void LifStatsInit();
    void LifStatsClear();

    // Lif state functions
    status_code_t SetLifLinkState();
    void NotifyLifLinkState();
    status_code_t UpdateHalLifAdminStatus(lif_state_t hal_lif_admin_state);

    static const char *lif_state_to_str(enum eth_lif_state state);
    static const char *port_status_to_str(uint8_t port_status);
    static const char *admin_state_to_str(uint8_t admin_state);
    static lif_state_t ConvertEthLifStateToLifState(enum eth_lif_state lif_state);

};

#endif /* __ETH_LIF_HPP__ */
