//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// lif datapath implementation
///
//----------------------------------------------------------------------------

#ifndef __LIF_IMPL_HPP__
#define __LIF_IMPL_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/include/sdk/qos.hpp"
#include "nic/sdk/platform/devapi/devapi_types.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_lif.hpp"

#define SDK_MAX_NAME_LEN            16

namespace api {
namespace impl {

#define PDS_LIF_IMPL_COUNTER_SIZE   sizeof(uint64_t)
#define PDS_LIF_IMPL_NUM_PPS_STATS  8

typedef struct lif_metrics_s {
    /* RX */
    uint64_t rx_ucast_bytes;
    uint64_t rx_ucast_packets;
    uint64_t rx_mcast_bytes;
    uint64_t rx_mcast_packets;
    uint64_t rx_bcast_bytes;
    uint64_t rx_bcast_packets;
    uint64_t rsvd0;
    uint64_t rsvd1;
    /* RX drops */
    uint64_t rx_ucast_drop_bytes;
    uint64_t rx_ucast_drop_packets;
    uint64_t rx_mcast_drop_bytes;
    uint64_t rx_mcast_drop_packets;
    uint64_t rx_bcast_drop_bytes;
    uint64_t rx_bcast_drop_packets;
    uint64_t rx_dma_error;
    uint64_t rsvd2;
    /* TX */
    uint64_t tx_ucast_bytes;
    uint64_t tx_ucast_packets;
    uint64_t tx_mcast_bytes;
    uint64_t tx_mcast_packets;
    uint64_t tx_bcast_bytes;
    uint64_t tx_bcast_packets;
    uint64_t rsvd3;
    uint64_t rsvd4;
    /* TX drops */
    uint64_t tx_ucast_drop_bytes;
    uint64_t tx_ucast_drop_packets;
    uint64_t tx_mcast_drop_bytes;
    uint64_t tx_mcast_drop_packets;
    uint64_t tx_bcast_drop_bytes;
    uint64_t tx_bcast_drop_packets;
    uint64_t tx_dma_error;
    uint64_t rsvd5;
    /* Rx Queue/Ring drops */
    uint64_t rx_queue_disabled;
    uint64_t rx_queue_empty;
    uint64_t rx_queue_error;
    uint64_t rx_desc_fetch_error;
    uint64_t rx_desc_data_error;
    uint64_t rsvd6;
    uint64_t rsvd7;
    uint64_t rsvd8;
    /* Tx Queue/Ring drops */
    uint64_t tx_queue_disabled;
    uint64_t tx_queue_error;
    uint64_t tx_desc_fetch_error;
    uint64_t tx_desc_data_error;
    uint64_t tx_queue_empty;
    uint64_t rsvd10;
    uint64_t rsvd11;
    uint64_t rsvd12;
    /* RDMA/ROCE TX */
    uint64_t tx_rdma_ucast_bytes;
    uint64_t tx_rdma_ucast_packets;
    uint64_t tx_rdma_mcast_bytes;
    uint64_t tx_rdma_mcast_packets;
    uint64_t tx_rdma_cnp_packets;
    uint64_t rsvd13;
    uint64_t rsvd14;
    uint64_t rsvd15;
    /* RDMA/ROCE RX */
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
    uint64_t tx_pkts;
    uint64_t tx_bytes;
    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t tx_pps;
    uint64_t tx_bytesps;
    uint64_t rx_pps;
    uint64_t rx_bytesps;
    /* RDMA/ROCE REQ Error/Debugs (768 - 895) */
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
    /* RDMA/ROCE RESP Error/Debugs (896 - 1023) */
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

// forward declaration
class lif_impl_state;

/// \defgroup PDS_LIF_IMPL - lif entry datapath implementation
/// \ingroup PDS_LIF
/// \@{
/// \brief LIF implementation

class lif_impl : public impl_base {
public:
    /// \brief  factory method to allocate and initialize a lif entry
    /// \param[in] spec    lif configuration parameters
    /// \return    new instance of lif or NULL, in case of error
    static lif_impl *factory(pds_lif_spec_t *spec);

    /// \brief  release all the s/w state associate with the given lif,
    ///         if any, and free the memory
    /// \param[in] impl    lif impl instance to be freed
    ///                  NOTE: h/w entries should have been cleaned up (by
    ///                  calling impl->cleanup_hw() before calling this
    static void destroy(lif_impl *impl);

    /// \brief     helper function to get key given lif entry
    /// \param[in] entry    pointer to lif instance
    /// \return    pointer to the lif instance's key
    static void *lif_key_func_get(void *entry) {
        lif_impl *lif = (lif_impl *)entry;
        return (void *)&(lif->key_);
    }

    /// \brief     helper function to get (internal) lif idgiven lif entry
    /// \param[in] entry    pointer to lif instance
    /// \return    pointer to the lif instance's key
    static void *lif_id_func_get(void *entry) {
        lif_impl *lif = (lif_impl *)entry;
        return (void *)&(lif->id_);
    }

    /// \brief    handle all programming during lif creation
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create(pds_lif_spec_t *spec);

    ///< \brief    program lif tx policer for given lif
    ///< param[in] policer    policer parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_tx_policer(sdk::qos::policer_t *policer);

    ///< \brief    program lif tx scheduler for given lif
    ///< param[in] spec   lif information
    /// \return    SDK_RET_OK on success, failure status code on error
    static sdk_ret_t program_tx_scheduler(pds_lif_spec_t *spec);

    ///< \brief    reserve lif tx scheduler for given lif
    ///< param[in] spec   lif information
    /// \return    SDK_RET_OK on success, failure status code on error
    static sdk_ret_t reserve_tx_scheduler(pds_lif_spec_t *spec);

    /// \brief     ifindex this lif is pinned to
    /// \return    pinned interface index
    if_index_t pinned_ifindex(void) const {
        return pinned_if_idx_;
    }

    /// \brief     return type of the lif
    /// \return    lif type
    lif_type_t type(void) const { return type_; }

    /// \brief     return lif key
    /// \return    key of the lif
    const pds_obj_key_t& key(void) const { return key_; }

    /// \brief     return (internal) lif id
    /// \return    id of the lif
    pds_lif_id_t id(void) const { return id_; }

    /// \brief     return encoded ifindex of lif
    /// \return    ifindex of the lif
    if_index_t ifindex(void) const { return ifindex_; }

    /// \brief     return vnic hw id of this lif
    /// \return    vnic hw id
    uint16_t vnic_hw_id(void) const { return vnic_hw_id_; }

    /// \brief     set/update the name of the lif
    /// \param[in] name    name of the device corresponding to this lif
    void set_name(const char *name);

    /// \brief     return the name of the lif
    /// \return    return lif name
    const char *name(void) const { return name_; }

    /// \brief     set/update the state of the lif
    /// \param[in] state    operational state to update the lif with
    void set_state(lif_state_t state) {
        state_ = state;
    }

    /// \brief     return the operational state of the lif
    /// \return    operational state of the lif
    lif_state_t state(void) const { return state_; }

    /// \brief     set/update the admin state of the lif
    /// \param[in] state    admin state to update the lif with
    void set_admin_state(lif_state_t state);

    /// \brief     return the admin state of the lif
    /// \return    admin state of the lif
    lif_state_t admin_state(void) const { return admin_state_; }

    /// \brief     set/update the mac address of the lif
    /// \param[in] mac    mac address of the device corresponding to this lif
    void set_mac(mac_addr_t mac) {
        memcpy(mac_, mac, ETH_ADDR_LEN);
    }

    /// \brief     set total queue count
    /// \param[in] total_qcount    total queue count
    void set_total_qcount(uint32_t total_qcount) {
        total_qcount_ = total_qcount;
    }

    /// \brief     set cos bitmap
    /// \param[in] cos_bmp    cos bitmap
    void set_cos_bmp(uint16_t cos_bmp) {
        cos_bmp_ = cos_bmp;
    }

    /// \brief     return total queue count
    /// \return    total queue count
    uint32_t total_qcount(void) const { return total_qcount_; }

    /// \brief     return cos bitmap
    /// \return    cos bmp
    uint16_t cos_bmp(void) const { return cos_bmp_; }

    /// \brief     return the MAC address corresponding to this lif
    /// \return    ethernet MAC address of this lif
    mac_addr_t& mac(void) { return mac_; }

    /// \brief    return the nexthop index corresponding to this lif
    /// \return   nexthop index of the lif
    uint32_t nh_idx(void) const { return nh_idx_; }

    /// \brief    save the tx scheduler offset and number of entries to this lif
    void set_tx_sched_info(uint32_t offset, uint32_t num_entries) {
        tx_sched_offset_ = offset;
        tx_sched_num_entries_ = num_entries;
    }

    /// \brief     return if initializtion of lifis done or not
    /// \return    true if lif initialization is done
    bool create_done(void) const { return create_done_; }

    /// \brief    retrieve tx scheduler offset and number of entries from this lif
    /// \return   true if it is valid, false otherwise
    bool tx_sched_info(uint32_t *offset, uint32_t *num_entries) {
        if (tx_sched_offset_ == INVALID_INDEXER_INDEX) {
            return false;
        }
        *offset = tx_sched_offset_;
        *num_entries = tx_sched_num_entries_;
        return true;
    }

    /// \brief      reset all the statistics associated with the lif
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t reset_stats(void);

    /// \brief      track pps for lif
    /// \param[in]  interval    sampling interval in secs
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t track_pps(uint32_t interval);

    /// \brief      dump all the statistics associated with the lif
    /// \param[in]  fd  file descriptor to which output is dumped
    void dump_stats(uint32_t fd);

private:
    ///< constructor
    ///< \param[in] spec    lif configuration parameters
    lif_impl(pds_lif_spec_t *spec);

    ///< destructor
    ~lif_impl() {}

    ///< \brief    program necessary h/w entries for oob mnic lif
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_oob_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary h/w entries for inband mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_inb_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary h/w entries for datapath lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_datapath_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for internal mgmt. mnic lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_internal_mgmt_mnic_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for host (data) lifs
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_host_lif_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for learn lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_learn_lif_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for control lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_control_lif_(pds_lif_spec_t *spec);

    ///< \brief    program necessary entries for P2P MNIC lif(s)
    ///< \param[in] spec    lif configuration parameters
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t create_p2p_mnic_(pds_lif_spec_t *spec);

private:
    pds_obj_key_t    key_;            ///< lif key
    pds_lif_id_t     id_;             ///< (s/w & h/w) internal lif id
    if_index_t       pinned_if_idx_;  ///< pinnned if index, if any
    lif_type_t       type_;           ///< type of the lif
    /// name of the lif, if any
    char             name_[SDK_MAX_NAME_LEN];
    mac_addr_t       mac_;            ///< MAC address of lif
    uint32_t         tx_sched_offset_;///< tx scheduler entry offset
    /// number of tx scheduler entries used
    uint32_t         tx_sched_num_entries_;
    uint32_t         total_qcount_;   ///< total queue count
    uint16_t         cos_bmp_;        ///< cos bmp

    /// operational state
    // TODO: we can have state per pipeline in this class
    //       ideally, we should have the concrete class inside pipeline specific
    //       dir and this should be a base class !!
    if_index_t       ifindex_;        ///< ifindex of this lif
    uint32_t         nh_idx_;         ///< nexthop idx of this lif
    uint16_t         vnic_hw_id_;     ///< vnic hw id
    lif_state_t      admin_state_;    ///< admin state
    lif_state_t      state_;          ///< operational state
    bool             create_done_;    ///< TRUE if lif create is done
    ht_ctxt_t        ht_ctxt_;        ///< hash table context
    ht_ctxt_t        id_ht_ctxt_;     ///< id based hash table context
    /// lif_impl_state is friend of lif_impl
    friend class lif_impl_state;
} __PACK__;

/// \@}

}    // namespace impl
}    // namespace api

#endif    //    __LIF_IMPL_HPP__
