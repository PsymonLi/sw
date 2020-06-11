#include "Config.h"
#include "NetCfg.h"
#include <fstream>
#include "EvtLogHelper.h"
#include "WMIhelper.h"

int
_cdecl
wmain(int argc, wchar_t* argv[])
{
    command_info info;
    bool cmd_found = false;

	std::cout.sync_with_stdio();

    info.cmd = CmdUsage();
    info.cmds.push_back(CmdVersion());
    info.cmds.push_back(CmdSetTrace());
    info.cmds.push_back(CmdGetTrace());
    info.cmds.push_back(CmdPort());
    info.cmds.push_back(CmdTxMode());
    info.cmds.push_back(CmdTxBudget());
    info.cmds.push_back(CmdRxBudget());
    info.cmds.push_back(CmdRxMiniBudget());
    info.cmds.push_back(CmdDevStats());
    info.cmds.push_back(CmdLifStats());
    info.cmds.push_back(CmdPortStats());
    info.cmds.push_back(CmdPerfStats());
    info.cmds.push_back(CmdRegKeys());
    info.cmds.push_back(CmdInfo());
    info.cmds.push_back(CmdQueueInfo());
    info.cmds.push_back(CmdCollectDbgInfo());
    //info.cmds.push_back(CmdOidStats());
    //info.cmds.push_back(CmdFwcmdStats());

    po::command_line_style::style_t style =
        (po::command_line_style::style_t)
        (po::command_line_style::allow_short |
         po::command_line_style::allow_dash_for_short |
         po::command_line_style::allow_slash_for_short |
         po::command_line_style::short_allow_next |
         po::command_line_style::allow_long |
         po::command_line_style::allow_long_disguise |
         po::command_line_style::long_allow_adjacent |
         po::command_line_style::long_allow_next |
         po::command_line_style::case_insensitive);

    // Parse top-level options
    try {
        po::options_description opts = info.cmd.opts(true);
        po::wparsed_options parsed = po::wcommand_line_parser(argc, argv)
            .allow_unregistered()
            .options(opts)
            .style(style)
            .run();
        po::store(parsed, info.vm);
        info.cmd_args = po::collect_unrecognized(parsed.options, po::include_positional);
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        info.status = 1;
        info.usage = 1;
    }

    if (info.vm.count("Help") != 0) {
        info.usage = 1;
    }
    info.hidden = info.vm.count("Hidden");
    info.dryrun = info.vm.count("DryRun");

    // Lookup subcommand
    if (!info.cmd_args.empty()) {
        info.cmd_name = to_bytes(info.cmd_args.front());
        info.cmd_args.erase(info.cmd_args.begin());

        // Remove prefix - -- or / if present
        std::string cmd_name = info.cmd_name;
        if (cmd_name[0] == '-') {
            if (cmd_name[1] == '-') {
                cmd_name.erase(0, 2);
            }
            else {
                cmd_name.erase(0, 1);
            }
        }
        else if (cmd_name[0] == '/') {
            cmd_name.erase(0, 1);
        }

        // Find in all commands, case insensitive
        for (auto& cmd_i : info.cmds) {
            if (boost::iequals(cmd_name, cmd_i.name)) {
                info.cmd = cmd_i;
                cmd_found = true;
            }
        }
    }

    // Parse subcommand options
    if (cmd_found && !info.usage) {
        try {
            po::options_description opts = info.cmd.opts(true);
            po::positional_options_description pos = info.cmd.pos();
            po::wparsed_options parsed = po::wcommand_line_parser(info.cmd_args)
                .options(opts)
                .positional(pos)
                .style(style)
                .run();
            po::store(parsed, info.vm);
            po::notify(info.vm);
            info.cmd_args = po::collect_unrecognized(parsed.options, po::include_positional);
        }
        catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            info.status = 1;
            info.usage = true;
        }
    }

    try {
        return info.cmd.run(info);
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
}

//
// Top-level options and list of commands
//

static
po::options_description
CmdUsageOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] [Command options...]");

    opts.add_options()
        ("Help,?,h", optype_flag(), "Display command specific usage");

    if (hidden) {
        opts.add_options()
            ("Hidden", optype_flag(), "Display hidden option in usage")
            ("DryRun", optype_flag(), "Demo output without running command");
    }

    return opts;
}

static
int
CmdUsageRun(command_info& info)
{
    if (!info.cmd_name.empty()) {
        std::cout << "unknown command: " << info.cmd_name << std::endl;
        info.status = 1;
    }

    std::cout << info.cmd.opts(info.hidden) << "Commands:" << std::endl;

    for (auto& cmd_i : info.cmds) {
        if (!cmd_i.hidden || info.hidden) {
            std::cout << "  " << std::left << std::setw(20)
                << cmd_i.name << "  " << cmd_i.desc << std::endl;
        }
    }

    return info.status;
}

command
CmdUsage()
{
    command cmd;

    cmd.name = "Help";
    cmd.desc = "Print usage";
    cmd.hidden = true;
    cmd.opts = CmdUsageOpts;
    cmd.run = CmdUsageRun;

    return cmd;
}

static
po::options_description
CmdVersionOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe Version");

    return opts;
}

#include "version.h"

static
int
CmdVersionRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    std::string VerStr = IONICCONFIG_VERSION_STRING "-" IONICCONFIG_VERSION_EXTENSION;

    std::cout << std::endl;
    std::cout << "Name:\t\t" << IONICCONFIG_TOOL_NAME << std::endl;
    std::cout << "Description:\t" << IONICCONFIG_PRODUCT_NAME << std::endl;
    std::cout << "Version:\t" << VerStr << std::endl;
    std::cout << std::endl;

    return 0;
}

command
CmdVersion()
{
    command cmd;

    cmd.name = "Version";
    cmd.desc = "Print IonicConfig.exe version";
    cmd.opts = CmdVersionOpts;
    cmd.run = CmdVersionRun;

    return cmd;
}


//
// TODO: Cleanup, move to Stats.cpp
//

#include "stdio.h"
#include <shlwapi.h>
#include <winioctl.h>

#include "UserCommon.h"

#define PRINT_BUFFER_SIZE   128

char *
GetLifTypeName(ULONG type);

void
DumpTxRingStats(const char *id, struct dev_tx_ring_stats *tx_stats, std::ostream& outfile)
{
    char TempBuf[PRINT_BUFFER_SIZE];

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_full:\t%I64u\n", id, tx_stats->full); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_wake:\t%I64u\n", id, tx_stats->wake); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_no_descs:\t%I64u\n", id, tx_stats->no_descs); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_doorbell:\t%I64u\n", id, tx_stats->doorbell_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_comp:\t%I64u\n", id, tx_stats->comp_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_defrag:\t%I64u\n", id, tx_stats->defrag_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_tso_defrag:\t%I64u\n", id, tx_stats->tso_defrag_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_tso_fail:\t%I64u\n", id, tx_stats->tso_fail_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_ucast_bytes:\t%I64u\n", id, tx_stats->directed_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_ucast_packets:\t%I64u\n", id, tx_stats->directed_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_bcast_bytes:\t%I64u\n", id, tx_stats->bcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_mcast_bytes:\t%I64u\n", id, tx_stats->mcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_mcast_packets:\t%I64u\n", id, tx_stats->mcast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_tso_bytes:\t%I64u\n", id, tx_stats->tso_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_tso_packets:\t%I64u\n", id, tx_stats->tso_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_max_tso_packet:\t%I64u\n", id, tx_stats->max_tso_sz); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_last_tso_packets:\t%I64u\n", id, tx_stats->last_tso_sz); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_encap_tso_bytes:\t%I64u\n", id, tx_stats->encap_tso_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_encap_tso_packets:\t%I64u\n", id, tx_stats->encap_tso_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_csum_none:\t%I64u\n", id, tx_stats->csum_none); outfile << TempBuf;
    //TODO: remove csum_partial counters, not used anymore
    //_snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_csum_partial:\t%I64u\n", id, tx_stats->csum_partial); outfile << TempBuf;
    //_snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_csum_partial_inner:\t%I64u\n", id, tx_stats->csum_partial_inner); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_csum_hw:\t%I64u\n", id, tx_stats->csum_hw); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_csum_hw_inner:\t%I64u\n", id, tx_stats->csum_hw_inner); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_vlan_inserted:\t%I64u\n", id, tx_stats->vlan_inserted); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\ttx%s_dma_map_error:\t%I64u\n", id, tx_stats->dma_map_error); outfile << TempBuf;
}

