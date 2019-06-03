//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// service mapping entry handling
///
//----------------------------------------------------------------------------

#ifndef __SERVICE_MAPPING_HPP__
#define __SERVICE_MAPPING_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_service.hpp"

namespace api {

// forward declaration
class svc_mapping_state;

/// \defgroup PDS_SVC_MAPPING - service mapping functionality
/// \ingroup PDS_SERVICE
/// @{

/// \brief    service mapping entry
class svc_mapping : public api_base {
public:
    /// \brief  factory method to allocate & initialize a service mapping entry
    /// \param[in] spec service mapping information
    /// \return    new instance of service mapping or NULL, in case of error
    static svc_mapping *factory(pds_svc_mapping_spec_t *spec);

    /// \brief  release all the s/w state associate with the given service
    ///         mapping, if any, and free the memory
    /// \param[in] service mapping service mappingto be freed
    ///                  NOTE: h/w entries should have been cleaned up (by
    ///                  calling impl->cleanup_hw() before calling this
    static void destroy(svc_mapping *vnic);

    /// \brief    build object given its key from the (sw and/or hw state we
    ///           have) and return an instance of the object (this is useful for
    ///           stateless objects to be operated on by framework during DELETE
    ///           or UPDATE operations)
    /// \param[in] key    key of object instance of interest
    static svc_mapping *build(pds_svc_mapping_key_t *key);

    /// \brief    free a stateless entry's temporary s/w only resources like
    ///           memory etc., for a stateless entry calling destroy() will
    ///          remove resources from h/w, which can't be done during ADD/UPD
    ///          etc. operations esp. when object is constructed on the fly
    /// \param[in] mapping     service mapping to be freed
    static void soft_delete(svc_mapping *mapping);

    /// \brief     initialize service mapping entry with the given config
    /// \param[in] api_ctxt API context carrying the configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t init_config(api_ctxt_t *api_ctxt) override;

    /// \brief     allocate h/w resources for this object
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        obj_ctxt_t *obj_ctxt) override;

    /// \brief    program all h/w tables relevant to this object except stage 0
    ///           table(s), if any
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_config(obj_ctxt_t *obj_ctxt) override;

    /// \brief          reprogram all h/w tables relevant to this object and
    ///                 dependent on other objects except stage 0 table(s),
    ///                 if any
    /// \param[in] api_op    API operation
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reprogram_config(api_op_t api_op) override {
        return SDK_RET_INVALID_OP;
    }

    /// \brief     free h/w resources used by this object, if any
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(void) override;

    /// \brief    cleanup all h/w tables relevant to this object except stage 0
    ///           table(s), if any, by updating packed entries with latest
    ///           epoch#
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_config(obj_ctxt_t *obj_ctxt) override;

    /// \brief    update all h/w tables relevant to this object except stage 0
    ///           table(s), if any, by updating packed entries with latest
    ///           epoch#
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_config(api_base *orig_obj,
                                    obj_ctxt_t *obj_ctxt) override;

    /// \brief    activate the epoch in the dataplane by programming stage 0
    ///           tables, if any
    /// \param[in] epoch       epoch being activated
    /// \param[in] api_op      api operation
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_config(pds_epoch_t epoch, api_op_t api_op,
                                      obj_ctxt_t *obj_ctxt) override;

    /// \brief re-activate config in the hardware stage 0 tables relevant to
    ///        this object, if any, this reactivation must be based on existing
    ///        state and any of the state present in the dirty object list
    ///        (like clone objects etc.) only and not directly on db objects
    /// \param[in] api_op API operation
    /// \return #SDK_RET_OK on success, failure status code on error
    /// NOTE: this method is called when an object is in the dependent/puppet
    ///       object list
    virtual sdk_ret_t reactivate_config(pds_epoch_t epoch,
                                        api_op_t api_op) override {
        return SDK_RET_INVALID_OP;
    }

    /// \brief    add given service mapping to the database
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_to_db(void) override;

    /// \brief    delete given service mapping from the database
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t del_from_db(void) override;

    /// \brief    this method is called on new object that needs to replace the
    ///           old version of the object in the DBs
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_db(api_base *orig_obj,
                                obj_ctxt_t *obj_ctxt) override;

    /// \brief    initiate delay deletion of this object
    virtual sdk_ret_t delay_delete(void) override;

    /// \brief        add all objects that may be affected if this object is
    ///               updated to framework's object dependency list
    /// \param[in]    obj_ctxt    transient state associated with this API
    ///                           processing
    /// \return       SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_deps(obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_INVALID_OP;
    }

    /// \brief    return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "svc-";
    }

    ///\brief read config
    ///\param[in]  key Pointer to the key object
    ///\param[out] info Pointer to the info object
    ///\return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t read(pds_svc_mapping_key_t *key, pds_svc_mapping_info_t *info);

    /// \brief     return impl instance of this service mapping object
    /// \return    impl instance of the service mapping object
    impl_base *impl(void) { return impl_; }

private:
    /// \brief    constructor
    svc_mapping();

    /// \brief    destructor
    ~svc_mapping();

    /// \brief    free h/w resources used by this object, if any
    ///           (this API is invoked during object deletes)
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t nuke_resources_(void);

private:
    impl_base                *impl_;            ///< impl object instance
} __PACK__;

/// @}     // end of PDS_SVC_MAPPING

}    // namespace api

using api::svc_mapping;

#endif    // __SVC_MAPPING_HPP__
