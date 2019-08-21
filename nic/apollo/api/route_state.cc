//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// route table database handling
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/route_state.hpp"

namespace api {

/// \defgroup PDS_ROUTE_TABLE_STATE - route table state/db functionality
/// \ingroup PDS_ROUTE
/// \@{

route_table_state::route_table_state() {
    // TODO: need to tune multi-threading related params later
    route_table_ht_ = ht::factory(PDS_MAX_ROUTE_TABLE >> 2,
                                  route_table::route_table_key_func_get,
                                  route_table::route_table_hash_func_compute,
                                  route_table::route_table_key_func_compare);
    SDK_ASSERT(route_table_ht() != NULL);

    route_table_slab_ = slab::factory("route-table", PDS_SLAB_ID_ROUTE_TABLE,
                                      sizeof(route_table), 16, true, true,
                                      true, NULL);
    SDK_ASSERT(route_table_slab() != NULL);
}

route_table_state::~route_table_state() {
    ht::destroy(route_table_ht());
    slab::destroy(route_table_slab());
}

route_table *
route_table_state::alloc(void) {
    return ((route_table *)route_table_slab()->alloc());
}

sdk_ret_t
route_table_state::insert(route_table *table) {
    return route_table_ht()->insert_with_key(&table->key_, table,
                                             &table->ht_ctxt_);
}

route_table *
route_table_state::remove(route_table *table) {
    return (route_table *)(route_table_ht()->remove(&table->key_));
}

sdk_ret_t
route_table_state::update(route_table *curr_table, route_table *new_table) {
    // lock();
    // remove(curr_table);
    // insert(new_table);
    // unlock();
    return sdk::SDK_RET_INVALID_OP;
}

void
route_table_state::free(route_table *rtable) {
    route_table_slab()->free(rtable);
}

route_table *
route_table_state::find(pds_route_table_key_t *route_table_key) const {
    return (route_table *)(route_table_ht()->lookup(route_table_key));
}

/// \@}    // end of PDS_ROUTE_TABLE_STATE

}    // namespace api
