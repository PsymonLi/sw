/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#include <cstdio>
#include <cstring>
#include <exception>
#include <arpa/inet.h>

#include "cmd_hndlr.h"
#include "lib/logger/logger.hpp"
#include "platform/fru/fru.hpp"
#include "lib/pal/pal.hpp"
#include "platform/pciehdevices/include/pci_ids.h"
#include "lib/catalog/catalog.hpp"
#include "dell-pkt-defs.h"
#include "platform/sensor/sensor.hpp"

using namespace sdk::types;
using sdk::types::port_event_t;

namespace sdk {
namespace platform {
namespace ncsi {
CmdHndlrFunc DellOemCmdTable[MAX_DELL_NCSI_CMDS];
std::string os_drv_name = "v1.0.0_Default_Version";
//port_event_info_t port_status[NCSI_CAP_CHANNEL_COUNT];
xcvr_event_info_t xcvr_dom_info[NCSI_CAP_CHANNEL_COUNT];
xcvr_event_info_t xcvr_status[NCSI_CAP_CHANNEL_COUNT];
bool ipc_event_registered;

void CmdHndler::xcvr_dom_event_handler(sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;
    uint8_t port;
    sdk::types::sdk_event_t *sdk_event = (sdk::types::sdk_event_t *)msg->data();
    sdk::types::xcvr_event_info_t *event = &sdk_event->xcvr_event_info;

    if (event->ifindex == 0x11010001)
        port = 0;
    else if (event->ifindex == 0x11020001)
        port = 1;
    else {
        SDK_TRACE_ERR("XCVR DOM event for Invalid port number: %d",
                event->ifindex);
    }
    SDK_TRACE_INFO("--------------  XCVR DOM event from LinkMgr for port: 0x%x, xcvr_type: 0x%x cable_type: 0x%x -------------",
                  port, event->type, event->cable_type);
    memcpy(&xcvr_dom_info[port], event, sizeof(xcvr_event_info_t));

	return;
}

void CmdHndler::xcvr_event_handler(sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;
    uint8_t port;
    sdk::types::sdk_event_t *sdk_event = (sdk::types::sdk_event_t *)msg->data();
    sdk::types::xcvr_event_info_t *event = &sdk_event->xcvr_event_info;

    if (event->ifindex == 0x11010001)
        port = 0;
    else if (event->ifindex == 0x11020001)
        port = 1;
    else {
        SDK_TRACE_ERR("XCVR event for Invalid port number: %d",
                event->ifindex);
    }

    SDK_TRACE_INFO("--------------  XCVR event from LinkMgr for port: 0x%x, xcvr type: 0x%x cable_type: 0x%x -------------",
                  port, event->type, event->cable_type);

    memcpy(&xcvr_status[port], event, sizeof(xcvr_event_info_t));
	return;
}

#if 0
void CmdHndler::port_event_handler(sdk::ipc::ipc_msg_ptr msg, const void *ctxt)
{
    sdk_ret_t ret = SDK_RET_OK;
    port_event_info_t *event = (port_event_info_t *)msg->data();
    memcpy(&port_status[0], &event->port, sizeof(port_status));

    SDK_TRACE_INFO("--------------  Port event from LinkMgr for port: 0x%x, event: %s speed: 0x%x -------------",
                  port_status[0].logical_port, (port_status[0].event == port_event_t::PORT_EVENT_LINK_UP) ? "Up" : "Down", (uint32_t)port_status[0].speed);

	return;
}
#endif
void CmdHndler::DellOemCmdHndlrInit()
{
    DellOemCmdTable[CMD_GET_INVENTORY]          = DellOEMCmdGetInventory;
    DellOemCmdTable[CMD_GET_EXT_CAPABILITIES]   = DellOEMCmdGetExtCap;
    DellOemCmdTable[CMD_GET_PARTITION_INFO]     = DellOEMCmdGetPartitionInfo;
    DellOemCmdTable[CMD_GET_TEMPERATURE]        = DellOEMCmdGetTemperature;
    DellOemCmdTable[CMD_GET_SUP_PAYLOAD_VER]    = DellOEMCmdGetSupPayloadVer;
    DellOemCmdTable[CMD_GET_OS_DRIVER_VER]      = DellOEMCmdGetOsDrvVer;
    DellOemCmdTable[CMD_GET_LLDP]               = DellOEMCmdGetLLDP;
    DellOemCmdTable[CMD_GET_INTF_INFO]          = DellOEMCmdGetIntfInfo;
    DellOemCmdTable[CMD_GET_INTF_SENSOR]        = DellOEMCmdGetIntfSensor;
    DellOemCmdTable[CMD_SEND_LLDP]              = DellOEMCmdSendLLDP;

    if (!ipc_event_registered)
    {
#if 0
        SDK_TRACE_INFO("subscribing for EVENT_ID_XCVR_DOM_STATUS:(%u) and EVENT_ID_XCVR_STATUS: (%u) events", event_id_t::EVENT_ID_XCVR_DOM_STATUS, event_id_t::EVENT_ID_XCVR_STATUS);
        sdk::ipc::subscribe(event_id_t::EVENT_ID_XCVR_DOM_STATUS,
                CmdHndler::xcvr_dom_event_handler, NULL);
        //sdk::ipc::subscribe(event_id_t::EVENT_ID_PORT_STATUS,
        //					CmdHndler::port_event_handler, NULL);
        sdk::ipc::subscribe(event_id_t::EVENT_ID_XCVR_STATUS,
                CmdHndler::xcvr_event_handler, NULL);
#endif
        ipc_event_registered = true;
    }

    for (uint8_t port = 0; port < 2; port++)
    {
        bool link_status = false;
        uint8_t link_speed = 0;

        if ((this->ipc->GetLinkStatus(0, link_status, link_speed, (uint8_t &)xcvr_status[port].cable_type, xcvr_status[port].sprom))) {

            SDK_TRACE_ERR("Failed to read link status during init");
        }
    }

}

void CmdHndler::DellOEMCmdHndlr(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int ret = 0;
    const uint8_t *buf = (uint8_t *)cmd_pkt;
    CmdHndler *hndlr = (CmdHndler *)obj;
    void *rsp = NULL;
    ssize_t rsp_sz = 0;
    uint8_t opcode;

    opcode = buf[DELL_NCSI_CMD_OPCODE_OFFSET];

    if (DellOemCmdTable[opcode]) {
        SDK_TRACE_DEBUG("Handling Ncsi command: 0x%x ", opcode);
        DellOemCmdTable[opcode](obj, cmd_pkt, cmd_sz);

        if (rsp && rsp_sz) {
            SDK_TRACE_INFO("Sending Ncsi Response for cmd: 0x%x ", opcode);

        }
    }
    else {
        SDK_TRACE_ERR("Ncsi command 0x%x is not supported",
                buf[DELL_NCSI_CMD_OPCODE_OFFSET]);
        //TODO: Send the NCSI response saying unsupported command
        hndlr->stats.unsup_cmd_rx_cnt++;
    }

    return;
}

void CmdHndler::DellOEMCmdGetInventory(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    std::string product_name, manufacturer;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetInvRespPkt *resp;
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;

    //read fru for BOARD_PRODUCTNAME_KEY and BOARD_MANUFACTURER_KEY

    sdk::platform::readfrukey(BOARD_PRODUCTNAME_KEY, product_name);
    sdk::platform::readfrukey(BOARD_MANUFACTURER_KEY, manufacturer);

    tlvs.push_back(new DellTLVFields(0, product_name.length(), product_name.c_str()));
    tlvs.push_back(new DellTLVFields(1, manufacturer.length(), manufacturer.c_str()));

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);
    
