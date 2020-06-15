//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport entry handling
///
//----------------------------------------------------------------------------

#ifndef __API_VPORT_HPP__
#define __API_VPORT_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_policer.hpp"
#include "nic/apollo/api/include/pds_vport.hpp"

namespace api {

// forward declaration
class vport_state;

#define PDS_VPORT_UPD_POLICY              0x1

/// \defgroup PDS_VPORT_ENTRY - vport functionality
/// \ingroup PDS_VPORT
/// @{

// attribute update bits for vport object
/// \brief    vport entry
class vport_entry : public api_base {
public:
    /// \brief  factory method to allocate and initialize a vport entry
    /// \param[in] pds_vport    vport information
    /// \return    new instance of vport or NULL, in case of error
    static vport_entry *factory(pds_vport_spec_t *pds_vport);

    /// \brief  release all the s/w state associate with the given vport,
    ///         if any, and free the memory
    /// \param[in] vport  vport to be freed
    ///                  NOTE: h/w entries should have been cleaned up (by
    ///                  calling impl->cleanup_hw() before calling this
    static void destroy(vport_entry *vport);

    /// \brief    clone this object and return cloned object
    /// \param[in]    api_ctxt API context carrying object related configuration
    /// \return       new object instance of current object
    virtual api_base *clone(api_ctxt_t *api_ctxt) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] vport    vport enry to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(vport_entry *vport);

    /// \brief     allocate h/w resources for this object
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     free h/w resources used by this object, if any
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(void) override;

    /// \brief     initialize vport entry with the given config
    /// \param[in] api_ctxt API context carrying the configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t init_config(api_ctxt_t *api_ctxt) override;

