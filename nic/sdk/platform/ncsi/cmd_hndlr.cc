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
#include "lib/utils/path_utils.hpp"

namespace sdk {
namespace platform {
namespace ncsi {

uint32_t CmdHndler::init_done = 0;
CmdHndlrFunc CmdHndler::CmdTable[MAX_NCSI_CMDS];
uint64_t CmdHndler::mac_addr_list[NCSI_CAP_CHANNEL_COUNT][NCSI_CAP_MIXED_MAC_FILTER_COUNT];
uint16_t CmdHndler::vlan_filter_list[NCSI_CAP_CHANNEL_COUNT][NCSI_CAP_VLAN_FILTER_COUNT];
uint8_t CmdHndler::vlan_mode_list[NCSI_CAP_CHANNEL_COUNT];
uint32_t CmdHndler::last_link_status[NCSI_CAP_CHANNEL_COUNT];
sdk::platform::utils::mpartition* CmdHndler::mempartition;

StateMachine CmdHndler::StateM[NCSI_CAP_CHANNEL_COUNT];
NcsiParamDb CmdHndler::NcsiDb[NCSI_CAP_CHANNEL_COUNT];
struct GetCapRespPkt CmdHndler::get_cap_resp;
struct GetVersionIdRespPkt CmdHndler::get_version_resp;

std::string fw_ver_file = "/nic/etc/VERSION.json";
ptree prop_tree;

static uint8_t *
memrev (uint8_t *block, size_t elnum)
{
    uint8_t *s, *t, tmp;
    for (s = block, t = s + (elnum - 1); s < t; s++, t--) {
        tmp = *s;
        *s = *t;
        *t = tmp;
    }
    return block;
}


void CmdHndler::populate_fw_name_ver()
{
    std::string fw_git_sha;
    std::string delimiter = ".";
    uint8_t ver_id[4] = {0,};
    uint32_t idx = 0;
    size_t pos = 0;
    std::string token;


    if (access(fw_ver_file.c_str(), R_OK) < 0) {
        SDK_TRACE_ERR("fw version file %s has no read permissions",
                fw_ver_file.c_str());
    }
    else
    {
        try {
            boost::property_tree::read_json(fw_ver_file, prop_tree);
            fw_git_sha = prop_tree.get<std::string>("sw.sha", "");
            strncpy((char*)get_version_resp.fw_name, fw_git_sha.c_str(),
                    sizeof(get_version_resp.fw_name));
            //    get_version_resp.fw_version
            std::string version = prop_tree.get<std::string>("sw.version", "");
            std::string s = version.substr(0, version.find("-"));
            while ((pos = s.find(delimiter)) != std::string::npos) {
                token = s.substr(0, pos);
                std::cout << token << std::endl;
                ver_id[idx] = std::stoi(token);
                idx++;
                s.erase(0, pos + delimiter.length());
            }

            std::cout << s << std::endl;
            ver_id[idx] = std::stoi(s);
        }
        catch(std::exception &err) {
            std::cout << "Conversion failure: "<< err.what() << std::endl;
        }

        get_version_resp.fw_version = htonl((uint32_t)(ver_id[0] | (ver_id[1] << 8) |
                (ver_id[2] << 16) | (ver_id[3] << 24)));
    }
}

#define PORT_MAC_STAT_REPORT_SIZE   (1024)
#define ASIC_HBM_REG_PORT_STATS    "port_stats"

void CmdHndler::ReadMacStats(uint32_t port, struct port_stats& stats)
{
    uint64_t port_stats_base = 0;

    if (port == 0x11010001) //first uplink
        port_stats_base = mempartition->start_addr(ASIC_HBM_REG_PORT_STATS);

    else if (port == 0x11020001) //second uplink
        port_stats_base = mempartition->start_addr(ASIC_HBM_REG_PORT_STATS) +
            PORT_MAC_STAT_REPORT_SIZE;
    else if (port == 0x11030001) //BX port
        port_stats_base = mempartition->start_addr(ASIC_HBM_REG_PORT_STATS) +
            (2 * PORT_MAC_STAT_REPORT_SIZE);

    if (port_stats_base)
        sdk::lib::pal_mem_read(port_stats_base, (uint8_t *)&stats,
                sizeof(struct port_stats));
    else
        SDK_TRACE_ERR("ReadMacStats: Invalid port number: %d\n", port);

    SDK_TRACE_DEBUG("Stats for port: %d\n", port);
    SDK_TRACE_DEBUG("======================================");
    SDK_TRACE_DEBUG(" frames_rx_ok                :%llu",     stats.frames_rx_ok              );
    SDK_TRACE_DEBUG(" frames_rx_all               :%llu",     stats.frames_rx_all             );
    SDK_TRACE_DEBUG(" frames_rx_bad_fcs           :%llu",     stats.frames_rx_bad_fcs         );
    SDK_TRACE_DEBUG(" frames_rx_bad_all           :%llu",     stats.frames_rx_bad_all         );
    SDK_TRACE_DEBUG(" octets_rx_ok                :%llu",     stats.octets_rx_ok              );
    SDK_TRACE_DEBUG(" octets_rx_all               :%llu",     stats.octets_rx_all             );
    SDK_TRACE_DEBUG(" frames_rx_unicast           :%llu",     stats.frames_rx_unicast         );
    SDK_TRACE_DEBUG(" frames_rx_multicast         :%llu",     stats.frames_rx_multicast       );
    SDK_TRACE_DEBUG(" frames_rx_broadcast         :%llu",     stats.frames_rx_broadcast       );
    SDK_TRACE_DEBUG(" frames_rx_pause             :%llu",     stats.frames_rx_pause           );
    SDK_TRACE_DEBUG(" frames_rx_bad_length        :%llu",     stats.frames_rx_bad_length      );
    SDK_TRACE_DEBUG(" frames_rx_undersized        :%llu",     stats.frames_rx_undersized      );
    SDK_TRACE_DEBUG(" frames_rx_oversized         :%llu",     stats.frames_rx_oversized       );
    SDK_TRACE_DEBUG(" frames_rx_fragments         :%llu",     stats.frames_rx_fragments       );
    SDK_TRACE_DEBUG(" frames_rx_jabber            :%llu",     stats.frames_rx_jabber          );
    SDK_TRACE_DEBUG(" frames_rx_pripause          :%llu",     stats.frames_rx_pripause        );
    SDK_TRACE_DEBUG(" frames_rx_stomped_crc       :%llu",     stats.frames_rx_stomped_crc     );
    SDK_TRACE_DEBUG(" frames_rx_too_long          :%llu",     stats.frames_rx_too_long        );
    SDK_TRACE_DEBUG(" frames_rx_vlan_good         :%llu",     stats.frames_rx_vlan_good       );
    SDK_TRACE_DEBUG(" frames_rx_dropped           :%llu",     stats.frames_rx_dropped         );
    SDK_TRACE_DEBUG(" frames_rx_less_than_64b     :%llu",     stats.frames_rx_less_than_64b   );
    SDK_TRACE_DEBUG(" frames_rx_64b               :%llu",     stats.frames_rx_64b             );
    SDK_TRACE_DEBUG(" frames_rx_65b_127b          :%llu",     stats.frames_rx_65b_127b        );
    SDK_TRACE_DEBUG(" frames_rx_128b_255b         :%llu",     stats.frames_rx_128b_255b       );
    SDK_TRACE_DEBUG(" frames_rx_256b_511b         :%llu",     stats.frames_rx_256b_511b       );
    SDK_TRACE_DEBUG(" frames_rx_512b_1023b        :%llu",     stats.frames_rx_512b_1023b      );
    SDK_TRACE_DEBUG(" frames_rx_1024b_1518b       :%llu",     stats.frames_rx_1024b_1518b     );
    SDK_TRACE_DEBUG(" frames_rx_1519b_2047b       :%llu",     stats.frames_rx_1519b_2047b     );
    SDK_TRACE_DEBUG(" frames_rx_2048b_4095b       :%llu",     stats.frames_rx_2048b_4095b     );
    SDK_TRACE_DEBUG(" frames_rx_4096b_8191b       :%llu",     stats.frames_rx_4096b_8191b     );
    SDK_TRACE_DEBUG(" frames_rx_8192b_9215b       :%llu",     stats.frames_rx_8192b_9215b     );
    SDK_TRACE_DEBUG(" frames_rx_other             :%llu",     stats.frames_rx_other           );
    SDK_TRACE_DEBUG(" frames_tx_ok                :%llu",     stats.frames_tx_ok              );
    SDK_TRACE_DEBUG(" frames_tx_all               :%llu",     stats.frames_tx_all             );
    SDK_TRACE_DEBUG(" frames_tx_bad               :%llu",     stats.frames_tx_bad             );
    SDK_TRACE_DEBUG(" octets_tx_ok                :%llu",     stats.octets_tx_ok              );
    SDK_TRACE_DEBUG(" octets_tx_total             :%llu",     stats.octets_tx_total           );
    SDK_TRACE_DEBUG(" frames_tx_unicast           :%llu",     stats.frames_tx_unicast         );
    SDK_TRACE_DEBUG(" frames_tx_multicast         :%llu",     stats.frames_tx_multicast       );
    SDK_TRACE_DEBUG(" frames_tx_broadcast         :%llu",     stats.frames_tx_broadcast       );
    SDK_TRACE_DEBUG(" frames_tx_pause             :%llu",     stats.frames_tx_pause           );
    SDK_TRACE_DEBUG(" frames_tx_pripause          :%llu",     stats.frames_tx_pripause        );
    SDK_TRACE_DEBUG(" frames_tx_vlan              :%llu",     stats.frames_tx_vlan            );
    SDK_TRACE_DEBUG(" frames_tx_less_than_64b     :%llu",     stats.frames_tx_less_than_64b   );
    SDK_TRACE_DEBUG(" frames_tx_64b               :%llu",     stats.frames_tx_64b             );
    SDK_TRACE_DEBUG(" frames_tx_65b_127b          :%llu",     stats.frames_tx_65b_127b        );
    SDK_TRACE_DEBUG(" frames_tx_128b_255b         :%llu",     stats.frames_tx_128b_255b       );
    SDK_TRACE_DEBUG(" frames_tx_256b_511b         :%llu",     stats.frames_tx_256b_511b       );
    SDK_TRACE_DEBUG(" frames_tx_512b_1023b        :%llu",     stats.frames_tx_512b_1023b      );
    SDK_TRACE_DEBUG(" frames_tx_1024b_1518b       :%llu",     stats.frames_tx_1024b_1518b     );
    SDK_TRACE_DEBUG(" frames_tx_1519b_2047b       :%llu",     stats.frames_tx_1519b_2047b     );
    SDK_TRACE_DEBUG(" frames_tx_2048b_4095b       :%llu",     stats.frames_tx_2048b_4095b     );
    SDK_TRACE_DEBUG(" frames_tx_4096b_8191b       :%llu",     stats.frames_tx_4096b_8191b     );
    SDK_TRACE_DEBUG(" frames_tx_8192b_9215b       :%llu",     stats.frames_tx_8192b_9215b     );
    SDK_TRACE_DEBUG(" frames_tx_other             :%llu",     stats.frames_tx_other           );
    SDK_TRACE_DEBUG(" frames_tx_pri_0             :%llu",     stats.frames_tx_pri_0           );
    SDK_TRACE_DEBUG(" frames_tx_pri_1             :%llu",     stats.frames_tx_pri_1           );
    SDK_TRACE_DEBUG(" frames_tx_pri_2             :%llu",     stats.frames_tx_pri_2           );
    SDK_TRACE_DEBUG(" frames_tx_pri_3             :%llu",     stats.frames_tx_pri_3           );
    SDK_TRACE_DEBUG(" frames_tx_pri_4             :%llu",     stats.frames_tx_pri_4           );
    SDK_TRACE_DEBUG(" frames_tx_pri_5             :%llu",     stats.frames_tx_pri_5           );
    SDK_TRACE_DEBUG(" frames_tx_pri_6             :%llu",     stats.frames_tx_pri_6           );
    SDK_TRACE_DEBUG(" frames_tx_pri_7             :%llu",     stats.frames_tx_pri_7           );
    SDK_TRACE_DEBUG(" frames_rx_pri_0             :%llu",     stats.frames_rx_pri_0           );
    SDK_TRACE_DEBUG(" frames_rx_pri_1             :%llu",     stats.frames_rx_pri_1           );
    SDK_TRACE_DEBUG(" frames_rx_pri_2             :%llu",     stats.frames_rx_pri_2           );
    SDK_TRACE_DEBUG(" frames_rx_pri_3             :%llu",     stats.frames_rx_pri_3           );
    SDK_TRACE_DEBUG(" frames_rx_pri_4             :%llu",     stats.frames_rx_pri_4           );
    SDK_TRACE_DEBUG(" frames_rx_pri_5             :%llu",     stats.frames_rx_pri_5           );
    SDK_TRACE_DEBUG(" frames_rx_pri_6             :%llu",     stats.frames_rx_pri_6           );
    SDK_TRACE_DEBUG(" frames_rx_pri_7             :%llu",     stats.frames_rx_pri_7           );
    SDK_TRACE_DEBUG(" tx_pripause_0_1us_count     :%llu",     stats.tx_pripause_0_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_1_1us_count     :%llu",     stats.tx_pripause_1_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_2_1us_count     :%llu",     stats.tx_pripause_2_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_3_1us_count     :%llu",     stats.tx_pripause_3_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_4_1us_count     :%llu",     stats.tx_pripause_4_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_5_1us_count     :%llu",     stats.tx_pripause_5_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_6_1us_count     :%llu",     stats.tx_pripause_6_1us_count   );
    SDK_TRACE_DEBUG(" tx_pripause_7_1us_count     :%llu",     stats.tx_pripause_7_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_0_1us_count     :%llu",     stats.rx_pripause_0_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_1_1us_count     :%llu",     stats.rx_pripause_1_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_2_1us_count     :%llu",     stats.rx_pripause_2_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_3_1us_count     :%llu",     stats.rx_pripause_3_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_4_1us_count     :%llu",     stats.rx_pripause_4_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_5_1us_count     :%llu",     stats.rx_pripause_5_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_6_1us_count     :%llu",     stats.rx_pripause_6_1us_count   );
    SDK_TRACE_DEBUG(" rx_pripause_7_1us_count     :%llu",     stats.rx_pripause_7_1us_count   );
    SDK_TRACE_DEBUG(" rx_pause_1us_count          :%llu",     stats.rx_pause_1us_count        );
    SDK_TRACE_DEBUG(" frames_tx_truncated         :%llu",     stats.frames_tx_truncated       );

    SDK_TRACE_DEBUG("======================================");

}

void CmdHndler::GetMacStats(uint32_t port, struct GetNicStatsRespPkt *resp)
{
    struct port_stats p_stats = {0,};

    if (port == 0)
        port = 0x11010001; //first uplink
    else if (port == 1)
        port = 0x11020001; //second uplink
    else {
        SDK_TRACE_ERR("Invalid port number: %d. Skipping reading mac stats",
                port);
        return;
    }

    ReadMacStats(port, p_stats);

    memcpy(&resp->rx_bytes, memrev((uint8_t*)&p_stats.octets_rx_all, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->tx_bytes, memrev((uint8_t*)&p_stats.octets_tx_total, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->rx_uc_pkts, memrev((uint8_t*)&p_stats.frames_rx_unicast, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->rx_mc_pkts, memrev((uint8_t*)&p_stats.frames_rx_multicast, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->rx_bc_pkts, memrev((uint8_t*)&p_stats.frames_rx_broadcast, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->tx_uc_pkts, memrev((uint8_t*)&p_stats.frames_tx_unicast, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->tx_mc_pkts, memrev((uint8_t*)&p_stats.frames_tx_multicast, sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&resp->tx_bc_pkts, memrev((uint8_t*)&p_stats.frames_tx_broadcast, sizeof(uint64_t)), sizeof(uint64_t));
    resp->fcs_err = htonl(p_stats.frames_rx_bad_fcs);
    //resp->align_err = p_stats.
    //resp->false_carrier = p_stats.
    resp->runt_pkts = htonl(p_stats.frames_rx_undersized);
    resp->jabber_pkts = htonl(p_stats.frames_rx_jabber);
    //resp->rx_pause_xon = ;
    resp->rx_pause_xoff = htonl(p_stats.frames_rx_pause);
    //resp->tx_pause_xon = p_stats.
    resp->tx_pause_xoff = htonl(p_stats.frames_tx_pause);
    //resp->tx_s_collision = p_stats.
    //resp->tx_m_collision = p_stats.
    //resp->l_collision = p_stats.
    //resp->e_collision = p_stats.
    //resp->rx_ctl_frames = p_stats.
    resp->rx_64_frames = htonl(p_stats.frames_rx_64b);
    resp->rx_127_frames = htonl(p_stats.frames_rx_65b_127b);
    resp->rx_255_frames = htonl(p_stats.frames_rx_128b_255b);
    resp->rx_511_frames = htonl(p_stats.frames_rx_512b_1023b);
    resp->rx_1023_frames = htonl(p_stats.frames_rx_1024b_1518b);
    resp->rx_1522_frames = htonl(p_stats.frames_rx_1519b_2047b);
    resp->rx_9022_frames = htonl(p_stats.frames_rx_1519b_2047b + p_stats.frames_rx_2048b_4095b + p_stats.frames_rx_4096b_8191b +p_stats.frames_rx_8192b_9215b);
    resp->tx_64_frames = htonl(p_stats.frames_tx_64b);
    resp->tx_127_frames = htonl(p_stats.frames_tx_65b_127b);
    resp->tx_255_frames = htonl(p_stats.frames_tx_128b_255b);
    resp->tx_511_frames = htonl(p_stats.frames_tx_256b_511b);
    resp->tx_1023_frames = htonl(p_stats.frames_tx_512b_1023b);
    resp->tx_1522_frames = htonl(p_stats.frames_tx_1024b_1518b);
    resp->tx_9022_frames = htonl(p_stats.frames_tx_1519b_2047b + p_stats.frames_tx_2048b_4095b + p_stats.frames_tx_4096b_8191b + p_stats.frames_tx_8192b_9215b);
    memcpy(&resp->rx_valid_bytes, memrev((uint8_t*)&p_stats.octets_rx_ok, sizeof(uint64_t)), sizeof(uint64_t));
    //resp->rx_runt_pkts = p_stats.
    ////resp->rx_jabber_pkts = p_stats.

    SDK_TRACE_DEBUG("Response message: ");
    SDK_TRACE_DEBUG("cnt_hi                :%u",   resp->cnt_hi            );
    SDK_TRACE_DEBUG("cnt_lo                :%u",   resp->cnt_lo            );
    SDK_TRACE_DEBUG("rx_bytes              :%llu", resp->rx_bytes          );
    SDK_TRACE_DEBUG("tx_bytes              :%llu", resp->tx_bytes          );
    SDK_TRACE_DEBUG("rx_uc_pkts            :%llu", resp->rx_uc_pkts        );
    SDK_TRACE_DEBUG("rx_mc_pkts            :%llu", resp->rx_mc_pkts        );
    SDK_TRACE_DEBUG("rx_bc_pkts            :%llu", resp->rx_bc_pkts        );
    SDK_TRACE_DEBUG("tx_uc_pkts            :%llu", resp->tx_uc_pkts        );
    SDK_TRACE_DEBUG("tx_mc_pkts            :%llu", resp->tx_mc_pkts        );
    SDK_TRACE_DEBUG("tx_bc_pkts            :%llu", resp->tx_bc_pkts        );
    SDK_TRACE_DEBUG("fcs_err               :%u",   resp->fcs_err           );
    SDK_TRACE_DEBUG("align_err             :%u",   resp->align_err         );
    SDK_TRACE_DEBUG("false_carrier         :%u",   resp->false_carrier     );
    SDK_TRACE_DEBUG("runt_pkts             :%u",   resp->runt_pkts         );
    SDK_TRACE_DEBUG("jabber_pkts           :%u",   resp->jabber_pkts       );
    SDK_TRACE_DEBUG("rx_pause_xon          :%u",   resp->rx_pause_xon      );
    SDK_TRACE_DEBUG("rx_pause_xoff         :%u",   resp->rx_pause_xoff     );
    SDK_TRACE_DEBUG("tx_pause_xon          :%u",   resp->tx_pause_xon      );
    SDK_TRACE_DEBUG("tx_pause_xoff         :%u",   resp->tx_pause_xoff     );
    SDK_TRACE_DEBUG("tx_s_collision        :%u",   resp->tx_s_collision    );
    SDK_TRACE_DEBUG("tx_m_collision        :%u",   resp->tx_m_collision    );
    SDK_TRACE_DEBUG("l_collision           :%u",   resp->l_collision       );
    SDK_TRACE_DEBUG("e_collision           :%u",   resp->e_collision       );
    SDK_TRACE_DEBUG("rx_ctl_frames         :%u",   resp->rx_ctl_frames     );
    SDK_TRACE_DEBUG("rx_64_frames          :%u",   resp->rx_64_frames      );
    SDK_TRACE_DEBUG("rx_127_frames         :%u",   resp->rx_127_frames     );
    SDK_TRACE_DEBUG("rx_255_frames         :%u",   resp->rx_255_frames     );
    SDK_TRACE_DEBUG("rx_511_frames         :%u",   resp->rx_511_frames     );
    SDK_TRACE_DEBUG("rx_1023_frames        :%u",   resp->rx_1023_frames    );
    SDK_TRACE_DEBUG("rx_1522_frames        :%u",   resp->rx_1522_frames    );
    SDK_TRACE_DEBUG("rx_9022_frames        :%u",   resp->rx_9022_frames    );
    SDK_TRACE_DEBUG("tx_64_frames          :%u",   resp->tx_64_frames      );
    SDK_TRACE_DEBUG("tx_127_frames         :%u",   resp->tx_127_frames     );
    SDK_TRACE_DEBUG("tx_255_frames         :%u",   resp->tx_255_frames     );
    SDK_TRACE_DEBUG("tx_511_frames         :%u",   resp->tx_511_frames     );
    SDK_TRACE_DEBUG("tx_1023_frames        :%u",   resp->tx_1023_frames    );
    SDK_TRACE_DEBUG("tx_1522_frames        :%u",   resp->tx_1522_frames    );
    SDK_TRACE_DEBUG("tx_9022_frames        :%u",   resp->tx_9022_frames    );
    SDK_TRACE_DEBUG("rx_valid_bytes        :%llu", resp->rx_valid_bytes    );
    SDK_TRACE_DEBUG("rx_runt_pkts          :%u",   resp->rx_runt_pkts      );
    SDK_TRACE_DEBUG("rx_jabber_pkts        :%u",   resp->rx_jabber_pkts    );


}

CmdHndler *
CmdHndler::factory(std::shared_ptr<IpcService> IpcObj, transport *xport,
                   std::string dev_feature_profile, EV_P)
{
    CmdHndler *cmd_hndlr;

    if (!init_done)
    {
        CmdTable[CMD_CLEAR_INIT_STATE]        = ClearInitState;
        CmdTable[CMD_SELECT_PACKAGE]          = SelectPackage;
        CmdTable[CMD_DESELECT_PACKAGE]        = DeselectPackage;
        CmdTable[CMD_EN_CHAN]                 = EnableChan;
        CmdTable[CMD_DIS_CHAN]                = EnableChan;
        CmdTable[CMD_RESET_CHAN]              = ResetChan;
        CmdTable[CMD_EN_CHAN_NW_TX]           = EnableChanNwTx;
        CmdTable[CMD_DIS_CHAN_NW_TX]          = EnableChanNwTx;
        CmdTable[CMD_SET_LINK]                = SetLink;
        CmdTable[CMD_GET_LINK_STATUS]         = GetLinkStatus;
        CmdTable[CMD_SET_VLAN_FILTER]         = SetVlanFilter;
        CmdTable[CMD_EN_VLAN]                 = EnableVlan;
        CmdTable[CMD_DIS_VLAN]                = DisableVlan;
        CmdTable[CMD_SET_MAC_ADDR]            = SetMacAddr;
        CmdTable[CMD_EN_BCAST_FILTER]         = EnableBcastFilter;
        CmdTable[CMD_DIS_BCAST_FILTER]        = DisableBcastFilter;
        CmdTable[CMD_EN_GLOBAL_MCAST_FILTER]  = EnableGlobalMcastFilter;
        CmdTable[CMD_DIS_GLOBAL_MCAST_FILTER] = DisableGlobalMcastFilter;
        CmdTable[CMD_GET_VER_ID]              = GetVersionId;
        CmdTable[CMD_GET_CAP]                 = GetCapabilities;
        CmdTable[CMD_GET_PARAMS]              = GetParams;
        CmdTable[CMD_GET_NIC_STATS]           = GetNicPktStats;
        CmdTable[CMD_GET_NCSI_STATS]          = GetNcsiStats;
        CmdTable[CMD_GET_NCSI_PASSTHRU_STATS] = GetNcsiPassthruStats;
        CmdTable[CMD_GET_PACKAGE_STATUS]      = GetPackageStatus;
        CmdTable[CMD_GET_PACKAGE_UUID]        = GetPackageUUID;
        CmdTable[CMD_OEM_COMMAND]             = HandleOEMCmd;

        get_cap_resp.cap = htonl(NCSI_CAP_HW_ARB | NCSI_CAP_HOST_NC_DRV_STATUS | NCSI_CAP_NC_TO_MC_FLOW_CTRL | NCSI_CAP_MC_TO_NC_FLOW_CTRL | NCSI_CAP_ALL_MCAST_ADDR_SUPPORT | NCSI_CAP_HW_ARB_IMPL_STATUS);
        get_cap_resp.bc_cap = htonl(NCSI_CAP_BCAST_FILTER_ARP | NCSI_CAP_BCAST_FILTER_DHCP_CLIENT | NCSI_CAP_BCAST_FILTER_DHCP_SERVER | NCSI_CAP_BCAST_FILTER_NETBIOS);
        get_cap_resp.mc_cap = htonl(NCSI_CAP_MCAST_IPV6_NEIGH_ADV | NCSI_CAP_MCAST_IPV6_ROUTER_ADV | NCSI_CAP_MCAST_DHCPV6_RELAY | NCSI_CAP_MCAST_DHCPV6_MCAST_SERVER_TO_CLIENT | NCSI_CAP_MCAST_IPV6_MLD | NCSI_CAP_MCAST_IPV6_NEIGH_SOL);
        get_cap_resp.buf_cap = htonl(NCSI_CAP_BUFFERRING);
        get_cap_resp.aen_cap = htonl(NCSI_CAP_AEN_CTRL_LINK_STATUS_CHANGE | NCSI_CAP_AEN_CTRL_CONFIG_REQUIRED | NCSI_CAP_AEN_CTRL_HOST_NC_DRV_STATUS_CHANGE | NCSI_CAP_AEN_CTRL_OEM_SPECIFIC);
        get_cap_resp.vlan_cnt = NCSI_CAP_VLAN_FILTER_COUNT;
        get_cap_resp.mc_cnt = NCSI_CAP_MCAST_FILTER_COUNT;
        get_cap_resp.uc_cnt = NCSI_CAP_UCAST_FILTER_COUNT;
        get_cap_resp.mixed_cnt = NCSI_CAP_MIXED_MAC_FILTER_COUNT;
        get_cap_resp.vlan_mode = NCSI_CAP_VLAN_MODE_SUPPORT;
        get_cap_resp.channel_cnt = NCSI_CAP_CHANNEL_COUNT;

        sdk::lib::catalog *catalog_db = sdk::lib::catalog::factory();
        //ncsi version 1.1.0 and alpha is 0
        get_version_resp.ncsi_version = htonl(0xF1F1F000);
        get_version_resp.pci_ids[0] = htons(catalog_db->pcie_vendorid());
        get_version_resp.pci_ids[1] = htons(PCI_DEVICE_ID_PENSANDO_ENET);
        get_version_resp.pci_ids[2] = htons(catalog_db->pcie_subvendorid());
        get_version_resp.pci_ids[3] = htons(catalog_db->pcie_subdeviceid());

        /* Pensando IANA enterprise ID as per:
         * https://www.iana.org/assignments/enterprise-numbers/enterprise-numbers */
		get_version_resp.mf_id = htonl(51886);

		char *cfg_path = std::getenv("CONFIG_PATH");
		char *pipeline = std::getenv("PIPELINE");

		if (!cfg_path)
			cfg_path = (char *)"./";

		SDK_TRACE_INFO("config_path:%s, pipeline:%s", cfg_path, pipeline);
		std::string mpart_json =
			sdk::lib::get_mpart_file_path(cfg_path, std::string(pipeline), catalog_db, 
					dev_feature_profile, platform_type_t::PLATFORM_TYPE_HW);
		mempartition =
			sdk::platform::utils::mpartition::factory(mpart_json.c_str());

		populate_fw_name_ver();

        assert(sdk::lib::pal_init(platform_type_t::PLATFORM_TYPE_HW) ==
                sdk::lib::PAL_RET_OK);
    }

    cmd_hndlr =  new CmdHndler(IpcObj, xport);

    if (cmd_hndlr->Init(EV_A))
    {
        SDK_TRACE_ERR("xport: %s: CmdHndler failed to initialize",
                xport->name().c_str());

        cmd_hndlr->~CmdHndler();
        return NULL;
    }

    return cmd_hndlr;

}

CmdHndler::CmdHndler(std::shared_ptr<IpcService> IpcObj, transport *XportObj)
{
    memset(&stats, 0, sizeof(struct NcsiStats));
    //memset(CmdTable, 0, sizeof(CmdTable));
    //memset(mac_addr_list, 0, sizeof(mac_addr_list));
    //memset(vlan_filter_list, 0, sizeof(vlan_filter_list));
    //memset(vlan_mode_list, 0, sizeof(vlan_mode_list));
    //memset(NcsiDb, 0, sizeof(NcsiDb));

#if 0
    for (uint8_t ncsi_channel = 0; ncsi_channel < NCSI_CAP_CHANNEL_COUNT;
            ncsi_channel++) {
        StateM[ncsi_channel] = new StateMachine();
        NcsiDb[ncsi_channel] = new NcsiParamDb();
    }
#endif

    ipc = IpcObj;
    xport = XportObj;

}

CmdHndler::~CmdHndler()
{
}

int CmdHndler::Init(EV_P)
{

    //Initialize the transport interface
    xport->Init();

    PktData = (uint8_t *)calloc(1514, sizeof(uint8_t));
    if(!PktData)
    {
        SDK_TRACE_ERR("xport: %s: Can't alloc mem for NCSI Packet handler",
                xport->name().c_str());
        return -1;
    }
#if 1
    this->loop = EV_A;
#else
    //Start timer thread to recv packets from transport interface
    if (!(xport->name().compare("RBT_oob_mnic0")))
        this->loop = EV_DEFAULT;
    else
        this->loop = ev_loop_new(0);
#endif

    xport_fd = xport->GetFd();

    if (xport_fd < 0) {
        SDK_TRACE_INFO("Cannot find valid socket to receive NCSI packets");
        free(PktData);
        return -1;
    }

    SDK_TRACE_INFO("xport->name() is: %s, fd: %d, this->loop: %p", xport->name().c_str(), xport_fd, this->loop);
    if (!(xport->name().compare("MCTP")))
        evutil_add_fd(EV_A_ xport_fd, CmdHndler::RecvNcsiMctpCtrlPkts, NULL, this);
    else {
#if 1
        evutil_add_fd(EV_A_ xport_fd, CmdHndler::RecvNcsiCtrlPkts, NULL, this);
#else
        if (!(xport->name().compare("RBT_oob_mnic0")))
            evutil_add_fd(EV_A_ xport_fd, CmdHndler::RecvNcsiCtrlPkts, NULL, this);
        else
            evutil_add_fd(EV_A_ xport_fd, CmdHndler::AltRecvNcsiCtrlPkts, NULL, this);
#endif
    }

    DellOemCmdHndlrInit();

    return 0;

}

void CmdHndler::RecvNcsiCtrlPkts(void *obj)
{
    int pkt_sz = 0;
    size_t ncsi_hdr_off = 0;

    CmdHndler *hndlr = (CmdHndler *)obj;

    //SDK_TRACE_INFO("%s: cmdHndlr: %p", __FUNCTION__, hndlr);
    pkt_sz = hndlr->xport->RecvPkt(hndlr->PktData, 1514, ncsi_hdr_off);
    if (pkt_sz < 0)
        return;

    hndlr->HandleCmd((hndlr->PktData + ncsi_hdr_off), (pkt_sz - ncsi_hdr_off));
}

void CmdHndler::AltRecvNcsiCtrlPkts(void *obj)
{
    int pkt_sz = 0;
    size_t ncsi_hdr_off = 0;

    CmdHndler *hndlr = (CmdHndler *)obj;

    SDK_TRACE_INFO("%s: cmdHndlr: %p", __FUNCTION__, hndlr);
    pkt_sz = hndlr->xport->RecvPkt(hndlr->PktData, 1514, ncsi_hdr_off);
    if (pkt_sz < 0)
        return;

    hndlr->HandleCmd((hndlr->PktData + ncsi_hdr_off), (pkt_sz - ncsi_hdr_off));
}

void CmdHndler::RecvNcsiMctpCtrlPkts(void *obj)
{
    ssize_t pkt_sz = 0;
    size_t ncsi_hdr_off = 0;

    CmdHndler *hndlr = (CmdHndler *)obj;

    pkt_sz = hndlr->xport->RecvPkt(hndlr->PktData, 1500, ncsi_hdr_off);
    if (pkt_sz < 0)
        return;

    hndlr->HandleCmd((hndlr->PktData + ncsi_hdr_off), (pkt_sz - ncsi_hdr_off));
}

int CmdHndler::SendNcsiCmdResponse(const void *buf, ssize_t sz)
{
    ssize_t ret;

    ret = xport->SendPkt(buf, sz);

    if (ret < 0) {
        SDK_TRACE_ERR("%s: sending cmd response failed with error code: %ld",
                __FUNCTION__, ret);
        return -1;
    }
    else {
        SDK_TRACE_DEBUG("%s: Response sent", __FUNCTION__);
        stats.tx_total_cnt++;
    }

    return 0;
}

int CmdHndler::ConfigVlanFilter(uint8_t filter_idx, uint16_t vlan,
        uint32_t port, bool enable)
{
    ssize_t ret = 0;
    VlanFilterMsg vlan_msg;

    SDK_TRACE_INFO("previous installed vlan: 0x%x, new vlan: 0x%x\n", 
                   vlan_filter_list[port][filter_idx], vlan);

    if (vlan_filter_list[port][filter_idx] == vlan)
    {
        SDK_TRACE_INFO("VLAN id 0x%x is already installed for filter_idx: %d"
                " Skipping filter install", vlan, filter_idx);

        return ret;
    }

    //Delete the previously configured vlan if its present
    if (vlan_filter_list[port][filter_idx]) {
        vlan_msg.filter_id = filter_idx;
        vlan_msg.port = port;
        vlan_msg.vlan_id = vlan_filter_list[port][filter_idx];
        vlan_msg.enable = false;

        ret = this->ipc->PostMsg(vlan_msg);

        if (ret) {
            SDK_TRACE_ERR("Vlan delete for filter_idx: %d , vlan_id: %d failed"
                    "with error code: %ld", filter_idx,
                    vlan_filter_list[port][filter_idx], ret);
            return ret;
        }
    }

    vlan_msg.filter_id = filter_idx;
    vlan_msg.port = port;
    vlan_msg.vlan_id = vlan;
    vlan_msg.enable = enable;

    ret = this->ipc->PostMsg(vlan_msg);

    if (!ret) {
        vlan_filter_list[port][filter_idx] = vlan;
        NcsiDb[vlan_msg.port].UpdateNcsiParam(vlan_msg);
    }
    else
        SDK_TRACE_ERR("Vlan update(%s) failed with error code: %ld",
                enable ? "Create" : "Remove", ret);

    return ret;
}

int CmdHndler::ConfigMacFilter(uint8_t filter_idx, const uint8_t* mac_addr,
        uint32_t port, uint8_t type, bool enable)
{
    ssize_t ret = 0;
    MacFilterMsg mac_filter_msg;

    if (!bcmp((uint8_t *)&mac_addr_list[port][filter_idx], mac_addr,
                sizeof(mac_filter_msg.mac_addr)))
    {
        SDK_TRACE_INFO("MAC addr %02x:%02x:%02x:%02x:%02x:%02x is already "
                       "installed for filter_idx: %d Skipping filter install",
                       mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
                       mac_addr[4], mac_addr[5], filter_idx);
        return ret;
    }

    //Delete the previously configured mac if its present
    if (mac_addr_list[port][filter_idx]) {
        mac_filter_msg.filter_id = filter_idx;
        mac_filter_msg.port = port;
        mac_filter_msg.addr_type = type;
        memcpy(mac_filter_msg.mac_addr, &mac_addr_list[port][filter_idx],
                sizeof(mac_filter_msg.mac_addr));
        mac_filter_msg.enable = false;

        ret = this->ipc->PostMsg(mac_filter_msg);

        if (ret) {
            SDK_TRACE_ERR("MAC delete for filter_idx: %d failed"
                    "with error code: %ld", filter_idx, ret);
            return ret;
        }
    }

    mac_filter_msg.filter_id = filter_idx;
    mac_filter_msg.port = port;
    memcpy(mac_filter_msg.mac_addr, mac_addr, sizeof(mac_filter_msg.mac_addr));
    mac_filter_msg.addr_type = type;
    mac_filter_msg.enable = enable;

    ret = this->ipc->PostMsg(mac_filter_msg);

    if (!ret) {
        memcpy(&mac_addr_list[port][filter_idx], mac_addr,
                sizeof(mac_filter_msg.mac_addr));
        NcsiDb[mac_filter_msg.port].UpdateNcsiParam(mac_filter_msg);
    }
    else
        SDK_TRACE_ERR("MAC update(%s) failed with error code: %ld",
                enable ? "Create" : "Remove", ret);

    return ret;
}

int CmdHndler::ConfigVlanMode(uint8_t vlan_mode, uint32_t port, bool enable)
{
    ssize_t ret;
    VlanModeMsg vlan_mode_msg;

    vlan_mode_msg.filter_id = port;
    vlan_mode_msg.port = port;
    vlan_mode_msg.mode = vlan_mode;
    vlan_mode_msg.enable = enable;

    NcsiDb[vlan_mode_msg.port].UpdateNcsiParam(vlan_mode_msg);

    ret = this->ipc->PostMsg(vlan_mode_msg);

    if (!ret)
        vlan_mode_list[port] = vlan_mode;

    return ret;
}
void CmdHndler::SetVlanFilter(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct SetVlanFilterCmdPkt *cmd = (SetVlanFilterCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    uint16_t vlan_id = (ntohs(cmd->vlan) & 0xFFFF);
    uint8_t filter_idx = (cmd->index);

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();

    SDK_TRACE_INFO("ncsi_channel: filter_idx: 0x%x, channel: 0x%x vlan_id: 0x%x, "
            "enable: 0x%x ", filter_idx, cmd->cmd.NcsiHdr.channel, vlan_id,
            cmd->enable & 0x1);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_SET_VLAN_FILTER);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    //Check vlan validity e.g. vlan can't be 0
    if (vlan_id) {
        ret = hndlr->ConfigVlanFilter(filter_idx, vlan_id,
                cmd->cmd.NcsiHdr.channel, (cmd->enable & 0x1) ? true:false);
        if (ret) {
            SDK_TRACE_ERR("Failed to set vlan filter");
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
        }
    }
    else
    {
        SDK_TRACE_ERR("vlan_id: 0 is not valid");
        resp->rsp.reason = htons(NCSI_REASON_INVLD_VLAN);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_SET_VLAN_FILTER);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::ClearInitState(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_CLEAR_INIT_STATE);

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

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_CLEAR_INIT_STATE);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::SelectPackage(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    uint8_t ncsi_channel = 0;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));
    resp->rsp.NcsiHdr.channel = 0;

    NCSI_CMD_BEGIN_BANNER();

    SDK_TRACE_INFO("ncsi_channel: 0x%x", ncsi_channel);

    sm_ret = StateM[ncsi_channel].UpdateState(CMD_SELECT_PACKAGE);

    if (sm_ret) {
        if (sm_ret == INVALID) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INVALID_CMD_ERR);
            SDK_TRACE_ERR("cmd: %p failed as its invalid cmd with "
                    "current ncsi state: 0x%x", cmd,
                    StateM[ncsi_channel].GetCurState());
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

    //TODO: Enable the filters which were applied on channel

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_SELECT_PACKAGE);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::DeselectPackage(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_DESELECT_PACKAGE);

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

    // disable both channel and disable chan tx for deselect package
    for (uint8_t ncsi_channel = 0; ncsi_channel< NCSI_CAP_CHANNEL_COUNT;
            ncsi_channel++) {
        hndlr->EnableChannelRx(ncsi_channel, false);
        hndlr->EnableChannelTx(ncsi_channel, false);
    }
error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DESELECT_PACKAGE);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

ssize_t CmdHndler::EnableFilters(uint8_t ncsi_chan)
{
    ssize_t ret;
    //struct EnableBcastFilterMsg bcast_filter_msg;
    //struct EnableGlobalMcastFilterMsg mcast_filter_msg;
    struct VlanFilterMsg vlan_filter_msg;
    struct VlanModeMsg vlan_mode_msg;
    struct MacFilterMsg mac_filter_msg;

    //TODO: enable bcast and mcast filters as well

    // enable vlan mode
    memset(&vlan_mode_msg, 0, sizeof(vlan_mode_msg));
    vlan_mode_msg.filter_id = ncsi_chan;
    vlan_mode_msg.port = ncsi_chan;
    vlan_mode_msg.mode = vlan_mode_list[ncsi_chan];

    ret = ipc->PostMsg(vlan_mode_msg);
    if (ret) {
        SDK_TRACE_ERR("IPC Failed to disable vlan mode on %d channel",
                ncsi_chan);
        return ret;
    }

    // enable vlan filters
    for (uint8_t idx=0; idx < NCSI_CAP_VLAN_FILTER_COUNT; idx++) {
        if (vlan_filter_list[ncsi_chan][idx]) {
            vlan_filter_msg.filter_id = (ncsi_chan * idx);
            vlan_filter_msg.port = ncsi_chan;
            vlan_filter_msg.vlan_id = vlan_filter_list[ncsi_chan][idx];
            vlan_filter_msg.enable = true;

            ret = ipc->PostMsg(vlan_filter_msg);
            if (ret) {
                SDK_TRACE_ERR("IPC Failed to enable vlan filters on %d channel"
                        , ncsi_chan);
                return ret;
            }
        }
    }

    //enable mac filters
    for (uint8_t idx=0; idx < NCSI_CAP_MIXED_MAC_FILTER_COUNT; idx++) {
        if (mac_addr_list[ncsi_chan][idx]) {
            mac_filter_msg.filter_id = (ncsi_chan * idx);
            mac_filter_msg.port = ncsi_chan;
            memcpy(mac_filter_msg.mac_addr, &mac_addr_list[ncsi_chan][idx],
                    sizeof(mac_filter_msg.mac_addr));
            mac_filter_msg.enable = true;

            ret = ipc->PostMsg(mac_filter_msg);
            if (ret) {
                SDK_TRACE_ERR("IPC Failed to enable mac filters on %d channel",
                        ncsi_chan);
                return ret;
            }
        }
    }

    return ret;
}

ssize_t CmdHndler::DisableFilters(uint8_t ncsi_chan)
{
    ssize_t ret;
    struct EnableBcastFilterMsg bcast_filter_msg;
    struct EnableGlobalMcastFilterMsg mcast_filter_msg;
    struct VlanFilterMsg vlan_filter_msg;
    struct VlanModeMsg vlan_mode_msg;
    struct MacFilterMsg mac_filter_msg;

    //disable bcast filters
    memset(&bcast_filter_msg, 0, sizeof(bcast_filter_msg));
    bcast_filter_msg.filter_id = ncsi_chan;
    bcast_filter_msg.port = ncsi_chan;

    ret = ipc->PostMsg(bcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("IPC Failed to disable bcast filters on %d channel",
                ncsi_chan);
        return ret;
    }

    //disable mcast filters
    memset(&mcast_filter_msg, 0, sizeof(mcast_filter_msg));
    mcast_filter_msg.filter_id = ncsi_chan;
    mcast_filter_msg.port = ncsi_chan;

    ret = ipc->PostMsg(mcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("IPC Failed to disable mcast filters on %d channel",
                ncsi_chan);
        return ret;
    }

    // disable vlan mode
    memset(&vlan_mode_msg, 0, sizeof(vlan_mode_msg));
    vlan_mode_msg.filter_id = ncsi_chan;
    vlan_mode_msg.port = ncsi_chan;
    vlan_mode_msg.mode = 0;

    ret = ipc->PostMsg(vlan_mode_msg);
    if (ret) {
        SDK_TRACE_ERR("IPC Failed to disable vlan mode on %d channel",
                ncsi_chan);
        return ret;
    }

    // disable vlan filters
    for (uint8_t idx=0; idx < NCSI_CAP_VLAN_FILTER_COUNT; idx++) {
        if (vlan_filter_list[ncsi_chan][idx]) {
            vlan_filter_msg.filter_id = (ncsi_chan * idx);
            vlan_filter_msg.port = ncsi_chan;
            vlan_filter_msg.vlan_id = vlan_filter_list[ncsi_chan][idx];
            vlan_filter_msg.enable = false;

            ret = ipc->PostMsg(vlan_filter_msg);
            if (ret) {
                SDK_TRACE_ERR("IPC Failed to disable vlan filters on %d channel"
                        , ncsi_chan);
                return ret;
            }
        }
    }

    //disable mac filters
    for (uint8_t idx=0; idx < NCSI_CAP_MIXED_MAC_FILTER_COUNT; idx++) {
        if (mac_addr_list[ncsi_chan][idx]) {
            mac_filter_msg.filter_id = (ncsi_chan * idx);
            mac_filter_msg.port = ncsi_chan;
            memcpy(mac_filter_msg.mac_addr, &mac_addr_list[ncsi_chan][idx],
                    sizeof(mac_filter_msg.mac_addr));
            mac_filter_msg.enable = false;

            ret = ipc->PostMsg(mac_filter_msg);
            if (ret) {
                SDK_TRACE_ERR("IPC Failed to disable mac filters on %d channel"
                        , ncsi_chan);
                return ret;
            }
        }
    }

    return ret;
}

ssize_t CmdHndler::EnableChannelRx(uint8_t ncsi_chan, bool enable)
{
    ssize_t ret;
    struct EnableChanMsg enable_ch_msg;

#if 0
    if (enable)
        EnableFilters(ncsi_chan);
    else
        DisableFilters(ncsi_chan);
#endif

    enable_ch_msg.enable = enable;
    enable_ch_msg.port = ncsi_chan;
    enable_ch_msg.filter_id = ncsi_chan;

    NcsiDb[enable_ch_msg.port].UpdateNcsiParam(enable_ch_msg);

    ret = ipc->PostMsg(enable_ch_msg);
    if (ret)
        SDK_TRACE_ERR("IPC Failed to enable/disable channel");

    return ret;
}

void CmdHndler::EnableChan(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    const uint8_t *buf = (uint8_t *)cmd_pkt;
    uint8_t opcode = buf[NCSI_CMD_OPCODE_OFFSET];
    bool enable;

    if (opcode == CMD_EN_CHAN)
        enable = true;
    else if (CMD_DIS_CHAN)
        enable = false;
    else
        SDK_TRACE_ERR("Invalid opcode: 0x%x\n", opcode);

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x, enable: 0x%x",
            cmd->cmd.NcsiHdr.channel, enable);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(
            enable ? CMD_EN_CHAN : CMD_DIS_CHAN);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    if (hndlr->EnableChannelRx(cmd->cmd.NcsiHdr.channel, enable)) {
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
        SDK_TRACE_ERR("cmd: %p failed due to internal err in ipc", cmd);
    }

error_out:
    if (enable)
        resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_EN_CHAN);
    else
        resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DIS_CHAN);

    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::ResetChan(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    struct ResetChanMsg reset_ch_msg;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct ResetChanCmdPkt *cmd = (ResetChanCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_RESET_CHAN);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    // first disable Rx and Tx for this channel
    hndlr->EnableChannelRx(cmd->cmd.NcsiHdr.channel, false);
    hndlr->EnableChannelTx(cmd->cmd.NcsiHdr.channel, false);

    // reset the channel
    reset_ch_msg.reset = true;
    reset_ch_msg.port = cmd->cmd.NcsiHdr.channel;
    reset_ch_msg.filter_id = cmd->cmd.NcsiHdr.channel;

    NcsiDb[reset_ch_msg.port].UpdateNcsiParam(reset_ch_msg);

    ret = hndlr->ipc->PostMsg(reset_ch_msg);
    if (ret) {
        SDK_TRACE_ERR("Failed to reset channel");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

    // zero out the local database of filters
    if (!resp->rsp.code) {
        memset(mac_addr_list, 0, sizeof(mac_addr_list));
        memset(vlan_filter_list, 0, sizeof(vlan_filter_list));
        memset(vlan_mode_list, 0, sizeof(vlan_mode_list));
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_RESET_CHAN);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;

}

ssize_t CmdHndler::EnableChannelTx(uint8_t ncsi_chan, bool enable)
{
    ssize_t ret;
    struct EnableChanTxMsg enable_ch_tx_msg;

    enable_ch_tx_msg.enable = enable;
    enable_ch_tx_msg.port = ncsi_chan;
    enable_ch_tx_msg.filter_id = ncsi_chan;

    NcsiDb[enable_ch_tx_msg.port].UpdateNcsiParam(enable_ch_tx_msg);

    ret = ipc->PostMsg(enable_ch_tx_msg);
    if (ret)
        SDK_TRACE_ERR("IPC Failed to enable/disable channel");

    return ret;
}
void CmdHndler::EnableChanNwTx(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    const uint8_t *buf = (uint8_t *)cmd_pkt;
    uint8_t opcode = buf[NCSI_CMD_OPCODE_OFFSET];
    bool enable;

    if (opcode == CMD_EN_CHAN_NW_TX)
        enable = true;
    else if (CMD_DIS_CHAN_NW_TX)
        enable = false;
    else
        SDK_TRACE_ERR("Invalid opcode: 0x%x\n", opcode);

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x, enable: 0x%x",
            cmd->cmd.NcsiHdr.channel, enable);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(
            enable ? CMD_EN_CHAN_NW_TX : CMD_DIS_CHAN_NW_TX);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    if (hndlr->EnableChannelTx(cmd->cmd.NcsiHdr.channel, enable)) {
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
        SDK_TRACE_ERR("cmd: %p failed due to internal err in ipc", cmd);
    }

error_out:
    if (enable)
        resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_EN_CHAN_NW_TX);
    else
        resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DIS_CHAN_NW_TX);

    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::SetLink(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    struct SetLinkMsg set_link_msg;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct SetLinkCmdPkt *cmd = (SetLinkCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    uint32_t oem_field_valid = (cmd->mode) & (1 << 11);
    uint8_t set_link = cmd->oem_mode & 0x1;

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x oem_field_valid: 0x%x, set_link: 0x%x ",
            cmd->cmd.NcsiHdr.channel, oem_field_valid, set_link);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_SET_LINK);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    if (oem_field_valid) {
        set_link_msg.link_up = set_link;
        set_link_msg.port = cmd->cmd.NcsiHdr.channel;
        set_link_msg.filter_id = cmd->cmd.NcsiHdr.channel;

        NcsiDb[set_link_msg.port].UpdateNcsiParam(set_link_msg);

        ret = hndlr->ipc->PostMsg(set_link_msg);
        if (ret) {
            SDK_TRACE_ERR("Failed to set link");
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
        }
    }
    else
    {
        SDK_TRACE_ERR("Only OEM specific link settings are allowed");
        resp->rsp.reason = htons(NCSI_REASON_INVALID_PARAM);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_SET_LINK);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetLinkStatus(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    bool link_status;
    uint32_t status;
    uint8_t link_speed;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetLinkStatusRespPkt *resp = (struct GetLinkStatusRespPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetLinkStatusRespPkt));

    if ((hndlr->ipc->GetLinkStatus(cmd->cmd.NcsiHdr.channel, link_status,
            link_speed))) {

        SDK_TRACE_ERR("Failed to read link status. Responding with last saved"
                "link status: 0x%x",last_link_status[cmd->cmd.NcsiHdr.channel]);
        status = last_link_status[cmd->cmd.NcsiHdr.channel];
    }
    else {
        status = ((link_status ? 1:0) | (link_speed << 1) | (link_speed << 24) |
            (0x3 << 5)/* autoneg */ | (1 << 20) /* serdes used */ );

        /* save the latest status on last_link_status */
        last_link_status[cmd->cmd.NcsiHdr.channel] = status;
    }

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    //NCSI_CMD_BEGIN_BANNER();
    resp->status = htonl(status);
    SDK_TRACE_INFO("ncsi_channel: 0x%x, link_status: 0x%x",
            cmd->cmd.NcsiHdr.channel, ntohl(resp->status));

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_LINK_STATUS);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_LINK_STATUS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_LINK_STATUS_RSP_PAYLOAD_LEN);

