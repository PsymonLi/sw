//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __IF_UTILS_HPP__
#define __IF_UTILS_HPP__

#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment.hpp"
#include "nic/hal/plugins/cfg/nw/session.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/cfg/nw/nw.hpp"
#include "nic/hal/plugins/cfg/nw/nh.hpp"
#include "nic/hal/plugins/cfg/nw/filter.hpp"
#include "nic/hal/plugins/cfg/nw/route.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include "nic/hal/plugins/cfg/lif/lif.hpp"
#include "nic/hal/plugins/cfg/aclqos/acl.hpp"
#include "gen/proto/types.pb.h"
#include "nic/sdk/include/sdk/ip.hpp"

namespace hal {

hal_ret_t pltfm_get_port_from_front_port_num(uint32_t fp_num,
                                             uint32_t *port_num);
// vrf
hal_ret_t vrf_handle_nwsec_update(vrf_t *vrf, nwsec_profile_t *nwsec_prof);
hal_ret_t vrf_add_l2seg(vrf_t *vrf, l2seg_t *l2seg);
hal_ret_t vrf_del_l2seg(vrf_t *vrf, l2seg_t *l2seg);
hal_ret_t vrf_add_acl(vrf_t *vrf, acl_t *acl);
hal_ret_t vrf_del_acl(vrf_t *vrf, acl_t *acl);
hal_ret_t vrf_add_route(vrf_t *vrf, route_t *route);
hal_ret_t vrf_del_route(vrf_t *vrf, route_t *route);

// l2seg
hal_ret_t l2seg_handle_nwsec_update(l2seg_t *l2seg,
                                    nwsec_profile_t *nwsec_prof);
hal_ret_t l2seg_add_if(l2seg_t *l2seg, if_t *hal_if);
hal_ret_t l2seg_del_if(l2seg_t *l2seg, if_t *hal_if);
hal_ret_t l2seg_add_back_if(l2seg_t *l2seg, if_t *hal_if);
hal_ret_t l2seg_del_back_if(l2seg_t *l2seg, if_t *hal_if);
hal_ret_t l2seg_add_acl(l2seg_t *l2seg, acl_t *acl);
hal_ret_t l2seg_del_acl(l2seg_t *l2seg, acl_t *acl);

// if
hal_ret_t if_handle_nwsec_update(l2seg_t *l2seg, if_t *hal_if,
                                 nwsec_profile_t *nwsec_prof);
hal_ret_t if_add_acl(if_t *hal_if, acl_t *acl, if_acl_ref_type_t type);
hal_ret_t if_del_acl(if_t *hal_if, acl_t *acl, if_acl_ref_type_t type);
hal_ret_t if_add_l2seg(if_t *hal_if, l2seg_t *l2seg);
hal_ret_t if_del_l2seg(if_t *hal_if, l2seg_t *l2seg);
hal_ret_t if_add_nh(if_t *hal_if, nexthop_t *nh);
hal_ret_t if_del_nh(if_t *hal_if, nexthop_t *nh);
hal_ret_t if_add_ep(if_t *hal_if, ep_t *ep);
hal_ret_t if_del_ep(if_t *hal_if, ep_t *ep);
hal_ret_t uplink_add_enicif(if_t *uplink, if_t *enic_if);
hal_ret_t uplink_del_enicif(if_t *uplink, if_t *enic_if);
hal_ret_t if_handle_lif_update(pd::pd_if_lif_update_args_t *args);

// lif
hal_ret_t lif_add_if(lif_t *lif, if_t *hal_if);
hal_ret_t lif_del_if(lif_t *lif, if_t *hal_if);
hal_ret_t lif_add_filter (lif_t *lif, filter_t *filter);
hal_ret_t lif_del_filter (lif_t *lif, filter_t *filter);

// ep
hal_ret_t ep_add_nh(ep_t *ep, nexthop_t *nh);
hal_ret_t ep_del_nh(ep_t *ep, nexthop_t *nh);

// nwsec prof
hal_ret_t nwsec_prof_add_vrf(nwsec_profile_t *nwsec, vrf_t *vrf);
hal_ret_t nwsec_prof_del_vrf(nwsec_profile_t *nwsec, vrf_t *vrf);

// network
hal_ret_t network_add_l2seg(network_t *nw, l2seg_t *l2seg);
hal_ret_t network_del_l2seg(network_t *nw, l2seg_t *l2seg);
hal_ret_t network_add_session(network_t *nw, session_t *sess);
hal_ret_t network_del_session(network_t *nw, session_t *sess);
hal_ret_t add_nw_to_security_group(uint32_t sg_id, hal_handle_t nw_handle_id);
hal_ret_t del_nw_from_security_group(uint32_t sg_id, hal_handle_t nw_handle_id);

// qos
hal_ret_t qos_class_add_lif_rx(qos_class_t *qos_class, lif_t *lif);
hal_ret_t qos_class_del_lif_rx(qos_class_t *qos_class, lif_t *lif);
hal_ret_t qos_class_add_lif_tx(qos_class_t *qos_class, lif_t *lif);
hal_ret_t qos_class_del_lif_tx(qos_class_t *qos_class, lif_t *lif);

//copp
hal_ret_t copp_add_acl(copp_t *copp, acl_t *acl);
hal_ret_t copp_del_acl(copp_t *copp, acl_t *acl);

// nexthop
hal_ret_t nexthop_add_route (nexthop_t *nh, route_t *route);
hal_ret_t nexthop_del_route (nexthop_t *nh, route_t *route);

}    // namespace hal

#endif    // __IF_UTILS_HPP__

