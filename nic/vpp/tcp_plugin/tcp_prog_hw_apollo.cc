//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <stddef.h>

#include "nic/include/base.hpp"
#include "nic/include/globals.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/table/tcam/tcam.hpp"
//#include "nic/apollo/api/impl/apollo/init/apollo_init.hpp"
#include "nic/sdk/platform/capri/capri_qstate.hpp"
#include "nic/sdk/platform/capri/capri_p4.hpp"
#include "nic/sdk/platform/capri/capri_cfg.hpp"
#include "nic/sdk/lib/pal/pal.hpp"
#include "nic/sdk/platform/capri/capri_state.hpp"
#include "nic/sdk/platform/capri/capri_tbl_rw.hpp"
#include <nic/sdk/include/sdk/table.hpp>
#include "gen/p4gen/p4/include/ftl.h"
#include "nic/apollo/p4/include/defines.h"
#include "gen/p4gen/apollo/include/p4pd.h"
#include "tcp_prog_hw.hpp"

extern "C" {

// externs
void ip4_proxy_set_params(uint32_t my_ip, uint32_t client_addr4,
        uint32_t server_addr4);

} // extern "C"

namespace vpp {
namespace tcp {


// Globals
static const char *pds_platform_name = "apollo";
#ifdef FIXME_NVME_POC
static struct {
    catalog *platform_catalog;
    struct ftlv4 *flow_table;
    p4pd_table_properties_t session_tbl_ctx;
} tcp_platform_main;
#endif

#define PDS_PLATFORM "apollo"
//#define PDS_MAX_TCP_SESSIONS 16384
#define PDS_MAX_TCP_SESSIONS 4096 // Increase after testing

const char *
pds_get_platform_name(void)
{
    return pds_platform_name;
}

uint32_t
pds_get_platform_max_tcp_qid(void)
{
    return PDS_MAX_TCP_SESSIONS;
}

// Capri specific
// TODO : We need an asic pd function for this
void
pds_platform_capri_init(asic_cfg_t *cfg)
{
#ifdef FIXME_NVME_POC
    capri_cfg_t capri_cfg;
    sdk_ret_t ret;

    SDK_ASSERT(cfg != NULL);
    capri_cfg.cfg_path = cfg->cfg_path;
    capri_cfg.mempartition = cfg->mempartition;
    ret = sdk::platform::capri::capri_state_pd_init(&capri_cfg);
    SDK_ASSERT(ret == SDK_RET_OK);

    sdk::platform::capri::capri_table_rw_init(&capri_cfg);
#endif
}

#ifdef FIXME_NVME_POC
static void
pds_platform_init_rings(asic_cfg_t *asic_cfg)
{
    // Initialize rings
    // FIXME_NVME_POC
    apollo::init::ring_config_init(asic_cfg);
}
#endif

#ifdef FIXME_NVME_POC
static void
pds_platform_init_lifs(asic_cfg_t *asic_cfg, lif_mgr **tcp_lif, lif_mgr **gc_lif,
                       lif_mgr **sock_lif)
{
    // Initialize lifs
    lif_qstate_t *qstate = NULL;
    int num_service_lifs = 0;
    apollo::init::service_lif_init(&qstate, &num_service_lifs, false, asic_cfg->mempartition, tcp_lif, gc_lif,
                                   //sock_lif);
}
#endif

typedef char* (*key2str_t)(void *key);
typedef char* (*appdata2str_t)(void *data);

#ifdef FIXME_NVME_POC
static ftlv4 *
flow_table_init(void *key2str,
                void *appdata2str,
                uint32_t thread_id)
{
    sdk::table::sdk_table_factory_params_t factory_params = {0};

    factory_params.table_id = P4TBL_ID_IPV4_FLOW;
    factory_params.num_hints = 2;
    factory_params.max_recircs = 8;
    factory_params.key2str = (key2str_t) (key2str);
    factory_params.appdata2str = (appdata2str_t) (appdata2str);
    factory_params.thread_id = thread_id;

    return sdk::table::ftlv4::factory(&factory_params);
}
#endif

#ifdef FIXME_NVME_POC
static int
flow_table_insert(ftlv4_entry_t *entry)
{
    sdk_table_api_params_t params = {0};

    params.entry = entry;
    if (SDK_RET_OK != tcp_platform_main.flow_table->insert(&params)) {
        return -1;
    }
    return 0;
}
#endif

#ifdef FIXME_NVME_POC
static int
flow_table_delete(ftlv4_entry_t *entry)
{
    sdk_table_api_params_t params = {0};

    params.entry = entry;
    if (SDK_RET_OK != tcp_platform_main.flow_table->remove(&params)) {
        return -1;
    }
    return 0;
}
#endif

#ifdef FIXME_NVME_POC
static char *
pds_flow4_key2str (void *key)
{
    static char str[256];
    flow_swkey_t *k = (flow_swkey_t *)key;
    char srcstr[INET_ADDRSTRLEN + 1];
    char dststr[INET_ADDRSTRLEN + 1];

    inet_ntop(AF_INET, k->key_metadata_src, srcstr, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, k->key_metadata_dst, dststr, INET_ADDRSTRLEN);
    sprintf(str, "Src:%s Dst:%s Dport:%u Sport:%u Proto:%u VNIC:%u",
            srcstr, dststr,
            k->key_metadata_dport, k->key_metadata_sport,
            k->key_metadata_proto, k->key_metadata_lkp_id);
    return str;
}
#endif

#ifdef FIXME_NVME_POC
static char *
pds_flow_appdata2str (void *appdata)
{
    static char str[512];
    flow_appdata_t *d = (flow_appdata_t *)appdata;
    sprintf(str, "session_index:%d flow_role:%d",
            d->session_index, d->flow_role);
    return str;
}
#endif

void
pds_platform_init(program_info *prog_info, asic_cfg_t *asic_cfg,
        catalog *platform_catalog, lif_mgr **tcp_lif, lif_mgr **gc_lif, lif_mgr **sock_lif)
{
#ifdef FIXME_NVME_POC
    p4pd_error_t p4pd_ret;
    p4pd_cfg_t p4pd_cfg = {
        .table_map_cfg_file  = PDS_PLATFORM "/capri_p4_table_map.json",
        .p4pd_pgm_name       = PDS_PLATFORM "_p4",
        .p4pd_rxdma_pgm_name = PDS_PLATFORM "_rxdma",
        .p4pd_txdma_pgm_name = PDS_PLATFORM "_txdma",
    };
    pal_ret_t pal_ret;
    sdk_ret_t ret;

    tcp_platform_main.platform_catalog = platform_catalog;

    /* initialize PAL */
    pal_ret = sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HW);
    SDK_ASSERT(pal_ret == sdk::lib::PAL_RET_OK);

    pds_platform_capri_init(asic_cfg);

    p4pd_cfg.cfg_path = asic_cfg->cfg_path.c_str();
    p4pd_ret = p4pd_init(&p4pd_cfg);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    ret = sdk::asic::pd::asicpd_program_hbm_table_base_addr(false);
    SDK_ASSERT(ret == SDK_RET_OK);

    p4pd_ret = p4pd_table_properties_get(P4TBL_ID_SESSION,
                                         &tcp_platform_main.session_tbl_ctx);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    tcp_platform_main.flow_table =
        flow_table_init((void *)pds_flow4_key2str,
                        (void *)pds_flow_appdata2str,
                        0);

    pds_platform_init_rings(asic_cfg);

    pds_platform_init_lifs(asic_cfg, tcp_lif, gc_lif, sock_lif);

    char *addr = getenv("TCP_PROXY_CLIENT_IPV4_ADDR");
    struct in_addr tcp_proxy_client_ip = { 0 };
    struct in_addr tcp_proxy_server_ip = { 0 };
    struct in_addr my_ip = { 0 };
    if (addr) {
        inet_aton(addr, &tcp_proxy_client_ip);

        addr = getenv("TCP_PROXY_SERVER_IPV4_ADDR");
        inet_aton(addr, &tcp_proxy_server_ip);

        addr = getenv("NVME_HOST_IPV4_ADDR");
        inet_aton(addr, &my_ip);

        ip4_proxy_set_params(my_ip.s_addr, tcp_proxy_client_ip.s_addr,
                tcp_proxy_server_ip.s_addr);
    }
#endif
}

uint8_t
pds_platform_get_pc_offset(program_info *prog_info,
        const char *prog, const char *label)
{
#ifdef FIXME_NVME_POC
    uint8_t pgm_offset = 0;
    sdk::platform::capri::get_pc_offset(prog_info, prog, label, &pgm_offset);
    return pgm_offset;
#else
    return 0;
#endif
}

static sdk_ret_t
flow_entry_program(uint32_t session_id, uint32_t uplink,
                   uint32_t src_ip, uint32_t dst_ip,
                   uint16_t src_port, uint16_t dst_port)
{
#ifdef FIXME_NVME_POC
    ftlv4_entry_t entry = { 0 };

    SDK_TRACE_DEBUG("programming TCP flow: src_ip 0x%x, dst_ip 0x%x"
                    ", src_port %d, dst_port %d: uplink 0x%x",
                    ntohl(src_ip), ntohl(dst_ip), ntohs(src_port),
                    ntohs(dst_port), uplink);

    entry.proto = 6;
    entry.dst = ntohl(dst_ip);
    entry.src = ntohl(src_ip);
    entry.dport = ntohs(dst_port);
    entry.sport = ntohs(src_port);

    entry.session_index = session_id;
    entry.redirect_pkt = 1;

    if (flow_table_insert(&entry) != 0) {
        SDK_TRACE_ERR("Failed to program flow entry for TCP flow");
        return SDK_RET_ERR;
    }
#endif

    return SDK_RET_OK;
}

static sdk_ret_t
flow_entry_delete(uint32_t src_ip, uint32_t dst_ip,
                  uint16_t src_port, uint16_t dst_port)
{
#ifdef FIXME_NVME_POC
    ftlv4_entry_t entry = { 0 };

    SDK_TRACE_DEBUG("deleting TCP flow: src_ip 0x%x, dst_ip 0x%x"
                    ", src_port %d, dst_port %d",
                    ntohl(src_ip), ntohl(dst_ip), ntohs(src_port),
                    ntohs(dst_port));

    entry.proto = 6;
    entry.dst = ntohl(dst_ip);
    entry.src = ntohl(src_ip);
    entry.dport = ntohs(dst_port);
    entry.sport = ntohs(src_port);

    if (flow_table_delete(&entry) != 0) {
        SDK_TRACE_ERR("Failed to delete flow entry for TCP flow");
        return SDK_RET_ERR;
    }
#endif

    return SDK_RET_OK;
}

static sdk_ret_t
session_info_program(uint32_t session_id, uint32_t qid, uint32_t pinned_uplink)
{
#ifdef FIXME_NVME_POC
    session_actiondata_t session_data = { 0 };
    uint32_t ifindex;

    ifindex = (pinned_uplink == 1 ? 0x11010001 : 0x11020001);

    session_data.action_u.session_session_info.lif = SERVICE_LIF_TCP_PROXY;
    session_data.action_u.session_session_info.qtype = 0;
    session_data.action_u.session_session_info.qid = qid;
    session_data.action_u.session_session_info.oport =
        tcp_platform_main.platform_catalog->ifindex_to_tm_port(ifindex);
    session_data.action_u.session_session_info.app_id = P4PLUS_APPTYPE_TCPTLS;

    if (p4pd_entry_write(P4TBL_ID_SESSION, session_id, NULL, NULL,
        &session_data) != P4PD_SUCCESS) {
        SDK_TRACE_DEBUG("session info write failed");
        return SDK_RET_ERR;
    }
#endif

    return SDK_RET_OK;
}

sdk_ret_t
pds_platform_prog_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
                           uint32_t rmt_ip, uint16_t lcl_port,
                           uint16_t rmt_port, bool proxy)
{
    sdk_ret_t ret;
    char *uplink = getenv("NVME_HOST_UPLINK_INTERFACE");
    uint32_t pinned_uplink = 0;
    uint32_t session_id;

    if (uplink) pinned_uplink = atoi(uplink);

    // uplink -> TCP service
    if (proxy) {
        char *addr;
        struct in_addr tcp_proxy_client_ip = { 0 };
        char *client_uplink = getenv("TCP_PROXY_CLIENT_UPLINK_INTERFACE");
        char *server_uplink = getenv("TCP_PROXY_SERVER_UPLINK_INTERFACE");

        addr = getenv("TCP_PROXY_CLIENT_IPV4_ADDR");
        inet_aton(addr, &tcp_proxy_client_ip);

        if (tcp_proxy_client_ip.s_addr == rmt_ip) {
            // proxy client --> server
            pinned_uplink = atoi(client_uplink);
        } else {
            // proxy server --> client
            pinned_uplink = atoi(server_uplink);
        }
    }

    //  iflow
    session_id = qid;
    ret = flow_entry_program(session_id, pinned_uplink,
                             rmt_ip, lcl_ip, rmt_port, lcl_port);
    if (ret == SDK_RET_OK) {
        SDK_TRACE_DEBUG("TCP iflow program success");
    } else {
        SDK_TRACE_DEBUG("TCP iflow program failed");
    }

    //  rflow
    ret = flow_entry_program(session_id, pinned_uplink,
                             lcl_ip, rmt_ip, lcl_port, rmt_port);
    if (ret == SDK_RET_OK) {
        SDK_TRACE_DEBUG("TCP iflow program success");
    } else {
        SDK_TRACE_DEBUG("TCP iflow program failed");
    }

    // session table
    ret = session_info_program(session_id, qid, pinned_uplink);
    if (ret == SDK_RET_OK) {
        SDK_TRACE_DEBUG("TCP session program success");
    } else {
        SDK_TRACE_DEBUG("TCP session program failed");
    }

    return ret;
}

sdk_ret_t
pds_platform_del_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
                          uint32_t rmt_ip, uint16_t lcl_port,
                          uint16_t rmt_port)
{
    sdk_ret_t ret;

    //  iflow
    ret = flow_entry_delete(rmt_ip, lcl_ip, rmt_port, lcl_port);
    if (ret == SDK_RET_OK) {
        SDK_TRACE_DEBUG("TCP iflow delete success");
    } else {
        SDK_TRACE_DEBUG("TCP iflow delete failed");
    }

    //  rflow
    ret = flow_entry_delete(lcl_ip, rmt_ip, lcl_port, rmt_port);
    if (ret == SDK_RET_OK) {
        SDK_TRACE_DEBUG("TCP iflow delete success");
    } else {
        SDK_TRACE_DEBUG("TCP iflow delete failed");
    }

    return ret;
}

} // namespace tcp
} // namespace vpp