    /// \brief    return true if this object needs to be circulated to other IPC
    ///           endpoints
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    true if we need to circulate this object or else false
    virtual bool circulate(api_obj_ctxt_t *obj_ctxt) override {
        return true;
    }

    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief    program all h/w tables relevant to this object except stage 0
    ///           table(s), if any
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_create(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief    cleanup all h/w tables relevant to this object except stage 0
    ///           table(s), if any, by updating packed entries with latest
    ///           epoch#
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_config(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief    compute the object diff during update operation compare the
    ///           attributes of the object on which this API is invoked and the
    ///           attrs provided in the update API call passed in the object
    ///           context (as cloned object + api_params) and compute the upd
    ///           bitmap (and stash in the object context for later use)
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t compute_update(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief        add all objects that may be affected if this object is
    ///               updated to framework's object dependency list
    /// \param[in]    obj_ctxt    transient state associated with this API
    ///                           processing
    /// \return       SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_deps(api_obj_ctxt_t *obj_ctxt) override {
        // no other objects are effected if vport is modified
        // NOTE: assumption is that none of key or immutable fields (e.g., type
        // of vport, vlan of the vport etc.) are modifiable and agent will catch
        // such errors
        return SDK_RET_OK;
    }

    /// \brief    update all h/w tables relevant to this object except stage 0
    ///           table(s), if any, by updating packed entries with latest
    ///           epoch#
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_update(api_base *orig_obj,
                                     api_obj_ctxt_t *obj_ctxt) override;

    /// \brief    activate the epoch in the dataplane by programming stage 0
    ///           tables, if any
    /// \param[in] epoch       epoch being activated
    /// \param[in] api_op      api operation
    /// \param[in] orig_obj old/original version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_config(pds_epoch_t epoch, api_op_t api_op,
                                      api_base *orig_obj,
                                      api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          reprogram all h/w tables relevant to this object and
    ///                 dependent on other objects except stage 0 table(s),
    ///                 if any
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reprogram_config(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief re-activate config in the hardware stage 0 tables relevant to
    ///        this object, if any, this reactivation must be based on existing
    ///        state and any of the state present in the dirty object list
    ///        (like clone objects etc.) only and not directly on db objects
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    /// NOTE: this method is called when an object is in the dependent/puppet
    ///       object list
    virtual sdk_ret_t reactivate_config(pds_epoch_t epoch,
                                        api_obj_ctxt_t *obj_ctxt) override;

    ///\brief      read config
    ///\param[out] info pointer to the info object
    ///\return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t read(pds_vport_info_t *info);

    /// \brief    add given vport to the database
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_to_db(void) override;

    /// \brief    delete given vport from the database
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t del_from_db(void) override;

    /// \brief    this method is called on new object that needs to replace the
    ///           old version of the object in the DBs
    /// \param[in] orig_obj    old version of the unmodified object
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_db(api_base *orig_obj,
                                api_obj_ctxt_t *obj_ctxt) override;

    /// \brief    initiate delay deletion of this object
    virtual sdk_ret_t delay_delete(void) override;

    /// \brief    return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "vport-"  + std::string(key_.str());
    }

    /// \brief     helper function to get key given vport entry
    /// \param[in] entry    pointer to vport instance
    /// \return    pointer to the vport instance's key
    static void *vport_key_func_get(void *entry) {
        vport_entry *vport = (vport_entry *)entry;
        return (void *)&(vport->key_);
    }

    /// \brief     return the key/id of this vport
    /// \return    key/id of the vport object
    pds_obj_key_t key(void) const { return key_; }

    /// \brief     return impl instance of this vport object
    /// \return    impl instance of the vport object
    impl_base *impl(void) { return impl_; }

    /// \brief     return vport encap information of vport
    /// \return    vport encap type and value
    pds_encap_t encap(void) const { return encap_; }

    /// \brief     return number of IPv4 ingress policies on the vport
    /// \return    number of IPv4 ingress policies on the vport
    uint8_t num_ing_v4_policy(void) const {
        return num_ing_v4_policy_;
    }

    /// \brief          return ingress IPv4 security policy of the vport
    /// \param[in] n    policy number being queried
    /// \return         ingress IPv4 security policy of the vport
    pds_obj_key_t ing_v4_policy(uint32_t n) const {
        return ing_v4_policy_[n];
    }

    /// \brief     return number of IPv6 ingress policies on the vport
    /// \return    number of IPv6 ingress policies on the vport
    uint8_t num_ing_v6_policy(void) const {
        return num_ing_v6_policy_;
    }

    /// \brief          return ingress IPv6 security policy of the vport
    /// \param[in] n    policy number being queried
    /// \return         ingress IPv6 security policy of the vport
    pds_obj_key_t ing_v6_policy(uint32_t n) const {
        return ing_v6_policy_[n];
    }

    /// \brief     return number of IPv4 egress policies on the vport
    /// \return    number of IPv4 egress policies on the vport
    uint8_t num_egr_v4_policy(void) const {
        return num_egr_v4_policy_;
    }

    /// \brief          return egress IPv4 security policy of the vport
    /// \param[in] n    policy number being queried
    /// \return         egress IPv4 security policy of the vport
    pds_obj_key_t egr_v4_policy(uint32_t n) const {
        return egr_v4_policy_[n];
    }

    /// \brief     return number of IPv6 egress policies on the vport
    /// \return    number of IPv6 egress policies on the vport
    uint8_t num_egr_v6_policy(void) const {
        return num_egr_v6_policy_;
    }

    /// \brief          return egress IPv6 security policy of the vport
    /// \param[in] n    policy number being queried
    /// \return         egress IPv6 security policy of the vport
    pds_obj_key_t egr_v6_policy(uint32_t n) const {
        return egr_v6_policy_[n];
    }

private:
    /// \brief    constructor
    vport_entry();

    /// \brief    destructor
    ~vport_entry();

    /// \brief      fill the vport sw spec
    /// \param[out] spec specification
    /// \return     SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_spec_(pds_vport_spec_t *spec);

    /// \brief    free h/w resources used by this object, if any
    ///           (this API is invoked during object deletes)
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t nuke_resources_(void);

private:
    pds_obj_key_t key_;                   ///< vport key
    pds_encap_t   encap_;                 ///< vport's vlan encap
    /// number of ingress IPv4 policies
    uint8_t       num_ing_v4_policy_;
    /// ingress IPv4 policies
    pds_obj_key_t ing_v4_policy_[PDS_MAX_VPORT_POLICY];
    /// number of ingress IPv6 policies
    uint8_t       num_ing_v6_policy_;
    /// ingress IPv6 policies
    pds_obj_key_t ing_v6_policy_[PDS_MAX_VPORT_POLICY];
    /// number of egress IPv4 policies
    uint8_t       num_egr_v4_policy_;
    /// egress IPv4 policies
    pds_obj_key_t egr_v4_policy_[PDS_MAX_VPORT_POLICY];
    /// number of egress IPv6 policies
    uint8_t       num_egr_v6_policy_;
    /// egress IPv6 policies
    pds_obj_key_t egr_v6_policy_[PDS_MAX_VPORT_POLICY];

    ht_ctxt_t     ht_ctxt_;              ///< hash table context
    impl_base     *impl_;                ///< impl object instance

    /// vport_state is friend of vport_entry
    friend class vport_state;
} __PACK__;

/// @}     // end of PDS_VPORT_ENTRY

}    // namespace api

using api::vport_entry;

#endif    // __API_VPORT_HPP__
