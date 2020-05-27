#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_input_properties_k.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nw.h"

struct input_properties_k_  k;
struct input_properties_d   d;
struct phv_                 p;

%%

input_properties:
    // if table lookup is miss, return
    nop.!c1.e
    or              r1, d.input_properties_d.vrf, \
                        d.input_properties_d.src_lport, 16
    phvwr           p.{control_metadata_src_lport, flow_lkp_metadata_lkp_vrf}, r1
    phvwr           p.control_metadata_dst_lport, d.input_properties_d.dst_lport

    or              r2, d.input_properties_d.mirror_on_drop_session_id, \
                        d.input_properties_d.mirror_on_drop_en, 8
    phvwr           p.{control_metadata_mirror_on_drop_en, \
                        control_metadata_mirror_on_drop_session_id}, r2

    phvwr           p.capri_intrinsic_tm_replicate_ptr, \
                        d.input_properties_d.flow_miss_idx
    phvwr           p.control_metadata_flow_miss_qos_class_id, \
                        d.input_properties_d.flow_miss_qos_class_id

    or              r1, d.input_properties_d.reg_mac_vrf, \
                        d.{input_properties_d.ipsg_enable, \
                        input_properties_d.clear_promiscuous_repl}, 16
    phvwr           p.{control_metadata_ipsg_enable, \
                        control_metadata_clear_promiscuous_repl, \
                        flow_lkp_metadata_lkp_reg_mac_vrf}, r1

    phvwr           p.control_metadata_mseg_bm_bc_repls, \
                        d.input_properties_d.mseg_bm_bc_repls
    phvwr           p.control_metadata_mseg_bm_mc_repls, \
                        d.input_properties_d.mseg_bm_mc_repls

    or             r1, d.{input_properties_d.if_label_check_en, \
                       input_properties_d.if_label_check_fail_drop}, \
                       d.input_properties_d.mdest_flow_miss_action, 3
    phvwrm         p.{control_metadata_mdest_flow_miss_action, \
                       control_metadata_registered_mac_miss, \
                       control_metadata_if_label_check_en, \
                       control_metadata_if_label_check_fail_drop}, r1, 0x1b

    or              r1, d.input_properties_d.bounce_vnid_sbit20_ebit23, \
                        d.input_properties_d.bounce_vnid_sbit0_ebit19, 4
    phvwr           p.rewrite_metadata_tunnel_vnid, r1
    phvwr           p.rewrite_metadata_rewrite_index, \
                        d.input_properties_d.rewrite_index
    phvwr           p.rewrite_metadata_tunnel_rewrite_index, \
                        d.input_properties_d.tunnel_rewrite_index

    phvwr           p.flow_lkp_metadata_lkp_dir, d.input_properties_d.dir
    phvwr.e         p.l4_metadata_profile_idx, d.input_properties_d.l4_profile_idx
    phvwr.f         p.{control_metadata_flow_learn, \
                        control_metadata_src_if_label, \
                        control_metadata_uuc_fl_pe_sup_en, \
                        control_metadata_has_prom_host_lifs}, \
                        d.{input_properties_d.flow_learn, \
                        input_properties_d.src_if_label, \
                        input_properties_d.uuc_fl_pe_sup_en, \
                        input_properties_d.has_prom_host_lifs}

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
input_properties_error:
    nop.e
    nop
