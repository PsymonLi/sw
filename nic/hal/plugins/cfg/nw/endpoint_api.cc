//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "nic/include/endpoint_api.hpp"
#include "nic/include/pd.hpp"
#include "nic/include/interface_api.hpp"
#include "nic/hal/plugins/cfg/nw/nw.hpp"
#include "nic/hal/src/utils/utils.hpp"

namespace hal {

l2seg_id_t
ep_get_l2segid (ep_t *pi_ep)
{
    return pi_ep->l2_key.l2_segid;
}

mac_addr_t *
ep_get_mac_addr (ep_t *pi_ep)
{
    return &(pi_ep->l2_key.mac_addr);
}

mac_addr_t *
ep_get_rmac (ep_t *pi_ep, l2seg_t *l2seg)
{
    network_t *nw;
    nw = ep_get_nw(pi_ep, l2seg);
    if (!nw) {
        HAL_ASSERT(0);
    }
    return (&nw->rmac_addr);
}

if_t *
ep_get_if (ep_t *pi_ep)
{
    return (find_if_by_handle(pi_ep->if_handle));
}

vrf_t *
ep_get_vrf (ep_t *pi_ep)
{
    return (vrf_lookup_by_handle(pi_ep->vrf_handle));
}

network_t *
ep_get_nw (ep_t *pi_ep, l2seg_t *l2seg)
{
    network_t                   *nw = NULL;
    ep_ip_entry_t               *pi_ip_entry = NULL;
    hal_handle_t                *p_hdl_id = NULL;
    dllist_ctxt_t               *lnode = NULL;

    if (dllist_empty(&pi_ep->ip_list_head)) {
        HAL_TRACE_ERR("Failed to find nw, no IPs present for EP");
        goto end;
    } else {
        lnode = pi_ep->ip_list_head.next;
        pi_ip_entry = (ep_ip_entry_t *)((char *)lnode - offsetof(ep_ip_entry_t,
                                                                 ep_ip_lentry));

        for (const void *ptr : *l2seg->nw_list) {
            p_hdl_id = (hal_handle_t *)ptr;
            nw = find_network_by_handle(*p_hdl_id);
            // Check if ip is in prefix
            if (ip_addr_in_ip_pfx(&pi_ip_entry->ip_addr, &nw->nw_key.ip_pfx)) {
                return nw;
            }
        }
    }

end:

    return nw;
}

//-----------------------------------------------------------------------------
// set PD EP in PI EP
//-----------------------------------------------------------------------------
void
ep_set_pd_ep (ep_t *pi_ep, pd::pd_ep_t *pd_ep)
{
    pi_ep->pd = pd_ep;
}

if_t *
ep_find_if_by_handle (ep_t *pi_ep)
{
    return find_if_by_handle(pi_ep->if_handle);
}

void *
ep_get_pd_ep (ep_t *pi_ep)
{
    return pi_ep->pd;
}

//------------------------------------------------------------------------------
// Check whether EP is remote.
//------------------------------------------------------------------------------
bool
is_ep_remote (const ep_t* dep)
{
    if_t *sif;

    sif = hal::find_if_by_handle(dep->if_handle);
    if (!sif || sif->if_type != intf::IF_TYPE_UPLINK) {
        HAL_TRACE_INFO("Source endpoint interface is not of type Uplink for proxy arp.");
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Check whether EP is management. EP is behing MNIC or management NICs.
//------------------------------------------------------------------------------
bool
is_ep_management (const ep_t* ep)
{
    lif_t *lif = NULL;

    // returns lif only if interface is ENIC.
    lif = find_lif_by_if_handle(ep->if_handle);
    if (lif) {
        return lif->is_management;
    }

    return false;
}

}    // namespace hal