void
DumpRxRingStats(const char *id, struct dev_rx_ring_stats *rx_stats, std::ostream& outfile)
{
    char TempBuf[PRINT_BUFFER_SIZE];

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_poll:\t%I64u\n", id, rx_stats->poll); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_arm:\t%I64u\n", id, rx_stats->arm); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_comp:\t%I64u\n", id, rx_stats->completion_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_comp_errors:\t%I64u\n", id, rx_stats->completion_errors); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_buffers_posted:\t%I64u\n", id, rx_stats->buffers_posted); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_pool_empty:\t%I64u\n", id, rx_stats->pool_empty); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_more_nbl:\t%I64u\n", id, rx_stats->more_nbl); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_ucast_bytes:\t%I64u\n", id, rx_stats->directed_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_ucast_packets:\t%I64u\n", id, rx_stats->directed_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_bcast_bytes:\t%I64u\n", id, rx_stats->bcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_bcast_packets:\t%I64u\n", id, rx_stats->bcast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_mcast_bytes:\t%I64u\n", id, rx_stats->mcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_mcast_packets:\t%I64u\n", id, rx_stats->mcast_packets); outfile << TempBuf;
    //TODO: remove lro_packets, we don't support
    //_snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_lro_packets:\t%I64u\n", id, rx_stats->lro_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_none:\t%I64u\n", id, rx_stats->csum_none); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_complete:\t%I64u\n", id, rx_stats->csum_complete); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_verified:\t%I64u\n", id, rx_stats->csum_verified); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_ip:\t%I64u\n", id, rx_stats->csum_ip); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_ip_bad:\t%I64u\n", id, rx_stats->csum_ip_bad); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_udp:\t%I64u\n", id, rx_stats->csum_udp); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_udp_bad:\t%I64u\n", id, rx_stats->csum_udp_bad); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_tcp:\t%I64u\n", id, rx_stats->csum_tcp); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_csum_tcp_bad:\t%I64u\n", id, rx_stats->csum_tcp_bad); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_vlan_stripped:\t%I64u\n", id, rx_stats->vlan_stripped); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_rsc_bytes:\t%I64u\n", id, rx_stats->rsc_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_rsc_packets:\t%I64u\n", id, rx_stats->rsc_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_rsc_events:\t%I64u\n", id, rx_stats->rsc_events); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\t\trx%s_rsc_aborts:\t%I64u\n", id, rx_stats->rsc_aborts); outfile << TempBuf;
}

DWORD
DumpDevStats(void *Stats, bool per_queue, std::ostream& outfile)
{
    DevStatsRespCB *resp = (DevStatsRespCB *)Stats;
    struct dev_port_stats *dev_stats = NULL;
    ULONG ulDevCount = 0;
    ULONG ulLifCount = 0;
    ULONG ulRxCnt = 0;
    ULONG ulTxCnt = 0;
    char TempBuf[PRINT_BUFFER_SIZE];

    while(resp->adapter_name[0])
    {
        struct dev_tx_ring_stats tx_total = {};
        struct dev_rx_ring_stats rx_total = {};
        WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
        char id[16] = {};
        dev_stats = &resp->stats;

        get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
                           resp->adapter_name, ADAPTER_NAME_MAX_SZ);

        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Adapter %S\n", resp->adapter_name); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Interface %S\n", name); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Mgmt %s RSS %s VMQ %s SRIOV %s Up %d Dwn %d\n", 
               (dev_stats->device_id == PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT)?"Yes":"No",
               (dev_stats->flags & IONIC_PORT_FLAG_RSS)?"Yes":"No",
               (dev_stats->flags & IONIC_PORT_FLAG_VMQ)?"Yes":"No",
               (dev_stats->flags & IONIC_PORT_FLAG_SRIOV)?"Yes":"No",
               (ULONG)dev_stats->link_up,
               (ULONG)dev_stats->link_dn); outfile << TempBuf;

        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tVendor %04lX\n", dev_stats->vendor_id); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tDevice %04lX\n", dev_stats->device_id); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tLif Count %d\n", dev_stats->lif_count); outfile << TempBuf;

        ulLifCount = 0;
        while (ulLifCount < dev_stats->lif_count)
        {
            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tLif %d\n", dev_stats->lif_stats[ ulLifCount].lif_id); outfile << TempBuf;
            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tLif type: %s", GetLifTypeName( dev_stats->lif_stats[ ulLifCount].lif_type)); outfile << TempBuf;

            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tLif name: %s\n", dev_stats->lif_stats[ ulLifCount].lif_name); outfile << TempBuf;

            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tRx Count: %d\n", dev_stats->lif_stats[ ulLifCount].rx_count); outfile << TempBuf;
            for (ulRxCnt = 0; ulRxCnt < dev_stats->lif_stats[ulLifCount].rx_count; ++ulRxCnt)
            {
                if (per_queue) {
                    snprintf(id, sizeof(id), "_%lu", ulRxCnt);
                    DumpRxRingStats(id, &dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt], outfile);
                }

                rx_total.poll += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].poll;
                rx_total.arm += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].arm;
                rx_total.completion_count += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].completion_count;
                rx_total.completion_errors += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].completion_errors;
                rx_total.buffers_posted += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].buffers_posted;
                rx_total.pool_empty += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].pool_empty;
                rx_total.more_nbl += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].more_nbl;
                rx_total.directed_bytes += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].directed_bytes;
                rx_total.directed_packets += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].directed_packets;
                rx_total.bcast_bytes += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].bcast_bytes;
                rx_total.bcast_packets += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].bcast_packets;
                rx_total.mcast_bytes += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].mcast_bytes;
                rx_total.mcast_packets += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].mcast_packets;
                //TODO: remove lro_packets, we don't support
                rx_total.lro_packets += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].lro_packets;
                rx_total.csum_none += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_none;
                rx_total.csum_complete += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_complete;
                rx_total.csum_verified += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_verified;
                rx_total.csum_ip += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_ip;
                rx_total.csum_ip_bad += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_ip_bad;
                rx_total.csum_udp += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_udp;
                rx_total.csum_udp_bad += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_udp_bad;
                rx_total.csum_tcp += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_tcp;
                rx_total.csum_tcp_bad += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].csum_tcp_bad;
                rx_total.vlan_stripped += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].vlan_stripped;
                rx_total.rsc_bytes += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].rsc_bytes;
                rx_total.rsc_packets += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].rsc_packets;
                rx_total.rsc_events += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].rsc_events;
                rx_total.rsc_aborts += dev_stats->lif_stats[ulLifCount].rx_ring[ulRxCnt].rsc_aborts;
            }
            id[0] = 0;
            DumpRxRingStats(id, &rx_total, outfile);

            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tTx Count: %d\n", dev_stats->lif_stats[ ulLifCount].tx_count); outfile << TempBuf;
            for (ulTxCnt = 0; ulTxCnt < dev_stats->lif_stats[ulLifCount].tx_count; ++ulTxCnt)
            {
                if (per_queue) {
                    snprintf(id, sizeof(id), "_%lu", ulTxCnt); outfile << TempBuf;
                    DumpTxRingStats(id, &dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt], outfile);
                }

                tx_total.full += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].full;
                tx_total.wake += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].wake;
                tx_total.no_descs += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].no_descs;
                tx_total.doorbell_count += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].doorbell_count;
                tx_total.comp_count += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].comp_count;
                tx_total.defrag_count += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].defrag_count;
                tx_total.tso_defrag_count += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].tso_defrag_count;
                tx_total.tso_fail_count += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].tso_fail_count;
                tx_total.directed_bytes += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].directed_bytes;
                tx_total.directed_packets += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].directed_packets;
                tx_total.bcast_bytes += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].bcast_bytes;
                tx_total.mcast_bytes += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].mcast_bytes;
                tx_total.mcast_packets += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].mcast_packets;
                tx_total.tso_bytes += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].tso_bytes;
                tx_total.tso_packets += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].tso_packets;
                tx_total.encap_tso_bytes += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].encap_tso_bytes;
                tx_total.encap_tso_packets += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].encap_tso_packets;
                tx_total.csum_none += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].csum_none;
                //TODO: remove csum_partial counters, not used anymore
                tx_total.csum_partial += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].csum_partial;
                tx_total.csum_partial_inner += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].csum_partial_inner;
                tx_total.csum_hw += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].csum_hw;
                tx_total.csum_hw_inner += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].csum_hw_inner;
                tx_total.vlan_inserted += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].vlan_inserted;
                tx_total.dma_map_error += dev_stats->lif_stats[ulLifCount].tx_ring[ulTxCnt].dma_map_error;
            }
            id[0] = 0;
            DumpTxRingStats(id, &tx_total, outfile);

            ulLifCount++;
        }

        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;

        ++resp;
        ++ulDevCount;
    }

    return ulDevCount;
}

