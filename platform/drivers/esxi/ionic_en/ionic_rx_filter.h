/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 */

#ifndef _IONIC_RX_FILTER_H_
#define _IONIC_RX_FILTER_H_

#include "ionic_api.h"

struct ionic_admin_ctx;

#define RXQ_INDEX_ANY           (0xFFFF)
struct rx_filter {
        u32 flow_id;
        u32 filter_id;
        u16 rxq_index;
        struct rx_filter_add_cmd cmd;
        struct lif *lif;
//        struct hlist_node by_hash;
//        struct hlist_node by_id;
};

#define RX_FILTER_HLISTS        (1 << 10)
#define RX_FILTER_MAC_MASK      0xffffffffffff
#define RX_FILTER_HLISTS_MASK   (RX_FILTER_HLISTS - 1)

struct rx_filters {
        spinlock_t lock;
//        struct hlist_head by_hash[RX_FILTER_HLISTS];    /* by skb hash */
//        struct hlist_head by_id[RX_FILTER_HLISTS];      /* by filter_id */
        vmk_HashTable by_hash;
        vmk_HashTable by_id;
};

VMK_ReturnStatus
ionic_rx_filters_init(struct lif *lif);

void
ionic_rx_filters_deinit(struct lif *lif);

void
ionic_rx_filter_free(struct rx_filter *f);

VMK_ReturnStatus
ionic_rx_filter_save(struct lif *lif, u32 flow_id, u16 rxq_index,
                     u32 hash, struct ionic_admin_ctx *ctx);

struct rx_filter *ionic_rx_filter_by_vlan(struct lif *lif, u16 vid);
struct rx_filter *ionic_rx_filter_by_addr(struct lif *lif, const u8 *addr);

#if 0
int ionic_rx_filter_del(struct lif *lif, struct rx_filter *f);
void ionic_rx_filters_deinit(struct lif *lif);
int ionic_rx_filter_save(struct lif *lif, u32 flow_id, u16 rxq_index,
                         u32 hash, struct ionic_admin_ctx *ctx);
struct rx_filter *ionic_rx_filter_by_vlan(struct lif *lif, u16 vid);
#endif

#endif /* _IONIC_RX_FILTER_H_ */

