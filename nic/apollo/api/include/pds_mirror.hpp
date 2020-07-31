//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines mirror specific APIs
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_MIRROR_HPP__
#define __INCLUDE_API_PDS_MIRROR_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_vpc.hpp"

/// \defgroup PDS_MIRROR    mirror session APIs
/// @{

#define PDS_MAX_MIRROR_SESSION    8   ///< maximum mirror sessions
/// minimum snap length for mirrored packets
#define PDS_MIRROR_SESSION_MIN_SNAP_LEN    64

/// \brief    RSPAN configuration
typedef struct pds_rspan_spec_s {
    pds_obj_key_t interface;    ///< outgoing (lif/uplink) interface
    pds_encap_t encap;          ///< encap details
} __PACK__ pds_rspan_spec_t;

/// \brief ERSPAN packet formats
typedef enum pds_erspan_type_s {
    PDS_ERSPAN_TYPE_NONE = 0,
    PDS_ERSPAN_TYPE_1    = 1,
    PDS_ERSPAN_TYPE_2    = 2,
    PDS_ERSPAN_TYPE_3    = 3,
} pds_erspan_type_t;

typedef enum pds_erspan_dst_type_e {
    PDS_ERSPAN_DST_TYPE_NONE    = 0,
    ///< ERSPAN collector is a configured/discovered tunnel in the underlay
    PDS_ERSPAN_DST_TYPE_TEP     = 1,
    ///< ERSPAN collector is an IP address in the given VPC
    PDS_ERSPAN_DST_TYPE_IP      = 2,
} pds_erspan_dst_type_t;

/// \brief    ERSPAN configuration
/// \remark    source IP used in the ERSPAN packet is either:
///            1. subnet VR IP in case DstIP is in a VPC of type VPC_TYPE_TENANT
///            2. local TEP (MyTEP) IP in case DstIP is in VPC  of type
///               VPC_TYPE_UNDERLAY
typedef struct pds_erspan_spec_s {
    pds_erspan_type_t type;           ///< ERSPAN type
    pds_obj_key_t vpc;                ///< vpc of the destination IP
    pds_erspan_dst_type_t dst_type;   ///< ERSPAN destination/collect type
    union {
		///< ip_addr is the ERSPAN collector IP
		///< NOTE: currently ip_addr is supported
		///< 1. when vpc is underlay VPC or
		///< 2. if it the IP address of a local/remote mapping in the overlay
		///<    if this IP is in the overlay but reachable via route and not a
		///<    local/remote mapping, then an error is returned
        ip_addr_t ip_addr;
        ///< ERSPAN destination is underlay TEP (vpc is underlay VPC in
        ///< this case)
        pds_obj_key_t tep;
    };
    uint32_t dscp;                    ///< DSCP value to use in the packet
    uint32_t span_id;                 ///< SPAN ID used in ERSPAN header
    /// strip dot1q tag of the pkt before mirroring
    bool vlan_strip_en;
} __PACK__ pds_erspan_spec_t;

/// \brief    mirror session type
typedef enum pds_mirror_session_type_e {
    PDS_MIRROR_SESSION_TYPE_RSPAN  = 0,    ///< RSPAN mirror session type
    PDS_MIRROR_SESSION_TYPE_ERSPAN = 1,    ///< ERSPAN mirror session type
    PDS_MIRROR_SESSION_TYPE_MAX    = PDS_MIRROR_SESSION_TYPE_ERSPAN,
} pds_mirror_session_type_t;

/// \brief    mirror session configuration
typedef struct pds_mirror_session_spec_s {
    pds_obj_key_t key;                    ///< key of the mirror session
    uint16_t snap_len;                    ///< max len. of pkt mirrored
    pds_mirror_session_type_t type;       ///< mirror session type
    union {
        pds_rspan_spec_t rspan_spec;      ///< RSPAN configuration
        pds_erspan_spec_t erspan_spec;    ///< ERSPAN configuration
    };
} __PACK__ pds_mirror_session_spec_t;

/// \brief    mirror session operational status
typedef struct pds_mirror_session_status_s {
    ///< h/w id of the mirror session
    uint16_t hw_id;
} __PACK__ pds_mirror_session_status_t;

/// \brief    mirror session statistics, if any
typedef struct pds_mirror_session_stats_s {
    uint64_t packet_count;     ///< number of packets mirrored
    uint64_t byte_count;       ///< number of bytes mirrored
} __PACK__ pds_mirror_session_stats_t;

/// \brief mirror session info
typedef struct pds_mirror_session_info_s {
    pds_mirror_session_spec_t spec;        ///< specification
    pds_mirror_session_status_t status;    ///< status
    pds_mirror_session_stats_t stats;      ///< statistics
} __PACK__ pds_mirror_session_info_t;

/// \brief    create mirror session
/// \param[in] spec    mirror session configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_create(pds_mirror_session_spec_t *spec,
                                    pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    read mirror session information
/// \param[in] key    key of the mirror session
/// \param[out] info    mirror session information
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_read(pds_obj_key_t *key,
                                  pds_mirror_session_info_t *info);

typedef void (*mirror_session_read_cb_t)(const pds_mirror_session_info_t *info, void *ctxt);

/// \brief    read all mirror session information
/// \param[in]  read_cb    callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_read_all(mirror_session_read_cb_t read_cb,
                                      void *ctxt);

/// \brief read mirror session
/// \param[in] key    pointer to mirror session key
/// \param[out] info  mirror session information
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_read(pds_obj_key_t *key,
                                  pds_mirror_session_info_t *info);

/// \brief    update mirror session
/// \param[in] spec    mirror session configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_update(pds_mirror_session_spec_t *spec,
                                    pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    delete mirror session
/// \param[in] key    mirror session key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_mirror_session_delete(pds_obj_key_t *key,
                                    pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    ///  __INCLUDE_API_PDS_MIRROR_HPP__
