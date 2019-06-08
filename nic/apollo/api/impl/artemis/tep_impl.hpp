//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// TEP implementation in the p4/hw
///
//----------------------------------------------------------------------------

#ifndef __TEP_IMPL_HPP__
#define __TEP_IMPL_HPP__

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "gen/p4gen/artemis_txdma/include/artemis_txdma_p4pd.h"

using sdk::table::handle_t;

namespace api {
namespace impl {

/// \defgroup PDS_TEP_IMPL - TEP functionality
/// \ingroup PDS_TEP
/// @{

/// \brief TEP implementation
class tep_impl : public impl_base {
public:
    /// \brief     factory method to allocate & initialize TEP impl instance
    /// \param[in] spec specification
    /// \return    new instance of TEP or NULL, in case of error
    static tep_impl *factory(pds_tep_spec_t *spec);

    /// \brief     release all the s/w state associated with the given TEP,
    ///            if any, and free the memory
    /// \param[in] impl TEP to be freed
    /// \NOTE      h/w entries should have been cleaned up (by calling
    ///            impl->cleanup_hw() before calling this
    static void destroy(tep_impl *impl);

    /// \brief     allocate/reserve h/w resources for this object
    /// \param[in] orig_obj old version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t reserve_resources(api_base *orig_obj,
                                        obj_ctxt_t *obj_ctxt) override;

    /// \brief     free h/w resources used by this object, if any
    /// \param[in] api_obj api object holding the resources
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t release_resources(api_base *api_obj) override;

    /// \brief     free h/w resources used by this object, if any
    ///            (this API is invoked during object deletes)
    /// \param[in] api_obj api object holding the resources
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t nuke_resources(api_base *api_obj) override;

    /// \brief      read spec, statistics and status from hw tables
    /// \param[in]  api_obj API object
    /// \param[in]  key  pointer to tep key. Not used.
    /// \param[out] info pointer to tep info
    /// \return     SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t read_hw(api_base *api_obj, obj_key_t *key,
                              obj_info_t *info) override;

    /// \brief     program all h/w tables relevant to this object except
    ///            stage 0 table(s), if any
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t program_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /// \brief     cleanup all h/w tables relevant to this object except
    ///            stage 0 table(s), if any, by updating packed entries with
    ///            latest epoch#
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t cleanup_hw(api_base *api_obj,
                                 obj_ctxt_t *obj_ctxt) override;

    /// \brief     update all h/w tables relevant to this object except stage 0
    ///            table(s), if any, by updating packed entries with
    ///            latest epoch#
    /// \param[in] orig_obj old version of the unmodified object
    /// \param[in] obj_ctxt transient state associated with this API
    /// \return    SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t update_hw(api_base *curr_obj, api_base *prev_obj,
                                obj_ctxt_t *obj_ctxt) override;

    /// \brief  return h/w index for this TEP
    /// \return h/w table index for this TEP
    uint16_t hw_id(void) const { return remote46_hw_id_; }

private:
    /// \brief constructor
    tep_impl() {
        remote46_hw_id_ = 0xFFFF;
    }

    /// \brief destructor
    ~tep_impl() {}

    /// \brief      populate specification with hardware information
    /// \param[in]  remote_46_mapping_data remote_46_mapping table data
    /// \param[out] spec specification
    void fill_spec_(remote_46_mapping_actiondata_t *remote_46_mapping_data,
                    pds_tep_spec_t *spec);

    /// \brief      populate status with hardware information
    /// \param[out] status status
    void fill_status_(pds_tep_status_t *status);

private:
    // P4 datapath specific state
    uint16_t    remote46_hw_id_;    ///< hardware id for this tep
    handle_t    tep1_rx_handle_;    ///< TEP1_RX table handle
};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __TEP_IMPL_HPP__
