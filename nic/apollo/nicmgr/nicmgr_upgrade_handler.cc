//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/include/upgrade_shmstore.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/internal/upgrade_ev.hpp"
#include "nic/apollo/api/upgrade_state.hpp"
#include "platform/src/lib/nicmgr/include/upgrade.hpp"
#include "platform/src/lib/nicmgr/include/dev.hpp"
#include "platform/src/lib/nicmgr/include/device.hpp"
#include "platform/src/lib/nicmgr/include/eth_dev.hpp"
#include "gen/proto/nicmgr.pb.h"
#include "gen/proto/port.pb.h"

extern DeviceManager *g_devmgr;

namespace nicmgr {

using api::upg_obj_stash_meta_t;

// ...WARNING.. these ids should be preserved across releases.
// so don't modify it. only append.
typedef enum backup_obj_id_s {
    NICMGR_BKUP_OBJ_INVALID         = 0,
    NICMGR_BKUP_OBJ_DEVINFO_ID      = 1,
    NICMGR_BKUP_OBJ_UPLINKINFO_ID   = 2,
} backup_obj_id_t;

#define NICMGR_BKUP_OBJS(pbuf_info, obj_name) {                         \
    uint32_t pbuf_byte_size = pbuf_info.ByteSizeLong();                 \
    uint32_t offset = ctx->obj_offset();                                \
    uint32_t size = ctx->obj_size();                                    \
    char *mem = ctx->mem();                                             \
                                                                        \
    if ((offset + pbuf_byte_size + 4) > size) {                         \
        PDS_TRACE_ERR("Failed to serialize %s", obj_name);              \
        ret = SDK_RET_OOM;                                              \
    }                                                                   \
    *(uint32_t *)&mem[offset] = pbuf_byte_size;                         \
    if (pbuf_info.SerializeToArray(&mem[offset + 4],                    \
                                  pbuf_byte_size) == false) {           \
        PDS_TRACE_ERR("Failed to serialize %s", obj_name);              \
        ret = SDK_RET_OOM;                                              \
    }                                                                   \
    offset += (pbuf_byte_size + 4);                                     \
    ctx->set_obj_offset(offset);                                        \
    ret = SDK_RET_OK;                                                   \
}

#define NICMGR_RESTORE_OBJS(pbuf_info, cb, oid, obj_name) {             \
    uint32_t pbuf_byte_size;                                            \
    char *mem = ctx->mem();                                             \
    upg_obj_stash_meta_t *meta = (upg_obj_stash_meta_t *)mem;           \
    uint32_t offset;                                                    \
                                                                        \
    offset = meta[oid].offset;                                          \
    if (meta[oid].obj_count) {                                          \
        SDK_ASSERT(meta[oid].obj_id == oid);                            \
    }                                                                   \
    for (uint32_t i = 0; i < meta[oid].obj_count; i++) {                \
        pbuf_byte_size =  *(uint32_t *)&mem[offset];                    \
                                                                        \
        if (pbuf_info.ParseFromArray(&mem[offset + 4],                  \
                                     pbuf_byte_size) == false) {        \
            PDS_TRACE_ERR("Failed to de-serialize %s", obj_name);       \
            ret = SDK_RET_OOM;                                          \
        }                                                               \
        cb(&pbuf_info);                                                 \
        offset += (pbuf_byte_size + 4);                                 \
    }                                                                   \
}

// ipc incoming msg pointer which should be saved and later
// used to respond to the caller
typedef struct upg_ev_info_s {
    sdk::ipc::ipc_msg_ptr msg_in;
} upg_ev_info_t;

using api::upg_ev_params_t;

// below functions are called from nicmgr thread context
static void
nicmgr_upg_process_response (sdk_ret_t status, void *cookie)
{
    upg_ev_info_t *info = (upg_ev_info_t *)cookie;
    upg_ev_params_t *params = (upg_ev_params_t *)info->msg_in->data();
    upg_ev_params_t resp;

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC response type %s, status %u",
                  upg_msgid2str(params->id), status);

    if (status == SDK_RET_IN_PROGRESS) {
        return;
    }

    resp.id = params->id;
    resp.rsp_code = status;
    sdk::ipc::respond(info->msg_in, (const void *)&resp, sizeof(resp));
    delete info;
}

