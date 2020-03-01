//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#ifndef __VXLAN_IPS_FEEDER_HPP__
#define __VXLAN_IPS_FEEDER_HPP__

#include "nic/metaswitch/stubs/test/hals/vxlan_test_params.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_util.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_li.hpp"
#include <hals_c_includes.hpp>
#include "nic/metaswitch/stubs/hals/pds_ms_hals_l3.hpp"
#include "nic/apollo/test/base/base.hpp"

namespace pds_ms_test {

class vxlan_ips_feeder_t final : public vxlan_input_params_t {
public:
   void init(const char *sip, const char* dip) override {
       vxlan_input_params_t::init(sip,dip);
       // Mock TunnelIfIndex and Underlay Nexthop 
       // for the IPS feeder case
       tnl_ifindex = 1000;
       unh_dp_idx = 100;
       indirect_pathset = 1;
    }

    ATG_LIPI_VXLAN_ADD_UPDATE generate_add_upd_ips(void) {
        ATG_LIPI_VXLAN_ADD_UPDATE add_upd;
      // generate_ips_header (add_upd); 
        add_upd.id.if_index = tnl_ifindex;
        pds_ms::pds_to_ms_ipaddr(source_ip, 
                                    &(add_upd.vxlan_settings.source_ip));
        pds_ms::pds_to_ms_ipaddr(dest_ip, &(add_upd.vxlan_settings.dest_ip));
        NBB_CORR_PUT_VALUE(add_upd.vxlan_settings.pathset_id, indirect_pathset);
        NBB_CORR_PUT_VALUE(add_upd.vxlan_settings.dp_pathset_correlator, unh_dp_idx);
        return add_upd;
    }

    void trigger_create(void) override {
        auto add_upd = generate_add_upd_ips();
        pds_ms::li_is()->vxlan_add_update(&add_upd);
    }

    void trigger_delete(void) override {
        pds_ms::li_is()->vxlan_delete(tnl_ifindex);
        NBB_CORRELATOR pathset_id_corr;
        NBB_CORR_PUT_VALUE(pathset_id_corr, indirect_pathset);
        hal_is.nhpi_destroy_ecmp(pathset_id_corr);
    }

    void trigger_update(void) override {
        auto add_upd = generate_add_upd_ips();
        pds_ms::li_is()->vxlan_add_update(&add_upd);
    }
    void modify(void) override {
        unh_dp_idx += 5;
    }
    void next(void) override {
        tnl_ifindex += 1;
        indirect_pathset += 1;
        test::increment_ip_addr (&dest_ip);
    }
    bool ips_mock() override {return true;}

private:
    pds_ms::hals_l3_integ_subcomp_t hal_is;
};

} // End Namespace

#endif