void
DumpMgmtStats(void *Stats, std::ostream& outfile)
{
    MgmtStatsRespCB *resp = (MgmtStatsRespCB *)Stats;
    struct mgmt_port_stats *mgmt_stats = &resp->stats;
    WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
    char TempBuf[PRINT_BUFFER_SIZE];

    // one adapter per response

    get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
                       resp->adapter_name, ADAPTER_NAME_MAX_SZ);

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Mgmt Port Stats: %S\n", resp->adapter_name); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Interface: %S\n", name); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames all: %I64u\n", mgmt_stats->frames_rx_all); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;
}

void
DumpPortStats(void *Stats, std::ostream& outfile)
{
    PortStatsRespCB *resp = (PortStatsRespCB *)Stats;
    struct port_stats *port_stats = &resp->stats;
    WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
    ULONGLONG ull9216Frames = 0;
    char TempBuf[PRINT_BUFFER_SIZE];

    // one adapter per response

    get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
                       resp->adapter_name, ADAPTER_NAME_MAX_SZ);

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Port Stats: %S\n", resp->adapter_name); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Interface: %S\n", name); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames ok:\t\t%I64u\n", port_stats->frames_rx_ok); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames all:\t\t%I64u\n", port_stats->frames_rx_all); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx fcs errors:\t\t%I64u\n", port_stats->frames_rx_bad_fcs); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx crc errors:\t\t%I64u\n", port_stats->frames_rx_bad_all); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx bytes ok:\t\t%I64u\n", port_stats->octets_rx_ok); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx bytes all:\t\t%I64u\n", port_stats->octets_rx_all); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx u-cast:\t\t%I64u\n", port_stats->frames_rx_unicast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx m-cast:\t\t%I64u\n", port_stats->frames_rx_multicast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx b-cast:\t\t%I64u\n", port_stats->frames_rx_broadcast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pause:\t\t%I64u\n", port_stats->frames_rx_pause); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx bad length:\t\t%I64u\n", port_stats->frames_rx_bad_length); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx undersized:\t\t%I64u\n", port_stats->frames_rx_undersized); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx oversized:\t\t%I64u\n", port_stats->frames_rx_oversized); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx fragments:\t\t%I64u\n", port_stats->frames_rx_fragments); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx jabber:\t\t%I64u\n", port_stats->frames_rx_jabber); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pfc frames:\t\t%I64u\n", port_stats->frames_rx_pripause); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx crc stomped:\t\t%I64u\n", port_stats->frames_rx_stomped_crc); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx too long:\t\t%I64u\n", port_stats->frames_rx_too_long); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx vlan ok:\t\t%I64u\n", port_stats->frames_rx_vlan_good); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx dropped frames:\t%I64u\n", port_stats->frames_rx_dropped); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames < 64:\t\t%I64u\n", port_stats->frames_rx_less_than_64b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames = 64:\t\t%I64u\n", port_stats->frames_rx_64b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 65-127:\t%I64u\n", port_stats->frames_rx_65b_127b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 128-255:\t%I64u\n", port_stats->frames_rx_128b_255b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 256-511:\t%I64u\n", port_stats->frames_rx_256b_511b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 512-1023:\t%I64u\n", port_stats->frames_rx_512b_1023b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 1024-1518:\t%I64u\n", port_stats->frames_rx_1024b_1518b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 1519-2047:\t%I64u\n", port_stats->frames_rx_1519b_2047b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 2048-4095:\t%I64u\n", port_stats->frames_rx_2048b_4095b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 4096-8191:\t%I64u\n", port_stats->frames_rx_4096b_8191b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames 8192-9215:\t%I64u\n", port_stats->frames_rx_8192b_9215b); outfile << TempBuf;

    ull9216Frames = port_stats->frames_rx_all - ( port_stats->frames_rx_dropped +
                                                    port_stats->frames_rx_less_than_64b +
                                                    port_stats->frames_rx_64b +
                                                    port_stats->frames_rx_65b_127b +
                                                    port_stats->frames_rx_128b_255b +
                                                    port_stats->frames_rx_256b_511b +
                                                    port_stats->frames_rx_512b_1023b +
                                                    port_stats->frames_rx_1024b_1518b +
                                                    port_stats->frames_rx_1519b_2047b +
                                                    port_stats->frames_rx_2048b_4095b +
                                                    port_stats->frames_rx_4096b_8191b +
                                                    port_stats->frames_rx_8192b_9215b);

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx frames >= 9216:\t%I64u\n", ull9216Frames); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames ok:\t\t%I64u\n", port_stats->frames_tx_ok); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames all:\t\t%I64u\n", port_stats->frames_tx_all); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames bad:\t\t%I64u\n", port_stats->frames_tx_bad); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx bytes ok:\t\t%I64u\n", port_stats->octets_tx_ok); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx bytes all:\t\t%I64u\n", port_stats->octets_tx_total); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx u-cast:\t\t%I64u\n", port_stats->frames_tx_unicast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx m-cast:\t\t%I64u\n", port_stats->frames_tx_multicast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx b-cast:\t\t%I64u\n", port_stats->frames_tx_broadcast); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pause frames:\t%I64u\n", port_stats->frames_tx_pause); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pfc frames:\t\t%I64u\n", port_stats->frames_tx_pripause); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx vlan frames:\t\t%I64u\n", port_stats->frames_tx_vlan); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames < 64:\t\t%I64u\n", port_stats->frames_tx_less_than_64b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames = 64:\t\t%I64u\n", port_stats->frames_tx_64b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 65-127:\t%I64u\n", port_stats->frames_tx_65b_127b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 128-255:\t%I64u\n", port_stats->frames_tx_128b_255b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 256-511:\t%I64u\n", port_stats->frames_tx_256b_511b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 512-1023:\t%I64u\n", port_stats->frames_tx_512b_1023b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 1024-1518:\t%I64u\n", port_stats->frames_tx_1024b_1518b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 1519-2047:\t%I64u\n", port_stats->frames_tx_1519b_2047b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 2048-4095:\t%I64u\n", port_stats->frames_tx_2048b_4095b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 4096-8191:\t%I64u\n", port_stats->frames_tx_4096b_8191b); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames 8192-9215:\t%I64u\n", port_stats->frames_tx_8192b_9215b); outfile << TempBuf;

    ull9216Frames = port_stats->frames_tx_all - ( port_stats->frames_tx_less_than_64b +
                                                    port_stats->frames_tx_64b +
                                                    port_stats->frames_tx_65b_127b +
                                                    port_stats->frames_tx_128b_255b +
                                                    port_stats->frames_tx_256b_511b +
                                                    port_stats->frames_tx_512b_1023b +
                                                    port_stats->frames_tx_1024b_1518b +
                                                    port_stats->frames_tx_1519b_2047b +
                                                    port_stats->frames_tx_2048b_4095b +
                                                    port_stats->frames_tx_4096b_8191b +
                                                    port_stats->frames_tx_8192b_9215b);

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx frames >= 9216:\t%I64u\n", ull9216Frames); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-0:\t\t%I64u\n", port_stats->frames_tx_pri_0); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-1:\t\t%I64u\n", port_stats->frames_tx_pri_1); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-2:\t\t%I64u\n", port_stats->frames_tx_pri_2); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-3:\t\t%I64u\n", port_stats->frames_tx_pri_3); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-4:\t\t%I64u\n", port_stats->frames_tx_pri_4); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-5:\t\t%I64u\n", port_stats->frames_tx_pri_5); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-6:\t\t%I64u\n", port_stats->frames_tx_pri_6); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-7:\t\t%I64u\n", port_stats->frames_tx_pri_7); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-0:\t\t%I64u\n", port_stats->frames_rx_pri_0); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-1:\t\t%I64u\n", port_stats->frames_rx_pri_1); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-2:\t\t%I64u\n", port_stats->frames_rx_pri_2); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-3:\t\t%I64u\n", port_stats->frames_rx_pri_3); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-4:\t\t%I64u\n", port_stats->frames_rx_pri_4); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-5:\t\t%I64u\n", port_stats->frames_rx_pri_5); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-6:\t\t%I64u\n", port_stats->frames_rx_pri_6); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-7:\t\t%I64u\n", port_stats->frames_rx_pri_7); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-0 pause:\t\t%I64u\n", port_stats->tx_pripause_0_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-1 pause:\t\t%I64u\n", port_stats->tx_pripause_1_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-2 pause:\t\t%I64u\n", port_stats->tx_pripause_2_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-3 pause:\t\t%I64u\n", port_stats->tx_pripause_3_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-4 pause:\t\t%I64u\n", port_stats->tx_pripause_4_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-5 pause:\t\t%I64u\n", port_stats->tx_pripause_5_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-6 pause:\t\t%I64u\n", port_stats->tx_pripause_6_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx pri-7 pause:\t\t%I64u\n", port_stats->tx_pripause_7_1us_count); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-0 pause:\t\t%I64u\n", port_stats->rx_pripause_0_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-1 pause:\t\t%I64u\n", port_stats->rx_pripause_1_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-2 pause:\t\t%I64u\n", port_stats->rx_pripause_2_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-3 pause:\t\t%I64u\n", port_stats->rx_pripause_3_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-4 pause:\t\t%I64u\n", port_stats->rx_pripause_4_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-5 pause:\t\t%I64u\n", port_stats->rx_pripause_5_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-6 pause:\t\t%I64u\n", port_stats->rx_pripause_6_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pri-7 pause:\t\t%I64u\n", port_stats->rx_pripause_7_1us_count); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx pause 1us  :\t\t%I64u\n", port_stats->rx_pause_1us_count); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tFrames truncated:\t%I64u\n", port_stats->frames_tx_truncated); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;
}

