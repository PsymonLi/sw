//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// apollo pipeline implementation of dev apis
///
//----------------------------------------------------------------------------

#ifndef __PDS_DEVAPI_IMPL_HPP__
#define __PDS_DEVAPI_IMPL_HPP__

#include "nic/sdk/platform/devapi/devapi.hpp"
#include "nic/apollo/framework/impl_base.hpp"

namespace api {
namespace impl {

using sdk::platform::devapi;

#define DEVAPI_IMPL_ADMIN_COS 1


/// \defgroup PDS_DEVAPI_IMPL - dev api implementation
/// \ingroup PDS_DEVAPI
/// @{

class devapi_impl : public impl_base, public devapi {
public:
    static devapi *factory(void);
    static void destroy(devapi *impl);

    // lif APIs
    virtual sdk_ret_t lif_create(lif_info_t *info) override;
    virtual sdk_ret_t lif_destroy(uint32_t lif_id) override;
    virtual sdk_ret_t lif_reset(uint32_t lif_id) override;
    virtual sdk_ret_t lif_add_mac(uint32_t lif_id, mac_t mac) override;
    virtual sdk_ret_t lif_del_mac(uint32_t lif_id, mac_t mac) override;
    virtual sdk_ret_t lif_add_vlan(uint32_t lif_id, vlan_t vlan) override;
    virtual sdk_ret_t lif_del_vlan(uint32_t lif_id, vlan_t vlan) override;
    virtual sdk_ret_t lif_add_macvlan(uint32_t lif_id, mac_t mac,
                                      vlan_t vlan) override;
    virtual sdk_ret_t lif_del_macvlan(uint32_t lif_id, mac_t mac,
                                      vlan_t vlan) override;
    virtual sdk_ret_t lif_upd_vlan_offload(uint32_t lif_id, bool vlan_strip,
                                           bool vlan_insert) override;
    virtual sdk_ret_t lif_upd_rx_mode(uint32_t lif_id, bool broadcast,
                                      bool all_multicast,
                                      bool promiscuous) override;
    virtual sdk_ret_t lif_upd_name(uint32_t lif_id, string name) override;
    virtual sdk_ret_t lif_get_max_filters(uint32_t *ucast_filters, uint32_t *mcast_filters) override;

    // qos APIs
    virtual sdk_ret_t qos_class_get(uint8_t group, qos_class_info_t *info) override;
    virtual sdk_ret_t qos_class_create(qos_class_info_t *info) override;
    virtual sdk_ret_t qos_class_delete(uint8_t group) override;
    virtual sdk_ret_t qos_get_txtc_cos(const string &group,
                                       uint32_t uplink_port,
                                       uint8_t *cos) override;

    // uplink APIs
    virtual sdk_ret_t uplink_create(uint32_t uplink_ifidx,
                                    pds_ifindex_t ifidx, bool is_oob) override;
    virtual sdk_ret_t uplink_destroy(pds_ifindex_t ifidx) {
        return SDK_RET_INVALID_OP;
    }

    // port APIs
    virtual sdk_ret_t port_get_status(uint32_t port_num,
                                      port_status_t *status) override;
    virtual sdk_ret_t port_get_config(pds_ifindex_t ifidx,
                                      port_config_t *config) override;
    virtual sdk_ret_t port_set_config(uint32_t port_num,
                                      port_config_t *config) override;

    // single wire management APIs
    virtual sdk_ret_t swm_update(bool enable, 
                                 uint32_t port_num, /* Uplink port number */ 
                                 uint32_t vlan, /* 0: untag */
                                 mac_t mac) override;

private:
    devapi_impl() {}
    ~devapi_impl() {}

    sdk_ret_t lif_program_tx_scheduler_(lif_info_t *info);
    uint16_t lif_get_cos_bmp_(lif_info_t *info);
    uint32_t lif_get_qcount_(lif_info_t *info);
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __PDS_DEVAPI_IMPL_HPP__
