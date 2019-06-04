#include "egress.h"
#include "EGRESS_p.h"
#include "EGRESS_mirror_k.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nw.h"

struct mirror_k_ k;
struct mirror_d  d;
struct phv_      p;

%%

nop:
  nop.e
  nop

.align
local_span:
  seq           c1, k.vlan_tag_valid, 1
  phvwr.c1      p.vlan_tag_valid, 0
  phvwr.c1      p.{vlan_tag_pcp...vlan_tag_etherType}, 0
  phvwrpair     p.p4_to_p4plus_mirror_session_id, \
                    k.capri_intrinsic_tm_span_session, \
                    p.p4_to_p4plus_mirror_original_len[13:0], \
                    k.capri_p4_intrinsic_packet_len
  phvwr         p.p4_to_p4plus_mirror_lif, k.capri_intrinsic_lif

  add           r1, r0, d.u.local_span_d.truncate_len
  phvwr         p.control_metadata_dst_lport, d.u.local_span_d.dst_lport
  seq           c1, d.u.local_span_d.qid_en, TRUE
  phvwr.c1      p.control_metadata_qid, d.u.local_span_d.qid
  sne           c7, r0, r0
  bne           r1, r0, span_truncation
  phvwrpair     p.rewrite_metadata_tunnel_rewrite_index, 0, \
                    p.control_metadata_dest_tm_oq[4:0], \
                    d.u.local_span_d.span_tm_oq
  phvwr.e       p.capri_intrinsic_tm_span_session, 0
  nop

.align
remote_span:
  seq           c1, k.vlan_tag_valid, 1
  phvwr.c1      p.vlan_tag_valid, 0
  phvwr.c1      p.{vlan_tag_pcp...vlan_tag_etherType}, 0
  add           r1, r0, d.u.remote_span_d.truncate_len
  phvwr         p.control_metadata_dst_lport, d.u.remote_span_d.dst_lport
  phvwrpair     p.rewrite_metadata_tunnel_rewrite_index[9:0], \
                    d.u.remote_span_d.tunnel_rewrite_index, \
                p.control_metadata_dest_tm_oq[4:0], \
                    d.u.remote_span_d.span_tm_oq
  sne           c7, r0, r0
  bne           r1, r0, span_truncation
  phvwr         p.rewrite_metadata_tunnel_vnid, d.u.remote_span_d.vlan
  phvwr.e       p.capri_intrinsic_tm_span_session, 0
  nop

.align
erspan_mirror:
  seq           c1, k.vlan_tag_valid, 1
  phvwr.c1      p.vlan_tag_valid, 0
  phvwr.c1      p.{vlan_tag_pcp...vlan_tag_etherType}, 0
  phvwrpair.c1  p.erspan_t3_vlan, \
                    k.vlan_tag_vid, \
                    p.erspan_t3_cos, k.vlan_tag_pcp
  phvwr         p.erspan_t3_span_id, k.capri_intrinsic_tm_span_session
  add           r1, r0, d.u.erspan_mirror_d.truncate_len
  phvwr         p.control_metadata_dst_lport, d.u.erspan_mirror_d.dst_lport
  seq           c7, r0, r0
  bne           r1, r0, span_truncation
  phvwrpair     p.rewrite_metadata_tunnel_rewrite_index[9:0], \
                    d.u.erspan_mirror_d.tunnel_rewrite_index, \
                p.control_metadata_dest_tm_oq[4:0], \
                    d.u.erspan_mirror_d.span_tm_oq
  phvwr.e       p.capri_intrinsic_tm_span_session, 0
  nop

.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
drop_mirror:
  phvwr         p.control_metadata_dst_lport, 0
  phvwr         p.capri_intrinsic_tm_span_session, 0
  phvwr.e       p.capri_intrinsic_drop, TRUE
  phvwr         p.control_metadata_egress_drop_reason[EGRESS_DROP_MIRROR], 1

span_truncation:
  sub           r2, k.capri_p4_intrinsic_packet_len, 14
  slt           c1, r1, r2
  nop.!c1.e
  phvwr         p.capri_intrinsic_tm_span_session, 0
  add           r3, r1, 14
  phvwr         p.capri_p4_intrinsic_packet_len, r3
  phvwr.c7      p.erspan_t3_truncated, TRUE
  phvwr.e       p.capri_deparser_len_trunc_pkt_len, r1
  phvwr         p.{capri_intrinsic_payload,capri_deparser_len_trunc}, 0x1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
mirror_error:
  nop.e
  nop
