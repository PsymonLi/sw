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

sdk_ret_t port_shutdown_all(void);

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
