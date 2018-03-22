/* 
 * ----------------------------------------------------------------------------
 *
 * interface_api.hpp
 *
 * Interface APIs exported by PI to PD.
 *
 * ----------------------------------------------------------------------------
 */
#ifndef __INTERFACE_API_HPP__
#define __INTERFACE_API_HPP__

#include "nic/hal/src/nw/interface.hpp"
#include "nic/gen/proto/hal/interface.pb.h"
#include "nic/hal/src/nw/l2segment.hpp"
#include "nic/hal/src/aclqos/qos.hpp"

namespace hal {

using hal::lif_t;
using hal::if_t;
using hal::l2seg_t;
using hal::qos_class_t;

// LIF APIs
uint32_t lif_get_lif_id(lif_t *pi_lif)  __attribute__((used));
uint8_t lif_get_qtype(lif_t *pi_lif, intf::LifQPurpose purpose) __attribute__((used));
void lif_set_pd_lif(lif_t *pi_lif, void *pd_lif);
void *lif_get_pd_lif(lif_t *pi_lif);
bool lif_get_enable_rdma(lif_t *pi_lif);
void lif_set_enable_rdma(lif_t *pi_lif, bool enable_rdma);
uint32_t lif_get_total_qcount (uint32_t hw_lif_id);
qos_class_t *lif_get_rx_qos_class(lif_t *pi_lif);
qos_class_t *lif_get_tx_qos_class(lif_t *pi_lif);

// Interface APIs
intf::IfType intf_get_if_type(if_t *pi_if);
uint32_t if_get_if_id(if_t *pi_if);
hal_handle_t if_get_hal_handle(if_t *pi_if);
uint32_t uplinkif_get_port_num(if_t *pi_if);
void if_set_pd_if(if_t *pi_if, void *pd_upif);
void *if_get_pd_if(if_t *pi_if);
bool is_l2seg_native(l2seg_t *l2seg, if_t *pi_if);
lif_t *if_get_lif(if_t *pi_if);
hal_ret_t if_l2seg_get_encap(if_t *pi_if, l2seg_t *pi_l2seg, 
                                     uint8_t *vlan_v, uint16_t *vlan_id);
uint32_t if_allocate_hwlif_id();

// Interface APIs for EnicIf
intf::IfEnicType if_get_enicif_type(if_t *pi_if);
vlan_id_t if_get_encap_vlan(if_t *pi_if);
mac_addr_t *if_get_mac_addr(if_t *pi_if);
void *if_enicif_get_pd_l2seg(if_t *pi_if);
void *if_enicif_get_pi_l2seg(if_t *pi_if);
void *if_enicif_get_pd_nwsec(if_t *pi_if);
void *if_enicif_get_pi_nwsec(if_t *pi_if);
uint32_t if_enicif_get_ipsg_en(if_t *pi_if);
hal_ret_t if_enicif_get_pinned_if(if_t *pi_if,
                                  if_t **uplink_if);
hal_ret_t if_enicif_get_native_l2seg_clsc_vlan(if_t *pi_if, 
                                               uint32_t *vlan_id);

//TODO Remove this when the above function works for all cases.
uint8_t
if_enicif_get_host_pinned_uplink(if_t *pi_if);

} // namespace hal
#endif // __INTERFACE_API_HPP__
