//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// PDS Metaswitch L3 HALS stub integration subcomponent APIs
//---------------------------------------------------------------

#include <hals_c_includes.hpp>
#include "nic/metaswitch/stubs/hals/pds_ms_hals_l3.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hals_ecmp.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_error.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hals_route.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_state.hpp"

namespace pds_ms {

using pds_ms::Error;
//-------------------------------------------------------------------------
// NHPI
//-------------------------------------------------------------------------
NBB_BYTE hals_l3_integ_subcomp_t::nhpi_add_update_ecmp(ATG_NHPI_ADD_UPDATE_ECMP *add_update_ecmp_ips) { 
    NBB_BYTE rc;
    try {
        hals_ecmp_t ecmp;
        rc = ecmp.handle_add_upd_ips(add_update_ecmp_ips);
    } catch (Error& e) {
        PDS_TRACE_ERR ("ECMP Add Update processing failed %s", e.what());
        rc = ATG_UNSUCCESSFUL;
    }
    return rc;
}

NBB_BYTE hals_l3_integ_subcomp_t::nhpi_delete_ecmp(ATG_NHPI_DELETE_ECMP *delete_ecmp_ips) {
    // Called in MS HALs stateless mode
    try {
        hals_ecmp_t ecmp;
        ecmp.handle_delete(delete_ecmp_ips->pathset_id);
    } catch (Error& e) {
        PDS_TRACE_ERR ("ECMP Delete processing failed %s", e.what());
    }
    return ATG_OK;
}

NBB_BYTE hals_l3_integ_subcomp_t::nhpi_destroy_ecmp(NBB_CORRELATOR ecmp_corr) {
    // Called in MS HALs stateful mode
    try {
        hals_ecmp_t ecmp;
        ecmp.handle_delete(ecmp_corr);
    } catch (Error& e) {
        PDS_TRACE_ERR ("ECMP Destroy processing failed %s", e.what());
    }
    return ATG_OK;
}

NBB_BYTE hals_l3_integ_subcomp_t::nhpi_add_ecmp_nh(NBB_CORRELATOR ecmp_corr,
                                                   ATG_NHPI_APPENDED_NEXT_HOP *next_hop,
                                                   NBB_BYTE cascaded) {
    return ATG_OK;
};

NBB_BYTE hals_l3_integ_subcomp_t::nhpi_delete_ecmp_nh(NBB_CORRELATOR ecmp_corr,
                                                      NBB_CORRELATOR nh_corr) {
    return ATG_OK;
};

NBB_BYTE hals_l3_integ_subcomp_t::ropi_update_route(ATG_ROPI_UPDATE_ROUTE *update_route) {
    NBB_BYTE rc;
    try {
        hals_route_t route;
        rc = route.handle_add_upd_ips(update_route);
    } catch (Error& e) {
        PDS_TRACE_ERR ("Route Add Update processing failed %s", e.what());
        rc = ATG_UNSUCCESSFUL;
    }
    return rc;
};

NBB_BYTE hals_l3_integ_subcomp_t::ropi_delete_route(ATG_ROPI_ROUTE_ID route_id) {
    // Called in MS HALs stateless mode
    try {
        hals_route_t route;
        route.handle_delete(route_id);
    } catch (Error& e) {
        PDS_TRACE_ERR ("Route Delete processing failed %s", e.what());
    }
    return ATG_OK;
};

NBB_BYTE hals_l3_integ_subcomp_t::nhpi_squash_cascaded_hops() { 
    // Metaswitch IP Routing (FTM) and EVPN components use Metaswitch PSM
    // to request paths for the Next-hop that they receive in respective
    // routes. For each request PSM programs ECMP nexthops to the HAL NHPI
    // stub as and when required.
    //
    // With Cascaded Pathset mode, Metaswitch creates a Indirect Pathset
    // for each such request irrespective of reachability and then
    // points this Indirect Pathset at the actual Direct Pathset containing
    // the immediate connected nexthop to reach the route's Nexthop.
    //
    // With Squashed Pathset mode, Metaswitch creates a Direct pathset
    // for each such request. This pathset contains the immediate connected
    // nexthops used to reach the route's next-hop.
    //
    // Our HAL NHPI stub programs each Direct Pathset as a PDS Nexthop Group
    // with multiple ECMP Nexthops.
    // This avoids maintaining state to track Indirect to Direct Pathset that
    // would have been required if we had chosen to use Indirect Pathset to
    // program HAL.
    //
    // Cascaded mode is ideal in our case to stitch overlay objects to
    // underlay nexthop groups in both overlay and non-overlay routing modes.
    //
    // In non-overlay routing mode, a separate IP track object is used to track
    // reachability to each underlay destination IP that is configured from the
    // controller over gRPC and is converted into a static IP route in Metaswitch.
    // Each such tracked object will get its own indirect pathset but many or all
    // of them could share the same direct pathset.
    //
    // In the overlay routing mode Metaswitch DC-EVPN requests
    // path to each remote Tunnel IP. All remote Tunnels are reachable via the
    // same set of immediate connected paths (ToRs) and hence end up sharing the
    // same Direct Pathset.
    //
    // If Squashed mode had been used in either of these cases then a separate
    // Direct pathset would have been created for each Tunnel or IP tracked object.
    //
    // However in Cascaded mode for route prefixes directly learnt by underlay BGP
    // (eg: default route received from ToR), MS FTM internally requests separate
    // path to each nexthop learnt by BGP for the prefix.
    // PSM creates a separate Direct Pathset for each ToR and then returns an
    // Indirect Nexthop pointing to 2 separate Direct Pathsets. In this case HAL
    // would not have ECMP Nexthop Group.
    // So if BGP underlay routes are required to be installed in the hardware
    // then the Squashed pathset mode would be better fit since PSM returns a
    // Direct pathset containing nexthops for each ToR which is ideal for us to
    // program HAL.
    //
    PDS_TRACE_INFO ("NHPI registration Enable Cascaded pathsets");
    return ATG_NO;
}
} // End namespace
