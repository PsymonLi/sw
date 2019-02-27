/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    tep_impl.hpp
 *
 * @brief   TEP implementation in the p4/hw
 */
#if !defined (__TEP_IMPL_HPP__)
#define __TEP_IMPL_HPP__

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/include/api/oci_tep.hpp"
#include "nic/apollo/p4/include/defines.h"
#include "gen/p4gen/apollo/include/p4pd.h"

namespace api {
namespace impl {

/**
 * @defgroup OCI_TEP_IMPL - tep functionality
 * @ingroup OCI_TEP
 * @{
 */

/**
 * @brief    TEP implementation
 */
class tep_impl : public impl_base {
public:
    /**
     * @brief    factory method to allocate & initialize tep impl instance
     * @param[in] oci_tep    tep information
     * @return    new instance of tep or NULL, in case of error
     */
    static tep_impl *factory(oci_tep_spec_t *oci_tep);

    /**
     * @brief    release all the s/w state associated with the given tep,
     *           if any, and free the memory
     * @param[in] tep     tep to be freed
     * NOTE: h/w entries should have been cleaned up (by calling
     *       impl->cleanup_hw() before calling this
     */
    static void destroy(tep_impl *impl);

    /**
     * @brief    allocate/reserve h/w resources for this object
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t reserve_resources(api_base *api_obj) override;

    /**
     * @brief     free h/w resources used by this object, if any
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /**
     * @brief read spec, statistics and status from hw tables
     * @param[out] info pointer to tep info
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t read_hw(oci_tep_info_t *info);

    /**
     * @brief    program all h/w tables relevant to this object except stage 0
     *           table(s), if any
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    cleanup all h/w tables relevant to this object except stage 0
     *           table(s), if any, by updating packed entries with latest epoch#
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief    update all h/w tables relevant to this object except stage 0
     *           table(s), if any, by updating packed entries with latest epoch#
     * @param[in] orig_obj    old version of the unmodified object
     * @param[in] obj_ctxt    transient state associated with this API
     * @return   SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                obj_ctxt_t *obj_ctxt) override;

    /**
     * @brief     return h/w index for this TEP
     * @return    h/w table index for this TEP
     */
    uint16_t hw_id(void) { return hw_id_; }

    /**
     * @brief     return nexthop index for this TEP
     * @return    nexthop index for this TEP
     */
    uint16_t nh_id(void) { return nh_id_; }

private:
    /**< @brief    constructor */
    tep_impl() {
        hw_id_ = 0xFFFF;
        nh_id_ = 0xFFFF;
    }

    /**< @brief    destructor */
    ~tep_impl() {}

    /**
     * @brief Populate specification with hardware information
     * @param[in] nh_tx_data Nexthop table data
     * @param[in] tep_tx_data TEP table data
     * @param[out] spec Specification
     */
    void fill_spec_(nexthop_tx_actiondata_t *nh_tx_data,
                    tep_tx_actiondata_t *tep_tx_data, oci_tep_spec_t *spec);

    /**
     * @brief Populate status with hardware information
     * @param[in] tep_tx_data TEP table data
     * @param[out] status Status
     */
    void fill_status_(tep_tx_actiondata_t *tep_tx_data,
                      oci_tep_status_t *status);

private:
    /**< P4 datapath specific state */
    uint16_t    hw_id_;    /**< hardware id for this tep */
    uint16_t    nh_id_;    /**< nexthop index for this tep */
} __PACK__;

/** @} */    // end of OCI_TEP_IMPL

}    // namespace impl
}    // namespace api

#endif    /** __TEP_IMPL_HPP__ */
