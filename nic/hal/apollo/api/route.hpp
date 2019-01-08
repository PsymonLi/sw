/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    route.hpp
 *
 * @brief   This file deals with route functionality
 */

#if !defined (__ROUTE_HPP__)
#define __ROUTE_HPP__

#include "nic/sdk/include/sdk/slab.hpp"
#include "nic/sdk/include/sdk/ht.hpp"
#include "nic/hal/apollo/framework/api_base.hpp"
#include "nic/hal/apollo/framework/impl_base.hpp"
#include "nic/hal/apollo/include/api/oci_route.hpp"

namespace api {

/**
 * @defgroup OCI_ROUTE_TABLE - route table functionality
 * @ingroup OCI_ROUTE
 * @{
 */

/**
 * @brief    route table
 */
class route_table : public api_base {
public:
    /**
     * @brief    factory method to allocate & initialize a route table instance
     * @param[in] oci_route_table    route table information
     * @return    new instance of route table or NULL, in case of error
     */
    static route_table *factory(oci_route_table_t *oci_route_table);

    /**
     * @brief    release all the s/w state associate with the given route table,
     *           if any, and free the memory
     * @param[in] rtable     route table to be freed
     * NOTE: h/w entries should have been cleaned up (by calling
     *       impl->cleanup_hw() before calling this
     */
    static void destroy(route_table *table);

    /**
     * @brief     initialize route table instance with the given config
     * @param[in] api_ctxt API context carrying the configuration
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t init_config(api_ctxt_t *api_ctxt) override;

    /**
     * @brief    program all h/w tables relevant to this object except stage 0
     *           table(s), if any
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t program_config(obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    cleanup all h/w tables relevant to this object except stage 0
     *           table(s), if any, by updating packed entries with latest epoch#
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t cleanup_config(obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    update all h/w tables relevant to this object except stage 0
     *           table(s), if any, by updating packed entries with latest epoch#
     * @param[in] orig_obj    old version of the unmodified object
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t update_config(api_base *orig_obj, obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    activate the epoch in the dataplane by programming stage 0
     *           tables, if any
     * @param[in] epoch       epoch being activated
     * @param[in] api_op      api operation
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t activate_config(oci_epoch_t epoch, api_op_t api_op,
                                      obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief     add given route table to the database
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t add_to_db(void) override;

    /**
     * @brief     delete route table from the database
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t del_from_db(void) override;

    /**
     * @brief    this method is called on new object that needs to replace the
     *           old version of the object in the DBs
     * @param[in] orig_obj    old version of the unmodified object
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t update_db(api_base *orig_obj, obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    initiate delay deletion of this object
     */
    virtual sdk_ret_t delay_delete(void) override;

    /**
     * @brief     helper function to get key given route table
     * @param[in] entry    pointer to route table instance
     * @return    pointer to the route table instance's key
     */
    static void *route_table_key_func_get(void *entry) {
        route_table *table = (route_table *)entry;
        return (void *)&(table->key_);
    }

    /**
     * @brief     helper function to compute hash value of route table key
     * @param[in] key        route table's key
     * @param[in] ht_size    hash table size
     * @return    hash value
     */
    static uint32_t route_table_hash_func_compute(void *key, uint32_t ht_size) {
        return hash_algo::fnv_hash(key, sizeof(oci_route_table_key_t)) % ht_size;
    }

    /**
     * @brief     helper function to compare two route table keys
     * @param[in] key1        pointer to route table's key
     * @param[in] key2        pointer to route table's key
     * @return    0 if keys are same or else non-zero value
     */
    static bool route_table_key_func_compare(void *key1, void *key2) {
        SDK_ASSERT((key1 != NULL) && (key2 != NULL));
        if (!memcmp(key1, key2, sizeof(oci_route_table_key_t))) {
            return true;
        }
        return false;
    }

    /**
     * @brief     return impl instance of this route table object
     * @return    impl instance of the rout table object
     */
    impl_base *impl(void) { return impl_; }

private:
    /**< @brief    constructor */
    route_table();

    /**< @brief    destructor */
    ~route_table();

    /**
     * @brief    allocate h/w resources for this object
     * @return    SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t reserve_resources_(void);

    /**
     * @brief     free h/w resources used by this object, if any
     * @return    SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t release_resources_(void);

private:
    oci_route_table_key_t    key_;        /**< route table key */
    ht_ctxt_t                ht_ctxt_;    /**< hash table context */
    impl_base                *impl_;      /**< impl object instance */
} __PACK__;

/** @} */    // end of OCI_ROUTE_TABLE

/**
 * @defgroup OCI_ROUTE_TABLE_STATE - route table state/db functionality
 * @ingroup OCI_ROUTE
 * @{
 */

/**
 * @brief    state maintained for route tables
 */
class route_table_state : public obj_base {
public:
    /**
     * @brief    constructor
     */
    route_table_state();

    /**
     * @brief    destructor
     */
    ~route_table_state();

    /**
     * @brief    allocate memory required for a route table instance
     * @return pointer to the allocated route table instance, NULL if no memory
     */
    route_table *route_table_alloc(void);

    /**
     * @brief      free route table instance back to slab
     * @param[in]  route_table   pointer to the allocated route table instance
     */
    void route_table_free(route_table *table);

    /**
     * @brief     lookup a route table in database given the key
     * @param[in] route_table_key route table key
     */
    route_table *route_table_find(oci_route_table_key_t *route_table_key) const;

    friend void slab_delay_delete_cb(void *timer, uint32_t slab_id, void *elem);

private:
    ht *route_table_ht(void) { return route_table_ht_; }
    slab *rout_table_slab(void) { return route_table_slab_; }
    friend class route_table;   /**< route_table class is friend of route_table_state */

private:
    ht      *route_table_ht_;      /**< route table database */
    slab    *route_table_slab_;    /**< slab to allocate route table instance */
};

/** @} */    // end of OCI_ROUTE_TABLE_STATE

}    // namespace api

using api::route_table;
using api::route_table_state;

#endif    /** __ROUTE_HPP__ */
