/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    rfc_utils.cc
 *
 * @brief   RFC library internal helper functions
 */

#include "include/sdk/platform.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/impl/lpm/lpm.hpp"
#include "nic/apollo/api/impl/rfc/rfc.hpp"
#include "nic/apollo/api/impl/rfc/rfc_tree.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/rfc/rfc_utils.hpp"

namespace rfc {

/**
 * @brief    compare two proto-port tree keys and return -1 or 1 based on where
 *           they fit on the number line
 * @param[in]    n1    pointer to 1st node
 * @param[in]    n2    pointer to 2nd node
 * @param[in]    ctxt  context carrying tree_type information
 * @return  -1 or 1 based on the comparison result
 * NOTE: when keys are same, return -1
 */
static inline int
inode_compare_cb (const void *n1, const void *n2, void *ctxt)
{
    inode_t         *inode1 = (inode_t *)n1;
    inode_t         *inode2 = (inode_t *)n2;
    itree_type_t    tree_type = reinterpret_cast<itree_type_t&>(ctxt);

    if ((tree_type == ITREE_TYPE_IPV4) ||
        (tree_type == ITREE_TYPE_IPV6)) {
        if (IPADDR_GT(&inode1->ipaddr, &inode2->ipaddr)) {
            return 1;
        }
        return -1;
    } else if (tree_type == ITREE_TYPE_PORT) {
        if (inode1->port > inode2->port) {
            return 1;
        }
        return  -1;
    } else {
        if (inode1->key32 > inode2->key32) {
            return 1;
        }
        return -1;
    }
}

static inline int
inode_compare_tag_cb (const void *n1, const void *n2, void *ctxt)
{
    inode_t    *inode1 = (inode_t *)n1;
    inode_t    *inode2 = (inode_t *)n2;

    if (inode1->key32 > inode2->key32) {
        return 1;
    }
    return -1;
}

/**
 * @brief    sort interval table entries in each of the phase 0
 *           LPM trees
 * @param[in]    rfc_ctxt    RFC context carrying all the intermediate state for
 *                           this policy
 * @return    SDK_RET_OK on success, failure status code on error
 */
sdk_ret_t
rfc_sort_itables (rfc_ctxt_t *rfc_ctxt)
{
    if (rfc_ctxt->policy->af == IP_AF_IPV4) {
        qsort_r(rfc_ctxt->sip_tree.itable.nodes,
                rfc_ctxt->sip_tree.itable.num_nodes,
                sizeof(inode_t), inode_compare_cb,
                (void *)ITREE_TYPE_IPV4);
        qsort_r(rfc_ctxt->dip_tree.itable.nodes,
                rfc_ctxt->dip_tree.itable.num_nodes,
                sizeof(inode_t), inode_compare_cb,
                (void *)ITREE_TYPE_IPV4);
    } else if (rfc_ctxt->policy->af == IP_AF_IPV6) {
        qsort_r(rfc_ctxt->sip_tree.itable.nodes,
                rfc_ctxt->sip_tree.itable.num_nodes,
                sizeof(inode_t), inode_compare_cb,
                (void *)ITREE_TYPE_IPV6);
        qsort_r(rfc_ctxt->dip_tree.itable.nodes,
                rfc_ctxt->dip_tree.itable.num_nodes,
                sizeof(inode_t), inode_compare_cb,
                (void *)ITREE_TYPE_IPV6);
    }
    qsort_r(rfc_ctxt->stag_tree.itable.nodes,
            rfc_ctxt->stag_tree.itable.num_nodes,
            sizeof(inode_t), inode_compare_tag_cb, NULL);
    qsort_r(rfc_ctxt->dtag_tree.itable.nodes,
            rfc_ctxt->dtag_tree.itable.num_nodes,
            sizeof(inode_t), inode_compare_tag_cb, NULL);
    qsort_r(rfc_ctxt->port_tree.itable.nodes,
            rfc_ctxt->port_tree.itable.num_nodes,
            sizeof(inode_t), inode_compare_cb,
            (void *)ITREE_TYPE_PORT);
    qsort_r(rfc_ctxt->proto_port_tree.itable.nodes,
            rfc_ctxt->proto_port_tree.itable.num_nodes,
            sizeof(inode_t), inode_compare_cb,
            (void *)ITREE_TYPE_PROTO_PORT);
    return SDK_RET_OK;
}

uint16_t
rfc_compute_class_id_cb (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table,
                         rte_bitmap *cbm, uint32_t cbm_size, void *ctxt)
{
    uint8_t       *bits;
    uint16_t      class_id;
    rte_bitmap    *cbm_new;

    auto it = rfc_table->cbm_map.find(cbm);
    if (it != rfc_table->cbm_map.end()) {
        /**< found the bit combination, return corresponding class id */
        return it->second;
    }
    class_id = rfc_table->num_classes++;
    PDS_TRACE_VERBOSE("class id allocated is %u", class_id);
    SDK_ASSERT(class_id < RFC_MAX_EQ_CLASSES);
    if (rfc_table->cbm_table[class_id].cbm) {
        bits = (uint8_t *)rfc_table->cbm_table[class_id].cbm;
    } else {
        posix_memalign((void **)&bits, CACHE_LINE_SIZE, cbm_size);
    }
    cbm_new = rte_bitmap_init(rfc_ctxt->policy->max_rules, bits, cbm_size);
    rte_bitmap_or(cbm, cbm_new, cbm_new);
    rfc_table->cbm_table[class_id].class_id = class_id;
    rfc_table->cbm_table[class_id].cbm = cbm_new;
    rfc_table->cbm_map[cbm_new] = class_id;
    return class_id;
}

uint16_t
rfc_compute_tag_class_id_cb (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table,
                             rte_bitmap *cbm, uint32_t cbm_size, void *ctxt)
{
    int rv;
    rule_t *rule;
    uint8_t *bits;
    sdk_ret_t ret;
    rte_bitmap *cbm_new;
    uint32_t class_id, idx;
    class_id_cb_ctxt_t *cb_ctxt = (class_id_cb_ctxt_t *)ctxt;
    inode_t *inode = cb_ctxt->inode;
    uint32_t start_posn = 0;
    uint64_t slab = 0;

    if (rfc_table->num_classes == 0) {
        idx = rfc_table->num_classes++;
        posix_memalign((void **)&bits, CACHE_LINE_SIZE, cbm_size);
        cbm_new = rte_bitmap_init(rfc_ctxt->policy->max_rules, bits, cbm_size);
        rfc_table->cbm_table[idx].class_id = (uint16_t)PDS_IMPL_RSVD_MAPPING_CLASS_ID;
        rfc_table->cbm_table[idx].cbm = cbm_new;
        rfc_table->cbm_map[cbm_new] = (uint16_t)PDS_IMPL_RSVD_MAPPING_CLASS_ID;
    }

    rule = &rfc_ctxt->policy->rules[inode->rfc.rule_no];
    rv = rte_bitmap_scan(cbm, &start_posn, &slab);
    // Check bitmap == 0
    if (rv == 0) {
        class_id = PDS_IMPL_RSVD_MAPPING_CLASS_ID;
        PDS_TRACE_VERBOSE("Reused classid %u for default", class_id);
        return (uint16_t)class_id;
    } else {
        if (cb_ctxt->tree->type == RFC_TREE_TYPE_STAG) {
            ret = rfc_ctxt->tag2class_cb(rule->attrs.match.l3_match.src_tag,
                                         &class_id, rfc_ctxt->tag2class_cb_ctxt);
            SDK_ASSERT(ret == SDK_RET_OK);
        } else if (cb_ctxt->tree->type == RFC_TREE_TYPE_DTAG) {
            ret = rfc_ctxt->tag2class_cb(rule->attrs.match.l3_match.dst_tag,
                                         &class_id, rfc_ctxt->tag2class_cb_ctxt);
            SDK_ASSERT(ret == SDK_RET_OK);
        } else {
            SDK_ASSERT(0);
        }
    }

    idx = rfc_table->num_classes++;
    posix_memalign((void **)&bits, CACHE_LINE_SIZE, cbm_size);
    cbm_new = rte_bitmap_init(rfc_ctxt->policy->max_rules, bits, cbm_size);
    rte_bitmap_or(cbm, cbm_new, cbm_new);
    rfc_table->cbm_table[idx].class_id = (uint16_t)class_id;
    rfc_table->cbm_table[idx].cbm = cbm_new;
    rfc_table->cbm_map[cbm_new] = (uint16_t)class_id;
    return (uint16_t)(class_id & 0xFFFF);
}

#if 0
uint16_t
rfc_compute_tag_class_id_cb (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table,
                             rte_bitmap *cbm, uint32_t cbm_size, void *ctxt)
{
    uint8_t *bits;
    uint16_t class_id, idx;
    rte_bitmap *cbm_new;
    class_id_cb_ctxt_t *cb_ctxt = (class_id_cb_ctxt_t *)ctxt;
    inode_t *inode = cb_ctxt->inode;

    if (cb_ctxt->tree->type == RFC_TREE_TYPE_STAG) {
        class_id =
            rfc_ctxt->policy->rules[inode->rfc.rule_no].match.l3_match.src_tag;
    } else if (cb_ctxt->tree->type == RFC_TREE_TYPE_DTAG) {
        class_id =
            rfc_ctxt->policy->rules[inode->rfc.rule_no].match.l3_match.dst_tag;
    } else {
        // unsupported tree type
        SDK_ASSERT(0);
    }
    idx = rfc_table->num_classes++;
    posix_memalign((void **)&bits, CACHE_LINE_SIZE, cbm_size);
    cbm_new = rte_bitmap_init(rfc_ctxt->policy->max_rules, bits, cbm_size);
    rte_bitmap_or(cbm, cbm_new, cbm_new);
    rfc_table->cbm_table[idx].class_id = class_id;
    rfc_table->cbm_table[idx].cbm = cbm_new;
    rfc_table->cbm_map[cbm_new] = class_id;
    return class_id;
}
#endif

}    // namespace rfc
