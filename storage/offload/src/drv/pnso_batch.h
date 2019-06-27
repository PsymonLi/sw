/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#ifndef __PNSO_BATCH_H__
#define __PNSO_BATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/atomic.h>
#include "pnso_req.h"
#include "pnso_mpool.h"
#include "pnso_chain_params.h"

#define PNSO_LIF_IO_ERROR		(EIO)

#define MAX_PAGE_ENTRIES_SHIFT_BITS	(4)	/* 16 entries per page */
#define MAX_PAGE_ENTRIES		(1 << MAX_PAGE_ENTRIES_SHIFT_BITS)
#define MAX_NUM_PAGES			(32)	/* 512 entries per batch */
#define MAX_NUM_BATCH_ENTRIES		(MAX_NUM_PAGES * MAX_PAGE_ENTRIES)
#define MAX_NUM_DESCS			MAX_NUM_PAGES

#define GET_NUM_PAGES_ACTIVE(n)	\
	((n + MAX_PAGE_ENTRIES - 1) / MAX_PAGE_ENTRIES)
#define GET_PAGE(bi, idx)	\
	bi->bi_pages[idx >> MAX_PAGE_ENTRIES_SHIFT_BITS]
#define GET_PAGE_ENTRY(p, idx)	\
	(&p->bp_entries[idx & ((1 << MAX_PAGE_ENTRIES_SHIFT_BITS) - 1)])

struct batch_page_tags {
	uint16_t bpt_num_hashes;	/* total # of hashes in a page */
	uint16_t bpt_num_chksums;	/* total # of chksums in a page */
};

struct batch_page {
	struct batch_page_tags bp_tags;
	uint32_t bp_src_data_len;
	uint32_t bp_dst_data_len;
	struct rate_limit_control bp_rl_control;
	struct batch_page_entry {
		struct pnso_service_request *bpe_req;
		struct pnso_service_result *bpe_res;
		struct service_chain *bpe_chain;
	} bp_entries[MAX_PAGE_ENTRIES];
};

/* batch flags */
#define BATCH_BFLAG_MODE_SYNC		(1 << 0)
#define BATCH_BFLAG_MODE_POLL		(1 << 1)
#define BATCH_BFLAG_RESERVED		(1 << 2) /* rsvd for chain/poll-async */
#define BATCH_BFLAG_MODE_ASYNC		(1 << 3)
#define BATCH_BFLAG_CHAIN_PRESENT	(1 << 4)

struct batch_info {
	uint16_t bi_flags;		/* for async/intr, must be 1st member */
	uint16_t bi_gen_id;		/* generation id */
	uint16_t bi_svc_type;		/* to ensure homogeneous request */
	enum mem_pool_type bi_mpool_type;	/* bulk desc */
	struct per_core_resource *bi_pcr;

	completion_cb_t	bi_req_cb;	/* caller supplied call-back */
	atomic64_t bi_req_cb_ctx;	/* caller supplied cb context */

	uint32_t bi_polled_idx;		/* last chain polled successfully */
	uint32_t bi_num_entries;	/* total # of requests */
	void *bi_bulk_desc[MAX_NUM_DESCS];	/* cpdc/crypto desc */
	struct batch_page *bi_pages[MAX_NUM_PAGES];
	uint64_t bi_poll_ts;		/* first poll timestamp */

	uint64_t bi_sw_latency_start;	/* track ring db timestamp for software latency */
	uint64_t bi_hw_latency_start;	/* track ring db timestamp for hardware latency */
};

pnso_error_t bat_add_to_batch(struct per_core_resource *pcr,
		struct pnso_service_request *svc_req,
		struct pnso_service_result *svc_res);

pnso_error_t bat_flush_batch(struct request_params *req_params);

void bat_destroy_batch(struct per_core_resource *pcr);

pnso_error_t bat_poller(struct batch_info *batch_info, uint16_t gen_id, bool is_timeout);
void bat_poll_timeout_all(struct per_core_resource *pcr);
pnso_error_t pnso_batch_poller(void *poll_ctx);

#ifdef __cplusplus
}
#endif

#endif /* __PNSO_BATCH_H__ */
