/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    port.hpp
 *
 * @brief   APIs for port implementation
 */

#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_if.hpp"

#if !defined (__PORT_HPP__)
#define __PORT_HPP__

namespace api {

#define PDS_MAX_PORT        16

#define PDS_MGMT_MAC_FRAMES_TX_ALL      24
#define PDS_MGMT_MAC_OCTETS_TX_TOTAL    27
#define PDS_MGMT_MAC_FRAMES_RX_ALL      1
#define PDS_MGMT_MAC_OCTETS_RX_ALL      5
#define PDS_FRAMES_TX_ALL               33
#define PDS_OCTETS_TX_TOTAL             36
#define PDS_FRAMES_RX_ALL               1
#define PDS_OCTETS_RX_ALL               5

/**
 * @brief        Handle transceiver insert/remove events
 * @param[in]    xcvr_event_info    transceiver info filled by linkmgr
 */
void xcvr_event_cb(xcvr_event_info_t *xcvr_event_info);

/**
 * @brief        Handle link UP/Down events
 * @param[in]    port_event_info port event information
 */
void port_event_cb(port_event_info_t *port_event_info);

/// \brief port quiesce function
sdk_ret_t port_quiesce_all(sdk::linkmgr::linkmgr_async_response_cb_t,
                           void *response_ctxt);

/**
  * @brief   reset port stats
  * @param[in]    key         key/uuid of the port or k_pds_obj_key_invalid/NULL
  *                           for all ports
  * @return    SDK_RET_OK on success, failure status code on error
  */
sdk_ret_t port_stats_reset(const pds_obj_key_t *key);

/// \brief     update port
/// \param[in] spec port specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    valid port spec should be passed
sdk_ret_t port_update(pds_if_spec_t *spec, pds_batch_ctxt_t bctxt);

/// \brief      read port
/// \param[in]  key key/uuid of the port
/// \param[out] info port interface information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t port_get(pds_obj_key_t *key, pds_if_info_t *info);

/// \brief Read all port information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t port_get_all(if_read_cb_t cb, void *ctxt);


}    // namespace api

#endif    /** __PORT_HPP__ */
