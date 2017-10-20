// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "nic/include/base.h"
#include "nic/gen/iris/include/p4pd.h"
#include "nic/hal/pd/iris/oif_list_pd_mgr.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/pd/iris/if_pd.hpp"
#include "nic/hal/pd/iris/if_pd_utils.hpp"
#include "hal_state_pd.hpp"

using namespace hal;

namespace hal {
namespace pd {

// Creates a new oif_list and returns handle
hal_ret_t oif_list_create(oif_list_id_t *list)
{
    return g_hal_state_pd->met_table()->create_repl_list(list);
}

// Takes an oiflis_handle and deletes it
hal_ret_t oif_list_delete(oif_list_id_t list)
{
    return g_hal_state_pd->met_table()->delete_repl_list(list);
}

// Adds an oif to list
hal_ret_t oif_list_add_oif(oif_list_id_t list, oif_t *oif)
{
    hal_ret_t ret;
    uint8_t is_tagged;
    uint16_t vlan_id;
    p4_replication_data_t data = { 0 };
    // if_t *pi_if = find_if_by_id(oif->if_id);
    // l2seg_t *pi_l2seg = find_l2seg_by_id(oif->l2_seg_id);
    if_t *pi_if = oif->intf;
    l2seg_t *pi_l2seg = oif->l2seg;

    HAL_ASSERT_RETURN(pi_if && pi_l2seg, HAL_RET_INVALID_ARG);

    ret = if_l2seg_get_encap(pi_if, pi_l2seg, &is_tagged, &vlan_id);

    if (ret != HAL_RET_OK) {
        return ret;
    }

    data.rewrite_index = g_hal_state_pd->rwr_tbl_decap_vlan_idx();

    if (is_tagged) {
        data.tunnel_rewrite_index = g_hal_state_pd->tnnl_rwr_tbl_encap_vlan_idx();
    } else {
        data.tunnel_rewrite_index = 0; // NOP. Theres no API in g_hal_state_pd for this.
    }

    data.lport = if_get_lport_id(pi_if);
    switch(hal::intf_get_if_type(pi_if)) {
        case intf::IF_TYPE_ENIC:
        {
            hal::lif_t *lif = if_get_lif(pi_if);
            if (lif == NULL){
                return HAL_RET_LIF_NOT_FOUND;
            }

            data.qtype = lif_get_qtype(lif, intf::LIF_QUEUE_PURPOSE_RX);
            data.qid_or_vnid = 0; // TODO refer to update_fwding_info()
            data.is_qid = 1;
            break;
        }
        case intf::IF_TYPE_UPLINK:
        case intf::IF_TYPE_UPLINK_PC:
        {
            data.qid_or_vnid = vlan_id;
            break;
        }
        case intf::IF_TYPE_TUNNEL:
            // TODO: Handle for Tunnel case
            data.is_tunnel = 1;
            break;
        default:
            HAL_ASSERT(0);
    }

    HAL_TRACE_DEBUG("Replication lport : {} qtype : {} qid : {}",
                   data.lport, data.qtype, data.qid_or_vnid);
    return g_hal_state_pd->met_table()->add_replication(list, (void*)&data);
}

// Removes an oif from list
hal_ret_t oif_list_remove_oif(oif_list_id_t list, oif_t *oif)
{
    p4_replication_data_t data = { 0 };

    // TODO: MET library expects the same data to be passed during
    //       removal as that passed during add. Otherwise it wont be
    //       able to find the replication.
    return g_hal_state_pd->met_table()->del_replication(list, (void*)&data);
}

// Check if an oif is present in the list
hal_ret_t oif_list_is_member(oif_list_id_t list, oif_t *oif) {
    return HAL_RET_OK;
}

// Get an array of all oifs in the list
hal_ret_t oif_list_get_num_oifs(oif_list_id_t list, uint32_t &num_oifs) {
    return HAL_RET_OK;
}

// Get an array of all oifs in the list
hal_ret_t oif_list_get_oif_array(oif_list_id_t list, uint32_t &num_oifs, oif_t *oifs) {
    return HAL_RET_OK;
}

}    // namespace pd
}    // namespace hal