void
DumpLifStats(void *Stats, std::ostream& outfile)
{
    LifStatsRespCB *resp = (LifStatsRespCB *)Stats;
    struct lif_stats *lif_stats = &resp->stats;
    WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
    char TempBuf[PRINT_BUFFER_SIZE];

    // one adapter per response

    get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
                       resp->adapter_name, ADAPTER_NAME_MAX_SZ);

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Adapter %S\n", resp->adapter_name); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Interface %S\n", name); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Lif %u Stats\n", resp->lif_index); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx u-cast bytes: \t\t%I64u\n", lif_stats->rx_ucast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx u-cast packets: \t\t%I64u\n", lif_stats->rx_ucast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx m-cast bytes: \t\t%I64u\n", lif_stats->rx_mcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx m-cast packets: \t\t%I64u\n", lif_stats->rx_mcast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx b-cast bytes: \t\t%I64u\n", lif_stats->rx_bcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx b-cast packets: \t\t%I64u\n", lif_stats->rx_bcast_packets); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop u-cast bytes: \t\t%I64u\n", lif_stats->rx_ucast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop u-cast packets: \t%I64u\n", lif_stats->rx_ucast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop m-cast bytes: \t\t%I64u\n", lif_stats->rx_mcast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop m-cast packets: \t%I64u\n", lif_stats->rx_mcast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop b-cast bytes: \t\t%I64u\n", lif_stats->rx_bcast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx drop b-cast packets: \t%I64u\n", lif_stats->rx_bcast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx dma error: \t\t\t%I64u\n", lif_stats->rx_dma_error); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx u-cast bytes: \t\t%I64u\n", lif_stats->tx_ucast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx u-cast packets: \t\t%I64u\n", lif_stats->tx_ucast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx m-cast bytes: \t\t%I64u\n", lif_stats->tx_mcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx m-cast packets: \t\t%I64u\n", lif_stats->tx_mcast_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx b-cast bytes: \t\t%I64u\n", lif_stats->tx_bcast_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx b-cast packets: \t\t%I64u\n", lif_stats->tx_bcast_packets); outfile << TempBuf;
    
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop u-cast bytes: \t\t%I64u\n", lif_stats->tx_ucast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop u-cast packets: \t%I64u\n", lif_stats->tx_ucast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop m-cast bytes: \t\t%I64u\n", lif_stats->tx_mcast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop m-cast packets: \t%I64u\n", lif_stats->tx_mcast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop b-cast bytes: \t\t%I64u\n", lif_stats->tx_bcast_drop_bytes); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx drop b-cast packets: \t%I64u\n", lif_stats->tx_bcast_drop_packets); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx dma error: \t\t\t%I64u\n", lif_stats->tx_dma_error); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx queue disabled drops: \t%I64u\n", lif_stats->rx_queue_disabled); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx queue empty drops: \t\t%I64u\n", lif_stats->rx_queue_empty); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx queue error count: \t\t%I64u\n", lif_stats->rx_queue_error); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx descriptor fetch errors: \t%I64u\n", lif_stats->rx_desc_fetch_error); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tRx descriptor data errors: \t%I64u\n", lif_stats->rx_desc_data_error); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx queue disabled drops: \t%I64u\n", lif_stats->tx_queue_disabled); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx queue error count: \t\t%I64u\n", lif_stats->tx_queue_error); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx descriptor fetch errors: \t%I64u\n", lif_stats->tx_desc_fetch_error); outfile << TempBuf;
    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tTx descriptor data errors: \t%I64u\n", lif_stats->tx_desc_data_error); outfile << TempBuf;

    _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;
}

char *
GetLifTypeName(ULONG type)
{

    char    *pType = (char *)"Default";

    if (type == IONIC_LIF_TYPE_VMQ) {
        pType = (char *)"VM Queue";
    }
    else if (type == IONIC_LIF_TYPE_VPORT) {
        pType = (char *)"VPort";
    }

    return pType;
}

ULONG
DumpPerfStats(void *Stats, DWORD Size, std::ostream& outfile)
{
    struct _PERF_MON_CB *perf_stats = (struct _PERF_MON_CB *)Stats;
    struct _PERF_MON_ADAPTER_STATS *adapter_stats = NULL;
    struct _PERF_MON_LIF_STATS *lif_stats = NULL;
    struct _PERF_MON_TX_QUEUE_STATS *tx_stats = NULL;
    struct _PERF_MON_RX_QUEUE_STATS *rx_stats = NULL;
    ULONG adapter_cnt = 0;
    ULONG lif_cnt = 0;
    ULONG rx_cnt = 0;
    ULONG tx_cnt = 0;
    char TempBuf[PRINT_BUFFER_SIZE];

    // XXX tricky data, must be careful not to advance the pointers out of bounds
    UNREFERENCED_PARAMETER(Size);

    adapter_stats = (struct _PERF_MON_ADAPTER_STATS *)((char *)perf_stats + sizeof( struct _PERF_MON_CB));

    for (adapter_cnt = 0; adapter_cnt < perf_stats->adapter_count; adapter_cnt++) {
        WCHAR name[ADAPTER_NAME_MAX_SZ] = {};

        get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
                           adapter_stats->name, ADAPTER_NAME_MAX_SZ);

        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Adapter: %S\n", adapter_stats->name); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Interface: %S\n", name); outfile << TempBuf;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "Mgmt: %s Lif cnt: %d Core redirect cnt: %d\n",  
               adapter_stats->mgmt_device ? "Yes" : "No",
               adapter_stats->lif_count,
               adapter_stats->core_redirection_count); outfile << TempBuf;

        lif_stats = (struct _PERF_MON_LIF_STATS *)((char *)adapter_stats + sizeof( struct _PERF_MON_ADAPTER_STATS));

        for (lif_cnt = 0; lif_cnt < adapter_stats->lif_count; lif_cnt++) {

            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\tLif: %s Rx cnt: %d Tx cnt: %d\n", 
                            lif_stats->name,
                            lif_stats->rx_queue_count,
                            lif_stats->tx_queue_count); outfile << TempBuf;

            rx_stats = (struct _PERF_MON_RX_QUEUE_STATS *)((char *)lif_stats + sizeof( struct _PERF_MON_LIF_STATS));

            for (rx_cnt = 0; rx_cnt < lif_stats->rx_queue_count; rx_cnt++) {

                _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tRx: %d queue len: %ld max queue len %ld\n", 
                       rx_cnt,
                       rx_stats->queue_len,
                       rx_stats->max_queue_len); outfile << TempBuf;

                _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tdpc total time: %lld dpc latency %lld\n"
                       "\t\tdpc to dpc time: %lld dpc indicate time %lld\n"
                       "\t\tdpc walk time: %lld dpc fill time %lld\n"
                       "\t\tdpc rate: %lld\n",
                       rx_stats->dpc_total_time,
                       rx_stats->dpc_latency,
                       rx_stats->dpc_to_dpc_time,
                       rx_stats->dpc_indicate_time,
                       rx_stats->dpc_walk_time,
                       rx_stats->dpc_fill_time,
                       rx_stats->dpc_rate); outfile << TempBuf;

                rx_stats = (struct _PERF_MON_RX_QUEUE_STATS *)((char *)rx_stats + sizeof( struct _PERF_MON_RX_QUEUE_STATS));
            }

            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;

            tx_stats = (struct _PERF_MON_TX_QUEUE_STATS *)rx_stats;

            for (tx_cnt = 0; tx_cnt < lif_stats->tx_queue_count; tx_cnt++) {

                _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tTx: %d queue len: %ld max queue len: %ld\n", 
                       tx_cnt,
                       tx_stats->queue_len,
                       tx_stats->max_queue_len); outfile << TempBuf;

                _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tnbl count: %ld nb count: %ld out nb: %ld\n", 
                       tx_stats->nbl_count,
                       tx_stats->nb_count,
                       tx_stats->outstanding_nb_count); outfile << TempBuf;

                _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\t\tdpc total time: %lld dpc to dpc time: %lld\n"
                       "\t\tdpc rate: %lld\n",
                       tx_stats->dpc_total_time,
                       tx_stats->dpc_to_dpc,
                       tx_stats->dpc_rate); outfile << TempBuf;

                tx_stats = (struct _PERF_MON_TX_QUEUE_STATS *)((char *)tx_stats + sizeof( struct _PERF_MON_TX_QUEUE_STATS));
            }

            lif_stats = (struct _PERF_MON_LIF_STATS *)tx_stats;
            _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;
        }
       
        adapter_stats = (struct _PERF_MON_ADAPTER_STATS *)lif_stats;
        _snprintf_s(TempBuf, PRINT_BUFFER_SIZE, _TRUNCATE, "\n"); outfile << TempBuf;
    }

    return adapter_cnt;
}