    resp_len = sizeof(struct DellNcsiGetInvRespPkt);

    for (i = 0; i < tlvs.size(); i++)
        resp_len += tlvs[i]->sizeof_tlv();

    resp = (struct DellNcsiGetInvRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->family_fw_ver = htonl(get_version_resp.fw_version);

    /* 25G cards support only SR and SFP+ media types */
    resp->media_type = (htons)(1 << 4 | 1 << 6); 
    resp->rsvd_ff = 0xFFFFFFFF;

    resp_tlvs = (uint8_t *)&resp->csum;
    
    for (i = 0; i < tlvs.size(); i++)
        resp_off = tlvs[i]->copy_tlv(resp_tlvs + resp_off);

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdGetExtCap(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret;
    uint8_t ncsi_channel = 0;
    NcsiStateErr sm_ret;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetExtCapRespPkt *resp = (struct DellNcsiGetExtCapRespPkt *)hndlr->xport->GetNcsiRspPkt(sizeof(struct DellNcsiGetExtCapRespPkt));
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->capabilities = ((1 << 3)/*oprom*/ | (1 << 4)/*uEFI*/ | (1 << 9)/* on-chip thermal sensor*/ ); 
    resp->dcb_cap = (1 << 1) /* PFC support*/;
    resp->nic_part_cap = 0;
    resp->eswitch_cap = 0;
    resp->num_pci_pf = 1;
    resp->num_pci_vf = 0;


error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(sizeof(struct DellNcsiGetExtCapRespPkt) - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(struct DellNcsiGetExtCapRespPkt));

    return;
}

void CmdHndler::DellOEMCmdGetPartitionInfo(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    std::string product_name;
    std::string iface_name;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetPartitionInfoRespPkt *resp;
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;

    //read fru for BOARD_PRODUCTNAME_KEY
    sdk::platform::readfrukey(BOARD_PRODUCTNAME_KEY, product_name);
    iface_name = product_name + " - LAN";
    tlvs.push_back(new DellTLVFields(0, product_name.length(), iface_name.c_str()));

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);
    
    resp_len = sizeof(struct DellNcsiGetPartitionInfoRespPkt);
    for (i = 0; i < tlvs.size(); i++)
        resp_len += tlvs[i]->sizeof_tlv();

    resp = (struct DellNcsiGetPartitionInfoRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->num_pci_pf_enabled = 1;
    resp->partition_id = 0;
    resp->partition_status = ((1 << 0) /*num personalities*/ | (1 < 3) /*LAN*/);

    resp_tlvs = (uint8_t *)&resp->csum;
    
    for (i = 0; i < tlvs.size(); i++)
        resp_off = tlvs[i]->copy_tlv(resp_tlvs + resp_off);

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdGetTemperature(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i, rc;
    ssize_t ret;
    uint8_t ncsi_channel = 0;
    NcsiStateErr sm_ret;
    system_temperature_t temperature;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetTempRespPkt *resp = (struct DellNcsiGetTempRespPkt *)hndlr->xport->GetNcsiRspPkt(sizeof(struct DellNcsiGetTempRespPkt));
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    rc = sdk::platform::sensor::read_temperatures(&temperature);

    resp->max_temperature = 95;
    resp->cur_temperature = temperature.hbmtemp;

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(sizeof(struct DellNcsiGetTempRespPkt) - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(struct DellNcsiGetTempRespPkt));

    return;
}

void CmdHndler::DellOEMCmdGetSupPayloadVer(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret;
    uint8_t ncsi_channel = 0;
    NcsiStateErr sm_ret;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetSupPayloadVerRespPkt *resp = (struct DellNcsiGetSupPayloadVerRespPkt *)hndlr->xport->GetNcsiRspPkt(sizeof(struct DellNcsiGetSupPayloadVerRespPkt));
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    resp->supported_ver = (1 << 2) /* version 2 */;

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(sizeof(struct DellNcsiGetSupPayloadVerRespPkt) - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(struct DellNcsiGetSupPayloadVerRespPkt));

    return;
}

void CmdHndler::DellOEMCmdGetOsDrvVer(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    std::string product_name, manufacturer;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetOSDrvVersionRespPkt *resp;
    //TODO: need to define specific cmdPkt struct for GetOSDriver
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;

    //TODO: read the OS driver version and populate tlv
    tlvs.push_back(new DellTLVFields(0, os_drv_name.length(), os_drv_name.c_str()));

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);
    
    resp_len = sizeof(struct DellNcsiGetOSDrvVersionRespPkt);

    for (i = 0; i < tlvs.size(); i++)
        resp_len += tlvs[i]->sizeof_tlv();

    resp = (struct DellNcsiGetOSDrvVersionRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->partition_id = 0;
    resp->num_actv_drvs_in_tlvs = 1;

    resp_tlvs = (uint8_t *)&resp->csum;
    
    for (i = 0; i < tlvs.size(); i++)
        resp_off = tlvs[i]->copy_tlv(resp_tlvs + resp_off);

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdGetLLDP(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    bool link_status;
    uint8_t link_speed;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    std::string product_name, manufacturer;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetLLDPRespPkt *resp;
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;
    std::string iface_name = (cmd->cmd.NcsiHdr.channel ? "inb_mnic1" : "inb_mnic0");
    sdk::lib::if_lldp_status_t lldp_status;
    uint8_t chassis_id, port_id;
    uint32_t chassis_val_len, port_val_len;
    char *chassis_value, *port_value;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    if ((hndlr->ipc->GetLinkStatus(cmd->cmd.NcsiHdr.channel, link_status,
            link_speed))) {

        SDK_TRACE_ERR("Failed to read link status. Responding with last saved"
                "link status: 0x%x",last_link_status[cmd->cmd.NcsiHdr.channel]);
        link_status = (last_link_status[cmd->cmd.NcsiHdr.channel] & 0x1) ? true:false;
    }

    // Check Link Status before populating LLDP TLVs
    if (link_status) //if link up
    {
        if (sdk::lib::if_lldp_status_get(iface_name, &lldp_status) == SDK_RET_OK)
        {
            /* prepare TLV for ChassisId */
            //chassis_id = lldp_status.neighbor_status.chassis_status.chassis_id.type;
            chassis_id = 0;
            chassis_value = (char*) lldp_status.neighbor_status.chassis_status.chassis_id.value;
            chassis_val_len = strlen(chassis_value);

            //if chassis_id val is 0 then nothing to populate
            if (chassis_val_len)
            {
                SDK_TRACE_INFO("chassis_id: 0x%x, chassis_val_len: 0x%x, chassis_value: %s", chassis_id, chassis_val_len, chassis_value);
                //LLDP TLV has 7b for type and 9b for len
                chassis_id = ((chassis_id << 1) | ((chassis_val_len >> 8) & 0x1));
                chassis_val_len = (chassis_val_len & 0xFF);

                tlvs.push_back(new DellTLVFields(chassis_id, chassis_val_len, chassis_value));
            }

            /* prepare TLV for PortId */
            //port_id = lldp_status.neighbor_status.port_status.port_id.type;
            port_id = 2;
            port_value = (char *) lldp_status.neighbor_status.port_status.port_id.value;
            port_val_len = strlen(port_value);

            //if port_id val is 0 then nothing to populate
            if (port_val_len)
            {
                SDK_TRACE_INFO("port_id: 0x%x, port_val_len: 0x%x, port_value: %s", port_id, port_val_len, port_value);
                //LLDP TLV has 7b for type and 9b for len
                port_id = ((port_id << 1) | ((port_val_len >> 8) & 0x1));
                port_val_len = (port_val_len & 0xFF);

                tlvs.push_back(new DellTLVFields(port_id, port_val_len, port_value));
            }
        }
        else
        {
            SDK_TRACE_ERR("if_lldp_status_get failed. cannot get LLDP info");
            resp_len = sizeof(struct DellNcsiGetLLDPRespPkt);
            resp = (struct DellNcsiGetLLDPRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INFO_NOT_AVAIL_ERR);

            goto error_out;
        }
    }
    else
    {
        SDK_TRACE_ERR("LLDP info is not available as Link is down");
    }

    /* prepare response packet */
    resp_len = sizeof(struct DellNcsiGetLLDPRespPkt);

    for (i = 0; i < tlvs.size(); i++)
        resp_len += tlvs[i]->sizeof_tlv();

    resp = (struct DellNcsiGetLLDPRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    resp->num_tlvs = tlvs.size();

    resp_tlvs = (uint8_t *)&resp->csum;

    for (i = 0; i < tlvs.size(); i++)
        resp_off = tlvs[i]->copy_tlv(resp_tlvs + resp_off);

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdGetIntfInfo(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    std::string product_name, manufacturer;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetInterfaceInfoRespPkt *resp;
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);
    
    resp_len = sizeof(struct DellNcsiGetInterfaceInfoRespPkt);

    if (xcvr_status[cmd->cmd.NcsiHdr.channel].cable_type == CABLE_TYPE_NONE)
		SDK_TRACE_INFO("SPROM data is not available for current cable type: 0x%x", xcvr_status[cmd->cmd.NcsiHdr.channel].cable_type);
	else
        resp_len += 96; //96 bytes of sprom
    
	resp = (struct DellNcsiGetInterfaceInfoRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

	if (xcvr_status[cmd->cmd.NcsiHdr.channel].cable_type != CABLE_TYPE_FIBER)
	{
		resp->rsp.reason = htons(NCSI_REASON_INFO_NOT_AVAIL_ERR);
        resp->intf_type = 0x2; //Direct Attached Copper
		goto error_out;
	}

    resp->intf_type = 0x3; //Optical Fiber
	resp_tlvs = (uint8_t *)&resp->csum;
    memcpy(resp_tlvs, xcvr_status[cmd->cmd.NcsiHdr.channel].sprom, 96);

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdGetIntfSensor(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i;
    ssize_t ret, resp_len;
    uint8_t ncsi_channel = 0, resp_off = 0;
    uint8_t *resp_tlvs;
    NcsiStateErr sm_ret;
    cable_type_t cable_type;
    std::string product_name, manufacturer, sprom_data;
    struct SfpSesnorInfo sensor_info;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiGetInterfaceSensorRespPkt *resp;
    const struct DellNcsiFixedCmdPkt *cmd = (struct DellNcsiFixedCmdPkt *)cmd_pkt;
    std::vector<DellTLVFields *> tlvs;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    resp_len = sizeof(struct DellNcsiGetInterfaceSensorRespPkt);

    if (xcvr_dom_info[cmd->cmd.NcsiHdr.channel].cable_type != CABLE_TYPE_FIBER)
        SDK_TRACE_INFO("Copper Cable doesn't have SPROM data");
    else
        resp_len += sizeof(struct SfpSesnorInfo);

    resp = (struct DellNcsiGetInterfaceSensorRespPkt *) hndlr->xport->GetNcsiRspPkt(resp_len);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    if (xcvr_dom_info[cmd->cmd.NcsiHdr.channel].cable_type != CABLE_TYPE_FIBER)
	{
		resp->rsp.reason = htons(NCSI_REASON_INFO_NOT_AVAIL_ERR);
		goto error_out;
	}

    switch(xcvr_dom_info[cmd->cmd.NcsiHdr.channel].type)
    {
        case XCVR_TYPE_SFP:
            resp->identifier = 0x3;
            break;
        case XCVR_TYPE_QSFP:
            resp->identifier = 0xC;
            break;
        case XCVR_TYPE_QSFP28:
            resp->identifier = 0x11;
            break;
        default:
            SDK_TRACE_ERR("UNKNOWN xcvr type: 0x%x", xcvr_dom_info[cmd->cmd.NcsiHdr.channel].type);
            resp->rsp.reason = htons(NCSI_REASON_INFO_NOT_AVAIL_ERR);
            goto error_out;
    }

    resp_tlvs = (uint8_t *)&resp->csum;
    memset(&sensor_info, 0, sizeof(sensor_info));

    memcpy(&sensor_info.temp_high_alarm_threshold, &xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[0], sizeof(__be16));
    memcpy(&sensor_info.temp_high_warn_threshold, &xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[4], sizeof(__be16));
    memcpy(&sensor_info.temperature, &xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[96], (5 * sizeof(__be16)));
    sensor_info.flag1 = xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[112];
    sensor_info.flag2 = xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[113];
    sensor_info.flag3 = xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[116];
    sensor_info.flag4 = xcvr_dom_info[cmd->cmd.NcsiHdr.channel].sprom[117];

    memcpy(resp_tlvs, &sensor_info, sizeof(sensor_info));

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(resp_len - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, resp_len);

    return;
}

void CmdHndler::DellOEMCmdSendLLDP(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    int i, rc;
    ssize_t ret;
    uint8_t ncsi_channel = 0;
    NcsiStateErr sm_ret;
    DellTLVFields *lldp_tlv = NULL;
    char lldpcli_cmd[256] = {0,};
    char port_str[32] = {0,};
    char str[64];
    uint32_t ttl_val = 0;

    CmdHndler *hndlr = (CmdHndler *)obj;
    struct DellNcsiMinResp *resp = (struct DellNcsiMinResp *)hndlr->xport->GetNcsiRspPkt(sizeof(struct DellNcsiMinResp));
    const struct DellNcsiSendLLDPCmdPkt *cmd = (struct DellNcsiSendLLDPCmdPkt *)cmd_pkt;

    NCSI_CMD_BEGIN_BANNER();
    
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_OEM_COMMAND);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[cmd->cmd.NcsiHdr.channel].GetCurState());
            goto error_out;
        }
        else {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;
        }
    }

    //TODO: call lldpcli to configure the LLDP params provided by BMC
    SDK_TRACE_INFO("lldp set interface cmd: %s rc: %d", lldpcli_cmd, rc);

    lldp_tlv = (DellTLVFields *)&cmd->csum;

    /* Update each LLDP param provided by BMC */
    for (i = 0; i < cmd->num_tlvs; i++)
    {
        //clear the string for tlv
        memset(str, 0, sizeof(str));
        memset(lldpcli_cmd, 0, sizeof(lldpcli_cmd));
        switch (lldp_tlv->type) //ChassisId
        {
            case 1:
                memcpy(str, lldp_tlv->val, lldp_tlv->len);
                snprintf(lldpcli_cmd, sizeof(lldpcli_cmd), "/usr/sbin/lldpcli configure system chassisid %s",str);
                break;

            case 3:
                memcpy(str, lldp_tlv->val, lldp_tlv->len);
                ttl_val = strtol(str, NULL, 16);
                if (!ttl_val)
                {
                    //TODO: remove all previous lldp config done by BMC
                }
                else
                {
                    /* tx-interval is always 30s so we will calculate tx-hold by ttl/30
                     * We also do rounding to the lower number if BMC provided 
                     * number is not in multiple of 30 
                     * */
                    snprintf(lldpcli_cmd, sizeof(lldpcli_cmd), "/usr/sbin/lldpcli configure lldp tx-hold %d", (((ttl_val/30)*30)/30));
                }
                break;

            case 4:
                memcpy(str, lldp_tlv->val, lldp_tlv->len);
                snprintf(lldpcli_cmd, sizeof(lldpcli_cmd), "/usr/sbin/lldpcli configure ports %s lldp portdescription  %s",cmd->cmd.NcsiHdr.channel ? "inb_mnic1":"inb_mnic0", str);
                break;

            case 0x7F:
                memcpy(str, lldp_tlv->val, lldp_tlv->len);
                //TODO: form the proper command for custom-tlv
                //snprintf(lldpcli_cmd, sizeof(lldpcli_cmd), "/usr/sbin/lldpcli configure ports %s lldp custom-tlv oui %s subtype 1 %s", cmd->cmd.NcsiHdr.channel ? "inb_mnic1":"inb_mnic0", oui, subtype, oui_info);
                break;

            default:
                SDK_TRACE_ERR("TLV type %d not supported", lldp_tlv->type);
        }

        rc = system(lldpcli_cmd);
        if (rc == -1)
        {
            SDK_TRACE_ERR("lldp command: %s failed with error %d",
                    lldpcli_cmd, rc);

            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            SDK_TRACE_ERR("cmd: %p failed due to internal err in state machine",
                    cmd);
            goto error_out;

        }

        SDK_TRACE_INFO("system cmd: %s. rc: %d", lldpcli_cmd, rc);

        lldp_tlv = (DellTLVFields *)(lldp_tlv->val + lldp_tlv->len);
	}

error_out:
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.mfg_id = htonl(DELL_NCSI_MFG_ID);
    resp->rsp.payload_ver = DELL_NCSI_MSG_PAYLOAD_VER;
    resp->rsp.cmd_id = cmd->cmd_id;

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(sizeof(struct DellNcsiMinResp) - sizeof(struct NcsiFixedResp));

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(struct DellNcsiMinResp));

    return;
}

} // namespace ncsi
} // namespace platform
} // namespace sdk

