//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------

#include "nic/metaswitch/stubs/test/hals/ipsfeeder/underlay_ecmp_ips_feeder.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"

namespace pds_ms_test {

void
load_underlay_ecmp_test_input ()
{
    static underlay_ecmp_ips_feeder_t g_underlay_ecmp_ips_feeder;
    test_params()->test_input = &g_underlay_ecmp_ips_feeder;
}

void underlay_ecmp_ips_feeder_t::init(std::vector<nhinfo_t>&& nhs) {
    /* Enter N-BASE hals stub context.  This allows this code to make N-BASE calls  */
    NBB_CREATE_STATIC_THREAD_CXT
    nbs_enter_shared_context(hals_proc_id, &saved_context NBB_CCXT);

    uint32_t l3_if = 200; // Dummy number used to generate L3If UUId
    auto state_ctxt = pds_ms::state_t::thread_context();
    for (auto& nhi: nhs) {
        // Form a dummy if spec
        pds_if_spec_t spec = {0};
        spec.key = pds_ms::msidx2pdsobjkey(l3_if);
        auto new_if_obj = new pds_ms::if_obj_t(nhi.l3_ifindex, spec);
        state_ctxt.state()->if_store().add_upd(nhi.l3_ifindex, new_if_obj);
        ms_iflist.push_back(nhi.l3_ifindex);
        ++ l3_if;
    }
    underlay_ecmp_input_params_t::init(std::move(nhs));
    pathset_id = 1;
}

void underlay_ecmp_ips_feeder_t::cleanup(void) {
    auto state_ctxt = pds_ms::state_t::thread_context();
    for (auto ms_ifindex: ms_iflist) {
        state_ctxt.state()->if_store().erase(ms_ifindex);
    }
    nbs_exit_shared_context(&saved_context);
    NBB_DESTROY_STATIC_THREAD_CXT
}

void underlay_ecmp_ips_feeder_t::fill_add_update_sz_array(
                NBB_BUF_SIZE (&size_array)[OFL_ATG_NHPI_ADD_UPDATE_ECMP + 1]) {

    static_assert(OFL_ATG_NHPI_ADD_UPDATE_ECMP == 5);
    size_array[0] = NBB_ALIGN_OFFSET(sizeof(ATG_NHPI_ADD_UPDATE_ECMP));
    bool reset_bh = false;
    if (bh) {
        std::cout << "To blackhole" << std::endl;
    }
    if (!bh && prev_bh_) {
        std::cout << "Recovering from blackhole" << std::endl;
        reset_bh = true;
    }
    // Total remaining nexthops
    if (bh) {
        // 1 BH
        size_array[1] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP), 1);
    } else {
        size_array[1] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP),
                                               nexthops.size());
    }
    // Deleted  nexthops
    if (bh) {
        // All existing nexthops are deleted
        size_array[3] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP),
                                               nexthops.size());
    } else if (reset_bh) {
        // 1 BH deleted
        size_array[3] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP), 1);
    } else {
        size_array[3] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP),
                                               deleted_nexthops.size());
    }
    // Added nexthops - can only happen in the create case and is same as total nexthops
    if (op_create_ || reset_bh) {
        size_array[2] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP),
                                               nexthops.size());
    } else if (bh) {
        // 1 BH
        size_array[2] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP), 1);
    } else {
        size_array[2] = NTL_OFF_LIST_CALC_SIZE(sizeof(ATG_NHPI_APPENDED_NEXT_HOP),
                                               0);
    }
    size_array[4] = size_array[1];
    size_array[5] = NTL_OFF_SIZE_ARRAY_TERMINATOR;
}