ULONG
DumpRegKeys(void *KeyInfo, ULONG Size)
{
    struct _REG_KEY_INFO_HDR *key_info = (struct _REG_KEY_INFO_HDR *)KeyInfo;
    struct _REG_KEY_ENTRY *entry = NULL;
    ULONG info_count = 0;
    ULONG processed_len = 0;
    ULONG reg_index = 0;
    ULONG pad = 0;

	while (processed_len < Size)
	{
		WCHAR name[ADAPTER_NAME_MAX_SZ] = {};

		get_interface_name(name, ADAPTER_NAME_MAX_SZ, NULL, 0,
				   key_info->name, ADAPTER_NAME_MAX_SZ);

		printf("Name: %S\n", key_info->name);
		printf("Interface: %S\n", name);
		printf("Location: %S\n", key_info->device_location);
		++info_count;

		printf("\t%-40s%-16s%-16s%-16s%-16s\n\n",
						"Registry Key",
						"Value",
						"Default",
						"Min",
						"Max");
		entry = (struct _REG_KEY_ENTRY *)((char *)key_info + sizeof( struct _REG_KEY_INFO_HDR));
		for (reg_index = 0; reg_index < key_info->entry_count; reg_index++) {
			printf("\t%-40S%d\t\t%d\t\t%d\t\t%d\n", 
						(WCHAR *)((char *)entry + entry->key_name_offset),
						entry->current_value,
						entry->default_value,
						entry->min_value,
						entry->max_value);

			if (entry->next_entry == 0) {
				break;
			}

			entry = (struct _REG_KEY_ENTRY *)((char *)entry + entry->next_entry);
		}

		if (key_info->next_entry == 0) {
			break;
		}

		processed_len += key_info->next_entry;
		key_info = (struct _REG_KEY_INFO_HDR *)((char *)key_info + key_info->next_entry);
		printf("\n");
	}

    return info_count;
}

char *
hw_state_to_str(ULONG state)
{
	
	char *string = NULL;

	switch (state) {

	case 0: {
		string = (char *)"Ready";
		break;
	}

	case 1: {
		string = (char *)"Initializing";
		break;
	}

	case 2: {
		string = (char *)"Reset";
		break;
	}

	case 3: {
		string = (char *)"Closing";
		break;
	}

	case 4: {
		string = (char *)"Not ready";
		break;
	}

	default: {
		string = (char *)"Unknown";
		break;
	}
	}

	return string;
}

char *
link_state_to_str(ULONG state)
{
	
	char *string = NULL;

	switch (state) {

	case 1: {
		string = (char *)"Link up";
		break;
	}

	case 2: {
		string = (char *)"Link down";
		break;
	}

	default: {
		string = (char *)"Unknown";
		break;
	}
	}

	return string;
}

ULONG
DumpAdapterInfo(void *info_buffer, ULONG Size)
{
        struct _ADAPTER_INFO_HDR *adapter_info = (struct _ADAPTER_INFO_HDR *)info_buffer;
        struct _ADAPTER_INFO *info = NULL;
        ULONG	count = 0;
        ULONG   max_info = 0;

        if (Size > sizeof(*adapter_info)) {
                max_info = (Size - sizeof(*adapter_info)) / sizeof(*info);
                if (max_info >= adapter_info->count) {
                        max_info = adapter_info->count;
                }
                else {
                        printf("Warning: bad adapter count\n\n");
                }
        }

	printf("NIC Info:\n");

	info = (struct _ADAPTER_INFO *)((char *)adapter_info + sizeof( struct _ADAPTER_INFO_HDR));

	for( count = 0; count < max_info; count++) {
		WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
		WCHAR index[ADAPTER_NAME_MAX_SZ] = {};

		get_interface_name(name, ADAPTER_NAME_MAX_SZ,
				   index, ADAPTER_NAME_MAX_SZ,
				   info->name, ADAPTER_NAME_MAX_SZ);

		printf("\tName: %S\n", info->name);
		printf("\tInterface Name: %S\n", name);
		printf("\tInterface Index: %S\n", index);
		printf("\tVendor Id: %04lX\n", info->vendor_id);
		printf("\tProduct Id: %04lX\n", info->product_id);
		printf("\tLocation: %S\n", info->device_location);
		printf("\tASIC Type: %d\n", info->asic_type);
		printf("\tASIC Rev: %d\n", info->asic_rev);
		printf("\tFW Version: %s\n", info->fw_version);
		printf("\tSerial Num: %s\n", info->serial_num);
		printf("\tHW State: %s\n", hw_state_to_str(info->hw_state));
		printf("\tLink State: %s\n", link_state_to_str(info->link_state));
		printf("\tMtu: %d\n", info->Mtu);
		printf("\tSpeed: %lldGbps\n", info->Speed/1000000000);
        DisplayInterface(info->name);
		printf("\n");

		info = (struct _ADAPTER_INFO *)((char *)info + sizeof( struct _ADAPTER_INFO));
	};


	return count;
}

ULONG
DumpQueueInfo(void *info_buffer, ULONG Size)
{
        struct _QUEUE_INFO_HDR *queue_info = (struct _QUEUE_INFO_HDR *)info_buffer;
        struct _QUEUE_INFO *info = NULL;
        ULONG	count = 0;
        ULONG   max_info = 0;

        if (Size > sizeof(*queue_info)) {
                max_info = (Size - sizeof(*queue_info)) / sizeof(*info);
                if (max_info >= queue_info->count) {
                        max_info = queue_info->count;
                }
                else {
                        printf("Warning: bad adapter count\n\n");
                }
        }

	printf("Queue Info:\n");

	info = (struct _QUEUE_INFO *)((char *)queue_info + sizeof( struct _QUEUE_INFO_HDR));

	for( count = 0; count < max_info; count++) {
		WCHAR name[ADAPTER_NAME_MAX_SZ] = {};
		WCHAR index[ADAPTER_NAME_MAX_SZ] = {};

		get_interface_name(name, ADAPTER_NAME_MAX_SZ,
				   index, ADAPTER_NAME_MAX_SZ,
				   info->name, ADAPTER_NAME_MAX_SZ);

		printf("\tName: %S\n", info->name);
		printf("\tInterface Name: %S\n", name);
		printf("\tInterface Index: %S\n", index);

		printf("\tlif: %s\n", info->lif);
		printf("\trx queue cnt: %d\n", info->rx_queue_cnt);
		printf("\ttx queue cnt: %d\n", info->tx_queue_cnt);
		printf("\ttx mode: %d\n", info->tx_mode);

		for (unsigned int queue_cnt = 0; queue_cnt < info->rx_queue_cnt; queue_cnt++) {
			printf("\t\tRx Queue %d isr %s group %d core %d msi %d\n",
					info->rx_queue_info[ queue_cnt].id,
					info->rx_queue_info[ queue_cnt].isr?"yes":"no",
					info->rx_queue_info[ queue_cnt].group,
					info->rx_queue_info[ queue_cnt].core,
					info->rx_queue_info[ queue_cnt].isr?info->rx_queue_info[ queue_cnt].msi_id:-1);
			printf("\t\tQueue head %d tail %d CompQueue head %d tail %d\n",
					info->rx_queue_info[ queue_cnt].q_head_index,
					info->rx_queue_info[ queue_cnt].q_tail_index,
					info->rx_queue_info[ queue_cnt].cq_head_index,
					info->rx_queue_info[ queue_cnt].cq_tail_index);
		}
		
		printf("\n");

		for (unsigned int queue_cnt = 0; queue_cnt < info->tx_queue_cnt; queue_cnt++) {
			printf("\t\tTx Queue %d isr %s group %d core %d msi %d\n",
					info->tx_queue_info[ queue_cnt].id,
					info->tx_queue_info[ queue_cnt].isr?"yes":"no",
					info->tx_queue_info[ queue_cnt].group,
					info->tx_queue_info[ queue_cnt].core,
					info->tx_queue_info[ queue_cnt].isr?info->tx_queue_info[ queue_cnt].msi_id:-1);
			printf("\t\tQueue head %d tail %d CompQueue head %d tail %d\n",
					info->tx_queue_info[ queue_cnt].q_head_index,
					info->tx_queue_info[ queue_cnt].q_tail_index,
					info->tx_queue_info[ queue_cnt].cq_head_index,
					info->tx_queue_info[ queue_cnt].cq_tail_index);
		}

		printf("\n");

		info = (struct _QUEUE_INFO *)((char *)info + sizeof( struct _QUEUE_INFO));
	};

	return count;
}

