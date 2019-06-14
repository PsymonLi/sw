//
// Copyright (c) 2019 Pensando Systems, Inc.
//
//----------------------------------------------------------------------------
///
/// \file
/// RFC tree related data structures and APIs
///
//----------------------------------------------------------------------------

#ifndef __RFC_TREE_HPP__
#define __RFC_TREE_HPP__

#include <unordered_map>
#include <rte_bitmap.h>
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/l4.hpp"
#include "nic/apollo/api/impl/artemis/rfc/rte_bitmap_utils.hpp"

using std::unordered_map;

namespace rfc {

typedef struct rfc_itree_node_data_s {
    uint16_t    class_id;      ///< RFC class id
    uint16_t    rule_no:12;    ///< rule number
    uint16_t    start:1;       ///< TRUE if this is start of the interval
    uint16_t    pad:3;         ///< pad bits
} __PACK__ rfc_itree_node_data_t;

///< interval tree node
typedef struct inode_s {
    union {
        ip_addr_t    ipaddr;    ///< IPv4/IPv6 address
        uint16_t     port;      ///< src/dst port
        uint32_t     key32;     ///< generic 32 bit key
    };
    union {
        rfc_itree_node_data_t    rfc;    ///< RFC specific data
    };
} __PACK__ inode_t;

///< interval node table
typedef struct itable_s {
    uint32_t    num_nodes;    ///< number of entries in interval table
    inode_t     *nodes;       ///< number of interval table nodes allocated
} __PACK__ itable_t;

#define RFC_MAX_EQ_CLASSES        1024
typedef unordered_map<rte_bitmap *, uint16_t,
                      rte_bitmap_hasher,
                      rte_bitmap_equal_to>    cbm_map_t;

///< generic RFC table thats used in all phases
typedef struct rfc_table_s {
    uint16_t      num_classes;
    uint16_t      max_classes;
    cbm_map_t     cbm_map;
    rte_bitmap    *cbm_table[RFC_MAX_EQ_CLASSES];
} rfc_table_t;

///< RFC tree table thats used in all phases
typedef struct rfc_tree_s {
    itable_t       itable;
    uint32_t       num_intervals;
    rfc_table_t    rfc_table;
} rfc_tree_t;

typedef struct rfc_ctxt_s {
    policy_t      *policy;        ///< user configured policy
    ///< phase 0 information
    rfc_tree_t    sip_tree;       ///< RFC tree for SIP
    rfc_tree_t    dip_tree;       ///< RFC tree for prefix
    rfc_tree_t    port_tree;      ///< RFC tree for port
    rfc_tree_t    proto_port_tree;///< RFC tree for protocol-port
    rfc_tree_t    stag_tree;      ///< RFC tree for source tags used in the policy
    rfc_tree_t    dtag_tree;      ///< RFC tree for source tags used in the policy
    ///< phase 1 information
    rfc_table_t   p1_table;       ///< phase 1 RFC table
    rfc_table_t   p2_table;       ///< phase 2 RFC table
    rte_bitmap    *cbm;           ///< RFC class bitmap instance used as scratch pad
    size_t        cbm_size;       ///< size of class-bit-map (CBM)
    mem_addr_t    base_addr;      ///< base address of the entire RFC block
    uint32_t      mem_size;       ///< RFC memory block size
} rfc_ctxt_t;

void rfc_ctxt_destroy(rfc_ctxt_t *rfc_ctxt);
sdk_ret_t rfc_ctxt_init(rfc_ctxt_t *rfc_ctxt, policy_t *policy,
                        mem_addr_t base_addr, uint32_t mem_size);
void itable_add_address_inodes(uint32_t rule, inode_t *addr_inode,
                               ip_prefix_t *pfx);
void itable_add_address_range_inodes(uint32_t rule, inode_t *addr_inode,
                                     ipvx_range_t *range);
void itable_add_tag_inodes(uint32_t rule, inode_t *tag_inode, uint32_t tag);
void itable_add_port_inodes(uint32_t rule, inode_t *port_inode,
                            port_range_t *port_range);
void itable_add_proto_port_inodes(uint32_t rule, inode_t *proto_port_inode,
                                  uint8_t ip_proto, port_range_t *port_range);
/// \brief    dump the contents of given equivalence table
/// \param[in] rfc_ctxt    RFC context carrying all of the previous phases
///                       information processed until now
void rfc_eq_class_table_dump(rfc_table_t *rfc_table);

}    // namespace rfc

#endif    // __RFC_TREE_HPP__
