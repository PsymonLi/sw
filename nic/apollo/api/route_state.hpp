//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// route table database handling
///
//----------------------------------------------------------------------------

#ifndef __ROUTE_STATE_HPP__
#define __ROUTE_STATE_HPP__

#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/route.hpp"

namespace api {

/// \defgroup PDS_ROUTE_TABLE_STATE - route table state/db functionality
/// \ingroup PDS_ROUTE
/// \@{

/// \brief    state maintained for route tables
class route_table_state : public state_base {
public:
    /// \brief constructor
    route_table_state();

    /// \brief destructor
    ~route_table_state();

     /// \brief  allocate memory required for a route table instance
     /// \return pointer to the allocated route table instance, NULL if no
     ///           memory
    route_table *alloc(void);

    /// \brief    insert given route table instance into the route table db
    /// \param[in] table    route table to be added to the db
    /// \return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t insert(route_table *table);

    /// \brief     remove the given instance of route table object from db
    /// \param[in] table    route table entry to be deleted from the db
    /// \return    pointer to the removed route table instance or NULL,
    ///            if not found
    route_table *remove(route_table *table);

    /// \brief    remove current object from the databse(s) and swap it with the
    ///           new instance of the obj (with same key)
    /// \param[in] curr_table    current instance of the route table
    /// \param[in] new_table     new instance of the route table
    /// \return   SDK_RET_OK on success, failure status code on error
    sdk_ret_t update(route_table *curr_table, route_table *new_table);

    /// \brief      free route table instance back to slab
    /// \param[in]  route_table   pointer to the allocated route table instance
    void free(route_table *table);

    /// \brief     lookup a route table in database given the key
    /// \param[in] route_table_key route table key
    route_table *find(pds_route_table_key_t *route_table_key) const;

    friend void slab_delay_delete_cb(void *timer, uint32_t slab_id, void *elem);

private:
    ht *route_table_ht(void) { return route_table_ht_; }
    slab *rout_table_slab(void) { return route_table_slab_; }
    friend class route_table;   // route_table is friend of route_table_state

private:
    ht      *route_table_ht_;      // route table database
    slab    *route_table_slab_;    // slab to allocate route table instance
};

/// \@}    // end of PDS_ROUTE_TABLE_STATE

}    // namespace api

using api::route_table_state;

#endif    // __ROUTE_STATE_HPP__
