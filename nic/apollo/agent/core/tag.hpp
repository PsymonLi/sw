//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#ifndef __CORE_TAG_HPP__
#define __CORE_TAG_HPP__

#include "nic/apollo/api/include/pds_tag.hpp"

namespace core {

typedef void (*tag_get_cb_t)(const pds_tag_info_t *spec, void *ctxt);

typedef struct tag_db_cb_ctxt_s {
    tag_get_cb_t cb;
    void         *ctxt;
} tag_db_cb_ctxt_t;

sdk_ret_t tag_create(pds_tag_key_t *key, pds_tag_spec_t *spec);
sdk_ret_t tag_update(pds_tag_key_t *key, pds_tag_spec_t *spec);
sdk_ret_t tag_delete(pds_tag_key_t *key);
sdk_ret_t tag_get(pds_tag_key_t *key, pds_tag_info_t *info);
sdk_ret_t tag_get_all(tag_get_cb_t tag_get_cb, void *ctxt);

}    // namespace core

#endif    // __CORE_TAG_HPP__
