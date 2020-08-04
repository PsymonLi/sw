//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// security policy implementation state
///
//----------------------------------------------------------------------------

#ifndef __SECURITY_POLICY_STATE_IMPL_HPP__
#define __SECURITY_POLICY_IMPL_STATE_HPP__

#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/pds_state.hpp"

// number of bytes per rule for rule stats
#define PDS_IMPL_SECURITY_POLICY_RULE_STATS_RECORD_SIZE    8

namespace api {
namespace impl {

/// \defgroup PDS_SECURITY_POLICY_IMPL_STATE - security policy impl state
///           functionality
/// \ingroup PDS_SECURITY_POLICY
/// @{

// forward declaration
class security_policy_impl;

/// \brief state maintained for security policies
class security_policy_impl_state : public state_base {
public:
    /// \brief constructor
    security_policy_impl_state(pds_state *state);

    /// \brief destructor
    ~security_policy_impl_state();

    /// \brief  allocate memory required for a security policy impl instance
    /// \return pointer to the allocated instance, NULL if no memory
    security_policy_impl *alloc(void);

    /// \brief     free security policy impl instance back
    /// \param[in] impl pointer to the allocated impl instance
    void free(security_policy_impl *impl);

    /// \brief  API to initiate transaction over all the table manamgement
    ///         library instances
    /// \return SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_begin(void);

    /// \brief  API to end transaction over all the table manamgement
    ///         library instances
    /// \return SDK_RET_OK on success, failure status code on error
    sdk_ret_t table_transaction_end(void);

    /// \brief  return policy region's base/start address in memory
    /// \param[in] af    address family of the policy
    /// \return memory address of the security policy region
    mem_addr_t security_policy_region_addr(uint8_t af) const {
        return (af == IP_AF_IPV4) ? v4_region_addr_ : v6_region_addr_;
    }

    /// \brief return security policy table's size
    /// \param[in] af    address family of the policy
    /// \return size of memory set aside for each security policy
    uint64_t security_policy_table_size(uint8_t af) const {
        return (af == IP_AF_IPV4) ? v4_table_size_ : v6_table_size_;
    }

    /// \brief return rule stats region's base/start address in memory
    /// \param[in] af    address family of the policy
    mem_addr_t rule_stats_region_addr(uint8_t af) const {
        return (af == IP_AF_IPV4) ?
            v4_rule_stats_region_addr_ : INVALID_MEM_ADDRESS;
    }

    /// \brief return security policy's rule stats block size
    /// \param[in] af    address family of the policy
    /// \return size of the rule stats memory set aside for each security policy
    uint16_t rule_stats_table_size(uint8_t af) const {
        return (af == IP_AF_IPV4) ? v4_rule_stats_table_size_ : 0;
    }

    /// \brief return max rules per security policy
    /// \param[in] af    address family of the policy
    uint32_t max_rules(uint8_t af) const {
        return (af == IP_AF_IPV4) ? v4_max_rules_ : v6_max_rules_;
    }

private:
    rte_indexer *security_policy_idxr(uint8_t af) {
        return (af == IP_AF_IPV4) ? v4_idxr_ : v6_idxr_;
    }
    friend class security_policy_impl;

private:
    /// indexer to allocate mem block for v4 policy tables
    rte_indexer *v4_idxr_;
    /// indexer to allocate mem block for v6 policy tables
    rte_indexer *v6_idxr_;
    /// base address for v4 policy region
    mem_addr_t  v4_region_addr_;
    /// size of each v4 policy table
    uint32_t    v4_table_size_;
    /// max IPv4 rules per policy
    uint32_t    v4_max_rules_;
    /// base address for v6 policy region
    mem_addr_t  v6_region_addr_;
    /// size of each v6 policy table
    uint32_t    v6_table_size_;
    /// max IPv6 rules per policy
    uint32_t    v6_max_rules_;
    /// IPv4 rule stats region based address or INVALID_MEM_ADDRESS in case
    /// rule stats are not supported
    mem_addr_t  v4_rule_stats_region_addr_;
    /// size of each policy's rule stats block size
    uint32_t    v4_rule_stats_table_size_;
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __SECURITY_POLICY_IMPL_STATE_HPP__