//
// TODO: move to Ioctl.cpp or something... after integration
//

DWORD
DryRunIoctl(DWORD dwIoControlCode,
            LPVOID lpInBuffer,
            DWORD nInBufferSize,
            LPVOID lpOutBuffer,
            DWORD nOutBufferSize)
{
    UNREFERENCED_PARAMETER(lpInBuffer);
    UNREFERENCED_PARAMETER(nInBufferSize);
    UNREFERENCED_PARAMETER(nOutBufferSize);

    if (lpOutBuffer == NULL) {
        return ERROR_SUCCESS;
    }

    switch (dwIoControlCode) {
    case IOCTL_IONIC_GET_TRACE: {
        static const char demo_trace[] =
            "00000000:Ionic System Log Demonstration 4-6-2020 17:4 Level 1 Subsystems 00FFFFFF\n"
            "00000001:OidRequest (NdisRequestFoo) Oid OID_GEN_FOO Status ABCD1234\n";
        memcpy(lpOutBuffer, demo_trace, min(nOutBufferSize, sizeof(demo_trace)));
        break;
    }
    case IOCTL_IONIC_GET_DEV_STATS: {
        DevStatsRespCB *resp = (DevStatsRespCB *)lpOutBuffer;
        resp->adapter_name[0] = L'Z';
        struct dev_port_stats *dev_stats = &resp->stats;
        dev_stats->vendor_id = 0x1dd8;
        dev_stats->device_id = 0xabcd;
        dev_stats->lif_count = 1;
        dev_stats->lif_stats[0].rx_count = 4;
        dev_stats->lif_stats[0].tx_count = 4;
        break;
    }
    case IOCTL_IONIC_GET_PERF_STATS: {
        struct _PERF_MON_CB *perf_stats = (struct _PERF_MON_CB *)lpOutBuffer;
        struct _PERF_MON_ADAPTER_STATS *adapter_stats = (struct _PERF_MON_ADAPTER_STATS *)(void *)&perf_stats[1];
        struct _PERF_MON_LIF_STATS *lif_stats = (struct _PERF_MON_LIF_STATS *)(void *)&adapter_stats[1];
        perf_stats->adapter_count = 1;
        adapter_stats->lif_count = 1;
        lif_stats->rx_queue_count = 4;
        lif_stats->tx_queue_count = 4;
        break;
    }
    case IOCTL_IONIC_GET_REG_KEY_INFO: {
        struct _REG_KEY_INFO_HDR *key_info = (struct _REG_KEY_INFO_HDR *)lpOutBuffer;
        struct _REG_KEY_ENTRY *entry = (struct _REG_KEY_ENTRY *)(void *)&key_info[1];
        wchar_t *keyname = (wchar_t *)(void *)&entry[1];
        key_info->name[0] = L'Z';
        key_info->entry_count = 1;
        key_info->next_entry = nOutBufferSize;
        entry->key_name_offset = sizeof(*entry);
        keyname[0] = L'K';
        break;
    }
    case IOCTL_IONIC_GET_ADAPTER_INFO: {
        struct _ADAPTER_INFO_HDR *adapter_info = (struct _ADAPTER_INFO_HDR *)lpOutBuffer;
        adapter_info->count = 1;
        break;
    }
    case IOCTL_IONIC_PORT_GET: {
        PortConfigCB *buf = (PortConfigCB *)lpOutBuffer;
        buf->AutoNeg = 1;
        buf->Speed = 100000;
        buf->FEC = 1;
        buf->Pause = 0x21;
        break;
    }
    }

    return ERROR_SUCCESS;
}

DWORD
DoIoctl(DWORD dwIoControlCode,
        LPVOID lpInBuffer,
        DWORD nInBufferSize,
        LPVOID lpOutBuffer,
        DWORD nOutBufferSize,
        PDWORD pnBytesReturned,
        bool dryrun)
{
    DWORD nBytesReturned;
    HANDLE hDevice = NULL;
    DWORD dwError = ERROR_SUCCESS;

    if (dryrun) {
        return DryRunIoctl(dwIoControlCode,
                           lpInBuffer,
                           nInBufferSize,
                           lpOutBuffer,
                           nOutBufferSize);
    }

    hDevice = CreateFile(IONIC_LINKNAME_STRING_USER,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        std::cout << "IonicConfig Failed to open adapter Error " << dwError << std::endl;
        return dwError;
    }

    if (pnBytesReturned == NULL) {
        pnBytesReturned = &nBytesReturned;
    }

    if (!DeviceIoControl(hDevice,
                         dwIoControlCode,
                         lpInBuffer,
                         nInBufferSize,
                         lpOutBuffer,
                         nOutBufferSize,
                         pnBytesReturned,
                         NULL)) {
        dwError = GetLastError();
        if (dwError != ERROR_MORE_DATA) {
            std::cout << "IonicConfig DeviceIoControl Failed Error " << dwError << std::endl;
        }
    }

    CloseHandle(hDevice);

    return dwError;
}

//
// TODO: move to Trace.cpp after integration
//

//
// -SetTrace
//

static
po::options_description
CmdSetTraceOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] SetTrace [options ...]");

    opts.add_options()
        ("Level,l", optype_long(), "Set debug level")
        ("Flags,f", optype_long(), "Set degut flags")
        ("Component,c", optype_long(), "Set component mask")
        ("Length,s", optype_long(), "Set trace buffer length in KB");

    return opts;
}

static
int
CmdSetTraceRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    TraceConfigCB stTrace;
    stTrace.Level = -1;
    stTrace.TraceFlags = -1;
    stTrace.Component = -1;
    stTrace.TraceBufferLength = -1;

    if (info.vm.count("Level")) {
        stTrace.Level = opval_long(info.vm, "Level");
    }

    if (info.vm.count("Flags")) {
        stTrace.TraceFlags = opval_long(info.vm, "Flags");
    }

    if (info.vm.count("Component")) {
        stTrace.Component = opval_long(info.vm, "Component");
    }

    if (info.vm.count("Length")) {
        stTrace.TraceBufferLength = opval_long(info.vm, "Length");
        if (stTrace.TraceBufferLength < 0 ||
            stTrace.TraceBufferLength > MAXIMUM_TRACE_BUFFER_SIZE) {

            std::cout << "buffer length not in range 0 to "
                << MAXIMUM_TRACE_BUFFER_SIZE << std::endl;
            return 1;
        }
    }

    return DoIoctl(IOCTL_IONIC_CONFIGURE_TRACE, &stTrace, sizeof(stTrace), NULL, 0, NULL, info.dryrun) != ERROR_SUCCESS;
}

command
CmdSetTrace()
{
    command cmd;

    cmd.name = "SetTrace";
    cmd.desc = "Set debug trace parameters";
    cmd.hidden = true;

    cmd.opts = CmdSetTraceOpts;
    cmd.run = CmdSetTraceRun;

    return cmd;
}

//
// -GetTrace
//

static
po::options_description
CmdGetTraceOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] GetTrace");

    return opts;
}

static
int
GetTrace(command_info& info, std::ostream& outfile)
{
    DWORD Size = MAXIMUM_TRACE_BUFFER_SIZE * 1024;
    char* pTraceBuffer = (char*)malloc((size_t)Size + 1);

    memset(pTraceBuffer, 0, (size_t)Size + 1);

    if (DoIoctl(IOCTL_IONIC_GET_TRACE, NULL, 0, pTraceBuffer, Size, NULL, info.dryrun) != ERROR_SUCCESS) {
        info.status = 1;
    }
    else {
        outfile << pTraceBuffer << std::endl;
    }

    free(pTraceBuffer);
    return info.status;
}


static
int
CmdGetTraceRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return GetTrace(info, std::cout);
}

command
CmdGetTrace()
{
    command cmd;

    cmd.name = "GetTrace";
    cmd.desc = "Get debug trace buffer content";
    cmd.hidden = true;

    cmd.opts = CmdGetTraceOpts;
    cmd.run = CmdGetTraceRun;

    return cmd;
}

//
// TODO: move to Stats.cpp after integration
//

//
// -PortStats
//

static
po::options_description
CmdPortStatsOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] PortStats [options ...]");

    OptAddDevName(opts);

    return opts;
}

