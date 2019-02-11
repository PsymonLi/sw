/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    pipeline_impl_base.hpp
 *
 * @brief   wrapper class for common pipeine APIs
 */
#if !defined (__PIPELINE_IMPL_HPP__)
#define __PIPELINE_IMPL_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/asic/asic.hpp"
#include "nic/apollo/framework/obj_base.hpp"
#include "nic/apollo/include/api/oci_init.hpp"

namespace impl {

/**
 * @defgroup OCI_PIPELINE_IMPL - pipeline wrapper implementation
 * @ingroup OCI_PIPELINE
 * @{
 */

typedef struct pipeline_cfg_s {
    std::string    name;    /**< name of the pipeline */
} pipeline_cfg_t;

/**
 * @brief    pipeline implementation
 */

class pipeline_impl_base : public obj_base {
public:
    /**
     * @brief    factory method to pipeline impl instance
     * @param[in] pipeline_cfg    pipeline configuration information
     * @return    new instance of pipeline impl or NULL, in case of error
     */
    static pipeline_impl_base *factory(pipeline_cfg_t *pipeline_cfg);

    /**
     * @brief    initialize program configuration
     * @param[in] init_params    initialization time parameters passed by app
     * @param[in] asic_cfg       asic configuration to be populated with program
     *                           information
     */
    virtual void program_config_init(oci_init_params_t *init_params,
                                     asic_cfg_t *asic_cfg) { }

    /**
     * @brief    initialize asm configuration
     * @param[in] init_params    initialization time parameters passed by app
     * @param[in] asic_cfg       asic configuration to be populated with asm
     *                           information
     */
    virtual void asm_config_init(oci_init_params_t *init_params,
                                 asic_cfg_t *asic_cfg) { }

    /**
     * @brief    init routine to initialize the pipeline
     * @return    SDK_RET_OK on success, failure status code on error
     */
    virtual sdk_ret_t pipeline_init(void) { return sdk::SDK_RET_ERR; }

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
                                           void *actiondata) {
        return SDK_RET_ERR;
    }

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
                                           void *actiondata) {
        return SDK_RET_ERR;
    }

    /**
     * @brief    dump all the debug information to given file
     * @param[in] fp    file handle
     */
    virtual void debug_dump(FILE *fp) { }
};

/** @} */    // end of OCI_PIPELINE_IMPL

}    // namespace impl

using impl::pipeline_cfg_t;
using impl::pipeline_impl_base;

#endif    /** __PIPELINE_IMPL_HPP__ */
