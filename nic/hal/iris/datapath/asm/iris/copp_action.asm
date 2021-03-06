#include "egress.h"
#include "EGRESS_p.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "asic/cmn/asic_common.hpp"

struct copp_action_k k;
struct copp_action_d d;
struct phv_          p;

%%

copp_action:
  seq         c1, k.copp_metadata_policer_color, POLICER_COLOR_RED
  bcf         [c1], copp_deny
  add         r7, d.copp_action_d.permitted_packets, 1
  bgti        r7, 0xF, copp_permitted_stats_overflow
  tblwr       d.copp_action_d.permitted_packets, r7[3:0]
  tbladd.e    d.copp_action_d.permitted_bytes, \
                k.policer_metadata_packet_len
  nop

copp_permitted_stats_overflow:
  add         r7, d.copp_action_d.permitted_bytes, \
                k.policer_metadata_packet_len
  addi        r6, r0, 0x1000010
  or          r7, r7, r6, 32
  or          r7, r7, r5[31:27], 58

  add         r5, r5, k.copp_metadata_policer_index, 5
  addi        r6, r0, ASIC_MEM_SEM_ATOMIC_ADD_START
  add         r6, r6, r5[26:0]

  memwr.dx.e  r6, r7
  tblwr       d.copp_action_d.permitted_bytes, 0

copp_deny:
  add         r7, d.copp_action_d.denied_packets, 1
  bgti        r7, 0xF, copp_denied_stats_overflow
  tblwr       d.copp_action_d.denied_packets, r7[3:0]
  tbladd.e    d.copp_action_d.denied_bytes, \
                k.policer_metadata_packet_len
  nop

copp_denied_stats_overflow:
  add         r7, d.copp_action_d.denied_bytes, \
                k.policer_metadata_packet_len
  addi        r6, r0, 0x1000010
  or          r7, r7, r6, 32
  or          r7, r7, r5[31:27], 58

  add         r5, r5, k.copp_metadata_policer_index, 5
  add         r5, r5, 16
  addi        r6, r0, ASIC_MEM_SEM_ATOMIC_ADD_START
  add         r6, r6, r5[26:0]

  memwr.dx.e  r6, r7
  tblwr       d.copp_action_d.denied_bytes, 0

/*
 * stats allocation in the atomic add region:
 * 8B permit bytes, 8B permit packets, 8B deny bytes, 8B deby packets
 * total per policer index = 32B
 */

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
copp_action_error:
  nop.e
  nop
