//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file handles subnet entry handling
///
//----------------------------------------------------------------------------

#if !defined(__API_SUBNET_HPP__)
#define __API_SUBNET_HPP__

#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"

namespace api {

// forward declaration
class subnet_state;

/// \defgroup PDS_SUBNET_ENTRY - subnet entry functionality
/// \ingroup PDS_SUBNET
/// @{

/// \brief    subnet entry
class subnet_entry : public api_base {
public:
    /// \brief          factory method to allocate and initialize a subnet entry
    /// \param[in]      pds_subnet    subnet information
    /// \return         new instance of subnet or NULL, in case of error
    static subnet_entry *factory(pds_subnet_spec_t *pds_subnet);

    /// \brief          release all the s/w state associate with the given
    ///                 subnet, if any, and free the memory
    /// \param[in]      subnet     subnet to be freed
    /// \NOTE: h/w entries should have been cleaned up (by calling
    ///        impl->cleanup_hw() before calling this
    static void destroy(subnet_entry *subnet);

    /// \brief          initialize subnet entry with the given config
    /// \param[in]      api_ctxt API context carrying the configuration
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t init_config(api_ctxt_t *api_ctxt) override;

    /// \brief          allocate h/w resources for this object
    /// \param[in]      orig_obj    old version of the unmodified object
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        obj_ctxt_t *obj_ctxt) override;

    /// \brief          program all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_config(obj_ctxt_t *obj_ctxt) override {
        // no hardware programming required for subnet config
        return SDK_RET_OK;
    }

    /// \brief          reprogram all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any
    /// \param[in] api_op    API operation
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reprogram_config(api_op_t api_op) override {
        // no hardware programming required for subnet config
        return SDK_RET_OK;
    }

    /// \brief          free h/w resources used by this object, if any
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(void) override;

    /// \brief          cleanup all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any, by updating packed entries
    ///                 with latest epoch#
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_config(obj_ctxt_t *obj_ctxt) override {
        return SDK_RET_OK;
    }

    /// \brief          update all h/w tables relevant to this object except
    ///                 stage 0 table(s), if any, by updating packed entries
    ///                 with latest epoch#
    /// \param[in]      orig_obj    old version of the unmodified object
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_config(api_base *orig_obj,
                                    obj_ctxt_t *obj_ctxt) override;

    /// \param[in]      epoch       epoch being activated
    /// \param[in]      api_op      api operation
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t activate_config(pds_epoch_t epoch, api_op_t api_op,
                                      obj_ctxt_t *obj_ctxt) override {
        // no h/w programming required for subnet config, so nothing to activate
        return SDK_RET_OK;
    }

    /// \brief re-activate config in the hardware stage 0 tables relevant to
    ///        this object, if any, this reactivation must be based on existing
    ///        state and any of the state present in the dirty object list
    ///        (like clone objects etc.) only and not directly on db objects
    /// \param[in] api_op API operation
    /// \return #SDK_RET_OK on success, failure status code on error
    /// NOTE: this method is called when an object is in the dependent/puppet
    ///       object list
    virtual sdk_ret_t reactivate_config(pds_epoch_t epoch,
                                        api_op_t api_op) override {
        return SDK_RET_OK;
    }

    /// \brief          add given subnet to the database
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_to_db(void) override;

    /// \brief          delete given subnet from the database
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t del_from_db(void) override;

    /// \brief          this method is called on new object that needs to
    ///                 replace theold version of the object in the DBs
    /// \param[in]      orig_obj    old version of the object being swapped out
    /// \param[in]      obj_ctxt    transient state associated with this API
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_db(api_base *orig_obj,
                                obj_ctxt_t *obj_ctxt) override;

    /// \brief          initiate delay deletion of this object
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t delay_delete(void) override;

    /// \brief          add all objects that may be affected if this object is
    ///                 updated to framework's object dependency list
    /// \param[in]      obj_ctxt    transient state associated with this API
    ///                             processing
    /// \return         SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t add_deps(obj_ctxt_t *obj_ctxt) override;

    /// \brief          return stringified key of the object (for debugging)
    virtual string key2str(void) const override {
        return "subnet-" + std::to_string(key_.id);
    }

    /// \brief          helper function to get key given subnet entry
    /// \param[in]      entry    pointer to subnet instance
    /// \return         pointer to the subnet instance's key
    static void *subnet_key_func_get(void *entry) {
        subnet_entry *subnet = (subnet_entry *)entry;
        return (void *)&(subnet->key_);
    }

    /// \brief   helper function to get size of key
    /// \return  size of key
    static uint32_t key_size(void) {
        return sizeof(pds_subnet_key_t);
    }

    /// \brief          return router mac of this subnet
    /// \return         virtual router (VR) mac of this subnet
    mac_addr_t &vr_mac(void) { return vr_mac_; }

    /// \brief          return the subnet key/id
    /// \return         key/id of the subnet
    pds_subnet_key_t key(void) const { return key_; }

    /// \brief          return h/w index for this subnet
    /// \return         h/w table index for this subnet
    uint16_t hw_id(void) const { return hw_id_; }
    pds_vpc_key_t vpc(void) const { return vpc_; }
    pds_route_table_key_t v4_route_table(void) const { return v4_route_table_; }
    pds_route_table_key_t v6_route_table(void) const { return v6_route_table_; }
    pds_policy_key_t ing_v4_policy(void) const { return ing_v4_policy_; }
    pds_policy_key_t ing_v6_policy(void) const { return ing_v6_policy_; }
    pds_policy_key_t egr_v4_policy(void) const { return egr_v4_policy_; }
    pds_policy_key_t egr_v6_policy(void) const { return egr_v6_policy_; }

private:
    /// \brief constructor
    subnet_entry();

    /// \brief destructor
    ~subnet_entry();

    /// \brief    free h/w resources used by this object, if any
    ///           (this API is invoked during object deletes)
    /// \return    SDK_RET_OK on success, failure status code on error
    sdk_ret_t nuke_resources_(void);

private:
    pds_subnet_key_t key_;                    ///< subnet key
    pds_vpc_key_t vpc_;                       ///< vpc of this subnet
    pds_route_table_key_t v4_route_table_;    ///< IPv4 route table id
    pds_route_table_key_t v6_route_table_;    ///< IPv6 route table id
    pds_policy_key_t ing_v4_policy_;          ///< ingress IPv4 policy id
    pds_policy_key_t ing_v6_policy_;          ///< ingress IPv6 policy id
    pds_policy_key_t egr_v4_policy_;          ///< ingress IPv4 policy id
    pds_policy_key_t egr_v6_policy_;          ///< ingress IPv6 policy id
    mac_addr_t vr_mac_;                       ///< virtual router MAC
    ht_ctxt_t ht_ctxt_;                       ///< hash table context

    // P4 datapath specific state
    uint32_t hw_id_;                 ///< hardware id

    friend class subnet_state;    ///< subnet_state is friend of subnet_entry
} __PACK__;

/// \@}    // end of PDS_SUBNET_ENTRY

}    // namespace api

using api::subnet_entry;

#endif    // __API_SUBNET_HPP__
