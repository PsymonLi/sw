//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains nexthop object utility routines
///
//----------------------------------------------------------------------------

#ifndef __API_NEXTHOP_UTILS_HPP__
#define __API_NEXTHOP_UTILS_HPP__

#include<iostream>
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/api/nexthop.hpp"

inline std::ostream&
operator<<(std::ostream& os, const pds_nexthop_key_t *key) {
    os << " id: " << key->id;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_nexthop_spec_t *spec) {
    os << &spec->key
       << " type: " << spec->type
       << " ip: " << spec->ip
       << " mac: " << macaddr2str(spec->mac)
       << " vlan: " << spec->vlan
       << " vpc: " << spec->vpc.id;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_nexthop_status_t *status) {
    os << " HW id: " << status->hw_id;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_nexthop_info_t *obj) {
    os << "NH info =>"
       << &obj->spec
       << &obj->status
       << &obj->stats
       << std::endl;
    return os;
}

#endif    // __API_NEXTHOP_UTILS_HPP__
