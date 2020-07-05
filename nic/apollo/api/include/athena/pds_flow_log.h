//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines Flow Log API
///
//----------------------------------------------------------------------------


#ifndef __PDS_FLOW_LOG_H__
#define __PDS_FLOW_LOG_H__

#include "pds_base.h"
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Maximum table indices
#define PDS_FLOW_LOG_IDX_MAX    (2097152)

typedef enum pds_flow_log_table_s {
    PDS_FLOW_LOG_TABLE0 = 0,
    PDS_FLOW_LOG_TABLE1,
} pds_flow_log_table_t;


typedef struct pds_flow_log_table_selector_spec_s {
    pds_flow_log_table_t   flow_log_table;    
} pds_flow_log_table_selector_spec_t;


//Key into Flow log table.
typedef struct pds_flow_log_table_key_s {
    uint32_t         entry_idx; //index in flow table to read.
                                //Required for read. Ignored
                                //for Iterate.
} pds_flow_log_table_key_t;

// Flow Parameters
typedef struct pds_flow_log_entry_key_s {
    pds_key_type_t      key_type;
    uint8_t             ip_saddr[IP6_ADDR8_LEN];
    uint8_t             ip_daddr[IP6_ADDR8_LEN];
    uint16_t            sport;
    uint16_t            dport;
    uint8_t             proto;
    bool                disposition;
    uint16_t            vnic_id;
} pds_flow_log_entry_key_t;

//Flow Meta data.
typedef struct pds_flow_log_entry_data_s {
    uint32_t      pkt_from_host;
    uint32_t      pkt_from_switch;
    uint64_t      bytes_from_host;
    uint64_t      bytes_from_switch;
    time_t        start_time;
    time_t        last_time;
    uint8_t       security_state;
} pds_flow_log_entry_data_t;

typedef struct pds_flow_log_entry_info_s {
    pds_flow_log_entry_key_t    key;
    pds_flow_log_entry_data_t   data;
} pds_flow_log_entry_info_t;


typedef struct pds_flow_log_table_iter_cb_arg_s {
    pds_flow_log_table_key_t          key;
    pds_flow_log_entry_info_t         info;
    void                              *app_data;
} pds_flow_log_table_iter_cb_arg_t;


/// \brief     select flow log table for hardware to update flow info.
/// \param[in] spec flow log table id specification
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    A valid Flow log table id should be passed
pds_ret_t pds_flow_log_table_selector_update(pds_flow_log_table_selector_spec_t *spec);

/// \brief      Read current active flow log table in hardware.
/// \param[out] Current Active flow log table id.
/// \return     #SDK_RET_OK on success, failure status code on error
pds_ret_t pds_flow_log_table_selector_read(pds_flow_log_table_t *info);

/// \brief     Flow Log table iterate callback function type
/// \remark    This function needs to be defined by the application.
typedef void (*pds_flow_log_table_iter_cb_t) (pds_flow_log_table_iter_cb_arg_t *);

/// \brief      Iterate through flow log table.
/// \param[in]  flow log table to read.
/// \param[in]  Callback function to call for a valid index.
/// \param[out] Flow log information.
/// \return     #SDK_RET_OK on success, failure status code on error
pds_ret_t pds_flow_log_table_entry_iterate(pds_flow_log_table_iter_cb_t     iter_cb,
                                           pds_flow_log_table_t                 flow_log_table,  
                                           pds_flow_log_table_iter_cb_arg_t *iter_cb_args);

/// \brief      Read an entry in a flow table identified by Key.
/// \param[in]  flow log table to read.
/// \param[in]  Key that identifies entry index to read.
/// \param[out] Flow log information.
/// \return     #SDK_RET_OK on success, failure status code on error
pds_ret_t pds_flow_log_table_entry_read(pds_flow_log_table_t flow_log_table,
                                        pds_flow_log_table_key_t *key,
                                        pds_flow_log_entry_info_t *info);

/// \brief     reset all entries in a flow log table
/// \param[in] Flow log table.
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    A valid flow log table id should be passed
pds_ret_t pds_flow_log_table_reset (pds_flow_log_table_t flow_log_table);

#ifdef __cplusplus
}
#endif

#endif  // __PDS_VNIC_H__
