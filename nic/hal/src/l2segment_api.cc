// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "nic/hal/src/l2segment.hpp"
#include "nic/include/l2segment_api.hpp"
#include "nic/hal/src/nwsec.hpp"

namespace hal {


// ----------------------------------------------------------------------------
// Returns L2 segment PD
// ----------------------------------------------------------------------------
void *
l2seg_get_pd(l2seg_t *seg)
{
    return seg->pd;
}

// ----------------------------------------------------------------------------
// Returns L2 segment ID
// ----------------------------------------------------------------------------
l2seg_id_t
l2seg_get_l2seg_id(l2seg_t *seg)
{
    return seg->seg_id;
}

// ----------------------------------------------------------------------------
// Returns access encap type
// ----------------------------------------------------------------------------
types::encapType 
l2seg_get_acc_encap_type(l2seg_t *seg)
{
    return seg->access_encap.type;
}

// ----------------------------------------------------------------------------
// Returns access encap value
// ----------------------------------------------------------------------------
uint32_t 
l2seg_get_acc_encap_val(l2seg_t *seg)
{
    return seg->access_encap.val;
}

// ----------------------------------------------------------------------------
// Returns fabric encap type
// ----------------------------------------------------------------------------
types::encapType 
l2seg_get_fab_encap_type(l2seg_t *seg)
{
    return seg->fabric_encap.type;
}

// ----------------------------------------------------------------------------
// Returns fabric encap value
// ----------------------------------------------------------------------------
uint32_t 
l2seg_get_fab_encap_val(l2seg_t *seg)
{
    return seg->fabric_encap.val;
}

// ----------------------------------------------------------------------------
// Returns nwsec for the l2segment
// ----------------------------------------------------------------------------
void *
l2seg_get_pi_nwsec(l2seg_t *l2seg)
{
    tenant_t            *pi_tenant = NULL;
    nwsec_profile_t     *pi_nwsec = NULL;

    // Check if if is enicif
    pi_tenant = tenant_lookup_by_handle(l2seg->tenant_handle);
    HAL_ASSERT_RETURN(pi_tenant != NULL, NULL);
    pi_nwsec = find_nwsec_profile_by_handle(pi_tenant->nwsec_profile_handle);
    if (!pi_nwsec) {
        return NULL;
    }
    return pi_nwsec;
}

// ----------------------------------------------------------------------------
// Returns ipsg_en for l2seg
// ----------------------------------------------------------------------------
uint32_t
l2seg_get_ipsg_en(l2seg_t *pi_l2seg)
{
    nwsec_profile_t     *pi_nwsec = NULL;

    pi_nwsec = (nwsec_profile_t *)l2seg_get_pi_nwsec(pi_l2seg);
    if (!pi_nwsec) {
        return 0;
    }

    return pi_nwsec->ipsg_en;
}

// ----------------------------------------------------------------------------
// Returns infra L2 segment
// ----------------------------------------------------------------------------
l2seg_t *
l2seg_get_infra_l2seg()
{
    return (l2seg_t *)g_hal_state->infra_l2seg();
}

// ----------------------------------------------------------------------------
// Returns bcast_fwd_policy
// ----------------------------------------------------------------------------
uint32_t
l2seg_get_bcast_fwd_policy(l2seg_t *pi_l2seg)
{
    return (uint32_t)pi_l2seg->bcast_fwd_policy;
}

// ----------------------------------------------------------------------------
// Returns bcast_oif_list
// ----------------------------------------------------------------------------
oif_list_id_t
l2seg_get_bcast_oif_list(l2seg_t *pi_l2seg)
{
    return pi_l2seg->bcast_oif_list;
}

}
