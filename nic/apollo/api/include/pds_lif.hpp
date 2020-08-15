//----------------------------------------------------------------------------
//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_LIF_HPP__
#define __INCLUDE_API_PDS_LIF_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_if.hpp"
#include "nic/apollo/framework/state_base.hpp"

typedef struct lif_metrics_s {
    ///< rx counters
    uint64_t rx_ucast_bytes;
    uint64_t rx_ucast_packets;
    uint64_t rx_mcast_bytes;
    uint64_t rx_mcast_packets;
    uint64_t rx_bcast_bytes;
    uint64_t rx_bcast_packets;
    uint64_t rsvd0;
    uint64_t rsvd1;
    ///< rx drop counters
    uint64_t rx_ucast_drop_bytes;
    uint64_t rx_ucast_drop_packets;
    uint64_t rx_mcast_drop_bytes;
    uint64_t rx_mcast_drop_packets;
    uint64_t rx_bcast_drop_bytes;
    uint64_t rx_bcast_drop_packets;
    uint64_t rx_dma_error;
    uint64_t rsvd2;
    ///< tx counters
    uint64_t tx_ucast_bytes;
    uint64_t tx_ucast_packets;
    uint64_t tx_mcast_bytes;
    uint64_t tx_mcast_packets;
    uint64_t tx_bcast_bytes;
    uint64_t tx_bcast_packets;
    uint64_t rsvd3;
    uint64_t rsvd4;
    ///< tx drop counters
    uint64_t tx_ucast_drop_bytes;
    uint64_t tx_ucast_drop_packets;
    uint64_t tx_mcast_drop_bytes;
    uint64_t tx_mcast_drop_packets;
    uint64_t tx_bcast_drop_bytes;
    uint64_t tx_bcast_drop_packets;
    uint64_t tx_dma_error;
    uint64_t rsvd5;
    ///< rx queue/ring drop counters
    uint64_t rx_queue_disabled;
    uint64_t rx_queue_empty;
    uint64_t rx_queue_error;
    uint64_t rx_desc_fetch_error;
    uint64_t rx_desc_data_error;
    uint64_t rsvd6;
    uint64_t rsvd7;
    uint64_t rsvd8;
    ///< tx queue/ring drop counters
    uint64_t tx_queue_disabled;
    uint64_t tx_queue_error;
    uint64_t tx_desc_fetch_error;
    uint64_t tx_desc_data_error;
    uint64_t tx_queue_empty;
    uint64_t rsvd10;
    uint64_t rsvd11;
    uint64_t rsvd12;
    ///< tx rdma counters
    uint64_t tx_rdma_ucast_bytes;
    uint64_t tx_rdma_ucast_packets;
    uint64_t tx_rdma_mcast_bytes;
    uint64_t tx_rdma_mcast_packets;
    uint64_t tx_rdma_cnp_packets;
    uint64_t rsvd13;
    uint64_t rsvd14;
    uint64_t rsvd15;
    ///< rx rdma counters
    uint64_t rx_rdma_ucast_bytes;
    uint64_t rx_rdma_ucast_packets;
    uint64_t rx_rdma_mcast_bytes;
    uint64_t rx_rdma_mcast_packets;
    uint64_t rx_rdma_cnp_packets;
    uint64_t rx_rdma_ecn_packets;
    uint64_t rsvd16;
    uint64_t rsvd17;
    uint64_t rsvd18;
    uint64_t rsvd19;
    uint64_t rsvd20;
    uint64_t rsvd21;
    uint64_t rsvd22;
    uint64_t rsvd23;
    uint64_t rsvd24;
    uint64_t rsvd25;
    uint64_t rsvd26;
    uint64_t rsvd27;
    uint64_t rsvd28;
    uint64_t rsvd29;
    uint64_t rsvd30;
    uint64_t rsvd31;
    uint64_t rsvd32;
    uint64_t rsvd33;
    uint64_t rsvd34;
    uint64_t rsvd35;
    uint64_t rsvd36;
    uint64_t rsvd37;
    uint64_t rsvd38;
    uint64_t rsvd39;
    uint64_t rsvd40;
    uint64_t rsvd41;
    ///< pps counters
    uint64_t tx_pkts;
    uint64_t tx_bytes;
    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t tx_pps;
    uint64_t tx_bps;
    uint64_t rx_pps;
    uint64_t rx_bps;
    ///< rdma req error counters
    uint64_t rdma_req_rx_pkt_seq_err;
    uint64_t rdma_req_rx_rnr_retry_err;
    uint64_t rdma_req_rx_remote_access_err;
    uint64_t rdma_req_rx_remote_inv_req_err;
    uint64_t rdma_req_rx_remote_oper_err;
    uint64_t rdma_req_rx_implied_nak_seq_err;
    uint64_t rdma_req_rx_cqe_err;
    uint64_t rdma_req_rx_cqe_flush_err;
    uint64_t rdma_req_rx_dup_responses;
    uint64_t rdma_req_rx_invalid_packets;
    uint64_t rdma_req_tx_local_access_err;
    uint64_t rdma_req_tx_local_oper_err;
    uint64_t rdma_req_tx_memory_mgmt_err;
    uint64_t rsvd42;
    uint64_t rsvd43;
    uint64_t rsvd44;
    ///< rdma resp error counters
    uint64_t rdma_resp_rx_dup_requests;
    uint64_t rdma_resp_rx_out_of_buffer;
    uint64_t rdma_resp_rx_out_of_seq_pkts;
    uint64_t rdma_resp_rx_cqe_err;
    uint64_t rdma_resp_rx_cqe_flush_err;
    uint64_t rdma_resp_rx_local_len_err;
    uint64_t rdma_resp_rx_inv_request_err;
    uint64_t rdma_resp_rx_local_qp_oper_err;
    uint64_t rdma_resp_rx_out_of_atomic_resource;
    uint64_t rdma_resp_tx_pkt_seq_err;
    uint64_t rdma_resp_tx_remote_inv_req_err;
    uint64_t rdma_resp_tx_remote_access_err;
    uint64_t rdma_resp_tx_remote_oper_err;
    uint64_t rdma_resp_tx_rnr_retry_err;
    uint64_t rsvd45;
    uint64_t rsvd46;
} lif_metrics_t;

