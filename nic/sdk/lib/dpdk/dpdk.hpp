//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
// Header file for DPDK wrapper library for SDK
//------------------------------------------------------------------------------

#ifndef __DPDK_HPP__
#define __DPDK_HPP__

#include <include/sdk/base.hpp>
#include <string>
#include <vector>
#include <map>

using namespace std;

namespace sdk {
namespace lib {
namespace dpdk {

#define DPDK_MAX_BURST_SIZE 256
#define DEFAULT_TX_QUEUES   1
#define DEFAULT_RX_QUEUES   1
#define DEFAULT_TX_DESC     1024
#define DEFAULT_RX_DESC     1024

// Defines DPDK initialisation parameters
typedef struct sdk_dpdk_params_s {
    // [Input] DPDK log file name
    string log_file_name;
    // [Input] DPDK mbuf pool name.
    string mbuf_pool_name;
    // [Input] Number of mbufs to be allocated for the system.
    uint32_t num_mbuf;
    // [Input] Size of each mbuf in bytes.
    uint16_t mbuf_size;
    // [Input] List of DPDK vdev to be probed.
    vector<string> vdev_list;
    // [Input] DPDK EAL initialisation parameters to be passed
    // transparently to DPDK.
    string eal_init_list;
} sdk_dpdk_params_t;

// Defines DPDK initialisation parameters
typedef struct dpdk_init_params_s {
    // [Input] DPDK device name.
    string dev_name;
    // [Input] Number of RX queues to be configured.
    uint32_t num_rx_queue;
    // [Input] Number of TX queues to be configured.
    uint32_t num_tx_queue;
    // [Input] Number of RX descriptors per rx queue to be configured.
    uint32_t num_rx_desc;
    // [Input] Number of TX descriptors per tx queue to be configured.
    uint32_t num_tx_desc;
} sdk_dpdk_device_params_t;

// Stores DPDK configurations
typedef struct dpdk_global_config_s {
    map<string, uint16_t> name_to_port_map;
    // We assume only one numa node, so only one pool
    struct rte_mempool *rte_mp;
} dpdk_global_config_t;

sdk_ret_t dpdk_init(sdk_dpdk_params_t *args);

typedef union dpdk_mbuf_ {
     struct rte_mbuf;
} dpdk_mbuf;

class dpdk_device {
private:
    uint16_t portid;
    uint16_t rx_queues;
    uint16_t tx_queues;
    uint16_t tx_desc;
    uint16_t rx_desc;
    vector<dpdk_mbuf **> mbufs;

    dpdk_device();
    ~dpdk_device(void);
    sdk_ret_t init_(sdk_dpdk_device_params_t *args);

public:
    static dpdk_device *factory(sdk_dpdk_device_params_t *args);
    static sdk_ret_t destroy(dpdk_device *dev);
    static char * remove_header(dpdk_mbuf *packet, uint16_t len);
    static char * add_header(dpdk_mbuf *packet, uint16_t len);
    static char * append_data(dpdk_mbuf *packet, uint16_t len);
    static char * get_data_ptr(dpdk_mbuf *packet);
    static int get_data_len(dpdk_mbuf *packet);
    dpdk_mbuf ** receive_packets(uint16_t rx_queue_id, uint16_t max_packets,
                                 uint16_t *recv_count);
    //Returns number of untransmitted packets
    uint16_t transmit_packets(uint16_t tx_queue_id, dpdk_mbuf **packets,
                              uint16_t num_packets);
    void drop_packets(dpdk_mbuf **packets, uint16_t num_packets);
    dpdk_mbuf *alloc_mbuf(void);
    uint32_t avail_mbuf_count(void);
};

}    // namespace dpdk
}    // namespace lib
}    // namespace sdk

using namespace sdk::lib::dpdk;

#endif /* __DPDK_HPP__ */