ATG_NHPI_ADD_UPDATE_ECMP* 
underlay_ecmp_ips_feeder_t::generate_add_upd_ips(void) {
    NBB_BUF_SIZE size_array[OFL_ATG_NHPI_ADD_UPDATE_ECMP + 1];
    fill_add_update_sz_array(size_array);
    auto ctrl_size = NTL_OFF_CALC_STRUCT_SIZE(size_array);
    auto add_update_ips = (ATG_NHPI_ADD_UPDATE_ECMP*) calloc(1, ctrl_size+100);
    // generate_ips_header (add_upd); 
    NBB_CORR_PUT_VALUE(add_update_ips->pathset_id, pathset_id);
    add_update_ips->cascaded = ATG_YES;
    add_update_ips->max_num_next_hop_objects = 2;

    bool reset_bh = false;
    if (bh) {
        std::cout << "To blackhole" << std::endl;
    }
    if (!bh && prev_bh_) {
        std::cout << "Recovering from blackhole" << std::endl;
        reset_bh = true;
    }
    NTL_OFF_INIT_STRUCT(add_update_ips,
                        size_array,
                        off_atg_nhpi_add_update_ecmp);
    if (bh) {
        ATG_NHPI_APPENDED_NEXT_HOP appended_next_hop = {0};
        appended_next_hop.total_length = sizeof(ATG_NHPI_APPENDED_NEXT_HOP);
        NBB_CORR_INIT(appended_next_hop.dp_correlator);
        ATG_NHPI_NEXT_HOP_PROPERTIES nh_prop = {0};
        nh_prop.destination_type = ATG_NHPI_NEXT_HOP_DEST_BH;
        appended_next_hop.next_hop_properties = nh_prop;
        // Add BH to both current and added
        NTL_OFF_LIST_APPEND(add_update_ips,
                            &add_update_ips->next_hop_objects,
                            appended_next_hop.total_length,
                            (NBB_VOID *)&appended_next_hop,
                            NULL);
        NTL_OFF_LIST_APPEND(add_update_ips,
                            &add_update_ips->added_next_hop_objects,
                            appended_next_hop.total_length,
                            (NBB_VOID *)&appended_next_hop,
                            NULL);
    } else {
        for (auto& nh: nexthops) {
            ATG_NHPI_APPENDED_NEXT_HOP appended_next_hop = {0};
            appended_next_hop.total_length = sizeof(ATG_NHPI_APPENDED_NEXT_HOP);
            NBB_CORR_INIT(appended_next_hop.dp_correlator);
            ATG_NHPI_NEXT_HOP_PROPERTIES nh_prop = {0};
            nh_prop.destination_type = ATG_NHPI_NEXT_HOP_DEST_PORT;
            nh_prop.direct_next_hop_properties.neighbor.neighbor_l3_if_index 
                = nh.l3_ifindex;
            memcpy (nh_prop.direct_next_hop_properties.neighbor.neighbor_id.mac_address,
                    nh.l3_dest_mac.m_mac, ETH_ADDR_LEN);
            appended_next_hop.next_hop_properties = nh_prop;
            if (op_create_ || reset_bh) {
                NTL_OFF_LIST_APPEND(add_update_ips,
                                    &add_update_ips->next_hop_objects,
                                    appended_next_hop.total_length,
                                    (NBB_VOID *)&appended_next_hop,
                                    NULL);
                NTL_OFF_LIST_APPEND(add_update_ips,
                                    &add_update_ips->added_next_hop_objects,
                                    appended_next_hop.total_length,
                                    (NBB_VOID *)&appended_next_hop,
                                    NULL);
            } else {
                NTL_OFF_LIST_APPEND(add_update_ips,
                                    &add_update_ips->next_hop_objects,
                                    appended_next_hop.total_length,
                                    (NBB_VOID *)&appended_next_hop,
                                    NULL);
            }
        }
    }
    if (bh) {
        for (auto& nh: nexthops) {
            ATG_NHPI_APPENDED_NEXT_HOP appended_next_hop = {0};
            appended_next_hop.total_length = sizeof(ATG_NHPI_APPENDED_NEXT_HOP);
            NBB_CORR_INIT(appended_next_hop.dp_correlator);
            ATG_NHPI_NEXT_HOP_PROPERTIES nh_prop = {0};
            nh_prop.destination_type = ATG_NHPI_NEXT_HOP_DEST_PORT;
            nh_prop.direct_next_hop_properties.neighbor.neighbor_l3_if_index 
                = nh.l3_ifindex;
            memcpy (nh_prop.direct_next_hop_properties.neighbor.neighbor_id.mac_address,
                    nh.l3_dest_mac.m_mac, ETH_ADDR_LEN);
            appended_next_hop.next_hop_properties = nh_prop;
            NTL_OFF_LIST_APPEND(add_update_ips,
                                &add_update_ips->deleted_next_hop_objects,
                                appended_next_hop.total_length,
                                (NBB_VOID *)&appended_next_hop,
                                NULL);
        }
    }
    else if (reset_bh) {
        ATG_NHPI_APPENDED_NEXT_HOP appended_next_hop = {0};
        appended_next_hop.total_length = sizeof(ATG_NHPI_APPENDED_NEXT_HOP);
        NBB_CORR_INIT(appended_next_hop.dp_correlator);
        ATG_NHPI_NEXT_HOP_PROPERTIES nh_prop = {0};
        nh_prop.destination_type = ATG_NHPI_NEXT_HOP_DEST_BH;
        appended_next_hop.next_hop_properties = nh_prop;
        // Add BH to deleted
        NTL_OFF_LIST_APPEND(add_update_ips,
                            &add_update_ips->deleted_next_hop_objects,
                            appended_next_hop.total_length,
                            (NBB_VOID *)&appended_next_hop,
                            NULL);
    } else {
        for (auto& nh: deleted_nexthops) {
            ATG_NHPI_APPENDED_NEXT_HOP appended_next_hop = {0};
            appended_next_hop.total_length = sizeof(ATG_NHPI_APPENDED_NEXT_HOP);
            NBB_CORR_INIT(appended_next_hop.dp_correlator);
            ATG_NHPI_NEXT_HOP_PROPERTIES nh_prop = {0};
            nh_prop.destination_type = ATG_NHPI_NEXT_HOP_DEST_PORT;
            nh_prop.direct_next_hop_properties.neighbor.neighbor_l3_if_index 
                = nh.l3_ifindex;
            memcpy (nh_prop.direct_next_hop_properties.neighbor.neighbor_id.mac_address,
                    nh.l3_dest_mac.m_mac, ETH_ADDR_LEN);
            appended_next_hop.next_hop_properties = nh_prop;
            NTL_OFF_LIST_APPEND(add_update_ips,
                                &add_update_ips->deleted_next_hop_objects,
                                appended_next_hop.total_length,
                                (NBB_VOID *)&appended_next_hop,
                                NULL);
        }
    }
    if (bh) {
        prev_bh_ = true;
    } else {
        prev_bh_ = false;
    }
    return add_update_ips;
}

} // End namespace
