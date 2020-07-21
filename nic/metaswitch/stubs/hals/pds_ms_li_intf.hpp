//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// MS LI stub HAL integration for uplink L3 interface
//---------------------------------------------------------------

#ifndef __PDS_MS_LI_INTF_HPP__
#define __PDS_MS_LI_INTF_HPP__

#include "nic/metaswitch/stubs/common/pds_ms_cookie.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include <nbase.h>
extern "C"
{
#include <a0build.h>
#include <a0glob.h>
#include <ntloffst.h>
#include <a0spec.h>
#include <o0mac.h>
#include <lipi.h>
}

namespace pds_ms {

sdk_ret_t li_intf_update_pds_ipaddr (NBB_ULONG ms_ifindex);

class li_intf_t {
public:    
    NBB_BYTE handle_add_upd_ips(ATG_LIPI_PORT_ADD_UPDATE* port_add_upd);
    NBB_BYTE intf_create(ms_ifindex_t ms_ifindex);
    void intf_delete(ms_ifindex_t ms_ifindex, bool li_ip_deleted); 

private:
    struct ips_info_t {
        ms_ifindex_t ifindex;
        bool         admin_state;
        bool         admin_state_updated = false;
        bool         switchport;
        bool         switchport_updated = false;
        char*        if_name = nullptr;
    };
    struct store_info_t {
        if_obj_t*    phy_port_if_obj;
    };

private:
    std::unique_ptr<cookie_t> cookie_uptr_;
    ips_info_t  ips_info_;
    store_info_t  store_info_;
    bool op_create_ = false;
    bool op_delete_ = false;

private:
    void init_if_(bool async);
    bool fetch_store_info_(pds_ms::state_t* state);
    void parse_ips_info_(ATG_LIPI_PORT_ADD_UPDATE* port_add_upd);
    NBB_BYTE handle_intf_add_upd_(bool async, ATG_LIPI_PORT_ADD_UPDATE* port_add_upd_ips);
    void init_fault_status_(if_obj_t::phy_port_properties_t& phy_port_prop,
                            bool force_fault = false);
    void enter_ms_ctxt_and_init_fault_status_(if_obj_t::phy_port_properties_t&
                                              phy_port_prop);
};

} // End namespace
#endif
