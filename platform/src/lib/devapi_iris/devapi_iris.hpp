//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------
#ifndef __DEVAPI_IRIS_HPP__
#define __DEVAPI_IRIS_HPP__

#include "include/sdk/lock.hpp"
#include "include/sdk/base.hpp"
#include "nic/sdk/platform/devapi/devapi.hpp"
#include "devapi_iris_types.hpp"

namespace iris {

using std::string;

class devapi_iris : public devapi {
public:
    static devapi *factory(void);
    static void destroy(devapi *dapi);

    // Lif APIs
    sdk_ret_t lif_create(lif_info_t *info);
    sdk_ret_t lif_destroy(uint32_t lif_id);
    sdk_ret_t lif_init(lif_info_t *info);
    sdk_ret_t lif_reset(uint32_t lif_id);
    sdk_ret_t lif_add_mac(uint32_t lif_id, mac_t mac);
    sdk_ret_t lif_del_mac(uint32_t lif_id, mac_t mac);
    sdk_ret_t lif_add_vlan(uint32_t lif_id, vlan_t vlan);
    sdk_ret_t lif_del_vlan(uint32_t lif_id, vlan_t vlan);
    sdk_ret_t lif_add_macvlan(uint32_t lif_id, mac_t mac, vlan_t vlan);
    sdk_ret_t lif_del_macvlan(uint32_t lif_id, mac_t mac, vlan_t vlan);
    sdk_ret_t lif_upd_vlan_offload(uint32_t lif_id, bool vlan_strip, bool vlan_insert);
    sdk_ret_t lif_upd_rx_mode(uint32_t lif_id, bool broadcast,
                              bool all_multicast, bool promiscuous);
    sdk_ret_t lif_upd_rx_bmode(uint32_t lif_id, bool broadcast);
    sdk_ret_t lif_upd_rx_mmode(uint32_t lif_id, bool all_multicast);
    sdk_ret_t lif_upd_rx_pmode(uint32_t lif_id, bool promiscuous);

    sdk_ret_t lif_upd_name(uint32_t lif_id, std::string name);
    sdk_ret_t lif_get_max_filters(uint32_t *ucast_filters,
                                  uint32_t *mcast_filters);
    sdk_ret_t lif_upd_state(uint32_t lif_id, lif_state_t state);
    sdk_ret_t lif_upd_rdma_sniff(uint32_t lif_id, bool rdma_sniff);
    sdk_ret_t lif_upd_bcast_filter(uint32_t lif_id, lif_bcast_filter_t bcast_filter);
    sdk_ret_t lif_upd_mcast_filter(uint32_t lif_id, lif_mcast_filter_t mcast_filter);
    sdk_ret_t lif_upd_rx_en(uint32_t lif_id, bool rx_en);
    sdk_ret_t lif_upd_max_tx_rate(uint32_t lif_id, uint64_t rate_in_Bps);
    sdk_ret_t lif_get_max_tx_rate(uint32_t lif_id, uint64_t *rate_in_Bps);

    // Eth APIs
    sdk_ret_t eth_dev_admin_status_update(uint32_t lif_id, lif_state_t state);

    // Qos APIs
    sdk_ret_t qos_class_get(uint8_t group, qos_class_info_t *info);
    sdk_ret_t qos_class_exist(uint8_t group);
    sdk_ret_t qos_class_create(qos_class_info_t *info);
    sdk_ret_t qos_class_update(qos_class_info_t *info);
    sdk_ret_t qos_class_delete(uint8_t group, bool clear_stats);
    sdk_ret_t qos_clear_stats(uint32_t port_num, uint8_t qos_group_bitmap);
    sdk_ret_t qos_get_txtc_cos(const std::string &group, uint32_t uplink_port,
                               uint8_t *cos);
    sdk_ret_t qos_class_set_global_pause_type(uint8_t pause_type);
    sdk_ret_t qos_reset(uint32_t group);

    // Uplink APIs
    sdk_ret_t uplink_create(uint32_t id, uint32_t port, bool is_oob);
    sdk_ret_t uplink_destroy(uint32_t port);
    sdk_ret_t uplink_available_count(uint8_t *count);

    // Generic APIs
    sdk_ret_t set_fwd_mode(sdk::lib::dev_forwarding_mode_t fwd_mode);

    // Generic APIs
    sdk_ret_t set_micro_seg_en(bool en);
    bool get_micro_seg_cfg_en(void);
    virtual uint32_t max_encap_hdr_len(void) const override {
        // iris pipeline doesnt add additional encap
        return 0;
    }
    static bool is_hal_up(void);