#define RATE_OF_X(x, curr_x, interval)                              \
    ((x == curr_x) ? 0 :                                            \
                     (((curr_x > x) ? (curr_x - x) :                \
                                      ((UINT64_MAX-x) + curr_x)) /  \
                                          interval))

typedef struct pds_lif_spec_s {
    ///< key for the lif
    pds_obj_key_t    key;
    ///< internal lif id
    pds_lif_id_t     id;
    ///< internal peer lif id (for P2P LIFs)
    pds_lif_id_t     peer_lif_id;
    ///< name of the lif, if any
    char             name[SDK_MAX_NAME_LEN];
    ///< if index of the pinned port/lif
    if_index_t       pinned_ifidx;
    ///< type of lif
    lif_type_t       type;
    ///< vlan_strip_en is set to true if vlan needs to be stripped in datapath
    bool             vlan_strip_en;
    ///< mac address of the device
    mac_addr_t       mac;
    ///< tx scheduler queue count
    uint32_t         total_qcount;
    ///< tx scheduler active cos
    uint16_t         cos_bmp;
    ///< tx scheduler table offset
    uint32_t         tx_sched_table_offset;
    ///< tx scheduler num table entries
    uint32_t         tx_sched_num_table_entries;
} pds_lif_spec_t;

/// \brief lif status
typedef struct pds_lif_status_s {
    ///< encoded interface index of this lif
    if_index_t     ifindex;
    ///< nexthop hw index of this lif, if any
    uint32_t       nh_idx;
    ///< name of the lif (as seen on host)
    char           name[SDK_MAX_NAME_LEN];
    ///< admin state of the lif
    pds_if_state_t admin_state;
    ///< operational status of the lif
    pds_if_state_t state;
    ///< vnic hw index of this lif
    uint32_t       vnic_hw_idx;
} __PACK__ pds_lif_status_t;

typedef struct pds_lif_info_s {
    pds_lif_spec_t spec;
    pds_lif_status_t status;
} __PACK__ pds_lif_info_t;

/// \brief read LIF information
///
/// \param[in]  key     key
/// \param[out] info    lif info
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_lif_read(pds_obj_key_t *key, pds_lif_info_t *info);

typedef void (*lif_read_cb_t)(void *spec, void *ctxt);

/// \brief read all LIF information
///
/// \param[in] cb      callback to be called on each lif_impl
/// \param[in] ctxt    context for the callback
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_lif_read_all(lif_read_cb_t cb, void *ctxt);

/// \brief clear LIF statistics
///
/// \param[in]  key     key
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_lif_stats_reset(pds_obj_key_t *key);

#endif    // __INCLUDE_API_PDS_LIF_HPP__
