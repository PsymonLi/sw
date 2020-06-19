//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// ipsec state handling
///
//----------------------------------------------------------------------------

#ifndef __API_IPSEC_STATE_HPP__
#define __API_IPSEC_STATE_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/ipsec.hpp"

namespace api {

/// \defgroup PDS_IPSEC_STATE - ipsec state functionality
/// \ingroup PDS_IPSEC
/// @{

/// \brief    state maintained for ipsec
class ipsec_sa_state : public state_base {
public:
    /// \brief constructor
    ipsec_sa_state();

    /// \brief destructor
    ~ipsec_sa_state();

    /// \brief      allocate memory required for an ipsec sa
    /// \return     pointer to the allocated ipsec sa, NULL if no memory
    ipsec_sa_entry *alloc(void);

    /// \brief    insert given ipsec sa instance into the ipsec sa db
    /// \param[in] ipsec sa    ipsec sa entry to be added to the db
    /// \return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(ipsec_sa_entry *ipsec_sa);

    /// \brief     remove the instance of ipsec sa object from db
    /// \param[in] ipsec sa    ipsec sa entry to be deleted from the db
    /// \return    pointer to the removed ipsec sa instance or NULL, if not found
    ipsec_sa_entry *remove(ipsec_sa_entry *ipsec_sa);

    /// \brief      free ipsec sa instance back to slab
    /// \param[in]  ipsec sa   pointer to the allocated ipsec sa
    void free(ipsec_sa_entry *ipsec_sa);

    /// \brief      lookup an ipsec sa in database given the key
    /// \param[in]  ipsec_sa_key ipsec sa key
    /// \return     pointer to the ipsec sa instance found or NULL
    ipsec_sa_entry *find(pds_obj_key_t *ipsec_sa_key) const;

    /// \brief API to walk all the db elements
    /// \param[in] walk_cb    callback to be invoked for every node
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t walk(state_walk_cb_t walk_cb, void *ctxt) override;

    /// \brief API to walk all the slabs
    /// \param[in] walk_cb    callback to be invoked for every slab
    /// \param[in] ctxt       opaque context passed back to the callback
    /// \return   SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t slab_walk(state_walk_cb_t walk_cb, void *ctxt) override;

    friend void slab_delay_delete_cb(void *timer, uint32_t slab_id, void *elem);

private:
    ht *ipsec_sa_ht(void) const { return ipsec_sa_ht_; }
    slab *ipsec_sa_slab(void) const { return ipsec_sa_slab_; }
    ///< ipsec_sa_entry class is friend of ipsec_sa_state
    friend class ipsec_sa_entry;

private:
    ht *ipsec_sa_ht_;               ///< hash table root
    slab *ipsec_sa_slab_;           ///< slab for allocating ipsec entry
};

static inline ipsec_sa_encrypt_entry *
ipsec_sa_encrypt_find (pds_obj_key_t *key)
{
    return (ipsec_sa_encrypt_entry *)api_base::find_obj(OBJ_ID_IPSEC_SA_ENCRYPT, key);
}

static inline ipsec_sa_decrypt_entry *
ipsec_sa_decrypt_find (pds_obj_key_t *key)
{
    return (ipsec_sa_decrypt_entry *)api_base::find_obj(OBJ_ID_IPSEC_SA_DECRYPT, key);
}

/// \@}    // end of PDS_IPSEC_STATE

}    // namespace api

using api::ipsec_sa_state;

#endif    // __API_IPSEC_STATE_HPP__