void
nicmgr_upg_backup_uplink_info (uplink_t *up, pds::Port *proto_obj)
{
    pds::PortSpec *spec = proto_obj->mutable_spec();
    char id[16];

    snprintf(id, sizeof(id), "%u", up->id);
    spec->set_id(id);
    spec->set_portnumber(up->port);
    spec->set_type(up->is_oob ? pds::PortType::PORT_TYPE_ETH_MGMT :
                   pds::PortType::PORT_TYPE_ETH);
    PDS_TRACE_DEBUG("Saving uplink id %u port %u", up->id, up->port);
}

void
nicmgr_upg_backup_eth_device_info (struct EthDevInfo *eth_dev_info,
                                   pds::nicmgr::EthDevice *proto_obj)
{
    pds::nicmgr::EthDeviceSpec *spec = proto_obj->mutable_spec();
    pds::nicmgr::EthDeviceStatus *status = proto_obj->mutable_status();

    //device name is the key
    spec->set_name(eth_dev_info->eth_spec->name);

    //save eth_dev resources
    status->set_lifbase(eth_dev_info->eth_res->lif_base);
    status->set_intrbase(eth_dev_info->eth_res->intr_base);
    status->set_regsmemaddr(eth_dev_info->eth_res->regs_mem_addr);
    status->set_portinfoaddr(eth_dev_info->eth_res->port_info_addr);
    status->set_cmbmemaddr(eth_dev_info->eth_res->cmb_mem_addr);
    status->set_cmbmemsize(eth_dev_info->eth_res->cmb_mem_size);
    status->set_rommemaddr(eth_dev_info->eth_res->rom_mem_addr);
    status->set_rommemsize(eth_dev_info->eth_res->rom_mem_size);

    // save eth_dev specs
    spec->set_devuuid(eth_dev_info->eth_spec->dev_uuid);
    spec->set_ethtype(Eth::eth_type_to_str(eth_dev_info->eth_spec->eth_type));
    spec->set_name(eth_dev_info->eth_spec->name);
    spec->set_hostdev(eth_dev_info->eth_spec->host_dev);
    spec->set_uplinkportnumber(eth_dev_info->eth_spec->uplink_port_num);
    spec->set_qosgroup(eth_dev_info->eth_spec->qos_group);
    spec->set_lifcount(eth_dev_info->eth_spec->lif_count);
    spec->set_rxqcount(eth_dev_info->eth_spec->rxq_count);
    spec->set_txqcount(eth_dev_info->eth_spec->txq_count);
    spec->set_eqcount(eth_dev_info->eth_spec->eq_count);
    spec->set_adminqcount(eth_dev_info->eth_spec->adminq_count);
    spec->set_intrcount(eth_dev_info->eth_spec->intr_count);
    spec->set_macaddr(eth_dev_info->eth_spec->mac_addr);
    spec->set_barmapsize(eth_dev_info->eth_spec->barmap_size);

    // save rdma spec
    spec->mutable_rdmaspec()->set_enable(eth_dev_info->eth_spec->enable_rdma);
    spec->mutable_rdmaspec()->set_ptecount(eth_dev_info->eth_spec->pte_count);
    spec->mutable_rdmaspec()->set_keycount(eth_dev_info->eth_spec->key_count);
    spec->mutable_rdmaspec()->set_ahcount(eth_dev_info->eth_spec->ah_count);
    spec->mutable_rdmaspec()->set_sqcount(eth_dev_info->eth_spec->rdma_sq_count);
    spec->mutable_rdmaspec()->set_rqcount(eth_dev_info->eth_spec->rdma_rq_count);
    spec->mutable_rdmaspec()->set_cqcount(eth_dev_info->eth_spec->rdma_eq_count);
    spec->mutable_rdmaspec()->set_eqcount(eth_dev_info->eth_spec->rdma_eq_count);
    spec->mutable_rdmaspec()->set_aqcount(eth_dev_info->eth_spec->rdma_aq_count);
    spec->mutable_rdmaspec()->set_pidcount(eth_dev_info->eth_spec->rdma_pid_count);

    // save pcie spec
    spec->mutable_pciespec()->set_opromtype(oprom_type_to_str(
            eth_dev_info->eth_spec->oprom));
    spec->mutable_pciespec()->set_port(eth_dev_info->eth_spec->pcie_port);
    spec->mutable_pciespec()->set_totalvfs(eth_dev_info->eth_spec->pcie_total_vfs);

    PDS_TRACE_DEBUG("Saving eth dev %s", eth_dev_info->eth_spec->name.c_str());
    return;
}

