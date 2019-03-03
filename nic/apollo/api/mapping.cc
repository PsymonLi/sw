/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    mapping.cc
 *
 * @brief   mapping entry handling
 */

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/mapping.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"

namespace api {

/**
 * @defgroup PDS_MAPPING_ENTRY - mapping entry functionality
 * @ingroup PDS_MAPPING
 * @{
 */

/**< @brief    constructor */
mapping_entry::mapping_entry() {
    stateless_ = true;
}

/**
 * @brief    factory method to allocate and initialize a mapping entry
 * @param[in] pds_mapping    mapping information
 * @return    new instance of mapping or NULL, in case of error
 */
mapping_entry *
mapping_entry::factory(pds_mapping_spec_t *pds_mapping) {
    mapping_entry *mapping;

    /**< create mapping entry with defaults, if any */
    mapping = mapping_db()->mapping_alloc();
    if (mapping) {
        new (mapping) mapping_entry();
        mapping->impl_ =
            impl_base::factory(impl::IMPL_OBJ_ID_MAPPING, pds_mapping);
        if (mapping->impl_ == NULL) {
            mapping_entry::destroy(mapping);
            return NULL;
        }
    }
    return mapping;
}

/**< @brief    destructor */
mapping_entry::~mapping_entry() {
}

/**
 * @brief    release all the s/w & h/w resources associated with this object
 *           and free the memory
 * @param[in] mapping     mapping to be freed
 * NOTE: h/w entries themselves should have been cleaned up (by calling
 *       imp->cleanup_hw() before calling this
 */
void
mapping_entry::destroy(mapping_entry *mapping) {
    mapping->release_resources();
    if (mapping->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_MAPPING, mapping->impl_);
    }
    mapping->~mapping_entry();
    mapping_db()->mapping_free(mapping);
}

/**
 * @brief     initialize mapping entry with the given config
 * @param[in] api_ctxt API context carrying the configuration
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::init_config(api_ctxt_t *api_ctxt) {
    return SDK_RET_OK;
}

 /**
  * @brief    allocate h/w resources for this object
  * @param[in] orig_obj    old version of the unmodified object
  * @param[in] obj_ctxt    transient state associated with this API
  * @return    SDK_RET_OK on success, failure status code on error
  */
 sdk_ret_t
mapping_entry::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, obj_ctxt);
}

/**
 * @brief    program all h/w tables relevant to this object except stage 0
 *           table(s), if any, during creation of the object
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::program_config(obj_ctxt_t *obj_ctxt) {
    return impl_->program_hw(this, obj_ctxt);
}

/**
 * @brief     free h/w resources used by this object, if any
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::release_resources(void) {
    return impl_->release_resources(this);
}

/**
 * @brief    cleanup all h/w tables relevant to this object except stage 0
 *           table(s), if any, by updating packed entries with latest epoch#
 * NOTE:     we shouldn't release h/w entries here !!
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::cleanup_config(obj_ctxt_t *obj_ctxt) {
    return impl_->cleanup_hw(this, obj_ctxt);
}

/**
 * @brief    update all h/w tables relevant to this object except stage 0
 *           table(s), if any, by updating packed entries with latest epoch#
 * @param[in] orig_obj    old version of the unmodified object
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::update_config(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    /**< update operation is not supported on mapping */
    return sdk::SDK_RET_INVALID_OP;
}

/**
 * @brief    activate the epoch in the dataplane by programming stage 0
 *           tables, if any
 * @param[in] epoch       epoch being activated
 * @param[in] api_op      api operation
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                           obj_ctxt_t *obj_ctxt) {
    // there is no stage 0 h/w programming for mapping , so nothing to activate
    return SDK_RET_OK;
}

/**
 * @brief    this method is called on new object that needs to replace the
 *           old version of the object in the DBs
 * @param[in] orig_obj    old version of the object being swapped out
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::update_db(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    /**
     * mappings are not added to s/w db, so its a no-op, however, since update
     * operation is not supported on this object, we shouldn't endup here
     */
    return sdk::SDK_RET_INVALID_OP;
}


/**
 * @brief add mapping to database
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::add_to_db(void) {
    /**< mappings are not added to s/w db, so its a no-op */
    return SDK_RET_OK;
}

/**
 * @brief delete mapping from database
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
mapping_entry::del_from_db(void) {
    /**< mappings are not added to s/w db, so its a no-op */
    return SDK_RET_OK;
}

/**
 * @brief    initiate delay deletion of this object
 */
sdk_ret_t
mapping_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_MAPPING, this);
}

/** @} */    // end of PDS_MAPPING_ENTRY

}    // namespace api
