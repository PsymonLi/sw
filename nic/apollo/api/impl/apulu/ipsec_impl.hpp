//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec sa implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __IPSEC_IMPL_HPP__
#define __IPSEC_IMPL_HPP__

#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_ipsec.hpp"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_IPSEC_IMPL - ipsec functionality
/// \ingroup PDS_IPSEC
/// @{

// initial epoch value when ipsec sa is created
#define PDS_IMPL_IPSEC_EPOCH_START          0x0

/// \brief ipsec sa implementation
class ipsec_sa_impl : public impl_base {
public:
    /// \brief     factory method to allocate & initialize ipsec impl instance
    /// \param[in] spec ipsec specification
    /// \return    new instance of ipsec or NULL, in case of error
    static ipsec_sa_impl *factory(void *spec, bool encrypt_sa);

    /// \brief     release all the s/w state associated with the given ipsec sa,
    ///            if any, and free the memory
    /// \param[in] impl ipsec sa impl instance to be freed
    /// \NOTE      h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this)
    static void destroy(ipsec_sa_impl *impl);

    /// \brief    clone this object by copying all the h/w resources
    ///           allocated for this object into new object and return the
    ///           cloned object
    /// \return    cloned impl instance
    virtual impl_base *clone(void) override;

    /// \brief    free all the memory associated with this object without
    ///           touching any of the databases or h/w etc.
    /// \param[in] impl impl instance to be freed
    /// \return   sdk_ret_ok or error code
    static sdk_ret_t free(ipsec_sa_impl *impl);

    /// \brief     allocate/reserve h/w resources for this object
    /// \param[in] api_obj  object for which resources are being reserved
    /// \param[in] orig_obj old version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *api_obj, api_base *orig_obj,
                                        api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     free h/w resources used by this object, if any
    /// \param[in] api_obj API object holding this resource
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /// \brief     free h/w resources used by this object, if any
    ///            (this API is invoked during object deletes)
    /// \param[in] api_obj API object holding the resources
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t nuke_resources(api_base *api_obj) override;

    /// \brief     cleanup all h/w tables relevant to this object except stage 0
    ///            table(s), if any, by updating packed entries with latest
    ///            epoch#
    /// \param[in] api_obj  API object holding this resource
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     update all h/w tables relevant to this object except stage 0
    ///            table(s), if any, by updating packed entries with latest
    ///            epoch#
    /// \param[in] curr_obj current version of the unmodified object
    /// \param[in] prev_obj previous version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     re-program config in the hardware
    ///            re-program all hardware tables relevant to this object
    ///            except stage 0 table(s), if any and this reprogramming must
    ///            be based on existing state and any of the state present in
    ///            the dirty object list (like clone objects etc.)
    /// \param[in] api_obj API object being activated
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    #SDK_RET_OK on success, failure status code on error
    /// \NOTE      this method is called when an object is in the
    ///            dependent/puppet object list
    virtual sdk_ret_t reprogram_hw(api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     return current epoch
    /// \return    epoch of the ipsec sa
    uint8_t epoch(void) const { return epoch_; }

    /// \brief     return h/w id
    /// \return    h/w id assigned to the ipsec sa
    uint16_t hw_id(void) const { return hw_id_; }

    mem_addr_t base_pa(void) const { return base_pa_; }

    uint16_t nh_idx(void) const { return nh_idx_; }

    /// \brief      get the key from entry in hash table context
    /// \param[in]  entry in the hash table context
    /// \return     hw id from the entry
    static void *key_get(void *entry) {
        ipsec_sa_impl *ipsec_sa = (ipsec_sa_impl *)entry;
        return (void *)&(ipsec_sa->hw_id_);
    }

    /// \brief      accessor API for key
    pds_obj_key_t *key(void) { return &key_; }

    /// \brief      accessor API for hash table context
    ht_ctxt_t *ht_ctxt(void) { return &ht_ctxt_; }

    bool is_encrypt_sa(void) { return encrypt_sa_; }

    void set_encrypt_sa(bool enc_sa) { encrypt_sa_ = enc_sa; }

protected:
    /// \brief constructor
    ipsec_sa_impl() {
        epoch_ = PDS_IMPL_IPSEC_EPOCH_START;
        hw_id_ = 0xFFFF;
        ht_ctxt_.reset();
    }

    /// \brief destructor
    ~ipsec_sa_impl() {}

    /// \brief     program ipsec related tables during ipsec sa delete
    /// \param[in] epoch epoch being activated
    /// \param[in] ipsec_sa  ipsec obj being programmed
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_delete_(pds_epoch_t epoch, ipsec_sa_entry *ipsec);

protected:
    // P4 datapath specific state
    mem_addr_t base_pa_; ///< base physical address of cb
    uint8_t epoch_;      ///< datapath epoch of the ipsec sa
    uint32_t hw_id_;     ///< hardware id
    uint32_t nh_idx_;    ///< nexthop index
    /// PI specific info
    struct {
        pds_obj_key_t key_;
    };
    ht_ctxt_t ht_ctxt_;
    bool encrypt_sa_;   ///< true if encrypt sa
};

/// \brief ipsec encrypt sa implementation
class ipsec_sa_encrypt_impl : public ipsec_sa_impl {
public:
    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] api_obj     api object associated with the impl instance
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg, api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     program all h/w tables relevant to this object except
    ///            stage 0 table(s), if any
    /// \param[in] api_obj API object holding this resource
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     activate the epoch in the dataplane by programming stage 0
    ///            tables, if any
    /// \param[in] api_obj  (cloned) API object being activated
    /// \param[in] orig_obj previous/original unmodified object
    /// \param[in] epoch    epoch being activated
    /// \param[in] api_op   api operation
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to ipsec sa key
    /// \param[out] info pointer to ipsec sa info
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

private:
    /// \brief constructor
    ipsec_sa_encrypt_impl() {}