    //NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::EnableVlan(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct EnVlanCmdPkt *cmd = (EnVlanCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    uint32_t vlan_mode = cmd->mode;

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x vlan_mode: 0x%x",
            cmd->cmd.NcsiHdr.channel, vlan_mode);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_EN_VLAN);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    //Check vlan mode validity. we support only vlan mode 1 and 2
    if (vlan_mode > 0 && vlan_mode < 3) {
        ret = hndlr->ConfigVlanMode(vlan_mode, cmd->cmd.NcsiHdr.channel, true);
        if (ret) {
            SDK_TRACE_ERR("Failed to set vlan Mode");
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            goto error_out;
        }
    }
    else
    {
        SDK_TRACE_ERR("vlan_mode: 0x%x is not supported", vlan_mode);
        resp->rsp.reason = htons(NCSI_REASON_INVALID_PARAM);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);

        goto error_out;
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_EN_VLAN);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::DisableVlan(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_DIS_VLAN);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    //Check vlan mode validity. we support only vlan mode 1 and 2
    ret = hndlr->ConfigVlanMode( /*don't care*/ 0, cmd->cmd.NcsiHdr.channel,
            false);
    if (ret) {
        SDK_TRACE_ERR("Failed to set vlan Mode");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DIS_VLAN);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::SetMacAddr(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct SetMacAddrCmdPkt *cmd = (SetMacAddrCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));
    uint8_t mac_addr_type = ((cmd->at_e & 0xE0) >> 5);
    uint64_t mac_addr = *((uint64_t *)cmd->mac);
    uint8_t filter_idx = cmd->index;
    bool enable = (cmd->at_e & 0x1) ? true:false;
    uint8_t mac_filter_num = cmd->index;

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();

    SDK_TRACE_INFO("ncsi_channel: 0x%x, mac_addr: %02x:%02x:%02x:%02x:%02x:%02x, "
            "filter_index: 0x%x, mac_addr_type: 0x%x, enable: 0x%x ",
            cmd->cmd.NcsiHdr.channel, cmd->mac[0], cmd->mac[1], cmd->mac[2],
            cmd->mac[3], cmd->mac[4], cmd->mac[5], filter_idx, mac_addr_type,
            enable);

    //valid mac address type are 0(Ucast) & 1(Mcast) only
    if (mac_addr_type > 1) {
        SDK_TRACE_ERR("mac_addr_type: 0x%x is not valid", mac_addr_type);
        resp->rsp.reason = htons(NCSI_REASON_INVLD_MAC_ADDR);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
    }

    if(mac_filter_num > 8) {
        SDK_TRACE_ERR("mac_filter_num: 0x%x is not out of range",
                mac_filter_num);
        resp->rsp.reason = htons(NCSI_REASON_INVLD_MAC_ADDR);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
    }

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_SET_MAC_ADDR);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    //Check mac addr validity e.g. mac addr can't be 0
    if (mac_addr) {
#if 0
        if (mac_addr_list[cmd->cmd.NcsiHdr.channel][mac_filter_num]) {

            ret = hndlr->ConfigMacFilter(mac_addr_list[cmd->cmd.NcsiHdr.channel][mac_filter_num],
                    cmd->cmd.NcsiHdr.channel, mac_addr_type, false);

            if (ret) {
                SDK_TRACE_ERR("Failed to remove old mac filter");
                resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
                resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);

                goto error_out;
            }
        }
