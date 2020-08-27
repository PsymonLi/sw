//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <stddef.h>

#include "nic/include/base.hpp"
#include "nic/include/globals.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/platform/utils/mpartition.hpp"
#include "nic/sdk/platform/utils/program.hpp"
#include "nic/sdk/platform/ring/ring.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/asic/asic.hpp"
#include "nic/sdk/lib/logger/logger.hpp"
#include "nic/sdk/lib/indexer/indexer.hpp"
#include "nic/include/tcp_common.h"
#include "nic/utils/tcpcb_pd/tcpcb_pd.hpp"
#include "nic/sdk/platform/utils/lif_mgr/lif_mgr.hpp"
#include "nic/sdk/asic/common/asic_common.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "tcp_prog_hw.hpp"

using sdk::platform::ring_meta_t;
using sdk::platform::ring;
using sdk::asic::lif_qstate_t;
using sdk::platform::utils::lif_mgr;

ring _serq;
ring _sesq;
ring rnmdpr_ring;
ring tnmdpr_ring;
ring gc_rnmdr_ring;
lif_mgr *_sock_lif;

static ring _ooo_rx2tx_q;
static asic_cfg_t _asic_cfg;
static lif_mgr *_tcp_lif;
static lif_mgr *_gc_lif;
static uint64_t _stats_base;
static uint64_t _global_base;
static program_info *_prog_info;
static catalog *_platform_catalog;
static indexer *_tcp_flow_idxr;
static indexer *_tcp_freed_flows_idxr; // This should ideally be a fifo

extern ring tcp_actl_q;

