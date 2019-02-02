/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    security_policy.cc
 *
 * @brief   security policy handling
 */

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/security_policy.hpp"
#include "nic/apollo/api/oci_state.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"

namespace api {

/**
 * @defgroup OCI_SECURITY_POLICY - security policy functionality
 * @ingroup OCI_SECURITY_POLICY
 * @{
 */

/**< @brief    constructor */
security_policy::security_policy() {
    //SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    ht_ctxt_.reset();
}

/**
 * @brief    factory method to allocate & initialize a security policy instance
 * @param[in] oci_security_policy    security policy information
 * @return    new instance of security policy or NULL, in case of error
 */
security_policy *
security_policy::factory(oci_security_policy_t *oci_security_policy) {
    security_policy    *policy;

    /**< create security policy instance with defaults, if any */
    policy = security_policy_db()->security_policy_alloc();
    if (policy) {
        new (policy) security_policy();
        policy->impl_ = impl_base::factory(impl::IMPL_OBJ_ID_SECURITY_POLICY,
                                           oci_security_policy);
        if (policy->impl_ == NULL) {
            security_policy::destroy(policy);
            return NULL;
        }
    }
    return policy;
}

/**< @brief    destructor */
security_policy::~security_policy() {
    // TODO: fix me
    //SDK_SPINLOCK_DESTROY(&slock_);
}

/**
 * @brief    release all the s/w & h/w resources associated with this object,
 *           if any, and free the memory
 * @param[in] policy     security policy to be freed
 * NOTE: h/w entries themselves should have been cleaned up (by calling
 *       impl->cleanup_hw() before calling this
 */
void
security_policy::destroy(security_policy *policy) {
    if (policy->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_SECURITY_POLICY, policy->impl_);
    }
    policy->release_resources();
    policy->~security_policy();
    security_policy_db()->security_policy_free(policy);
}

/**
 * @brief     initialize security policy instance with the given config
 * @param[in] api_ctxt API context carrying the configuration
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::init_config(api_ctxt_t *api_ctxt) {
    oci_security_policy_t    *oci_security_policy;
    
    oci_security_policy = &api_ctxt->api_params->security_policy_info;
    memcpy(&this->key_, &oci_security_policy->key,
           sizeof(oci_security_policy_key_t));
    return SDK_RET_OK;
}

/**
 * @brief    allocate h/w resources for this object
 * @param[in] orig_obj    old version of the unmodified object
 * @param[in] obj_ctxt    transient state associated with this API
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this);
}

/**
 * @brief    program all h/w tables relevant to this object except stage 0
 *           table(s), if any
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::program_config(obj_ctxt_t *obj_ctxt) {
    OCI_TRACE_DEBUG("Programming security policy %u", key_.id);
    return impl_->program_hw(this, obj_ctxt);
}

/**
 * @brief     free h/w resources used by this object, if any
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::release_resources(void) {
    return impl_->release_resources(this);
}

/**
 * @brief    cleanup all h/w tables relevant to this object except stage 0
 *           table(s), if any, by updating packed entries with latest epoch#
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::cleanup_config(obj_ctxt_t *obj_ctxt) {
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
security_policy::update_config(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    //return impl_->update_hw();
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
security_policy::activate_config(oci_epoch_t epoch, api_op_t api_op,
                            obj_ctxt_t *obj_ctxt) {
    OCI_TRACE_DEBUG("Activating security policy %u config", key_.id);
    return impl_->activate_hw(this, epoch, api_op, obj_ctxt);
}

/**
 * @brief    this method is called on new object that needs to replace the
 *           old version of the object in the DBs
 * @param[in] orig_obj    old version of the object being swapped out
 * @param[in] obj_ctxt    transient state associated with this API
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::update_db(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

/**
 * @brief add security policy to database
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::add_to_db(void) {
    return security_policy_db()->security_policy_ht()->insert_with_key(&key_,
                                                           this, &ht_ctxt_);
}

/**
 * @brief delete security policy from database
 * @return   SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
security_policy::del_from_db(void) {
    security_policy_db()->security_policy_ht()->remove(&key_);
    return SDK_RET_OK;
}

/**
 * @brief    initiate delay deletion of this object
 */
sdk_ret_t
security_policy::delay_delete(void) {
    return delay_delete_to_slab(OCI_SLAB_ID_SECURITY_POLICY, this);
}
/** @} */    // end of OCI_SECURITY_POLICY

}    // namespace api
