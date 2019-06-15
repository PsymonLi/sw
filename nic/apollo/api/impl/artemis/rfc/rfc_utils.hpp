/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    rfc_utils.hpp
 *
 * @brief   RFC library internal helper functions
 */

#if !defined (__RFC_UTILS_HPP__)
#define __RFC_UTILS_HPP__

#include "nic/apollo/rfc/rfc.hpp"
#include "nic/apollo/api/impl/artemis/rfc/rfc_tree.hpp"

namespace rfc {

typedef struct class_id_cb_ctxt_s {
    rfc_tree_t *tree;
    inode_t *inode;
} class_id_cb_ctxt_t;

sdk_ret_t rfc_build_lpm_trees(rfc_ctxt_t *rfc_ctxt,
                              mem_addr_t rfc_tree_root_addr, uint32_t mem_size);

/**
 * @brief    given a class bitmap (cbm), check if that exists in the RFC table
 *           already and if not assign new class-id, if it exists already,
 *           use the current class id for that class bitmap
 * @param[in]    rfc_ctxt  RFC context carrying all the intermediate state for
 *                         this policy
 * @param[in]    rfc_table RFC table to add the class id to
 * @param[in]    cbm       class bitmap that needs class id to be computed for
 * @param[in]    cbm_size  class bitmap size
 * @param[in]    ctxt      opaque context passed to this callback
 * @return    SDK_RET_OK on success, failure status code on error
 */
uint16_t rfc_compute_class_id_cb(rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table,
                                 rte_bitmap *cbm, uint32_t cbm_size,
                                 void *ctxt);

/**
 * @brief    given a class bitmap (cbm), check if that exists in the RFC table
 *           already and if not assign new class-id, if it exists already,
 *           use the current class id for that class bitmap
 * @param[in]    rfc_ctxt  RFC context carrying all the intermediate state for
 *                         this policy
 * @param[in]    rfc_table RFC table to add the class id to
 * @param[in]    cbm       class bitmap that needs class id to be computed for
 * @param[in]    cbm_size  class bitmap size
 * @param[in]    ctxt      opaque context passed to this callback
 * @return    SDK_RET_OK on success, failure status code on error
 */
uint16_t rfc_compute_tag_class_id_cb(rfc_ctxt_t *rfc_ctxt,
                                     rfc_table_t *rfc_table,
                                     rte_bitmap *cbm, uint32_t cbm_size,
                                     void *ctxt);

/**
 * @brief    sort interval table entries in each of the phase 0
 *           LPM trees
 * @param[in]    rfc_ctxt    RFC context carrying all the intermediate state for
 *                           this policy
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t rfc_sort_itables(rfc_ctxt_t *rfc_ctxt);

/**
 * @brief    given the class bitmap tables of phase0 & phase1, compute class
 *           bitmap table(s) of RFC phase 2, and set the results bits
 * @param[in] rfc_ctxt    RFC context carrying all of the previous phases
 *                        information processed until now
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t rfc_build_eqtables(rfc_ctxt_t *rfc_ctxt);

}    // namespace rfc

#endif    /** __RFC_UTILS_HPP__ */