    // Single Wire Management(SWM) APIs
    sdk_ret_t swm_enable(void);
    sdk_ret_t swm_disable(void);
    sdk_ret_t swm_create_channel(uint32_t channel, uint32_t port_num, uint32_t lif_id);
    sdk_ret_t swm_get_channels_info(std::set<channel_info_t *>* channels_info);
    sdk_ret_t swm_add_mac(mac_t mac, uint32_t channel);
    sdk_ret_t swm_del_mac(mac_t mac, uint32_t channel);
    sdk_ret_t swm_add_vlan(vlan_t vlan, uint32_t channel);
    sdk_ret_t swm_del_vlan(vlan_t vlan, uint32_t channel);
    sdk_ret_t swm_upd_rx_bmode(bool broadcast, uint32_t channel);
    sdk_ret_t swm_upd_rx_mmode(bool all_multicast, uint32_t channel);
    sdk_ret_t swm_upd_rx_pmode(bool promiscuous, uint32_t channel);
    sdk_ret_t swm_upd_bcast_filter(lif_bcast_filter_t bcast_filter, uint32_t channel);
    sdk_ret_t swm_upd_mcast_filter(lif_mcast_filter_t mcast_filter, uint32_t channel);
    sdk_ret_t swm_enable_tx(uint32_t channel);
    sdk_ret_t swm_disable_tx(uint32_t channel);
    sdk_ret_t swm_enable_rx(uint32_t channel);
    sdk_ret_t swm_disable_rx(uint32_t channel);
    sdk_ret_t swm_reset_channel(uint32_t channel);
    sdk_ret_t swm_upd_vlan_mode(bool enable, uint32_t mode, uint32_t channel);

    // Port APIs
    sdk_ret_t port_get_status(uint32_t port_num,
                              port_status_t *status);
    sdk_ret_t port_get_config(uint32_t port_num,
                              port_config_t *config);
    sdk_ret_t port_set_config(uint32_t port_num,
                              port_config_t *config);
    // Accel APIs
    sdk_ret_t accel_rgroup_add(string name,
                               uint64_t metrics_mem_addr,
                               uint32_t metrics_mem_size);
    sdk_ret_t accel_rgroup_del(string name);
    sdk_ret_t accel_rgroup_ring_add(string name,
                                    std::vector<std::pair<const std::string,uint32_t>>& ring_vec);
    sdk_ret_t accel_rgroup_ring_del(string name,
                                    std::vector<std::pair<const std::string,uint32_t>>& ring_vec);
    sdk_ret_t accel_rgroup_reset_set(string name, uint32_t sub_ring,
                                     bool reset_sense);
    sdk_ret_t accel_rgroup_enable_set(string name, uint32_t sub_ring,
                                      bool enable_sense);
    sdk_ret_t accel_rgroup_pndx_set(string name, uint32_t sub_ring,
                                    uint32_t val, bool conditional);
    sdk_ret_t accel_rgroup_info_get(string name, uint32_t sub_ring,
                                    accel_rgroup_rinfo_rsp_cb_t rsp_cb_func,
                                    void *user_ctx,
                                    uint32_t *ret_num_entries);
    sdk_ret_t accel_rgroup_indices_get(string name, uint32_t sub_ring,
                                       accel_rgroup_rindices_rsp_cb_t rsp_cb_func,
                                       void *user_ctx,
                                       uint32_t *ret_num_entries);
    sdk_ret_t accel_rgroup_metrics_get(string name, uint32_t sub_ring,
                                       accel_rgroup_rmetrics_rsp_cb_t rsp_cb_func,
                                       void *user_ctx,
                                       uint32_t *ret_num_entries);
    sdk_ret_t accel_rgroup_misc_get(string name, uint32_t sub_ring,
                                    accel_rgroup_rmisc_rsp_cb_t rsp_cb_func,
                                    void *user_ctx,
                                    uint32_t *ret_num_entries);
    sdk_ret_t crypto_key_index_upd(uint32_t key_index,
                                   crypto_key_type_t type,
                                   void *key, uint32_t key_size);
    void inc_num_int_mgmt_mnics(void);
    void dec_num_int_mgmt_mnics(void);
    uint32_t num_int_mgmt_mnics(void) { return num_int_mgmt_mnics_; }

private:
    devapi_iris() {
        SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    }
    ~devapi_iris(){
        SDK_SPINLOCK_DESTROY(&slock_);
    }
    sdk_ret_t init_(void);
    sdk_ret_t micro_seg_halupdate_(bool en);

private:
    bool mirco_seg_en_;
    sdk::lib::dev_forwarding_mode_t fwd_mode_;
    sdk_spinlock_t slock_;
    uint32_t num_int_mgmt_mnics_;
};

} // namespace iris

using iris::devapi_iris;

#endif  // __DEVAPI_IRIS_HPP__
