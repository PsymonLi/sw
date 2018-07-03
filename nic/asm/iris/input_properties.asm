#include "ingress.h"
#include "INGRESS_p.h"
#include "../../p4/iris/include/defines.h"
#include "nw.h"

struct input_properties_k k;
struct input_properties_d d;
struct phv_               p;

%%

input_properties:
  // if table lookup is miss, return
  K_DBG_WR(0x00)
  DBG_WR(0x08, r1)
  nop.!c1.e
  phvwr         p.control_metadata_nic_mode, r5[0]
  DBG_WR(0x09,  k.capri_intrinsic_lif_sbit0_ebit2)
  DBG_WR(0x0a,  k.capri_intrinsic_lif_sbit3_ebit10)
  DBG_WR(0x0b,  d.input_properties_d.dst_lport)
  DBG_WR(0x0c,  d.input_properties_d.src_lport)
  DBG_WR(0x0d,  d.input_properties_d.vrf)
  DBG_WR(0x0e,  d.input_properties_d.flow_miss_idx)
  phvwr         p.flow_lkp_metadata_lkp_dir, d.input_properties_d.dir
  phvwr         p.flow_lkp_metadata_lkp_vrf, d.input_properties_d.vrf
  phvwrpair     p.control_metadata_dst_lport[10:0], d.input_properties_d.dst_lport, \
                    p.control_metadata_src_lport[10:0], d.input_properties_d.src_lport
  phvwrpair     p.control_metadata_src_lif[10:8], k.capri_intrinsic_lif_sbit0_ebit2, \
                    p.control_metadata_src_lif[7:0], k.capri_intrinsic_lif_sbit3_ebit10
  phvwr         p.control_metadata_flow_miss_qos_class_id, \
                    d.input_properties_d.flow_miss_qos_class_id
  phvwrpair     p.control_metadata_ipsg_enable, d.input_properties_d.ipsg_enable, \
                    p.control_metadata_mdest_flow_miss_action, \
                    d.input_properties_d.mdest_flow_miss_action
  phvwr         p.control_metadata_allow_flood, d.input_properties_d.allow_flood
  phvwr         p.flow_miss_metadata_tunnel_vnid, d.input_properties_d.bounce_vnid
  phvwr         p.{control_metadata_mirror_on_drop_en, \
                   control_metadata_mirror_on_drop_session_id}, \
                    d.{input_properties_d.mirror_on_drop_en, \
                       input_properties_d.mirror_on_drop_session_id}
  phvwr         p.control_metadata_clear_promiscuous_repl, \
                    d.input_properties_d.clear_promiscuous_repl
  phvwr.e       p.control_metadata_flow_miss_idx, d.input_properties_d.flow_miss_idx
  phvwr.f       p.l4_metadata_profile_idx, d.input_properties_d.l4_profile_idx

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
input_properties_error:
  nop.e
  nop
