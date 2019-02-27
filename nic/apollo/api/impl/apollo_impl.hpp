/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    apollo_impl.hpp
 *
 * @brief   apollo pipeline implementation
 */
#if !defined (__APOLLO_IMPL_HPP__)
#define __APOLLO_IMPL_HPP__

#include <vector>
#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/p4/loader/loader.hpp"
#include "nic/apollo/framework/pipeline_impl_base.hpp"

namespace api {
namespace impl {

/**
 * @defgroup PDS_PIPELINE_IMPL - pipeline wrapper implementation
 * @ingroup PDS_PIPELINE
 * @{
 */

/**
 * @brief    pipeline implementation
 */
class apollo_impl : public pipeline_impl_base {
public:
    /**
     * @brief    factory method to pipeline impl instance
     * @param[in] pipeline_cfg    pipeline configuration information
     * @return    new instance of apollo pipeline impl or NULL, in case of error
     */
    static apollo_impl *factory(pipeline_cfg_t *pipeline_cfg);

    /**
     * @brief    initialize program configuration
     * @param[in] init_params    initialization time parameters passed by app
     * @param[in] asic_cfg       asic configuration to be populated with program
     *                           information
     */
    virtual void program_config_init(pds_init_params_t *init_params,
                                     asic_cfg_t *asic_cfg) override;

    /**
     * @brief    initialize asm configuration
     * @param[in] init_params    initialization time parameters passed by app
     * @param[in] asic_cfg       asic configuration to be populated with asm
     *                           information
     */
    virtual void asm_config_init(pds_init_params_t *init_params,
                                 asic_cfg_t *asic_cfg) override;

    /**
     * @brief    init routine to initialize the pipeline
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t pipeline_init(void) override;

    /**
     * @brief    generic API to write to rxdma tables
     * @param[in]    addr         memory address to write the data to
     * @param[in]    tableid      table id
     * @param[in]    action_id    action id to write
     * @param[in]    action_data  action data to write
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t write_to_rxdma_table(mem_addr_t addr, uint32_t tableid,
                                   uint8_t action_id,
                                   void *actiondata) override;

    /**
     * @brief    generic API to write to txdma tables
     * @param[in]    addr         memory address to write the data to
     * @param[in]    tableid      table id
     * @param[in]    action_id    action id to write
     * @param[in]    action_data  action data to write
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t write_to_txdma_table(mem_addr_t addr, uint32_t tableid,
                                           uint8_t action_id,
                                           void *actiondata) override;

    /**
     * @brief    dump all the debug information to given file
     * @param[in] fp    file handle
     */
    void debug_dump(FILE *fp);

private:
    /**< constructor */
    apollo_impl() {}

    /**< destructor */
    ~apollo_impl() {}

    /*
     * @brief    initialize an instance of apollo impl class
     * @param[in] pipeline_cfg    pipeline information
     * @return    SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t init_(pipeline_cfg_t *pipeline_cfg);

    /**
     * @brief    init routine to initialize key native table
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t key_native_init_(void);

    /**
     * @brief    init routine to initialize key tunnel table
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t key_tunneled_init_(void);

    /**
     * @brief    initialize ingress drop stats table
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t ingress_drop_stats_init_(void);

    /**
     * @brief    initialize egress drop stats table
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t egress_drop_stats_init_(void);

    /**
     * @brief    initialize all the stats tables, where needed
     * @return   SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t stats_init_(void);

    /**
     * @brief    program all p4/p4+ tables that require one time initialization
     * @return    SDK_RET_OK on success, failure status code on error
     */
    sdk_ret_t table_init_(void);

    /**
     * @brief    dump ingress drop statistics
     * @param[in] fp       file handle
     */
    void dump_ingress_drop_stats_(FILE *fp);

    /**
     * @brief    dump egress drop statistics
     * @param[in] fp       file handle
     */
    void dump_egress_drop_stats_(FILE *fp);

    /*
     * @brief    apollo specific mpu program sort function
     * @param[in] program information
     */
    static void sort_mpu_programs_(std::vector<std::string>& programs);

private:
    pipeline_cfg_t      pipeline_cfg_;
};

}    // namespace impl
}    // namespace api

#endif    /** __APOLLO_IMPL_HPP__ */
