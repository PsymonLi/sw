//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file handles ipsec entry handling
///
//----------------------------------------------------------------------------

#ifndef __API_IPSEC_HPP__
#define __API_IPSEC_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"

namespace api {

// forward declaration
class ipsec_sa_state;

/// \defgroup PDS_IPSEC_ENTRY - ipsec entry functionality
/// \ingroup PDS_IPSEC
/// @{

class ipsec_sa_entry : public api_base {
public:
    /// \brief          factory method to allocate and initialize a ipsec entry
    /// \param[in]      pds_ipsec    ipsec information
    /// \return         new instance of ipsec or NULL, in case of error
    static ipsec_sa_entry *factory(void *spec, bool encrypt_sa);

    /// \brief    clone this object and return cloned object
    /// \param[in]    api_ctxt API context carrying object related configuration
    /// \return       new object instance of current object
    virtual api_base *clone(api_ctxt_t *api_ctxt) override;

    /// \brief    return true if this object needs to be circulated to other IPC
    ///           endpoints
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return    true if we need to circulate this object or else false
    virtual bool circulate(api_obj_ctxt_t *obj_ctxt) override {
        return true;
    }

    /// \brief          allocate h/w resources for this object
    /// \param[in]      orig_obj    old version of the unmodified object
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          free h/w resources used by this object, if any
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(void) override;

    /// \brief          initialize ipsec sa entry with the given config
    /// \param[in]      api_ctxt API context carrying the configuration
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t init_config(api_ctxt_t *api_ctxt) override;

    /// \brief          program all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_create(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          cleanup all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any, by updating packed entries
    ///                 with latest epoch#
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_config(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          add all objects that may be affected if this object is
    ///                 updated to framework's object dependency list
    /// \param[in]      obj_ctxt    transient state associated with this API
    ///                             processing
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_deps(api_obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief          update all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any, by updating packed entries
    ///                 with latest epoch#
    /// \param[in]      orig_obj    old version of the unmodified object
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_update(api_base *orig_obj,
                                     api_obj_ctxt_t *obj_ctxt) override;

    /// \param[in]      epoch       epoch being activated
    /// \param[in]      api_op      api operation
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \param[in]      orig_obj old/original version of the unmodified object
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_config(pds_epoch_t epoch, api_op_t api_op,
                                      api_base *orig_obj,
                                      api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          reprogram all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reprogram_config(api_obj_ctxt_t *obj_ctxt) override;

    /// \brief re-activate config in the hardware stage 0 tables relevant to
    ///        this object, if any, this reactivation must be based on existing
    ///        state and any of the state present in the dirty object list
    ///        (like clone objects etc.) only and not directly on db objects
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    /// NOTE: this method is called when an object is in the dependent/puppet
    ///       object list
    virtual sdk_ret_t reactivate_config(pds_epoch_t epoch,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          add given ipsec sa to the database
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_to_db(void) override;

    /// \brief          delete given ipsec sa from the database
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t del_from_db(void) override;

    /// \brief          initiate delay deletion of this object
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t delay_delete(void) override;

    /// \brief          release all the s/w state associate with the given
    ///                 ipsec sa, if any, and free the memory
    /// \param[in]      ipsec sa     ipsec sa to be freed
    /// \NOTE: h/w entries should have been cleaned up (by calling
    ///        impl->cleanup_hw() before calling this
    static void destroy(ipsec_sa_entry *ipsec_sa);

    /// \brief     return impl instance of this ipsec sa object
    /// \return    impl instance of the ipsec sa object
    impl_base *impl(void) { return impl_; }

    bool is_encrypt_sa(void) { return encrypt_sa_; }
    void set_encrypt_sa(bool enc_sa) { encrypt_sa_ = enc_sa; }

    /// \brief          helper function to get key given ipsec sa entry
    /// \param[in]      entry    pointer to ipsec sa instance
    /// \return         pointer to the ipsec sa instance's key
    static void *ipsec_sa_key_func_get(void *entry) {
        ipsec_sa_entry *ipsec_sa = (ipsec_sa_entry *)entry;
        return (void *)&(ipsec_sa->key_);
    }

    /// \brief     return the key/id of this object
    /// \return    key/id of the ipsec sa object
    pds_obj_key_t key(void) const { return key_; }

    /// \brief    free h/w resources used by this object, if any
    ///           (this API is invoked during object deletes)
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t nuke_resources_(void);
protected:
    /// \brief constructor
    ipsec_sa_entry();

    /// \brief destructor
    ~ipsec_sa_entry();

    pds_obj_key_t key_;               ///< ipsec sa key

    /// operational state
    bool encrypt_sa_;                 ///< true if encrypt sa

    // P4 datapath specific state
    impl_base *impl_;                 ///< impl object instance

    ht_ctxt_t ht_ctxt_;               ///< hash table context
    friend class ipsec_sa_state;
} __PACK__;

/// \brief    ipsec entry
class ipsec_sa_encrypt_entry : public ipsec_sa_entry {
public:

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] ipsec sa    ipsec sa to be freed
    /// \return   sdk_ret_ok or error code
    //
    static sdk_ret_t free(ipsec_sa_encrypt_entry *ipsec_sa_encrypt);
    /// \brief          read config
    /// \param[out]     info pointer to the info object
    /// \return         SDK_RET_OK on success, failure status code on error
    sdk_ret_t read(pds_ipsec_sa_encrypt_info_t *info);

    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "ipsec_sa_encrypt-" + std::string(key_.str());
    }

private:
    /// \brief constructor
    ipsec_sa_encrypt_entry();

    /// \brief destructor
    ~ipsec_sa_encrypt_entry();

    /// \brief      fill the ipsec sa sw spec
    /// \param[out] spec specification
    void fill_spec_(pds_ipsec_sa_encrypt_spec_t *spec);
} __PACK__;

class ipsec_sa_decrypt_entry : public ipsec_sa_entry {
public:

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] ipsec sa    ipsec sa to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(ipsec_sa_decrypt_entry *ipsec_sa_decrypt);

    /// \brief          read config
    /// \param[out]     info pointer to the info object
    /// \return         SDK_RET_OK on success, failure status code on error
    sdk_ret_t read(pds_ipsec_sa_decrypt_info_t *info);

    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief          return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "ipsec_sa_decrypt-" + std::string(key_.str());
    }

private:
    /// \brief constructor
    ipsec_sa_decrypt_entry();

    /// \brief destructor
    ~ipsec_sa_decrypt_entry();

    /// \brief      fill the ipsec sa sw spec
    /// \param[out] spec specification
    void fill_spec_(pds_ipsec_sa_decrypt_spec_t *spec);
} __PACK__;

/// \@}    // end of PDS_IPSEC_ENTRY

}    // namespace api

using api::ipsec_sa_entry;
using api::ipsec_sa_encrypt_entry;
using api::ipsec_sa_decrypt_entry;

#endif    // __API_IPSEC_HPP__