static void
nicmgr_upg_restore_eth_device_info (pds::nicmgr::EthDevice *proto_obj)
{
    struct EthDevInfo *eth_dev_info = new EthDevInfo();
    struct eth_devspec *eth_spec = new eth_devspec();
    struct eth_dev_res *eth_res = new eth_dev_res();
    pds::nicmgr::EthDeviceSpec *spec = proto_obj->mutable_spec();
    pds::nicmgr::EthDeviceStatus *status = proto_obj->mutable_status();

    // populate the eth resource
    eth_res->lif_base = status->lifbase();
    eth_res->intr_base = status->intrbase();
    eth_res->regs_mem_addr = status->regsmemaddr();
    eth_res->port_info_addr = status->portinfoaddr();
    eth_res->cmb_mem_addr = status->cmbmemaddr();
    eth_res->cmb_mem_size = status->cmbmemsize();
    eth_res->rom_mem_addr = status->rommemaddr();
    eth_res->rom_mem_size = status->rommemsize();

    // populate the eth spec
    eth_spec->dev_uuid = spec->devuuid();
    eth_spec->eth_type = Eth::str_to_eth_type(spec->ethtype());
    eth_spec->name = spec->name();
    eth_spec->host_dev = spec->hostdev();
    eth_spec->uplink_port_num = spec->uplinkportnumber();
    eth_spec->qos_group = spec->qosgroup();
    eth_spec->lif_count = spec->lifcount();
    eth_spec->rxq_count = spec->rxqcount();
    eth_spec->txq_count = spec->txqcount();
    eth_spec->adminq_count = spec->adminqcount();
    eth_spec->eq_count = spec->eqcount();
    eth_spec->barmap_size = spec->barmapsize();
    eth_spec->mac_addr = spec->macaddr();
    eth_spec->intr_count = spec->intrcount();

    // populate pcie spec
    eth_spec->oprom = str_to_oprom_type(spec->mutable_pciespec()->opromtype());
    eth_spec->pcie_port = spec->mutable_pciespec()->port();
    eth_spec->pcie_total_vfs = spec->mutable_pciespec()->totalvfs();

    // poplulate rdma
    eth_spec->enable_rdma = spec->mutable_rdmaspec()->enable();
    eth_spec->pte_count = spec->mutable_rdmaspec()->ptecount();
    eth_spec->key_count = spec->mutable_rdmaspec()->keycount();
    eth_spec->ah_count = spec->mutable_rdmaspec()->ahcount();
    eth_spec->rdma_sq_count = spec->mutable_rdmaspec()->sqcount();
    eth_spec->rdma_rq_count = spec->mutable_rdmaspec()->rqcount();
    eth_spec->rdma_cq_count = spec->mutable_rdmaspec()->cqcount();
    eth_spec->rdma_eq_count = spec->mutable_rdmaspec()->eqcount();
    eth_spec->rdma_aq_count = spec->mutable_rdmaspec()->aqcount();
    eth_spec->rdma_pid_count = spec->mutable_rdmaspec()->pidcount();

    eth_dev_info->eth_res = eth_res;
    eth_dev_info->eth_spec = eth_spec;

    PDS_TRACE_DEBUG("Restore eth dev %s", eth_spec->name.c_str());
    g_devmgr->RestoreDevice(ETH, eth_dev_info);
}