#endif
        ret = hndlr->ConfigMacFilter(filter_idx, cmd->mac, cmd->cmd.NcsiHdr.channel,
                mac_addr_type, enable);

        if (ret) {
            SDK_TRACE_ERR("Failed to set mac filter");
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
            goto error_out;
        }
        else {
            mac_addr_list[cmd->cmd.NcsiHdr.channel][mac_filter_num] = mac_addr;
        }
    }
    else
    {
        SDK_TRACE_ERR("All zero mac_addr is not supported");
        resp->rsp.reason = htons(NCSI_REASON_INVLD_MAC_ADDR);
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_SET_MAC_ADDR);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::EnableBcastFilter(void *obj, const void *cmd_pkt,
		ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    struct EnableBcastFilterMsg bcast_filter_msg;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    const struct EnBcastFilterCmdPkt *cmd = (EnBcastFilterCmdPkt *)cmd_pkt;

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_EN_BCAST_FILTER);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    bcast_filter_msg.filter_id = cmd->cmd.NcsiHdr.channel;
    bcast_filter_msg.port = cmd->cmd.NcsiHdr.channel;
    bcast_filter_msg.enable_arp = !!(ntohl(cmd->mode) & NCSI_CAP_BCAST_FILTER_ARP);
    bcast_filter_msg.enable_dhcp_client = !!(ntohl(cmd->mode) & NCSI_CAP_BCAST_FILTER_DHCP_CLIENT);
    bcast_filter_msg.enable_dhcp_server = !!(ntohl(cmd->mode) & NCSI_CAP_BCAST_FILTER_DHCP_SERVER);
    bcast_filter_msg.enable_netbios = !!(ntohl(cmd->mode) & NCSI_CAP_BCAST_FILTER_NETBIOS);

    NcsiDb[bcast_filter_msg.port].UpdateNcsiParam(bcast_filter_msg);

    ret = hndlr->ipc->PostMsg(bcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("Failed to program bcast filters");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_EN_BCAST_FILTER);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::DisableBcastFilter(void *obj, const void *cmd_pkt,
		ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    EnableBcastFilterMsg bcast_filter_msg;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_DIS_BCAST_FILTER);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    bcast_filter_msg.filter_id = cmd->cmd.NcsiHdr.channel;
    bcast_filter_msg.port = cmd->cmd.NcsiHdr.channel;
    bcast_filter_msg.enable_arp = false;
    bcast_filter_msg.enable_dhcp_client = false;
    bcast_filter_msg.enable_dhcp_server = false;
    bcast_filter_msg.enable_netbios = false;

    NcsiDb[bcast_filter_msg.port].UpdateNcsiParam(bcast_filter_msg);

    ret = hndlr->ipc->PostMsg(bcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("Failed to disable bcast filters");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DIS_BCAST_FILTER);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::EnableGlobalMcastFilter(void *obj, const void *cmd_pkt,
		ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    struct EnableGlobalMcastFilterMsg mcast_filter_msg;
    const struct EnGlobalMcastFilterCmdPkt *cmd =
        (EnGlobalMcastFilterCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_EN_GLOBAL_MCAST_FILTER);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    mcast_filter_msg.filter_id = cmd->cmd.NcsiHdr.channel;
    mcast_filter_msg.port = cmd->cmd.NcsiHdr.channel;
    mcast_filter_msg.enable_ipv6_neigh_adv = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_IPV6_NEIGH_ADV);
    mcast_filter_msg.enable_ipv6_router_adv = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_IPV6_ROUTER_ADV);
    mcast_filter_msg.enable_dhcpv6_relay = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_DHCPV6_RELAY);
    mcast_filter_msg.enable_dhcpv6_mcast = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_DHCPV6_MCAST_SERVER_TO_CLIENT);
    mcast_filter_msg.enable_ipv6_mld = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_IPV6_MLD);
    mcast_filter_msg.enable_ipv6_neigh_sol = !!(ntohl(cmd->mode) & NCSI_CAP_MCAST_IPV6_NEIGH_SOL);

    NcsiDb[mcast_filter_msg.port].UpdateNcsiParam(mcast_filter_msg);

    ret = hndlr->ipc->PostMsg(mcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("Failed to program mcast filters");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_EN_GLOBAL_MCAST_FILTER);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::DisableGlobalMcastFilter(void *obj, const void *cmd_pkt,
		ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    struct EnableGlobalMcastFilterMsg mcast_filter_msg;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_DIS_GLOBAL_MCAST_FILTER);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    mcast_filter_msg.filter_id = cmd->cmd.NcsiHdr.channel;
    mcast_filter_msg.port = cmd->cmd.NcsiHdr.channel;
    mcast_filter_msg.enable_ipv6_neigh_adv = false;
    mcast_filter_msg.enable_ipv6_router_adv = false;
    mcast_filter_msg.enable_dhcpv6_relay = false;
    mcast_filter_msg.enable_dhcpv6_mcast = false;
    mcast_filter_msg.enable_ipv6_mld = false;
    mcast_filter_msg.enable_ipv6_neigh_sol = false;

    NcsiDb[mcast_filter_msg.port].UpdateNcsiParam(mcast_filter_msg);

    ret = hndlr->ipc->PostMsg(mcast_filter_msg);
    if (ret) {
        SDK_TRACE_ERR("Failed to dsiable mcast filters");
        resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
        resp->rsp.reason = htons(NCSI_REASON_INTERNAL_ERR);
    }

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_DIS_GLOBAL_MCAST_FILTER);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetVersionId(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;

    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetVersionIdRespPkt *resp = (struct GetVersionIdRespPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetVersionIdRespPkt));
    memcpy(resp, &get_version_resp, sizeof(*resp));
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    //FIXME: As of now we are ignoring the ncsi state machine for this cmd

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_VER_ID);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_VER_ID_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetCapabilities(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetCapRespPkt *resp = (struct GetCapRespPkt *)hndlr->xport->GetNcsiRspPkt(sizeof(struct GetCapRespPkt));

    memcpy(resp, &get_cap_resp, sizeof(*resp));
    memset(&resp->rsp, 0, sizeof(resp->rsp));
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_CAP);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_CAP);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_CAP_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetParams(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetParamRespPkt *resp = (struct GetParamRespPkt *)hndlr->xport->GetNcsiRspPkt(sizeof (struct GetParamRespPkt));
    NcsiDb[cmd->cmd.NcsiHdr.channel].GetNcsiParamRespPacket(resp);

    memset(&resp->rsp, 0, sizeof(resp->rsp));
    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_PARAMS);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_PARAMS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_PARAM_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetNicPktStats(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetNicStatsRespPkt *resp = (struct GetNicStatsRespPkt*) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetNicStatsRespPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_NIC_STATS);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    hndlr->GetMacStats(cmd->cmd.NcsiHdr.channel, resp);

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_NIC_STATS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_NIC_STATS_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetNcsiStats(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetNCSIStatsRespPkt *resp = (struct GetNCSIStatsRespPkt*) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetNCSIStatsRespPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_NCSI_STATS);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    resp->rx_cmds = htonl(hndlr->stats.valid_cmd_rx_cnt);
    resp->dropped_cmds = htonl(hndlr->stats.rx_drop_cnt);
    resp->cmd_type_errs = htonl(hndlr->stats.unsup_cmd_rx_cnt);
    resp->cmd_csum_errs = htonl(hndlr->stats.invalid_chksum_rx_cnt);
    resp->rx_pkts = htonl(hndlr->stats.rx_total_cnt);

    /* include this response as part os tx stats counter*/
    resp->tx_pkts = htonl(hndlr->stats.tx_total_cnt + 1);

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_NCSI_STATS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_NCSI_STATS_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetNcsiPassthruStats(void *obj, const void *cmd_pkt,
		ssize_t cmd_sz)
{
    ssize_t ret;
    NcsiStateErr sm_ret;
    struct port_stats p_stats;
    struct mgmt_port_stats *mgmt_p_stats = (struct mgmt_port_stats *) &p_stats;
    uint64_t passthru_tx_pkts;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetPassThruStatsRespPkt *resp = (struct GetPassThruStatsRespPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetPassThruStatsRespPkt));
    uint32_t port = 0x11030001; //BX port in capri

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    sm_ret = StateM[cmd->cmd.NcsiHdr.channel].UpdateState(CMD_GET_NCSI_PASSTHRU_STATS);

    if (sm_ret) {
        if (sm_ret == INIT_REQRD) {
            resp->rsp.code = htons(NCSI_RSP_COMMAND_FAILED);
            resp->rsp.reason = htons(NCSI_REASON_INTF_INIT_REQRD);
            SDK_TRACE_ERR("cmd: %p failed as channel is not initialized", cmd);
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

    //FIXME: Need to use some macro instead of hard coded 9 here for oob port
    hndlr->ReadMacStats(port, p_stats);

    passthru_tx_pkts = mgmt_p_stats->frames_tx_all - hndlr->stats.tx_total_cnt;
    memcpy(&resp->tx_pkts, memrev((uint8_t *) &passthru_tx_pkts, sizeof(uint64_t)), sizeof(uint64_t));
    resp->tx_dropped = htonl((uint32_t)mgmt_p_stats->frames_tx_bad);
    resp->tx_us_err = 0;
    resp->rx_pkts = htonl((uint32_t)mgmt_p_stats->frames_rx_all);
    resp->rx_dropped = htonl((uint32_t)mgmt_p_stats->frames_rx_bad_all);
    resp->rx_us_err = htonl((uint32_t)mgmt_p_stats->frames_rx_undersized);
    resp->rx_os_err = htonl((uint32_t)mgmt_p_stats->frames_rx_oversized);

    resp->tx_channel_err = 0; //FIXME
    resp->tx_os_err = 0; //FIXME
    resp->rx_channel_err = 0;//FIXME

    SDK_TRACE_DEBUG("Response for GetNcsiPassthruStats:");
    SDK_TRACE_DEBUG("tx_pkts (BE)    :%llu",resp->tx_pkts       );
    SDK_TRACE_DEBUG("tx_dropped      :%d",ntohl(resp->tx_dropped    ));
    SDK_TRACE_DEBUG("tx_channel_err  :%d",ntohl(resp->tx_channel_err));
    SDK_TRACE_DEBUG("tx_us_err       :%d",ntohl(resp->tx_us_err     ));
    SDK_TRACE_DEBUG("tx_os_err       :%d",ntohl(resp->tx_os_err     ));
    SDK_TRACE_DEBUG("rx_pkts         :%d",ntohl(resp->rx_pkts       ));
    SDK_TRACE_DEBUG("rx_dropped      :%d",ntohl(resp->rx_dropped    ));
    SDK_TRACE_DEBUG("rx_channel_err  :%d",ntohl(resp->rx_channel_err));
    SDK_TRACE_DEBUG("rx_us_err       :%d",ntohl(resp->rx_us_err     ));
    SDK_TRACE_DEBUG("rx_os_err       :%d",ntohl(resp->rx_os_err     ));

error_out:
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_NCSI_PASSTHRU_STATS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_PASSTHRU_STATS_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetPackageStatus(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetPkgStatusRespPkt *resp = (struct GetPkgStatusRespPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetPkgStatusRespPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    //FIXME: As of now ignoring the state machine for this cmd

    /* send all 0s in response as we don't suppport HW arb in package */
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_PACKAGE_STATUS);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_PKG_STATUS_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::GetPackageUUID(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    ssize_t ret;
    std::string serial_num;
    CmdHndler *hndlr = (CmdHndler *)obj;
    const struct NcsiFixedCmdPkt *cmd = (NcsiFixedCmdPkt *)cmd_pkt;
    struct GetPkgUUIDRespPkt *resp = (struct GetPkgUUIDRespPkt *) hndlr->xport->GetNcsiRspPkt(sizeof(struct GetPkgUUIDRespPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    NCSI_CMD_BEGIN_BANNER();
    SDK_TRACE_INFO("ncsi_channel: 0x%x", cmd->cmd.NcsiHdr.channel);

    //FIXME: As of now ignoring the state machine for this cmd

    //TODO: Implement the logic here
    sdk::platform::readfrukey(BOARD_SERIALNUMBER_KEY, serial_num);
    strncpy((char*)resp->uuid, serial_num.c_str(), sizeof(resp->uuid));
    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_GET_PACKAGE_UUID);
    resp->rsp.NcsiHdr.length = htons(NCSI_GET_PKG_UUID_RSP_PAYLOAD_LEN);

    NCSI_CMD_END_BANNER();

    hndlr->SendNcsiCmdResponse(resp, sizeof(*resp));

    return;
}

void CmdHndler::HandleOEMCmd(void *obj, const void *cmd_pkt, ssize_t cmd_sz)
{
    const struct OEMCmdPkt *cmd = (struct OEMCmdPkt *)cmd_pkt;
    const uint8_t *pkt = (const uint8_t *)cmd_pkt;
    CmdHndler *hndlr = (CmdHndler *)obj;

    if (ntohl(cmd->mfg_id) == NCSI_OEM_MFG_ID_DELL)
        DellOEMCmdHndlr(obj, cmd_pkt, cmd_sz);
    else
    {
        SDK_TRACE_ERR("NCSI OEM commands are not supported for MFG_ID: 0x%x", cmd->mfg_id);
        hndlr->SendNcsiUnSupCmdResponse(cmd_pkt);
    }

    return;
}

void CmdHndler::SendNcsiUnSupCmdResponse(const void *cmd_pkt)
{
    struct NcsiFixedCmdPkt *cmd = (struct NcsiFixedCmdPkt *)cmd_pkt;
    struct NcsiRspPkt *resp = (struct NcsiRspPkt *) xport->GetNcsiRspPkt(sizeof(struct NcsiRspPkt));

    memcpy(&resp->rsp.NcsiHdr, &cmd->cmd.NcsiHdr, sizeof(resp->rsp.NcsiHdr));

    resp->rsp.code = htons(NCSI_RSP_COMMAND_UNSUPPORTED);
    resp->rsp.reason = htons(NCSI_REASON_UNKNOWN_CMD);

    resp->rsp.NcsiHdr.type = ncsi_cmd_resp_opcode(CMD_OEM_COMMAND);
    resp->rsp.NcsiHdr.length = htons(NCSI_FIXED_RSP_PAYLOAD_LEN);

    SendNcsiCmdResponse(resp, sizeof(*resp));
}

int CmdHndler::ValidateCmdPkt(const void *pkt, ssize_t sz)
{
    const uint8_t *buf = (uint8_t *)pkt;

    if ( !buf || !sz ) {
        SDK_TRACE_ERR("Zero sized packets cannot be validated");
        return -1;
    }

    /* 802.3 frame size restriction is for NCSI over RBT only */
    if (!(xport->name().compare("RBT")))
    {
        if (sz < MIN_802_3_FRAME_PAYLOAD_SZ) {
            SDK_TRACE_ERR("NCSI packet size (%ld) is less than min frame payload"
                    "size for 802.3", sz);
            stats.rx_drop_cnt++;
        }
    }

    if (buf[NCSI_HDR_REV_OFFSET] != SUPPORTED_NCSI_REV) {
        SDK_TRACE_ERR("NCSI Header Rev %d not supported. Expected Rev is %d",
                buf[NCSI_HDR_REV_OFFSET], SUPPORTED_NCSI_REV);
        stats.rx_drop_cnt++;
    }

    stats.rx_total_cnt++;
    stats.valid_cmd_rx_cnt++;

    return 0;
}

int CmdHndler::HandleCmd(const void* pkt, ssize_t sz)
{
    int ret = 0;
    const uint8_t *buf = (uint8_t *)pkt;
    void *rsp = NULL;
    ssize_t rsp_sz = 0;
    uint8_t opcode;

    ret = ValidateCmdPkt(pkt, sz);

    if (ret) {
        SDK_TRACE_ERR("Received malformed or invalid NCSI command packet: ret: %d "
                , ret);
        return ret;
    }

    opcode = buf[NCSI_CMD_OPCODE_OFFSET];

    if (CmdTable[opcode]) {
        SDK_TRACE_DEBUG("Handling Ncsi command: 0x%x ", opcode);
        CmdTable[opcode](this, pkt, sz);

        if (rsp && rsp_sz) {
            SDK_TRACE_INFO("Sending Ncsi Response for cmd: 0x%x ", opcode);

        }
    }
    else {
        SDK_TRACE_ERR("Ncsi command 0x%x is not supported",
                buf[NCSI_CMD_OPCODE_OFFSET]);
        //TODO: Send the NCSI response saying unsupported command
        stats.unsup_cmd_rx_cnt++;
    }

    return 0;
}

} // namespace ncsi
} // namespace platform
} // namespace sdk

