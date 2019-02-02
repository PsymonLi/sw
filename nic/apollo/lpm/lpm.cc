/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    lpm.hpp
 *
 * @brief   LPM library implementation
 */

#include <math.h>
#include <stack>
#include "nic/apollo/lpm/lpm.hpp"
#include "nic/sdk/asic/rw/asicrw.hpp"
#include "gen/p4gen/apollo_txdma/include/apollo_txdma_p4pd.h"

using std::stack;

/**< invalid nexthop */
#define LPM_NEXTHOP_INVALID            -1
#define LPM_NEXTHOP_SIZE                2    /**< 2 byte nexthop */

/**< LPM interval tree node */
typedef struct lpm_inode_s {
    ip_addr_t    ipaddr;
    uint32_t     nhid;
} lpm_inode_t;

/**< LPM interval node table */
typedef struct lpm_itable_s {
    uint32_t       num_intervals;    /**< number of entries in interval table */
    lpm_inode_t    *nodes;           /**< interval table nodes */
} lpm_itable_t;

/**< temporary stack elements maintained while building the interval table */
typedef struct itable_stack_elem_s {
    lpm_inode_t    interval;         /**< interval table node (to be emitted) */
    uint32_t       fallback_nhid;    /**< fallback nexthop */
} itable_stack_elem_t;

/**< each LPM table is 64 bytes (512 bits/flit) */
#define LPM_TABLE_SIZE                 (CACHE_LINE_SIZE)
/**
 * key size is 4 bytes for IPv4 and 8 bytes (assuming max prefix length of 64)
 * for IPv6
 */
#define LPM_ENTRY_KEY_SIZE(ip_af)      (((ip_af) == IP_AF_IPV4) ? 4 : 8)

/**
 * data is next hop index
 */
#define LPM_ENTRY_DATA_SIZE(ip_af)     (LPM_NEXTHOP_SIZE)

/**
 * number of keys per table is CACHE_LINE_SIZE/(sizeof(ipv4_addr_t) = 16 for
 * IPv4 and CACHE_LINE_SIZE/8 = 8 for IPv6 (assuming max prefix length of 64)
 * NOTE: this doesn't apply to last stage where each node also has 2 byte
 * nexthop
 */
#define LPM_KEYS_PER_TABLE(ip_af)      (((ip_af) == IP_AF_IPV4) ? 15 : 7)

#define LPM_MAX_STAGES    8    /**< 8 stages can serve more than 1M v6 routes */

/**
 * each LPM stage consists of a set of LPM index tables and this stage meta
 * contains temporary state we maintain while building the table
 */

typedef struct lpm_stage_info_s {
    mem_addr_t    curr_table_addr;  /**< current table address */
    uint32_t      curr_index;       /**< current entry (key/key+nh) being populated */
    uint8_t       curr_table[LPM_TABLE_SIZE]; /**< current table being populated */
} lpm_stage_info_t;

typedef struct lpm_stage_meta_s {
    uint8_t             af;
    uint32_t            key_sz;          /**< key size for the tree */
    uint32_t            keys_per_table;  /**< no. of keys per table is same in
                                              all stages except last stage */
    /**< per stage info */
    lpm_stage_info_t    stage_info[LPM_MAX_STAGES];
} lpm_stage_meta_t;

/**
 * @brief    compute the number of stages needed for LPM lookup given the
 *           route table scale
 * @param[in]    num_routes    number of routes in the route table
 * @return       number of lookup stages (aka. depth of the interval tree)
 *
 * The computation is done as follows for IPv4:
 *     log16(2 * num_routes) = log16(num_routes << 1) =
 *     log2(num_routes << 1)/log2(16) = log2(num_routes << 1)/4
 *     div by 4 is accomplished by right shift as we need to round up at the end
 * and for IPv6:
 *     log4(2 * num_routes)
 */
