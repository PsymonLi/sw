//
//  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
//

#include "pdsa_uds_hdlr.h"
#include "nic/vpp/infra/ipc/uds_internal.h"
#include "nic/sdk/include/sdk/uds.hpp"
#include <gen/proto/types.pb.h>
#include <gen/proto/debug.pb.h>
#include <gen/proto/session.pb.h>
#include <nic/sdk/include/sdk/table.hpp>
#include <gen/p4gen/p4/include/ftl.h>
#include <lib/table/ftl/ftl_base.hpp>
#include <ftl_utils.hpp>
#include <nic/p4/common/defines.h>
#include <sys/types.h>

typedef struct ftl_cb_data_s {
    int io_fd;
    int sock_fd;
    bool summary;
} ftl_cb_data_t;

uint32_t ftlv4_entry_count;
bool g_ftl_skip_walk = false;

static void
ftlv4_entry_iter_cb (sdk::table::sdk_table_api_params_t *params)
{
    ipv4_flow_hash_entry_t *hwentry = (ipv4_flow_hash_entry_t *) params->entry;
    ftl_cb_data_t *cbdata = (ftl_cb_data_t *)(params->cbdata);
    int fd = cbdata->io_fd;
    bool summary = cbdata->summary;
    struct in_addr src, dst;
    char srcstr[INET_ADDRSTRLEN + 1], dststr[INET_ADDRSTRLEN + 1];

    if (!g_ftl_skip_walk && hwentry->get_entry_valid()) {
        ftlv4_entry_count++;
        if (summary) {
            return;
        }
        uint32_t ses_id = hwentry->get_session_index();
        bool drop = pds_flow_get_session_drop(ses_id);
        bool from_host = pds_flow_get_flow_from_host(ses_id,
                                                     hwentry->get_flow_role());
        src.s_addr = ntohl(hwentry->get_key_metadata_ipv4_src());
        dst.s_addr = ntohl(hwentry->get_key_metadata_ipv4_dst());
        inet_ntop(AF_INET, &src, srcstr, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &dst, dststr, INET_ADDRSTRLEN);
        dprintf(fd, "%-8d%s/%-8s%-6d%-20s%-10d%-20s%-12d%-7s%-8s\n",
                ses_id,
                hwentry->get_flow_role() ? "R" : "I",
                from_host ? "H" : "U",
                ftlv4_get_lookup_id(hwentry),
                srcstr,
                hwentry->get_key_metadata_sport(),
                dststr,
                hwentry->get_key_metadata_dport(),
                pds_ip_protocol_to_str(hwentry->get_key_metadata_proto()),
                drop ? "D" : "A");
        if ((0 == (ftlv4_entry_count % UDS_SOCK_ALIVE_CHECK_COUNT)) &&
            !is_uds_socket_alive(cbdata->sock_fd)) {
            g_ftl_skip_walk = true;
        }
    }
}

void
ftlv6_entry_iter_cb(sdk::table::sdk_table_api_params_t *params)
{
    flow_hash_entry_t *hwentry = (flow_hash_entry_t *) params->entry;
    int *fd = (int *)params->cbdata;
    uint8_t src[16], dst[16];

    if (hwentry->get_entry_valid()) {
        types::ServiceResponseMessage proto_rsp;
        google::protobuf::Any *any_resp = proto_rsp.mutable_response();
        pds::FlowMsg flow_msg = pds::FlowMsg();
        pds::Flow *flow = flow_msg.mutable_flowentry();
        types::IPFlowKey *ipflowkey = flow->mutable_key()->mutable_ipflowkey();

        ipflowkey->mutable_l4info()->mutable_tcpudpinfo()->set_srcport(hwentry->get_key_metadata_sport());
        ipflowkey->mutable_l4info()->mutable_tcpudpinfo()->set_dstport(hwentry->get_key_metadata_dport());
        ipflowkey->set_ipprotocol(hwentry->get_key_metadata_proto());
        ipflowkey->mutable_srcip()->set_af(types::IPAF::IP_AF_INET6);
        hwentry->get_key_metadata_src(src);
        ipflowkey->mutable_srcip()->set_v6addr((const char *)src);
        ipflowkey->mutable_dstip()->set_af(types::IPAF::IP_AF_INET6);
        hwentry->get_key_metadata_dst(dst);
        ipflowkey->mutable_dstip()->set_v6addr((const char *)dst);

        flow->set_sessionidx(hwentry->get_session_index());
        flow->set_flowrole(hwentry->get_flow_role());
        // Epoch is not present in apollo pipeline so commenting out for now
        // flow->set_epoch(hwentry->get_epoch());

        flow_msg.set_flowentrycount(1);
        any_resp->PackFrom(flow_msg);
        proto_rsp.set_apistatus(types::ApiStatus::API_STATUS_OK);
        proto_rsp.SerializeToFileDescriptor(*fd);
    }
}

// Callback to dump flow entries via UDS
static void
vpp_uds_flow_dump(int sock_fd, int io_fd, bool summary)
{
    ftl_cb_data_t cbdata;
    types::ServiceResponseMessage proto_rsp;
    sdk::table::sdk_table_api_params_t params = {0};
    sdk::table::ftl_base *table4 =
        (sdk::table::ftl_base *)pds_flow_get_table4();
    //sdk::table::ftl_base *table6 =
        //(sdk::table::ftl_base *)pds_flow_get_table6_or_l2();

    cbdata.io_fd = io_fd;
    cbdata.sock_fd = sock_fd;
    cbdata.summary = summary;

    ftlv4_entry_count = 0;
    g_ftl_skip_walk = false;
    params.itercb = ftlv4_entry_iter_cb;
    params.cbdata = &cbdata;
    params.force_hwread = false;
    table4->iterate(&params);

    dprintf(io_fd, "No. of flows: %d\n", ftlv4_entry_count);
    // disable this until v6/l2 packet corruption is solved
    //params.itercb = ftlv6_entry_iter_cb;
    //params.cbdata = &fd;
    //params.force_hwread = false;
    //table6->iterate(&params);

    close(io_fd);
}

// initializes callbacks for flow dump
void
pds_flow_dump_init(void)
{
    vpp_uds_register_cb(VPP_UDS_FLOW_DUMP, vpp_uds_flow_dump);
}