static void
nicmgr_upg_restore_uplink_info (pds::Port *proto_obj)
{
    pds::PortSpec *spec = proto_obj->mutable_spec();
    bool is_oob = spec->type() ==  pds::PortType::PORT_TYPE_ETH_MGMT ? 1 : 0;

    PDS_TRACE_DEBUG("Restore uplink id %u, port %u",
                  atoi(spec->id().c_str()), spec->portnumber());
    g_devmgr->CreateUplink(atoi(spec->id().c_str()), spec->portnumber(), is_oob);
}

static sdk_ret_t
backup_objs (upg_mode_t mode)
{
    std::vector <struct EthDevInfo*> dev_info;
    std::map<uint32_t, uplink_t*> up_links;
    pds::nicmgr::EthDevice proto_dev;
    pds::Port proto_port;
    api::upg_ctxt *ctx;
    upg_obj_stash_meta_t *meta;
    sdk_ret_t ret = SDK_RET_OK;

    ctx = api::upg_shmstore_objctx_create(core::PDS_THREAD_ID_NICMGR,
                                          PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG,
                                          PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG_SIZE,
                                          UPGRADE_SVC_SHMSTORE_TYPE_BACKUP);
    SDK_ASSERT(ctx);
    meta = (upg_obj_stash_meta_t *)ctx->mem();

    dev_info = g_devmgr->GetEthDevStateInfo();
    PDS_TRACE_DEBUG("Saving %lu objects of EthDevInfo", dev_info.size());

    meta[NICMGR_BKUP_OBJ_DEVINFO_ID].obj_id = NICMGR_BKUP_OBJ_DEVINFO_ID;
    meta[NICMGR_BKUP_OBJ_DEVINFO_ID].obj_count = dev_info.size();
    meta[NICMGR_BKUP_OBJ_DEVINFO_ID].offset = ctx->obj_offset();

    //for each element in dev_info convert it to protobuf and then setobject to shm
    for (uint32_t eth_dev_idx = 0; eth_dev_idx < dev_info.size(); eth_dev_idx++) {
        nicmgr_upg_backup_eth_device_info(dev_info[eth_dev_idx], &proto_dev);
        NICMGR_BKUP_OBJS(proto_dev, "ethdev");
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("EthDevInfo save failed");
            goto err_exit;
        }
    }

    up_links = g_devmgr->GetUplinks();
    PDS_TRACE_DEBUG("Saving %lu objects of uplinkinfo", up_links.size());

    meta[NICMGR_BKUP_OBJ_UPLINKINFO_ID].obj_id = NICMGR_BKUP_OBJ_UPLINKINFO_ID;
    meta[NICMGR_BKUP_OBJ_UPLINKINFO_ID].obj_count = up_links.size();
    meta[NICMGR_BKUP_OBJ_UPLINKINFO_ID].offset = ctx->obj_offset();

    for (auto it = up_links.begin(); it != up_links.end(); it++) {
        uplink_t *uplink = it->second;
        nicmgr_upg_backup_uplink_info(uplink, &proto_port);
        NICMGR_BKUP_OBJS(proto_port, "uplink");
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Uplink save failed");
            goto err_exit;
        }
    }

err_exit:
    ctx->destroy(ctx);
    return ret;
}

static sdk_ret_t
restore_objs (upg_mode_t mode)
{
    sdk_ret_t ret = SDK_RET_OK;
    pds::nicmgr::EthDevice proto_dev;
    pds::Port proto_port;
    api::upg_ctxt *ctx;

    PDS_TRACE_DEBUG("Retrieving saved objects");

    ctx = api::upg_shmstore_objctx_create(core::PDS_THREAD_ID_NICMGR,
                                          PDS_NICMGR_UPGRADE_SHMSTORE_OBJ_SEG,
                                          0, UPGRADE_SVC_SHMSTORE_TYPE_RESTORE);
    SDK_ASSERT(ctx);

    NICMGR_RESTORE_OBJS(proto_port, nicmgr_upg_restore_uplink_info,
                        NICMGR_BKUP_OBJ_UPLINKINFO_ID, "uplink");
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Uplink restore failed");
        goto err_exit;
    }

    NICMGR_RESTORE_OBJS(proto_dev, nicmgr_upg_restore_eth_device_info,
                        NICMGR_BKUP_OBJ_DEVINFO_ID, "ethdev");

    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Eth device restore failed");
        goto err_exit;
    }

