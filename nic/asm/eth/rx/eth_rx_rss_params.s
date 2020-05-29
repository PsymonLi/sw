
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_eth_rx_rss_params_k.h"

#include "defines.h"

struct phv_ p;
struct eth_rx_rss_params_k_ k;
struct eth_rx_rss_params_eth_rx_rss_params_d d;

%%

.param  eth_rx_rss_skip

.align
eth_rx_rss_params:
#ifdef PHV_DEBUG
  seq                 c7, d.debug, 1
  phvwr.c7            p.p4_intr_global_debug_trace, 1
  trace.c7            0x1
#endif

  bbeq                k.p4_to_p4plus_rss_override, 1, eth_rx_rss_none
  and                 r1, d.rss_type, k.p4_to_p4plus_pkt_type
  beq                 r1, r0, eth_rx_rss_none
  nop

  // Write RSS key to PHV
  phvwr               p.{toeplitz_key0_data, toeplitz_key1_data}, d.rss_key[319:64]
  phvwr               p.toeplitz_key2_data[127:64], d.rss_key[63:0]

  indexb              r7, r1, [RSS_IPV4_UDP, RSS_IPV4_TCP, RSS_IPV4, RSS_NONE], 0
  indexb              r7, r1, [RSS_IPV6_UDP, RSS_IPV6_TCP, RSS_IPV6], 1
  seq                 c7, r7, ~0x0
  indexb.c7           r7, r1, [PKT_TYPE_IPV4_UDP, PKT_TYPE_IPV4_TCP, 0xff, 0], 0
  indexb.c7           r7, r1, [PKT_TYPE_IPV6_UDP, PKT_TYPE_IPV6_TCP, 0], 1

.brbegin
  br                  r7[2:0]
  nop

  .brcase 0
    b                   eth_rx_rss_none
    nop

   // Clear all table valid bits so the topelitz table can launch in next stage
  .brcase 1
    b                   eth_rx_rss_ipv4
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 2
    b                   eth_rx_rss_ipv4_l4
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 3
    b                   eth_rx_rss_ipv4_l4
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 4
    b                   eth_rx_rss_ipv6
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 5
    b                   eth_rx_rss_ipv6_l4
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 6
    b                   eth_rx_rss_ipv6_l4
    phvwri              p.{app_header_table0_valid...app_header_table3_valid}, 0

  .brcase 7
    b                   eth_rx_rss_none
    nop

.brend

eth_rx_rss_none:
  phvwri              p.{app_header_table0_valid...app_header_table3_valid}, (1 << 3)
  phvwr               p.common_te0_phv_table_lock_en, 0
  phvwri.e            p.common_te0_phv_table_pc, eth_rx_rss_skip[38:6]
  phvwri.f            p.common_te0_phv_table_raw_table_size, CAPRI_RAW_TABLE_SIZE_MPU_ONLY

// Write RSS input to PHV
eth_rx_rss_ipv4_l4:
  phvwr               p.toeplitz_input0_data[63:32], k.{p4_to_p4plus_l4_sport, p4_to_p4plus_l4_dport}
eth_rx_rss_ipv4:
#if defined(APOLLO) || defined(APULU) || defined(ARTEMIS) || defined(ATHENA)
  phvwr               p.toeplitz_input0_data[127:96], k.p4_to_p4plus_ip_sa_s88_e127[31:0]
  phvwr.e             p.toeplitz_input0_data[95:64], k.p4_to_p4plus_ip_da[31:0]
#else
  phvwr               p.toeplitz_input0_data[127:72], k.p4_to_p4plus_ip_sa_s0_e87[55:0]
  phvwr.e             p.toeplitz_input0_data[71:64], k.p4_to_p4plus_ip_sa_s88_e127[39:32]
#endif
  phvwr.f             p.toeplitz_key2_data[33:0], k.p4_rxdma_intr_qstate_addr

eth_rx_rss_ipv6_l4:
  phvwr               p.toeplitz_input2_data, k.{p4_to_p4plus_l4_sport, p4_to_p4plus_l4_dport}
eth_rx_rss_ipv6:
#if defined(APULU)
  phvwr               p.toeplitz_input0_data[127:16], k.p4_to_p4plus_ip_sa_s0_e111
  phvwr               p.toeplitz_input0_data[15:0], k.p4_to_p4plus_ip_sa_s88_e127[39:23]
#else
  phvwr               p.toeplitz_input0_data[127:40], k.p4_to_p4plus_ip_sa_s0_e87
  phvwr               p.toeplitz_input0_data[39:0], k.p4_to_p4plus_ip_sa_s88_e127
#endif
  phvwr.e             p.toeplitz_input1_data, k.p4_to_p4plus_ip_da
  phvwr.f             p.toeplitz_key2_data[33:0], k.p4_rxdma_intr_qstate_addr