static inline uint32_t
lpm_stages (uint8_t ip_af, uint32_t num_routes)
{
    if (ip_af == IP_AF_IPV4) {
        return (uint32_t)ceil(log2f((double)(num_routes << 1))/4.0); // 16-way tree
    } else {
        return (uint32_t)ceil(log2f((double)(num_routes << 1))/2.0); // 8-way tree
    }
}

/**
 * @brief    compare two given routes and return -1, 0 or 1 based on where they
 *           fit on the number line, breaking the tie by comparing prefix
 *           lengths
 * @param[in]    r1    pointer to 1st route
 * @param[in]    r2    pointer to 2nd route
 * @return  -1, 0, 1 based on the comparison result
 */
static int
route_compare_cb (const void *route1, const void *route2)
{
    route_t    *r1 = (route_t *)route1;
    route_t    *r2 = (route_t *)route2;

    if (IPADDR_LT(&r1->prefix.addr, &r2->prefix.addr)) {
        return -1;
    } else if (IPADDR_GT(&r1->prefix.addr, &r2->prefix.addr)) {
        return 1;
    } else if (IPADDR_EQ(&r1->prefix.addr, &r2->prefix.addr)) {
        if (r1->prefix.len < r2->prefix.len) {
            return -1;
        } else if (r1->prefix.len > r2->prefix.len) {
            return 1;
        }
    }
    /**< two exact same prefixes, configuration error */
    SDK_ASSERT(0);
    return 0;
}

/**
 * @brief    given number of routes, compute the number of intervals in the
 *           interval tree
 * @param[in]    num_routes number of routes
 * @return    number of intervale tree nodes needed to support the given route
 *            scale
 */
static inline uint32_t
compute_intervals (uint32_t num_routes)
{
    num_routes--;
    num_routes |= num_routes >> 1;
    num_routes |= num_routes >> 2;
    num_routes |= num_routes >> 4;
    num_routes |= num_routes >> 8;
    num_routes |= num_routes >> 16;
    num_routes++;

    return (1 << num_routes);
}

/**
 * @brief    build an interval table corresponding to the give route table
 * @param[in]    route_table    route table
 * @param[out]   itable         interval node table
 * @return    number of intervals created
 */