static
po::positional_options_description
CmdPortStatsPos()
{
    po::positional_options_description pos;

    pos.add("Name", 1);

    return pos;
}
static 
int
PortStats(command_info& info, std::ostream& outfile) {
    DWORD error = ERROR_SUCCESS;
    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 10 * 1024 * 1024;
    char *pStatsBuffer = (char *)malloc(Size);

    do {
        memset(pStatsBuffer, 0, Size);

        error = DoIoctl(IOCTL_IONIC_GET_PORT_STATS, &cb, sizeof(cb), pStatsBuffer, Size, NULL, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        DumpPortStats(pStatsBuffer, outfile);
        ++cb.Skip;
    } while (error == ERROR_MORE_DATA);

    free(pStatsBuffer);
    return info.status;
}

static
int
CmdPortStatsRun(command_info& info)
{

    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return PortStats(info, std::cout);
}

command
CmdPortStats()
{
    command cmd;

    cmd.name = "PortStats";
    cmd.desc = "Read port mac counters from device";

    cmd.opts = CmdPortStatsOpts;
    cmd.pos = CmdPortStatsPos;
    cmd.run = CmdPortStatsRun;

    return cmd;
}

//
// -LifStats
//

static
po::options_description
CmdLifStatsOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] LifStats [options ...]");

    OptAddDevName(opts);

    opts.add_options()
        ("Lif,l", optype_long()->default_value("0"), "Lif index");

    return opts;
}

static
po::positional_options_description
CmdLifStatsPos()
{
    po::positional_options_description pos;

    pos.add("Name", 1);
    pos.add("Lif", 2);

    return pos;
}

static
int
LifStats(command_info& info, ULONG Index, std::ostream& outfile)
{
    ULONG error;

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);
    cb.Index = Index;

    DWORD Size = 10 * 1024 * 1024;
    char *pStatsBuffer = (char *)malloc(Size);

    do {
        memset(pStatsBuffer, 0, Size);

        error = DoIoctl(IOCTL_IONIC_GET_LIF_STATS, &cb, sizeof(cb), pStatsBuffer, Size, NULL, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        DumpLifStats(pStatsBuffer, outfile);
        ++cb.Skip;
    } while (error == ERROR_MORE_DATA);

    free(pStatsBuffer);
    return info.status;
}

static
int
CmdLifStatsRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return LifStats(info, opval_long(info.vm, "Lif"), std::cout);
}

command
CmdLifStats()
{
    command cmd;

    cmd.name = "LifStats";
    cmd.desc = "Read interface counters from device";

    cmd.opts = CmdLifStatsOpts;
    cmd.pos = CmdLifStatsPos;
    cmd.run = CmdLifStatsRun;

    return cmd;
}

//
// -MgmtStats
//

static
po::options_description
CmdMgmtStatsOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] MgmtStats");

    OptAddDevName(opts);

    return opts;
}

static
int
MgmtStats(command_info& info, std::ostream& outfile)
{
    ULONG error = ERROR_SUCCESS;

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 10 * 1024 * 1024;
    char *pStatsBuffer = (char *)malloc(Size);

    do {
        memset(pStatsBuffer, 0, Size);

        error = DoIoctl(IOCTL_IONIC_GET_MGMT_STATS, &cb, sizeof(cb), pStatsBuffer, Size, NULL, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        DumpMgmtStats(pStatsBuffer, outfile);
        ++cb.Skip;
    } while (error == ERROR_MORE_DATA);

    free(pStatsBuffer);
    return info.status;
}

static
int
CmdMgmtStatsRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return MgmtStats(info, std::cout);
}

command
CmdMgmtStats()
{
    command cmd;

    cmd.name = "MgmtStats";
    cmd.desc = "Read management interface counters";
    cmd.hidden = true;

    cmd.opts = CmdMgmtStatsOpts;
    cmd.run = CmdMgmtStatsRun;

    return cmd;
}

//
// -DevStats
//

static
po::options_description
CmdDevStatsOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] DevStats [options ...]");

    OptAddDevName(opts);

    opts.add_options()
        ("Totals,t", optype_flag(), "Suppress per-queue stats, print totals only.")
        ("Reset,r", optype_flag(), "Reset counters.");

    return opts;
}

static
int
DevStats(command_info& info, std::ostream& outfile)
{
    ULONG error = ERROR_SUCCESS;
    bool per_queue;

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    if (info.vm.count("Reset")) {
        cb.Flags |= ADAPTER_FLAG_RESET;
    }

    per_queue = !info.vm.count("Totals");

    DWORD Size = 10 * 1024 * 1024;
    char *pStatsBuffer = (char *)malloc(Size);

    do {
        memset(pStatsBuffer, 0, Size);

        error = DoIoctl(IOCTL_IONIC_GET_DEV_STATS, &cb, sizeof(cb), pStatsBuffer, Size, NULL, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        cb.Skip += DumpDevStats(pStatsBuffer, per_queue, outfile);
    } while (error == ERROR_MORE_DATA);

    free(pStatsBuffer);
    return info.status;
}

static
int
CmdDevStatsRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return DevStats(info, std::cout);
}

command
CmdDevStats()
{
    command cmd;

    cmd.name = "DevStats";
    cmd.desc = "Read device counters";
    cmd.hidden = true;

    cmd.opts = CmdDevStatsOpts;
    cmd.run = CmdDevStatsRun;

    return cmd;
}

//
// -PerfStats
//

static
po::options_description
CmdPerfStatsOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] PerfStats");

    OptAddDevName(opts);

    return opts;
}

static
int
PerfStats(command_info& info, std::ostream& outfile)
{
    ULONG error = ERROR_SUCCESS;

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 10 * 1024 * 1024;
    char *pStatsBuffer = (char *)malloc(Size);

    do {
        memset(pStatsBuffer, 0, Size);

        DWORD BytesReturned = 0;

        error = DoIoctl(IOCTL_IONIC_GET_PERF_STATS, &cb, sizeof(cb), pStatsBuffer, Size, &BytesReturned, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        cb.Skip += DumpPerfStats(pStatsBuffer, BytesReturned, outfile);
    } while (error == ERROR_MORE_DATA);

    free(pStatsBuffer);
    return info.status;
}

static
int
CmdPerfStatsRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }
    return PerfStats(info, std::cout);
}

command
CmdPerfStats()
{
    command cmd;

    cmd.name = "PerfStats";
    cmd.desc = "Read performance counters";
    cmd.hidden = true;

    cmd.opts = CmdPerfStatsOpts;
    cmd.run = CmdPerfStatsRun;

    return cmd;
}

//
// TODO: move to Registry.cpp after integration
//

//
// -RxBudget
//

static
po::options_description
CmdRxBudgetOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] RxBudget [options ...]");

    OptAddDevName(opts);
    
	opts.add_options()
        ("Budget,b", optype_long()->required(), "Receive polling budget");

    return opts;
}

static
po::positional_options_description
CmdRxBudgetPos()
{
    po::positional_options_description pos;

    pos.add("Name", 1);
    pos.add("Budget", 2);

    return pos;
}

static
int
CmdRxBudgetRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    SetBudgetCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    cb.Budget = opval_long(info.vm, "Budget");
	cb.Type = BUDGET_TYPE_RX;

    return DoIoctl(IOCTL_IONIC_SET_BUDGET, &cb, sizeof(SetBudgetCB), NULL, 0, NULL, info.dryrun) != ERROR_SUCCESS;
}

command
CmdRxBudget()
{
    command cmd;

    cmd.name = "RxBudget";
    cmd.desc = "Set receive polling budget";

    cmd.opts = CmdRxBudgetOpts;
    cmd.pos = CmdRxBudgetPos;
    cmd.run = CmdRxBudgetRun;

    return cmd;
}

//
// -RxMiniBudget
//

static
po::options_description
CmdRxMiniBudgetOpts(bool hidden)
{

    po::options_description opts("IonicConfig.exe [-h] RxMiniBudget [options ...]");

    OptAddDevName(opts);
    
	opts.add_options()
        ("Budget,b", optype_long()->required(), "Receive polling mini budget");

    return opts;
}

static
po::positional_options_description
CmdRxMiniBudgetPos()
{
    po::positional_options_description pos;

    pos.add("Name", 1);
    pos.add("Budget", 2);

    return pos;
}

static
int
CmdRxMiniBudgetRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    SetBudgetCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    cb.Budget = opval_long(info.vm, "Budget");
	cb.Type = BUDGET_TYPE_RX_MINI;

    return DoIoctl(IOCTL_IONIC_SET_BUDGET, &cb, sizeof(SetBudgetCB), NULL, 0, NULL, info.dryrun) != ERROR_SUCCESS;
}

command
CmdRxMiniBudget()
{
    command cmd;

    cmd.name = "RxMiniBudget";
    cmd.desc = "Set receive polling mini budget";

    cmd.opts = CmdRxMiniBudgetOpts;
    cmd.pos = CmdRxMiniBudgetPos;
    cmd.run = CmdRxMiniBudgetRun;

    return cmd;
}

//
// -TxBudget
//