extern "C" {

#ifdef VPP_TRACE_ENABLE
int
tcp_printf (sdk_trace_level_e trace_level, const char *format, ...)
{
    va_list args;
    int ret = 0;
    static FILE *fp = NULL;

    if (!fp) {
        fp = fopen("/var/log/pensando/vpp.log", "a");
    }

    va_start(args, format);

    ret = vfprintf(fp, format, args);
    fprintf(fp, "\n");
    fflush(fp);

    va_end(args);
    return ret;
}

char*
tcp_rawstr(void *data, uint32_t len) {
    static char str[512];
    uint32_t i = 0;
    uint32_t slen = 0;
    for (i = 0; i < len; i++) {
        slen += sprintf(str+slen, "%02x", ((uint8_t*)data)[i]);
    }
    str[slen] = 0;
    return str;
}
#endif

void
pds_tcp_init_rings(void)
{
    ring_meta_t *ring_meta;

    // RNMDPR
    ring_meta = ring::find_ring(std::string("RNMDR"), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    rnmdpr_ring.init(ring_meta, _asic_cfg.mempartition, false);

    // TNMDPR
    ring_meta = ring::find_ring(std::string("TNMDR"), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    tnmdpr_ring.init(ring_meta, _asic_cfg.mempartition, false);

    // SERQ
    ring_meta = ring::find_ring(std::string(RING_NAME_SERQ), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    _serq.init(ring_meta, _asic_cfg.mempartition, false);

    // SESQ
    ring_meta = ring::find_ring(std::string(RING_NAME_SESQ), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    _sesq.init(ring_meta, _asic_cfg.mempartition, false);

    // OOO_RX2TX
    ring_meta = ring::find_ring(std::string(RING_NAME_TCP_OOO_RX2TX),
            _asic_cfg.num_rings, _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    _ooo_rx2tx_q.init(ring_meta, _asic_cfg.mempartition, false);

    // TCP_ACTL_Q
    ring_meta = ring::find_ring(std::string(RING_NAME_ACTL_Q), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    tcp_actl_q.init(ring_meta, _asic_cfg.mempartition, false);

    // GC_RNMDR
    ring_meta = ring::find_ring(std::string(RING_NAME_GC_RNMDR), _asic_cfg.num_rings,
            _asic_cfg.ring_meta);
    SDK_ASSERT(ring_meta);
    gc_rnmdr_ring.init(ring_meta, _asic_cfg.mempartition, false);
}

int 
pds_get_max_num_tcp_flows ()
{
    int num_tcp_flows;

    num_tcp_flows = vpp::tcp::pds_get_platform_max_tcp_qid();
    return(num_tcp_flows);
}

int
pds_init(void)
{
#ifdef FIXME_NVME_POC
    pal_ret_t pal_ret;
#endif
    int num_tcp_flows;

    // Initialize logger
#ifdef VPP_TRACE_ENABLE
    sdk::lib::logger::init(tcp_printf);
#endif
    SDK_TRACE_DEBUG("TCP init starting");

    // Initialize pal
#ifdef FIXME_NVME_POC
    pal_ret = sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HW);
    SDK_ASSERT(pal_ret == sdk::lib::PAL_RET_OK);
#endif

    // Initialize mempartition
    _asic_cfg.cfg_path = std::string(std::getenv("HAL_CONFIG_PATH"));
    _platform_catalog =  catalog::factory(_asic_cfg.cfg_path,
            "", platform_type_t::PLATFORM_TYPE_HW);
    uint32_t mem_sz = _platform_catalog->memory_capacity();
    std::string mem_path = "";
    if (mem_sz == 8) {
        mem_path = "/8g";
    } else if (mem_sz == 4) {
        mem_path = "/4g";
    }
    std::string mpart_json = _asic_cfg.cfg_path + "/" +
        vpp::tcp::pds_get_platform_name() + mem_path + "/hbm_mem.json";
    _asic_cfg.mempartition =
        sdk::platform::utils::mpartition::factory(mpart_json.c_str());

    num_tcp_flows = vpp::tcp::pds_get_platform_max_tcp_qid();

    // Initialize symbols
    _prog_info = program_info::factory((_asic_cfg.cfg_path +
            "/gen/mpu_prog_info.json").c_str());
    SDK_ASSERT(_prog_info);

    // Initialize platform
    vpp::tcp::pds_platform_init(_prog_info, &_asic_cfg, _platform_catalog,
            &_tcp_lif, &_gc_lif, &_sock_lif);

    // Initialize rings
    pds_tcp_init_rings();

    // Initialize globals
    _stats_base = _asic_cfg.mempartition->start_addr("tcp_proxy_per_flow_stats");
    _global_base = _asic_cfg.mempartition->start_addr("tcp_global");
    SDK_TRACE_DEBUG("global_base = 0x%lx", _global_base);

    // Indexer
    _tcp_flow_idxr = indexer::factory(num_tcp_flows);
    _tcp_freed_flows_idxr = indexer::factory(num_tcp_flows);
    SDK_ASSERT(_tcp_flow_idxr != NULL);
    SDK_ASSERT(_tcp_freed_flows_idxr != NULL);
    SDK_TRACE_DEBUG("TCP init done");

    return 0;
}

void
pds_tcp_fill_def_params(tcpcb_pd_t *tcpcb)
{
    tcpcb->delay_ack = true;
    tcpcb->ooo_queue = true;
    tcpcb->sack_perm = true;
    tcpcb->timestamps = true;
    tcpcb->keepalives = true;
    tcpcb->ecn = false;
    tcpcb->abc_l_var = 2;
    tcpcb->ato = TCP_ATO_USEC;
    tcpcb->state = TCP_ESTABLISHED;
    // pred_flags
    //   header len = 8 (32 bytes with timestamp)
    //   flags = ACK
    tcpcb->pred_flags = 0x80100000;

    // In iris pipeline, for proxy use case we set source_lif to the original
    // source lif. For apollo, let's use service lif for now.
    tcpcb->source_lif = SERVICE_LIF_TCP_PROXY;
}

void
pds_tcp_fill_app_params(tcpcb_pd_t *tcpcb, pds_sockopt_hw_offload_t *offload,
        uint32_t data_len)
{
    tcpcb->app_type = offload->app_type;
    tcpcb->app_qid = offload->app_qid;
    if (tcpcb->app_type != TCP_APP_TYPE_BYPASS) {
        tcpcb->app_qtype = offload->app_qtype;
        tcpcb->app_lif = offload->app_lif;
        tcpcb->app_ring = offload->app_ring;
        tcpcb->serq_base = offload->serq_base;
        tcpcb->sesq_prod_ci_addr = offload->sesq_prod_ci_addr;
        tcpcb->lg2_app_num_slots = log2(offload->serq_ring_size);
    }
}

void
pds_tcp_fill_app_return_params(tcpcb_pd_t *tcpcb,
        pds_sockopt_hw_offload_t *offload, uint32_t data_len)
{
    offload->hw_qid = tcpcb->qid;
    offload->sesq_base = tcpcb->sesq_base;
    offload->serq_prod_ci_addr = tcpcb->serq_prod_ci_addr;
    offload->sesq_ring_size = 1 << tcpcb->lg2_sesq_num_slots;
    offload->tcp_lif = SERVICE_LIF_TCP_PROXY;
    offload->tcp_qtype = TCP_TX_QTYPE;
    offload->tcp_ring = TCP_SCHED_RING_SESQ;
    offload->sesq_desc_size = ASIC_SESQ_ENTRY_SIZE;
}

void
pds_tcp_fill_proxy_return_params(tcpcb_pd_t *tcpcb,
        pds_sockopt_hw_offload_t *offload, uint32_t data_len)
{
    offload->app_lif = tcpcb->app_lif;
    offload->app_qtype = tcpcb->app_qtype;
    offload->app_ring = tcpcb->app_ring;
    offload->serq_base = tcpcb->serq_base;
    offload->sesq_prod_ci_addr = tcpcb->sesq_prod_ci_addr;
    offload->serq_ring_size = 1 << tcpcb->lg2_app_num_slots;
}

static int
pds_tcp_fill_meta_params(tcpcb_pd_t *tcpcb)
{
    SDK_ASSERT(_asic_cfg.mempartition);

    if (tcpcb->app_type == TCP_APP_TYPE_BYPASS) {
        tcpcb->debug_dol = TCP_DDOL_BYPASS_BARCO;
        tcpcb->debug_dol_tx = TCP_TX_DDOL_BYPASS_BARCO;
    }

    tcpcb->base_pa[0] = _tcp_lif->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY,
            0, tcpcb->qid);
    tcpcb->base_pa[1] = _tcp_lif->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY,
            1, tcpcb->qid);

    tcpcb->app_nde_shift = ASIC_SERQ_ENTRY_SIZE_SHIFT;
    tcpcb->app_nde_offset = ASIC_SERQ_DESC_OFFSET;
    tcpcb->gc_base_addr = _gc_lif->get_lif_qstate_addr(SERVICE_LIF_GC,
            ASIC_HBM_GC_TNMDR_QTYPE, ASIC_TNMDR_GC_TCP_RING_PRODUCER) +
            TCP_GC_CB_SW_PI_OFFSET;
    tcpcb->rx_gc_base_addr = _gc_lif->get_lif_qstate_addr(SERVICE_LIF_GC,
            ASIC_HBM_GC_RNMDR_QTYPE, ASIC_RNMDR_GC_TCP_RX_RING_PRODUCER) +
            TCP_GC_CB_SW_PI_OFFSET;

    tcpcb->sesq_base = _sesq.get_base_addr(tcpcb->qid);
    tcpcb->serq_prod_ci_addr = tcpcb->base_pa[0] + TCP_TCB_TX2RX_SHARED_OFFSET + 24;
    tcpcb->lg2_sesq_num_slots = ASIC_SESQ_RING_SLOTS_SHIFT;
    SDK_TRACE_DEBUG("TCP qid %d, qstate_addr 0x%lx qtype 1 qstate_addr 0x%lx",
            tcpcb->qid, tcpcb->base_pa[0], tcpcb->base_pa[1]);

    tcpcb->rxdma_action_id = vpp::tcp::pds_platform_get_pc_offset(_prog_info,
            "rxdma_stage0.bin", "tcp_rx_stage0");
    tcpcb->txdma_action_id = vpp::tcp::pds_platform_get_pc_offset(_prog_info,
            "txdma_stage0.bin", "tcp_tx_stage0");
    tcpcb->txdma_ooq_action_id = vpp::tcp::pds_platform_get_pc_offset(_prog_info,
            "txdma_stage0.bin", "tcp_ooq_rx2tx_stage0");
    SDK_TRACE_DEBUG("action_id rxdma %d, txdma %d, txdma_ooq %d",
            tcpcb->rxdma_action_id, tcpcb->txdma_action_id,
            tcpcb->txdma_ooq_action_id);

    tcpcb->stats_base = _stats_base + tcpcb->qid * TCP_PER_FLOW_STATS_SIZE +
        TCP_RX_PER_FLOW_STATS_OFFSET;
    tcpcb->ooo_rx2tx_qbase = _ooo_rx2tx_q.get_base_addr(tcpcb->qid);
    SDK_TRACE_DEBUG("stats_base = 0x%lx, rx2tx_qbase = 0x%lx", tcpcb->stats_base,
            tcpcb->ooo_rx2tx_qbase);
    tcpcb->ooo_rx2tx_free_pi_addr = _global_base + TCP_PROXY_MEM_OOO_RX2TX_FREE_PI_OFFSET;
    tcpcb->ooo_prod_ci_addr = tcpcb->base_pa[0] + TCP_TCB_OOO_QADDR_CI_OFFSET;
    return 0;
}

static int
pds_tcp_fill_proxy_meta_params(tcpcb_pd_t *tcpcb)
{
    if (tcpcb->qid % 2 == 0) {
        tcpcb->app_qid = tcpcb->qid + 1;
        tcpcb->other_qid = tcpcb->qid + 1;
    } else {
        tcpcb->app_qid = tcpcb->qid - 1;
        tcpcb->other_qid = tcpcb->qid - 1;
    }

    SDK_ASSERT(_asic_cfg.mempartition);

    tcpcb->other_base_pa[0] = _tcp_lif->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY,
            0, tcpcb->other_qid);
    tcpcb->other_base_pa[1] = _tcp_lif->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY,
            1, tcpcb->other_qid);

    tcpcb->app_qtype = TCP_TX_QTYPE;
    tcpcb->app_lif = SERVICE_LIF_TCP_PROXY;
    tcpcb->app_ring = TCP_SCHED_RING_SESQ;
    tcpcb->serq_base = _sesq.get_base_addr(tcpcb->other_qid);
    tcpcb->sesq_prod_ci_addr = tcpcb->other_base_pa[0] + TCP_TCB_TX2RX_SHARED_OFFSET + 24;
    tcpcb->lg2_app_num_slots = ASIC_SESQ_RING_SLOTS_SHIFT;
    tcpcb->app_nde_shift = ASIC_SESQ_ENTRY_SIZE_SHIFT;
    tcpcb->app_nde_offset = ASIC_SESQ_DESC_OFFSET;
    tcpcb->gc_base_addr = _gc_lif->get_lif_qstate_addr(SERVICE_LIF_GC,
            ASIC_HBM_GC_RNMDR_QTYPE, ASIC_RNMDR_GC_TCP_RING_PRODUCER) +
            TCP_GC_CB_SW_PI_OFFSET;
    tcpcb->read_notify_addr = tcpcb->other_base_pa[0] +
            TCP_TCB_RETX_READ_NOTIFY_OFFSET;

    return 0;
}

static int
pds_tcp_fill_arm_socket_meta_params(tcpcb_pd_t *tcpcb)
{
    SDK_ASSERT(_asic_cfg.mempartition);

    tcpcb->app_qid = tcpcb->qid;
    tcpcb->app_qtype = 0;
    tcpcb->app_lif = SERVICE_LIF_ARM_SOCKET_APP;
    tcpcb->app_ring = 0;
    tcpcb->serq_base = _serq.get_base_addr(tcpcb->qid);
    // Using a valid location in socket lif qstate.
    tcpcb->sesq_prod_ci_addr = _sock_lif->get_lif_qstate_addr(SERVICE_LIF_ARM_SOCKET_APP,
                                  0, tcpcb->qid) + 32;
    tcpcb->lg2_app_num_slots = ASIC_SERQ_RING_SLOTS_SHIFT;
    tcpcb->app_nde_shift = ASIC_SESQ_ENTRY_SIZE_SHIFT; // TODO : do we want 8 byte or 32 byte descriptor entry
    tcpcb->app_nde_offset = ASIC_SESQ_DESC_OFFSET;
    tcpcb->gc_base_addr = _gc_lif->get_lif_qstate_addr(SERVICE_LIF_GC,
            ASIC_HBM_GC_TNMDR_QTYPE, ASIC_TNMDR_GC_TCP_RING_PRODUCER) +
            TCP_GC_CB_SW_PI_OFFSET;

    return 0;
}

//-----------------------------------------------------------------------------
// alloc qid. if full, free entire freed list and try again
//-----------------------------------------------------------------------------
sdk_ret_t
pds_tcp_alloc_qid(uint32_t *qid, uint8_t n, int attempt)
{
    indexer::status ret;

    if (!n) {
        return SDK_RET_OK;
    }

    ret = _tcp_flow_idxr->alloc_block(qid, n);
    if (ret == indexer::SUCCESS) {
        return SDK_RET_OK;
    } else if (ret == indexer::INDEXER_FULL && attempt) {
        int num_tcp_flows = vpp::tcp::pds_get_platform_max_tcp_qid();
        // free from free list and allocated list
        for (int i = 0; i < num_tcp_flows; i++) {
            if (_tcp_freed_flows_idxr->is_index_allocated(i)) {
                _tcp_freed_flows_idxr->free(i);
                _tcp_flow_idxr->free(i);
            }
        }

        // try again
        return pds_tcp_alloc_qid(qid, n, --attempt);
    }  else if (ret == indexer::INDEXER_FULL) {
        return SDK_RET_NO_RESOURCE;
    }


    return SDK_RET_ERR;
}

//-----------------------------------------------------------------------------
// free qid. add to freed list to prevent index from being reused immediately.
//-----------------------------------------------------------------------------
sdk_ret_t
pds_tcp_free_qid(uint32_t qid)
{
    _tcp_freed_flows_idxr->alloc_withid(qid);

    return SDK_RET_OK;
}

int
pds_tcp_prog_tcpcb(tcpcb_pd_t *tcpcb)
{
    uint8_t num_qids_to_alloc = 1;

    if (tcpcb->app_type == TCP_APP_TYPE_BYPASS && tcpcb->app_qid) {
        tcpcb->qid = tcpcb->app_qid;
        tcpcb->app_qid = tcpcb->app_qid - 1;
        num_qids_to_alloc = 0;
    } else if (tcpcb->app_type == TCP_APP_TYPE_BYPASS) {
        num_qids_to_alloc = 2;
    }
    if (pds_tcp_alloc_qid(&tcpcb->qid, num_qids_to_alloc, 1) != SDK_RET_OK) {
        SDK_TRACE_ERR("TCP CB qid alloc failed");
        return -1;
    }
    pds_tcp_fill_meta_params(tcpcb);
    if (tcpcb->app_type == TCP_APP_TYPE_BYPASS) {
        pds_tcp_fill_proxy_meta_params(tcpcb);
    } else if ((tcpcb->app_type == TCP_APP_TYPE_ARM_SOCKET)) {
        pds_tcp_fill_arm_socket_meta_params(tcpcb);
        uint8_t action_id = vpp::tcp::pds_platform_get_pc_offset(_prog_info,
            "txdma_stage0.bin", "arm_sock_app_stage0");
        sdk::tcp::sock_qstate_init(_sock_lif, tcpcb->qid, action_id);
    }

#ifdef VPP_TRACE_ENABLE
    SDK_TRACE_DEBUG("qid %d, hdr template: %s", tcpcb->qid,
            tcp_rawstr(tcpcb->header_template, 64));
#endif

    if (sdk::tcp::tcpcb_pd_create(tcpcb) != SDK_RET_OK) {
        SDK_TRACE_ERR("Create failed for TCP CB qid %d", tcpcb->qid);
        return -1;
    }

    return 0;
}

int
pds_tcp_prog_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
        uint32_t rmt_ip, uint16_t lcl_port, uint16_t rmt_port,
        bool proxy)
{
    sdk_ret_t ret;
    ret = vpp::tcp::pds_platform_prog_ip4_flow(qid, is_ipv4, lcl_ip, rmt_ip,
            lcl_port, rmt_port, proxy);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to program flow for qid %d", qid);
        return -1;
    }
    
    return 0;
}

//-----------------------------------------------------------------------------
// Delete P4 flow, cleanup CB and free qid
//-----------------------------------------------------------------------------
int
pds_tcp_del_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
                     uint32_t rmt_ip, uint16_t lcl_port, uint16_t rmt_port)
{
    sdk_ret_t ret;
    ret = vpp::tcp::pds_platform_del_ip4_flow(qid, is_ipv4, lcl_ip, rmt_ip,
            lcl_port, rmt_port);
    if (ret != SDK_RET_OK) {
        SDK_TRACE_ERR("Failed to delete flow for qid %d", qid);
        return -1;
    }
    
    return 0;
}

int
pds_tcp_del_tcpcb(uint32_t qid)
{
    sdk_ret_t ret;

    // TODO : any cleanup to be done in CB, free OOQ etc.

    ret = pds_tcp_free_qid(qid);
    SDK_TRACE_DEBUG("Free qid %d returned %d", qid, ret);

    return 0;
}

} // extern "C"