static inline sdk_ret_t
lpm_build_interval_table (route_table_t *route_table, lpm_itable_t *itable)
{
    stack<itable_stack_elem_t>    s;
    itable_stack_elem_t           elem, end;
    uint32_t                      num_intervals = 0;

    /**< Create a stack elem with the highest prefix & invalid nh */
    end.interval.nhid = LPM_NEXTHOP_INVALID;
    end.interval.ipaddr.af = route_table->af;
    if (route_table->af == IP_AF_IPV4) {
        end.interval.ipaddr.addr.v4_addr = 0xFFFFFFFF;
    } else if (route_table->af == IP_AF_IPV6) {
        for (uint32_t i = 0; i < IP6_ADDR32_LEN; i++) {
            end.interval.ipaddr.addr.v6_addr.addr32[i] = 0xFFFFFF;
        }
    }

    /**< This elem would provide the catch all nh id. */
    end.fallback_nhid = route_table->default_nhid;
    /**< Push this elem to the stack */
    s.push(end);

    for (uint32_t i = 0; i < route_table->num_routes; i++) {
        /**< add itree node corresponding to the start of the route prefix */
        ip_prefix_ip_low(&route_table->routes[i].prefix, &elem.interval.ipaddr);
        elem.interval.nhid = route_table->routes[i].nhid;
        /**
         * if there is anything to pop from stack before pushing the new
         * interval node corresponding to this route, pop those nodes first
         */
        while (IPADDR_LT(&s.top().interval.ipaddr, &elem.interval.ipaddr)) {
            /**
             * we hit this case when this route prefix is outside previous one
             * e.g. route1 = 10.10.0.0/16 and route2 = 11.0.0.0/8
             * while processing 11.0.0.0/8, top of stack would be 10.11.0.0
             * with fallback nexthop
             * pop the stack and emit the interval
             */
            itable->nodes[num_intervals++] = s.top().interval;
            s.pop();
        }

        /**< start address of prefix is always emitted to the interval table */
        if ((num_intervals > 0) &&
            IPADDR_EQ(&itable->nodes[num_intervals-1].ipaddr,
                      &elem.interval.ipaddr)) {
            /**
             * overwrite the previous itable entry with this more specific
             * route, we hit this case when the current route is starting
             * exactly at the previous route
             * e.g. route1 = 10.10.0.0/16 and route2 = 10.10.0.0/24
             * while processing route2, route1 is already emitted to interval
             * table, but we should overwrite that with more specific route2's
             * nexthop
             */
            itable->nodes[num_intervals-1] = elem.interval;
        } else {
            itable->nodes[num_intervals++] = elem.interval;
        }

        /**
         * if stack top is same as current, pop it since we already emitted this
         * interval. we hit this case, when this route is adjacent to the one on
         * the stack
         * e.g. route1 = 10.10.0.0/16 and route2 = 10.11.0.0/16
         * while processing route1, 10.11.0.0 is pushed to top of the stack
         * but when 10.11.0.0/16 is seen, we don't need that interval as we have
         * actual user configured route for it
         */
        SDK_ASSERT(s.empty() == false);
        if (IPADDR_EQ(&(s.top().interval.ipaddr), &elem.interval.ipaddr)) {
            s.pop();
        }

        /**< add node corresponding to the IP after the end of route prefix */
        ip_prefix_ip_next(&route_table->routes[i].prefix,
                          &elem.interval.ipaddr);
        /**< nexthop id for the IP beyond this prefix is the fallback nexthop */
        elem.interval.nhid = s.top().fallback_nhid;
        /**
         * fallback nexthop for anyroute within the current prefix is this
         * route's nexthop
         */
        elem.fallback_nhid = route_table->routes[i].nhid;

        /**
         * if stack top is same as the interval's end node, pop it as we will
         * push more specific node now, we hit this case, when one route is
         * overlpping with the other towards right edge of the larger prefix
         * e.g. route1 = 11.11.0.0/16 and route2 = 11.11.255.0/24
         * while processing route2, top of the stack wil have 11.12.0.0 with
         * a fallback nexthop that is not relevant anymore once we saw
         * 11.11.255.0, so we should pop it
         */
        SDK_ASSERT(s.empty() == false);
        if (IPADDR_EQ(&(s.top().interval.ipaddr), &elem.interval.ipaddr)) {
            s.pop();
        }

        /**< push this interval's end node to stack */
        s.push(elem);
    }

    /**< emit whatever is remaining on the stack now */
    SDK_ASSERT(s.empty() == false);
    while (IPADDR_LT(&(s.top().interval.ipaddr), &end.interval.ipaddr)) {
        itable->nodes[num_intervals++] = s.top().interval;
        s.pop();
    }
    itable->num_intervals = num_intervals;
    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_write_table(mem_addr_t addr, uint32_t tableid, uint8_t action_id, void *actiondata)
{
    sdk_ret_t ret;
    uint32_t  len;
    uint8_t   packed_entry[LPM_TABLE_SIZE];
    p4pd_apollo_txdma_raw_table_hwentry_query(tableid, action_id, &len);
    p4pd_apollo_txdma_entry_pack(tableid, action_id, actiondata, packed_entry);
    ret = asic_mem_write(addr, packed_entry, len >> 3, ASIC_WRITE_MODE_WRITE_THRU);
    SDK_ASSERT(ret == SDK_RET_OK);
    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_ipv4_add_key_to_stage(lpm_stage_info_t *stage, lpm_inode_t *lpm_inode)
{
    auto table = (route_lpm_s0_actiondata_t *) stage->curr_table;
    switch (stage->curr_index) {
        case 0:
            table->action_u.route_lpm_s0_route_lpm_s0.key0 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 1:
            table->action_u.route_lpm_s0_route_lpm_s0.key1 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 2:
            table->action_u.route_lpm_s0_route_lpm_s0.key2 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 3:
            table->action_u.route_lpm_s0_route_lpm_s0.key3 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 4:
            table->action_u.route_lpm_s0_route_lpm_s0.key4 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 5:
            table->action_u.route_lpm_s0_route_lpm_s0.key5 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 6:
            table->action_u.route_lpm_s0_route_lpm_s0.key6 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 7:
            table->action_u.route_lpm_s0_route_lpm_s0.key7 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 8:
            table->action_u.route_lpm_s0_route_lpm_s0.key8 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 9:
            table->action_u.route_lpm_s0_route_lpm_s0.key9 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 10:
            table->action_u.route_lpm_s0_route_lpm_s0.key10 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 11:
            table->action_u.route_lpm_s0_route_lpm_s0.key11 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 12:
            table->action_u.route_lpm_s0_route_lpm_s0.key12 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 13:
            table->action_u.route_lpm_s0_route_lpm_s0.key13 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        case 14:
            table->action_u.route_lpm_s0_route_lpm_s0.key14 = lpm_inode->ipaddr.addr.v4_addr;
            break;
        default:
            break;
    }

    stage->curr_index++;
    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_ipv4_set_default_data(lpm_stage_info_t *stage, uint32_t default_data)
{
    auto table = (route_lpm_s2_actiondata_t *) stage->curr_table;
    table->action_u.route_lpm_s2_route_lpm_s2.data_ = (uint16_t)default_data;
    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_ipv4_add_key_to_last_stage(lpm_stage_info_t *stage, lpm_inode_t *lpm_inode)
{
    auto table = (route_lpm_s2_actiondata_t *) stage->curr_table;
    switch (stage->curr_index) {
        case 0:
            table->action_u.route_lpm_s2_route_lpm_s2.key0 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data0 = lpm_inode->nhid;
            break;
        case 1:
            table->action_u.route_lpm_s2_route_lpm_s2.key1 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data1 = lpm_inode->nhid;
            break;
        case 2:
            table->action_u.route_lpm_s2_route_lpm_s2.key2 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data2 = lpm_inode->nhid;
            break;
        case 3:
            table->action_u.route_lpm_s2_route_lpm_s2.key3 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data3 = lpm_inode->nhid;
            break;
        case 4:
            table->action_u.route_lpm_s2_route_lpm_s2.key4 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data4 = lpm_inode->nhid;
            break;
        case 5:
            table->action_u.route_lpm_s2_route_lpm_s2.key5 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data5 = lpm_inode->nhid;
            break;
        case 6:
            table->action_u.route_lpm_s2_route_lpm_s2.key6 = lpm_inode->ipaddr.addr.v4_addr;
            table->action_u.route_lpm_s2_route_lpm_s2.data6 = lpm_inode->nhid;
            break;
        default:
            break;
    }

    stage->curr_index++;
    return SDK_RET_OK;
}


static inline sdk_ret_t
lpm_add_key_to_stage(uint8_t af, lpm_stage_info_t *stage, lpm_inode_t *lpm_inode)
{
    switch (af) {
        case IP_AF_IPV4:
            lpm_ipv4_add_key_to_stage(stage, lpm_inode);
            break;
        default:
            break;
    }

    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_write_stage_table(uint8_t af, lpm_stage_info_t *stage)
{
    switch (af) {
        case IP_AF_IPV4:
            lpm_write_table(stage->curr_table_addr,
                            P4_APOLLO_TXDMA_TBL_ID_ROUTE_LPM_S0,
                            ROUTE_LPM_S0_ROUTE_LPM_S0_ID,
                            stage->curr_table);
            break;
        default:
            break;
    }

    memset(stage->curr_table, 0xFF, LPM_TABLE_SIZE);
    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_add_key_to_last_stage(uint8_t af, lpm_stage_info_t *stage, lpm_inode_t *lpm_inode)
{
    switch (af) {
        case IP_AF_IPV4:
            lpm_ipv4_add_key_to_last_stage(stage, lpm_inode);
            break;
        default:
            break;
    }

    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_set_default_data(uint8_t af, lpm_stage_info_t *stage, uint32_t default_data)
{
    switch (af) {
        case IP_AF_IPV4:
            lpm_ipv4_set_default_data(stage, default_data);
            break;
        default:
            break;
    }

    return SDK_RET_OK;
}

static inline sdk_ret_t
lpm_write_last_stage_table(uint8_t af, lpm_stage_info_t *stage)
{
    switch (af) {
        case IP_AF_IPV4:
            lpm_write_table(stage->curr_table_addr,
                            P4_APOLLO_TXDMA_TBL_ID_ROUTE_LPM_S2,
                            ROUTE_LPM_S2_ROUTE_LPM_S2_ID,
                            stage->curr_table);
            break;
        default:
            break;
    }

    memset(stage->curr_table, 0xFF, LPM_TABLE_SIZE);
    return SDK_RET_OK;
}

/**
 * @brief    Flushes the partially filled tabled to HW at the end of processing.
 * @param[in] smeta                  meta information for the stages
 * @param[in] nstages                the number of stages
 * @return    SDK_RET_OK on success, failure status code on error
 */
static sdk_ret_t
lpm_flush_partial_tables (lpm_stage_meta_t *smeta, uint32_t nstages, uint32_t curr_default)
{
    for (uint32_t i = 0; i < (nstages-1); i++) {
        lpm_write_stage_table(smeta->af, &smeta->stage_info[i]);
    }

    if (nstages > 0) {
        /**< table is ready to write. set the default data */
        lpm_set_default_data(smeta->af, &smeta->stage_info[nstages-1], curr_default);
        /**< Write the table to HW memory */
        lpm_write_last_stage_table(smeta->af, &smeta->stage_info[nstages-1]);

    }
    return SDK_RET_OK;
}

static inline void
lpm_promote_route (lpm_inode_t *inode, uint32_t stage, lpm_stage_meta_t *smeta)
{
    sdk_ret_t           ret;
    lpm_stage_info_t    *curr_stage;

    SDK_ASSERT(0 <= stage && stage < LPM_MAX_STAGES);
    curr_stage = &smeta->stage_info[stage];
    if (curr_stage->curr_index == smeta->keys_per_table) {
        /**< Flush current table to hw since its full */
        ret = lpm_write_stage_table(smeta->af, curr_stage);
        SDK_ASSERT(ret == SDK_RET_OK);
        /**< time to promote this to previous stage */
        lpm_promote_route(inode, stage - 1, smeta);
        /**< update this stage meta for next time */
        curr_stage->curr_index = 0;
        curr_stage->curr_table_addr += LPM_TABLE_SIZE;
        return;
    }

    /**< we need to save this node in this stage */
    ret = lpm_add_key_to_stage(smeta->af, curr_stage, inode);
    SDK_ASSERT(ret == SDK_RET_OK);
}

/**
 * @brief    build LPM tree based on the interval node table
 * @param[in] itable                interval node table
 * @param[in] num_intervals         number of intervals in the interval table
 * @param[in] lpm_tree_root_addr    pointer to the memory address at which tree
 *                                  should be built
 * @param[in] lpm_mem_size          size of LPM memory block
 * @return    SDK_RET_OK on success, failure status code on error
 */
static inline sdk_ret_t
lpm_build_tree (route_table_t *route_table, lpm_itable_t *itable,
                mem_addr_t lpm_tree_root_addr, uint32_t lpm_mem_size)
{
    sdk_ret_t           ret;
    mem_addr_t          addr;
    lpm_stage_info_t    *last_stage_info;
    lpm_stage_meta_t    smeta = { 0 };
    uint32_t            nstages, nkeys_per_table;
    uint32_t            curr_default = route_table->default_nhid;

    /**< compute the # of stages, required including the def route */
    nstages = lpm_stages(route_table->af, route_table->max_routes);
    SDK_ASSERT(nstages <= LPM_MAX_STAGES);

    /**< initialize all the stage meta */
    smeta.af = route_table->af;
    smeta.key_sz = LPM_ENTRY_KEY_SIZE(route_table->af);
    smeta.keys_per_table = LPM_KEYS_PER_TABLE(route_table->af);
    addr = lpm_tree_root_addr;
    for (uint32_t i = 0, ntables = 1; i < nstages; i++) {
        smeta.stage_info[i].curr_table_addr = addr;
        addr += ntables * LPM_TABLE_SIZE;
        ntables *= (smeta.keys_per_table + 1);
        memset(smeta.stage_info[i].curr_table, 0xFF, LPM_TABLE_SIZE);
    }
    SDK_ASSERT(addr <= lpm_tree_root_addr + lpm_mem_size);

    /**< walk all the interval tree nodes & from the interval tree bottom up */
    nkeys_per_table = (uint32_t)LPM_KEYS_PER_TABLE(route_table->af) / 2;
    last_stage_info = &smeta.stage_info[nstages-1];
    for (uint32_t i = 0; i < itable->num_intervals; i++) {
        lpm_add_key_to_last_stage(smeta.af, last_stage_info, &itable->nodes[i]);

        if (last_stage_info->curr_index == nkeys_per_table) {
            /**< table is ready to write. set the default data */
            lpm_set_default_data(smeta.af, last_stage_info, curr_default);
            /**< Write the table to HW memory */
            lpm_write_last_stage_table(smeta.af, last_stage_info);

            if (++i < itable->num_intervals) {
                /**< propogate this node to one of the previous stages */
                lpm_promote_route(&itable->nodes[i], nstages - 2, &smeta);
                /**< default data comes from the most recently promoted entry */
                curr_default = itable->nodes[i].nhid;
                /**< update the table address  */
                last_stage_info->curr_index = 0;
                last_stage_info->curr_table_addr += LPM_TABLE_SIZE;
            }
        }
    }

    ret = lpm_flush_partial_tables(&smeta, nstages, curr_default);
    SDK_ASSERT(ret == SDK_RET_OK);
    return SDK_RET_OK;
}

/**
 * @brief    build interval tree based LPM tree at the given memory address
 * @param[in] route_table           pointer to the route configuration
 * @param[in] lpm_tree_root_addr    pointer to the memory address at which tree
 *                                  should be built
 * @param[in] lpm_mem_size          LPM memory block size provided (for error
 *                                  detection)
 * @return    SDK_RET_OK on success, failure status code on error
 *
 * NOTE: route_table will be modified internally as the library does
 *       in-place sorting on the given routing table
 */
sdk_ret_t
lpm_tree_create (route_table_t *route_table,
                 mem_addr_t lpm_tree_root_addr, uint32_t lpm_mem_size)
{
    sdk_ret_t       ret;
    lpm_itable_t    itable = { 0 };

    if (route_table->num_routes == 0) {
        return sdk::SDK_RET_INVALID_ARG;
    }
    /**< sort the given route table */
    qsort(route_table->routes, route_table->num_routes,
          sizeof(route_t), route_compare_cb);
    /**< allocate memory for creating all interval tree nodes */
    //num_intervals = compute_intervals(route_table->num_routes);
    itable.nodes =
            (lpm_inode_t *)malloc(sizeof(lpm_inode_t) *
                                  (route_table->num_routes << 1));
    if (itable.nodes == NULL) {
        return sdk::SDK_RET_OOM;
    }
    ret = lpm_build_interval_table(route_table, &itable);
    SDK_ASSERT(ret == SDK_RET_OK);
    ret = lpm_build_tree(route_table, &itable, lpm_tree_root_addr, lpm_mem_size);
    SDK_ASSERT(ret == SDK_RET_OK);
    free(itable.nodes);
    return SDK_RET_OK;
}