static
po::options_description
CmdTxBudgetOpts(bool hidden)
{

    po::options_description opts("IonicConfig.exe [-h] TxBudget [options ...]");

    OptAddDevName(opts);
    
	opts.add_options()
        ("Budget,b", optype_long()->required(), "Transmit polling budget");
    
	return opts;
}

static
po::positional_options_description
CmdTxBudgetPos()
{
    po::positional_options_description pos;

    pos.add("Name", 1);
    pos.add("Budget", 2);

    return pos;
}

static
int
CmdTxBudgetRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    SetBudgetCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    cb.Budget = opval_long(info.vm, "Budget");
	cb.Type = BUDGET_TYPE_TX;

    return DoIoctl(IOCTL_IONIC_SET_BUDGET, &cb, sizeof(SetBudgetCB), NULL, 0, NULL, info.dryrun) != ERROR_SUCCESS;
}

command
CmdTxBudget()
{
    command cmd;

    cmd.name = "TxBudget";
    cmd.desc = "Set receive polling budget";

    cmd.opts = CmdTxBudgetOpts;
    cmd.pos = CmdTxBudgetPos;
    cmd.run = CmdTxBudgetRun;

    return cmd;
}

//
// -TxMode
//

static
po::options_description
CmdTxModeOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] TxMode [-x] <tx mode>");

    opts.add_options()
        ("TxMode,x", optype_long()->required(), "Tx Processing Mode");

    return opts;
}

static
po::positional_options_description
CmdTxModePos()
{
    po::positional_options_description pos;

    pos.add("TxMode", 1);

    return pos;
}

static
int
CmdTxModeRun(command_info& info)
{
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    DWORD dwTxMode = opval_long(info.vm, "TxMode");

    return DoIoctl(IOCTL_IONIC_SET_TX_MODE, &dwTxMode, sizeof(dwTxMode), NULL, 0, NULL, info.dryrun) != ERROR_SUCCESS;
}

command
CmdTxMode()
{
    command cmd;

    cmd.name = "TxMode";
    cmd.desc = "Set Tx Processing Mode";

    cmd.opts = CmdTxModeOpts;
    cmd.pos = CmdTxModePos;
    cmd.run = CmdTxModeRun;

    return cmd;
}

//
// -RegKeys
//

static
po::options_description
CmdRegKeysOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] RegKeys");

    OptAddDevName(opts);

    return opts;
}

static
int
CmdRegKeysRun(command_info& info)
{
    DWORD error = ERROR_SUCCESS;

    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 10 * 1024 * 1024;
    void *pBuffer = malloc(Size);
    if (pBuffer == NULL) {
        info.status = 1;
        return info.status;
    }

    memset(pBuffer, 0x00, Size);

    do {
        memset(pBuffer, 0, Size);

        DWORD BytesReturned = 0;

        error = DoIoctl(IOCTL_IONIC_GET_REG_KEY_INFO, &cb, sizeof(cb), pBuffer, Size, &BytesReturned, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        cb.Skip += DumpRegKeys(pBuffer, BytesReturned);
    } while (error == ERROR_MORE_DATA);

    return info.status;
}

command
CmdRegKeys()
{
    command cmd;

    cmd.name = "RegKeys";
    cmd.desc = "List registry keys accessible in Advanced Properties";

    cmd.opts = CmdRegKeysOpts;
    cmd.run = CmdRegKeysRun;

    return cmd;
}

//
// -Info
//

static
po::options_description
CmdInfoOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] Info");

    OptAddDevName(opts);

    return opts;
}

static
int
CmdInfoRun(command_info& info)
{
    DWORD error = ERROR_SUCCESS;

    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 1024 * 1024;
    void *pBuffer = malloc(Size);
    if (pBuffer == NULL) {
        info.status = 1;
        return info.status;
    }

    do {
        memset(pBuffer, 0, Size);

        DWORD BytesReturned = 0;

        error = DoIoctl(IOCTL_IONIC_GET_ADAPTER_INFO, &cb, sizeof(cb), pBuffer, Size, &BytesReturned, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        cb.Skip += DumpAdapterInfo(pBuffer, BytesReturned);
    } while (error == ERROR_MORE_DATA);

    return info.status;
}

command
CmdInfo()
{
    command cmd;

    cmd.name = "Info";
    cmd.desc = "Provide adapter information";

    cmd.opts = CmdInfoOpts;
    cmd.run = CmdInfoRun;

    return cmd;
}

//
// -QueueInfo
//

static
po::options_description
CmdQueueInfoOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] QueueInfo");

    OptAddDevName(opts);

    return opts;
}

static
int
CmdQueueInfoRun(command_info& info)
{
    DWORD error = ERROR_SUCCESS;

    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    AdapterCB cb = {};
    OptGetDevName(info, cb.AdapterName, sizeof(cb.AdapterName), false);

    DWORD Size = 1024 * 1024;
    void *pBuffer = malloc(Size);
    if (pBuffer == NULL) {
        info.status = 1;
        return info.status;
    }

    do {
        memset(pBuffer, 0, Size);

        DWORD BytesReturned = 0;

        error = DoIoctl(IOCTL_IONIC_GET_QUEUE_INFO, &cb, sizeof(cb), pBuffer, Size, &BytesReturned, info.dryrun);
        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            info.status = 1;
            break;
        }

        cb.Skip += DumpQueueInfo(pBuffer, BytesReturned);
    } while (error == ERROR_MORE_DATA);

    return info.status;
}

command
CmdQueueInfo()
{
    command cmd;

    cmd.name = "QueueInfo";
    cmd.desc = "Provide queue information";

    cmd.opts = CmdQueueInfoOpts;
    cmd.run = CmdQueueInfoRun;

    return cmd;
}



//
// -CollectDbgInfo
//

static
po::options_description
CmdCollectDbgInfoOpts(bool hidden)
{
    po::options_description opts("IonicConfig.exe [-h] CollectDbgInfo [options ...]");

    opts.add_options()
        ("lbfo,l", optype_flag(), "Collect debug info about Team NIC");

    return opts;
}

static
int
CmdCollectDbgInfoRun(command_info& info)
{
    DWORD error = ERROR_SUCCESS;
    BOOL bLbfo = FALSE;
    if (info.usage) {
        std::cout << info.cmd.opts(info.hidden) << info.cmd.desc << std::endl;
        return info.status;
    }

    if (info.vm.count("lbfo")) {
        bLbfo = TRUE;
    }
    boost::filesystem::path dstFolder = "DebugInfo";
    if (boost::filesystem::exists(dstFolder)) {
        boost::filesystem::remove_all(dstFolder);
    }
    boost::filesystem::create_directory(dstFolder);

    std::wofstream fs1("DebugInfo\\IonicEvtLog.csv");
    EvtLogHelperGetEvtLogs(bLbfo ? EVTLOG_IONICLBFO_QUERY : EVTLOG_IONIC_QUERY, fs1);
    fs1 << std::endl << std::flush;

    std::wofstream fs2("DebugInfo\\IonicNetAdapter.txt");
    if (SUCCEEDED(WMIHelperInitialize())) {
        WMIhelperGetIonicNetAdapters(fs2);
        if (bLbfo) {
            std::wofstream fs3("DebugInfo\\LbfoInfo.txt");
            WMIhelperGetLbfoInfo(fs3);
            fs3 << std::endl << std::flush;
        }
        WMIHelperRelease();
    }
    fs2 << std::endl << std::flush;

    std::cout << "Collecting Trace Logs" << std::endl;
    std::ofstream fs4("DebugInfo\\IonicTraceLog.txt");
    GetTrace(info, fs4);
    fs4 << std::endl << std::flush;
    std::cout << "Finished collecting Trace Logs" << std::endl;

    // should we get hidden stats?
    std::cout << "Collecting Stats" << std::endl;
    std::ofstream fs5("DebugInfo\\IonicStats.txt");
    std::cout << "Collecting DevStats" << std::endl;
    DevStats(info, fs5);
    std::cout << "Collecting LifStats" << std::endl;
    LifStats(info, 0, fs5);
    std::cout << "Collecting PortStats" << std::endl;
    PortStats(info, fs5);
    std::cout << "Collecting PerfStats" << std::endl;
    PerfStats(info, fs5);
    std::cout << "Collecting MgmtStats" << std::endl;
    MgmtStats(info, fs5);
    fs5 << std::endl << std::flush;
    std::cout << std::endl << "Finished collecting Ionic Stats" << std::endl;

    return info.status;
}

command
CmdCollectDbgInfo()
{
    command cmd;

    cmd.name = "CollectDbgInfo";
    cmd.desc = "Collect debug information logs in \"DebugInfo\" folder";

    cmd.opts = CmdCollectDbgInfoOpts;
    cmd.run = CmdCollectDbgInfoRun;

    return cmd;
}
