#include "egress.h"
#include "EGRESS_p.h"
#include "../../p4/nw/include/defines.h"

struct compute_checksum_k k;
struct compute_checksum_d d;
struct phv_               p;

%%

nop:
  nop.e
  nop

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum1:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv4_csum, TRUE
  phvwr.e     p.ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum2:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr.e     p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum3:
  phvwr.e     p.ipv4_csum, TRUE
  nop

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum4:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum5:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum6:
  phvwr.e     p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum7:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr       p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum8:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr       p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv4_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum9:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum10:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum11:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum12:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv6_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum13:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv4_csum, TRUE
  phvwr       p.ipv4_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv6_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum14:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr.e     p.ipv6_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum15:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr.e     p.ipv6_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum16:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum17:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum18:
  phvwr.e     p.inner_ipv4_csum, TRUE
  nop

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum19:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv6_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum20:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv6_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE
  phvwr.e     p.inner_ipv4_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum21:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.ipv6_udp_csum, TRUE
  phvwr.e     p.udp_csum, TRUE
  phvwr       p.inner_ipv4_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum22:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr       p.ipv6_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv6_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum23:
  phvwr       p.capri_deparser_len_l4_payload_len, k.udp_len
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr       p.ipv6_udp_csum, TRUE
  phvwr       p.udp_csum, TRUE
  phvwr.e     p.inner_ipv6_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum24:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, \
                k.l4_metadata_tcp_data_len
  phvwr.e     p.inner_ipv6_tcp_csum, TRUE
  phvwr       p.tcp_csum, TRUE

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
compute_checksum25:
  phvwr       p.capri_deparser_len_inner_l4_payload_len, k.inner_udp_len
  phvwr.e     p.inner_ipv6_udp_csum, TRUE
  phvwr       p.inner_udp_csum, TRUE
