#include "egress.h"
#include "EGRESS_p.h"
#include "../../p4/nw/include/defines.h"
#include "../../include/capri_common.h"

struct egress_policer_action_k k;
struct egress_policer_action_d d;
struct phv_                    p;

%%

egress_policer_action:
  seq         c1, k.policer_metadata_egress_policer_color, POLICER_COLOR_GREEN
  bcf         [!c1], egress_policer_deny
  add         r7, d.egress_policer_action_d.permitted_packets, 1
  bgti        r7, 0xF, egress_policer_permitted_stats_overflow
  tblwr       d.egress_policer_action_d.permitted_packets, r6[3:0]
  tbladd.e    d.egress_policer_action_d.permitted_bytes, k.control_metadata_packet_len
  nop

egress_policer_permitted_stats_overflow:
  add         r7, d.egress_policer_action_d.permitted_bytes, k.control_metadata_packet_len
  addi        r6, r0, 0x10000F
  or          r7, r7, r6, 32
  or          r7, r7, r5[31:27], 58

  add         r5, r5, k.policer_metadata_egress_policer_index, 5
  addi        r6, r0, CAPRI_MEM_SEM_ATOMIC_ADD_START
  add         r6, r6, r5[26:0]

  memwr.d.e   r6, r7
  tblwr       d.egress_policer_action_d.permitted_bytes, 0

egress_policer_deny:
  add         r7, d.egress_policer_action_d.denied_packets, 1
  bgti        r7, 0xF, egress_policer_denied_stats_overflow
  tblwr       d.egress_policer_action_d.denied_packets, r6[3:0]
  tbladd.e    d.egress_policer_action_d.denied_bytes, k.control_metadata_packet_len
  nop

egress_policer_denied_stats_overflow:
  add         r7, d.egress_policer_action_d.denied_bytes, k.control_metadata_packet_len
  addi        r6, r0, 0x10000F
  or          r7, r7, r6, 32
  or          r7, r7, r5[31:27], 58

  add         r5, r5, k.policer_metadata_egress_policer_index, 5
  add         r5, r5, 16
  addi        r6, r0, CAPRI_MEM_SEM_ATOMIC_ADD_START
  add         r6, r6, r5[26:0]

  memwr.d.e   r6, r7
  tblwr       d.egress_policer_action_d.denied_bytes, 0

/*
 * stats allocation in the atomic add region:
 * 8B permit bytes, 8B permit packets, 8B deny bytes, 8B deby packets
 * total per policer index = 32B
 */