err_exit:
    ctx->destroy(ctx);
    return ret;
}



// callback handler from nicmgr library when the hostdev
// reset is completed. some pipelines may need this
static int
nicmgr_device_reset_status_cb (void)
{
    return 0;
}

static void
nicmgr_upg_ev_compat_check_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_info_s *info = new upg_ev_info_t();
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC request compat check");
    info->msg_in = msg;
    ret = nicmgr::upg::upg_compat_check_handler();
    return nicmgr_upg_process_response(ret, info);
}

static void
nicmgr_upg_ev_backup_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_info_s *info = new upg_ev_info_t();
    sdk_ret_t ret;
    upg_ev_params_t *params = (upg_ev_params_t *)msg->data();

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC request backup");
    info->msg_in = msg;
    ret = backup_objs(params->mode);
    return nicmgr_upg_process_response(ret, info);
}

static void
nicmgr_upg_ev_link_down_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_info_s *info = new upg_ev_info_t();
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC request linkdown");
    info->msg_in = msg;
    ret = nicmgr::upg::upg_link_down_handler(info);
    return nicmgr_upg_process_response(ret, info);
}

static void
nicmgr_upg_ev_hostdev_reset_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_info_s *info = new upg_ev_info_t();
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC request hostdev reset");
    info->msg_in = msg;
    ret = nicmgr::upg::upg_host_down_handler(info);
    return nicmgr_upg_process_response(ret, info);
}

static void
nicmgr_upg_ev_pre_respawn_hdlr (sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    upg_ev_info_s *info = new upg_ev_info_t();
    sdk_ret_t ret;

    PDS_TRACE_DEBUG("Upgrade nicmgr IPC request pre respawn");
    info->msg_in = msg;
    ret = nicmgr::upg::upg_failed_handler(info);

    // work is going to be the same in case of pre respawn or respawn
    // it is just json way of differentiating, sysmgr respawn from rest of the
    // services
    return nicmgr_upg_process_response(ret, info);
}

sdk_ret_t
nicmgr_upg_init (void)
{
    upg_mode_t upg_init_mode = api::g_upg_state->upg_init_mode();
    sdk_ret_t ret;

    sdk::ipc::reg_request_handler(UPG_MSG_ID_COMPAT_CHECK,
                                  nicmgr_upg_ev_compat_check_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_BACKUP,
                                  nicmgr_upg_ev_backup_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_LINK_DOWN,
                                  nicmgr_upg_ev_link_down_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_HOSTDEV_RESET,
                                  nicmgr_upg_ev_hostdev_reset_hdlr, NULL);
    sdk::ipc::reg_request_handler(UPG_MSG_ID_PRE_RESPAWN,
                                  nicmgr_upg_ev_pre_respawn_hdlr, NULL);

    // register the async handlers to nicmgr library
    nicmgr::upg::upg_ev_init(nicmgr_device_reset_status_cb,
                             nicmgr_upg_process_response);

    // load the states
    if (upg_init_mode == upg_mode_t::UPGRADE_MODE_GRACEFUL) {
        ret = restore_objs(upg_init_mode);
        if (ret != SDK_RET_OK) {
            // api::g_upg_state->set_upg_init_error();
        }
    } else if (upg_init_mode == upg_mode_t::UPGRADE_MODE_HITLESS) {
        // ret = restore_objs();
        // if (ret != SDK_RET_OK) {
            // api::g_upg_state->set_upg_init_error();
        // } @Karthi, please fill the details
    }
    return SDK_RET_OK;
}

}    // namespace nicmgr
