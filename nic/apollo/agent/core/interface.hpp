//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#ifndef __CORE_INTERFACE_HPP__
#define __CORE_INTERFACE_HPP__

#include "nic/apollo/api/include/pds_if.hpp"

namespace core {

sdk_ret_t interface_create(pds_if_spec_t *spec,
                           pds_batch_ctxt_t bctxt);
sdk_ret_t interface_update(pds_if_spec_t *spec,
                           pds_batch_ctxt_t bctxt);
sdk_ret_t interface_delete(pds_obj_key_t *key, pds_batch_ctxt_t bctxt);
sdk_ret_t interface_get(pds_obj_key_t *key, pds_if_info_t *info);
sdk_ret_t interface_get_all(if_read_cb_t cb, void *ctxt);
sdk_ret_t interface_lldp_status_get(uint16_t lif_id, pds_if_lldp_status_t *lldp_status);
sdk_ret_t interface_lldp_stats_get(uint16_t lif_id, pds_if_lldp_stats_t *lldp_stats);
sdk_ret_t lldp_config_init(void);
sdk_ret_t lldp_sysname_update(const char *sysname);

}    // namespace core

#endif    // __CORE_INTERFACE_HPP__
