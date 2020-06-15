//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport state handling
///
//----------------------------------------------------------------------------

#ifndef __API_VPORT_STATE_HPP__
#define __API_VPORT_STATE_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/lib/indexer/indexer.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/vport.hpp"

namespace api {

/// \defgroup PDS_VPORT_STATE - VPORT state functionality
/// \ingroup PDS_VPORT
/// @{

/// \brief state maintained for vports
class vport_state : public state_base {
public:
    /// \brief constructor
    vport_state();

    /// \brief destructor
    ~vport_state();

    /// \brief  allocate memory required for a vport instance
    /// \return pointer to the allocated vport, NULL if no memory
    vport_entry *alloc(void);

    /// \brief     insert given vport instance into the vport db
    /// \param[in] vport vport entry to be added to the db
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(vport_entry *vport);

    /// \brief     remove the given instance of vport object from db
    /// \param[in] vport vport entry to be deleted from the db
    /// \return    pointer to the removed vport instance or NULL, if not found
    vport_entry *remove(vport_entry *vport);

    /// \brief     free vport instance back to slab
    /// \param[in] vport pointer to the allocated vport
    void free(vport_entry *vport);

    /// \brief     lookup a vport in database given the key
    /// \param[in] vport key for the vport object
    /// \return    pointer to the vport instance found or NULL
    vport_entry *find(pds_obj_key_t *key) const;

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

    /// \brief     return the vport hash table instance
    /// \return    pointer to the vport hash table instance
    ht *vport_ht(void) const { return vport_ht_; }

    friend void slab_delay_delete_cb(void *timer, uint32_t slab_id, void *elem);

private:
    slab *vport_slab(void) const { return vport_slab_; }
    friend class vport_entry;    ///< vport_entry class is friend of vport_state

private:
    ht *vport_ht_;      ///< vport hash table
    slab *vport_slab_;  ///< slab for allocating vport entry
};

static inline vport_entry *
vport_find (pds_obj_key_t *key)
{
    return (vport_entry *)api_base::find_obj(OBJ_ID_VPORT, key);
}

/// \@}

}    // namespace api

using api::vport_state;

#endif    // __API_VPORT_STATE_HPP__
