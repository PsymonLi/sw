/*
    NOTE: Do not move these defines, because these need to be defined
          before including the common p4 defines.
*/

#include "../common-p4+/common_rxdma_dummy.p4"

#define common_p4plus_stage0_app_header_table_action_dummy eth_rx_app_header
#define rx_table_s0_t0_action eth_rx_app_header
// The following tables & their respective actions are defined in common-p4+
// Stage0 - table=eth_rx_rss_params
// Stage1 - table=eth_rx_rss_indir
#define rx_table_s1_t0_action eth_rx_rss_skip
#define rx_table_s2_t0_action eth_rx_fetch_desc
#define rx_table_s3_t0_action eth_rx_packet
#define rx_table_s4_t0_action eth_rx_completion

#include "../common-p4+/common_rxdma.p4"
#include "eth_rxdma.p4"
#include "defines.h"

/******************************************************************************
 * Action functions
 * - These action functions are used to generate k+i and d structures.
 *****************************************************************************/

action eth_rx_app_header(PARAMS_ETH_RX_QSTATE)
{
    // --- For K+I struct generation

    // K: From Intrinsic & RXDMA headers
    modify_field(p4_intr_global_scratch.lif, p4_intr_global.lif);
    modify_field(p4_intr_global_scratch.tm_iq, p4_intr_global.tm_iq);
    modify_field(p4_rxdma_intr_scratch.qid, p4_rxdma_intr.qid);
    modify_field(p4_rxdma_intr_scratch.qtype, p4_rxdma_intr.qtype);
    modify_field(p4_rxdma_intr_scratch.qstate_addr, p4_rxdma_intr.qstate_addr);

    // I: From APP header
    modify_field(p4_to_p4plus_scratch.p4plus_app_id, p4_to_p4plus.p4plus_app_id);
    modify_field(p4_to_p4plus_scratch.table0_valid, p4_to_p4plus.table0_valid);
    modify_field(p4_to_p4plus_scratch.table1_valid, p4_to_p4plus.table1_valid);
    modify_field(p4_to_p4plus_scratch.table2_valid, p4_to_p4plus.table2_valid);
    modify_field(p4_to_p4plus_scratch.table3_valid, p4_to_p4plus.table3_valid);

    modify_field(p4_to_p4plus_scratch.vlan_pcp, p4_to_p4plus.vlan_pcp);
    modify_field(p4_to_p4plus_scratch.vlan_dei, p4_to_p4plus.vlan_dei);
    modify_field(p4_to_p4plus_scratch.vlan_vid, p4_to_p4plus.vlan_vid);
    modify_field(p4_to_p4plus_scratch.vlan_valid, p4_to_p4plus.vlan_valid);
    modify_field(p4_to_p4plus_scratch.packet_len, p4_to_p4plus.packet_len);
    modify_field(p4_to_p4plus_scratch.csum, p4_to_p4plus.csum);
    modify_field(p4_to_p4plus_scratch.csum_ip_bad, p4_to_p4plus.csum_ip_bad);
    modify_field(p4_to_p4plus_scratch.csum_ip_ok, p4_to_p4plus.csum_ip_ok);
    modify_field(p4_to_p4plus_scratch.csum_udp_bad, p4_to_p4plus.csum_udp_bad);
    modify_field(p4_to_p4plus_scratch.csum_udp_ok, p4_to_p4plus.csum_udp_ok);
    modify_field(p4_to_p4plus_scratch.csum_tcp_bad, p4_to_p4plus.csum_tcp_bad);
    modify_field(p4_to_p4plus_scratch.csum_tcp_ok, p4_to_p4plus.csum_tcp_ok);

    modify_field(p4_to_p4plus_scratch.pkt_type, p4_to_p4plus.pkt_type);

    // --- For D-struct generation
    MODIFY_ETH_RX_QSTATE
}

action eth_rx_rss_skip()
{
    // --- For K+I struct generation

    MODIFY_ETH_RX_GLOBAL
    MODIFY_ETH_RX_T0_S2S

    // --- For D-struct generation
}

action eth_rx_fetch_desc(PARAMS_ETH_RX_QSTATE)
{
    // --- For K+I struct generation

    MODIFY_ETH_RX_GLOBAL
    MODIFY_ETH_RX_T0_S2S

    // --- For D-struct generation
    modify_field(eth_rx_qstate.pc, pc);
    MODIFY_ETH_RX_QSTATE
}

action eth_rx_packet(PARAM_RX_DESC())
{
    // --- For K+I struct generation

    MODIFY_ETH_RX_GLOBAL
    MODIFY_ETH_RX_T0_S2S

    // --- For D-struct generation
    MODIFY_RX_DESC()
}

action eth_rx_completion()
{
    // --- For K+I struct generation

    MODIFY_ETH_RX_GLOBAL
    MODIFY_ETH_RX_T0_S2S

    // --- For D-struct generation
}