    /// \brief destructor
    ~ipsec_sa_encrypt_impl() {}

    /// \brief  constructor with spec
    ipsec_sa_encrypt_impl(pds_ipsec_sa_encrypt_spec_t *spec) {
        ipsec_sa_encrypt_impl();
        key_ = spec->key;
    }

    /// \brief     program ipsec related tables during ipsec sa create
    /// \param[in] epoch epoch being activated
    /// \param[in] ipsec  ipsec sa obj being programmed
    /// \param[in] spec  ipsec configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, ipsec_sa_entry *ipsec_sa,
                               pds_ipsec_sa_encrypt_spec_t *spec);

    /// \brief      fill the ipsec sa stats
    /// \param[out] stats statistics
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_stats_(pds_ipsec_sa_encrypt_stats_t *stats);

    /// \brief      fill the ipsec sa status
    /// \param[out] status status
    void fill_status_(pds_ipsec_sa_encrypt_status_t *status);

    friend class ipsec_sa_impl;
};

/// \brief ipsec decrypt sa implementation
class ipsec_sa_decrypt_impl : public ipsec_sa_impl {
public:
    /// \brief populate the IPC msg with object specific information
    ///        so it can be sent to other components
    /// \param[in] msg         IPC message to be filled in
    /// \param[in] api_obj     api object associated with the impl instance
    /// \param[in] obj_ctxt    transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t populate_msg(pds_msg_t *msg, api_base *api_obj,
                                   api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     program all h/w tables relevant to this object except
    ///            stage 0 table(s), if any
    /// \param[in] api_obj API object holding this resource
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 api_obj_ctxt_t *obj_ctxt) override;

    /// \brief     activate the epoch in the dataplane by programming stage 0
    ///            tables, if any
    /// \param[in] api_obj  (cloned) API object being activated
    /// \param[in] orig_obj previous/original unmodified object
    /// \param[in] epoch    epoch being activated
    /// \param[in] api_op   api operation
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_hw(api_base *api_obj, api_base *orig_obj,
                                  pds_epoch_t epoch, api_op_t api_op,
                                  api_obj_ctxt_t *obj_ctxt) override;

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj  API object
    /// \param[in]  key  pointer to ipsec sa key
    /// \param[out] info pointer to ipsec sa info
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

private:
    /// \brief constructor
    ipsec_sa_decrypt_impl() {}

    /// \brief destructor
    ~ipsec_sa_decrypt_impl() {}

    /// \brief  constructor with spec
    ipsec_sa_decrypt_impl(pds_ipsec_sa_decrypt_spec_t *spec) {
        ipsec_sa_decrypt_impl();
        key_ = spec->key;
    }

    /// \brief     program ipsec related tables during ipsec sa create
    /// \param[in] epoch epoch being activated
    /// \param[in] ipsec  ipsec sa obj being programmed
    /// \param[in] spec  ipsec configuration
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_create_(pds_epoch_t epoch, ipsec_sa_entry *ipsec_sa,
                               pds_ipsec_sa_decrypt_spec_t *spec);

    /// \brief      fill the ipsec sa stats
    /// \param[out] stats statistics
    /// \return     #SDK_RET_OK on success, failure status code on error
    sdk_ret_t fill_stats_(pds_ipsec_sa_decrypt_stats_t *stats);

    /// \brief      fill the ipsec sa status
    /// \param[out] status status
    void fill_status_(pds_ipsec_sa_decrypt_status_t *status);

    friend class ipsec_sa_impl;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __IPSEC_IMPL_HPP__
