/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#include "sonic_dev.h"
#include "sonic_lif.h"

#include "osal.h"

#include "pnso_mpool.h"
#include "pnso_batch.h"
#include "pnso_cpdc.h"
#include "pnso_cpdc.h"
#include "pnso_cpdc_cmn.h"
#include "pnso_crypto_cmn.h"
#include "pnso_utils.h"

#ifdef NDEBUG
#define PPRINT_BATCH_INFO(bi)
#define PPRINT_SERVICE_RESULT(sr)
#else
#define PPRINT_BATCH_INFO(bi)						\
	do {								\
		OSAL_LOG_DEBUG("%.*s", 30, "=========================================");\
		pprint_batch_info(bi);					\
		OSAL_LOG_DEBUG("%.*s", 30, "=========================================");\
	} while (0)
#define PPRINT_SERVICE_RESULT(sr)				\
	do {								\
		OSAL_LOG_INFO("%.*s", 30, "=========================================");\
		pprint_service_result(sr);					\
		OSAL_LOG_INFO("%.*s", 30, "=========================================");\
	} while (0)
#endif

static void __attribute__((unused))
pprint_batch_info(struct batch_info *batch_info)
{
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	int i;

	if (!batch_info)
		return;

	OSAL_LOG_DEBUG("%30s: 0x" PRIx64, "=== batch_info",
			(uint64_t) batch_info);
	OSAL_LOG_DEBUG("%30s: %d", "bi_flags", batch_info->bi_flags);
	OSAL_LOG_DEBUG("%30s: %d", "bi_svc_type", batch_info->bi_svc_type);
	OSAL_LOG_DEBUG("%30s: %d", "bi_mpool_type", batch_info->bi_mpool_type);
	OSAL_LOG_DEBUG("%30s: 0x" PRIx64, "bi_pcr",
			(uint64_t) batch_info->bi_pcr);

	OSAL_LOG_DEBUG("%30s: %d", "bi_polled_idx", batch_info->bi_polled_idx);
	OSAL_LOG_DEBUG("%30s: %d", "bi_num_entries",
			batch_info->bi_num_entries);
#ifndef TEMP_DEBUG_TO_BE_REMOVED
	OSAL_LOG_DEBUG("%30s: %d", "core_id", batch_info->core_id);
	OSAL_LOG_DEBUG("%30s: %d", "color_out", batch_info->color.color_out);
	OSAL_LOG_DEBUG("%30s: %d", "color_in", batch_info->color.color_in);
#endif

	for (i = 0; i < batch_info->bi_num_entries;  i++) {
		batch_page = GET_PAGE(batch_info, i);
		page_entry = GET_PAGE_ENTRY(batch_page, i);
		OSAL_LOG_DEBUG("%30s: (%d)", "", i);
		OSAL_LOG_DEBUG("%30s: 0x" PRIx64 "/0x" PRIx64,
				"",
				(uint64_t) batch_page, (uint64_t) page_entry);

		OSAL_LOG_DEBUG("%30s: 0x" PRIx64 "/0x" PRIx64 "/0x" PRIx64 "/%u/%u",
				"",
				(uint64_t) page_entry->bpe_req,
				(uint64_t) page_entry->bpe_res,
				(uint64_t) page_entry->bpe_chain,
				batch_page->bp_tags.bpt_num_hashes,
				batch_page->bp_tags.bpt_num_chksums);
	}

	OSAL_LOG_DEBUG("%30s: 0x" PRIx64, "bi_req_cb",
			(uint64_t) batch_info->bi_req_cb);
	OSAL_LOG_DEBUG("%30s: 0x" PRIx64, "bi_req_cb_ctx",
			(uint64_t) batch_info->bi_req_cb_ctx);
}

static void __attribute__((unused))
pprint_service_result(struct pnso_service_result *res)
{
	uint32_t i;

	if (!res)
		return;

	OSAL_LOG_DEBUG("%30s: %p", "=== pnso_service_result", res);

	OSAL_LOG_DEBUG("%30s: %d", "err", res->err);

	OSAL_LOG_DEBUG("%30s: %d", "num_services", res->num_services);
	for (i = 0; i < res->num_services; i++) {
		OSAL_LOG_DEBUG("%30s: %d", "service #", i+1);

		OSAL_LOG_DEBUG("%30s: %d", "err", res->svc[i].err);
		OSAL_LOG_DEBUG("%30s: %d", "svc_type", res->svc[i].svc_type);
		OSAL_LOG_DEBUG("%30s: %d", "rsvd_1", res->svc[i].rsvd_1);

		switch (res->svc[i].svc_type) {
		case PNSO_SVC_TYPE_ENCRYPT:
		case PNSO_SVC_TYPE_DECRYPT:
		case PNSO_SVC_TYPE_COMPRESS:
		case PNSO_SVC_TYPE_DECOMPRESS:
		case PNSO_SVC_TYPE_DECOMPACT:
			OSAL_LOG_DEBUG("%30s: %d", "data_len",
					res->svc[i].u.dst.data_len);
			break;
		case PNSO_SVC_TYPE_HASH:
			OSAL_LOG_DEBUG("%30s: %d", "num_tags",
					res->svc[i].u.hash.num_tags);
			OSAL_LOG_DEBUG("%30s: %d", "rsvd_2",
					res->svc[i].u.hash.rsvd_2);
			break;
		case PNSO_SVC_TYPE_CHKSUM:
			OSAL_LOG_DEBUG("%30s: %d", "num_tags",
					res->svc[i].u.chksum.num_tags);
			OSAL_LOG_DEBUG("%30s: %d", "rsvd_3",
					res->svc[i].u.chksum.rsvd_3);
			break;
		default:
			OSAL_ASSERT(0);
			break;
		}
	}
}

static void *
get_mpool_batch_object(struct per_core_resource *pcr,
		enum mem_pool_type pool_type)
{
	pnso_error_t err = EINVAL;
	struct mem_pool *batch_page_mpool, *batch_info_mpool;
	void *p = NULL;

	OSAL_LOG_DEBUG("enter ...");

	OSAL_ASSERT((pool_type == MPOOL_TYPE_BATCH_PAGE) ||
			(pool_type == MPOOL_TYPE_BATCH_INFO));

	batch_page_mpool = pcr->mpools[MPOOL_TYPE_BATCH_PAGE];
	batch_info_mpool = pcr->mpools[MPOOL_TYPE_BATCH_INFO];
	if (!batch_page_mpool || !batch_info_mpool) {
		OSAL_ASSERT(0);
		goto out;
	}

	p =  (void *) ((pool_type == MPOOL_TYPE_BATCH_PAGE) ?
			mpool_get_object(batch_page_mpool) :
			mpool_get_object(batch_info_mpool));
	if (!p) {
		OSAL_ASSERT(0);
		goto out;
	}

	OSAL_LOG_DEBUG("obtained batch page/info object from mpool. pcr: 0x " PRIx64 " pool_type: %d",
			(uint64_t) pcr, pool_type);
	OSAL_LOG_DEBUG("exit!");
	return p;

out:
	OSAL_LOG_ERROR("exit! failed to obtain batch page/info object from mpool. pcr: 0x" PRIx64 " pool_type: %d err: %d",
			(uint64_t) pcr, pool_type, err);
	return p;
}

static pnso_error_t
put_mpool_batch_object(struct per_core_resource *pcr,
		enum mem_pool_type pool_type, void *p)
{
	pnso_error_t err = EINVAL;
	struct mem_pool *batch_page_mpool, *batch_info_mpool;

	OSAL_ASSERT((pool_type == MPOOL_TYPE_BATCH_PAGE) ||
			(pool_type == MPOOL_TYPE_BATCH_INFO));

	batch_page_mpool = pcr->mpools[MPOOL_TYPE_BATCH_PAGE];
	batch_info_mpool = pcr->mpools[MPOOL_TYPE_BATCH_INFO];
	if (!batch_page_mpool || !batch_info_mpool) {
		OSAL_ASSERT(0);
		goto out;
	}

	if (pool_type == MPOOL_TYPE_BATCH_PAGE)
		mpool_put_object(batch_page_mpool, p);
	else
		mpool_put_object(batch_info_mpool, p);

	OSAL_LOG_DEBUG("returned batch page/info object to mpool. pcr: 0x " PRIx64 " pool_type: %d",
			(uint64_t) pcr, pool_type);
	return err;

out:
	OSAL_LOG_ERROR("exit! failed to return batch page/info object from mpool. pcr: 0x " PRIx64 " pool_type: %d err: %d",
			(uint64_t) pcr, pool_type, err);
	return err;
}

static pnso_error_t
get_bulk_batch_desc(struct batch_info *batch_info, uint32_t page_idx)
{
	pnso_error_t err;
	struct mem_pool *mpool;
	struct per_core_resource *pcr;

	pcr = batch_info->bi_pcr;
	if (batch_info->bi_mpool_type == MPOOL_TYPE_CRYPTO_DESC_VECTOR) {
		mpool = pcr->mpools[batch_info->bi_mpool_type];
		batch_info->bi_bulk_desc[page_idx] =
			crypto_get_batch_bulk_desc(mpool);
	} else {
		mpool = pcr->mpools[batch_info->bi_mpool_type];
		batch_info->bi_bulk_desc[page_idx] =
			cpdc_get_batch_bulk_desc(mpool);
	}

	if (!batch_info->bi_bulk_desc[page_idx]) {
		err = ENOMEM;
		OSAL_LOG_ERROR("failed to obtain batch bulk desc. pcr: 0x " PRIx64 " pool_type: %d page_idx: %d err: %d",
			(uint64_t) pcr, batch_info->bi_mpool_type, page_idx, err);
		goto out;
	}

	err = PNSO_OK;

	OSAL_LOG_DEBUG("obtained batch desc. pcr: 0x" PRIx64 " pool_type: %d page_idx: %d desc: 0x" PRIx64,
			(uint64_t) pcr, batch_info->bi_mpool_type, page_idx,
			(uint64_t) batch_info->bi_bulk_desc[page_idx]);
out:
	return err;
}

static void
put_bulk_batch_desc(struct batch_info *batch_info, uint32_t page_idx)
{
	struct mem_pool *mpool;
	struct per_core_resource *pcr;

	pcr = batch_info->bi_pcr;
	if (batch_info->bi_mpool_type == MPOOL_TYPE_CRYPTO_DESC_VECTOR) {
		mpool = pcr->mpools[batch_info->bi_mpool_type];
		crypto_put_batch_bulk_desc(mpool,
				batch_info->bi_bulk_desc[page_idx]);
	} else {
		/* both cpdc desc and pb vector as well covered */
		mpool = pcr->mpools[batch_info->bi_mpool_type];
		cpdc_put_batch_bulk_desc(mpool,
				batch_info->bi_bulk_desc[page_idx]);
	}

	OSAL_LOG_DEBUG("returned batch desc. pcr: 0x" PRIx64 " pool_type: %d page_idx: %d desc: 0x" PRIx64,
			(uint64_t) pcr, batch_info->bi_mpool_type, page_idx,
			(uint64_t) batch_info->bi_bulk_desc[page_idx]);
}

static inline enum mem_pool_type
get_batch_mpool_type(struct pnso_service *svc)
{
	enum mem_pool_type mpool_type;
	struct pnso_hash_desc *hash_desc;
	struct pnso_checksum_desc *chksum_desc;

	switch (svc->svc_type) {
	case PNSO_SVC_TYPE_ENCRYPT:
	case PNSO_SVC_TYPE_DECRYPT:
		mpool_type = MPOOL_TYPE_CRYPTO_DESC_VECTOR;
		break;
	case PNSO_SVC_TYPE_COMPRESS:
	case PNSO_SVC_TYPE_DECOMPRESS:
		mpool_type = MPOOL_TYPE_CPDC_DESC_VECTOR;
		break;
	case PNSO_SVC_TYPE_HASH:
		hash_desc = &svc->u.hash_desc;
		mpool_type = (hash_desc->flags & PNSO_HASH_DFLAG_PER_BLOCK) ?
			MPOOL_TYPE_CPDC_DESC_PB_VECTOR :
			MPOOL_TYPE_CPDC_DESC_VECTOR;
		break;
	case PNSO_SVC_TYPE_CHKSUM:
		chksum_desc = &svc->u.chksum_desc;
		mpool_type = (chksum_desc->flags &
				PNSO_CHKSUM_DFLAG_PER_BLOCK) ?
			MPOOL_TYPE_CPDC_DESC_PB_VECTOR :
			MPOOL_TYPE_CPDC_DESC_VECTOR;
		break;
	default:
		mpool_type = MPOOL_TYPE_NONE;
		OSAL_LOG_DEBUG("invalid pool type! mpool_type: %d",
				mpool_type);
		OSAL_ASSERT(0);
		break;
	}

	return mpool_type;
}

static pnso_error_t
poll_all_chains(struct batch_info *batch_info)
{
	pnso_error_t err = EINVAL;
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	uint32_t idx;

	OSAL_LOG_DEBUG("poll batch/chain bi_polled_idx: %d",
			batch_info->bi_polled_idx);

	for (idx = batch_info->bi_polled_idx;
			idx < batch_info->bi_num_entries; idx++) {
		batch_page = GET_PAGE(batch_info, idx);
		page_entry = GET_PAGE_ENTRY(batch_page, idx);

		OSAL_LOG_DEBUG("polling batch/chain idx: %d", idx);

		err = chn_poll_all_services(page_entry->bpe_chain);
		if (err)
			break;
	}
	batch_info->bi_polled_idx = idx;

	return err;
}

static struct batch_info *
init_batch_info(struct pnso_service_request *req)
{
	struct per_core_resource *pcr = putil_get_per_core_resource();
	enum mem_pool_type mpool_type;
	struct batch_info *batch_info = NULL;

	mpool_type = get_batch_mpool_type(&req->svc[0]);
	if (mpool_type == MPOOL_TYPE_NONE)
		goto out;

	batch_info = (struct batch_info *)
		get_mpool_batch_object(pcr, MPOOL_TYPE_BATCH_INFO);
	if (!batch_info)
		goto out;

#ifndef TEMP_DEBUG_TO_BE_REMOVED
	memset(batch_info, 0, offsetof(struct batch_info, color));
	batch_info->color.color_out ^= 1;
#else
	memset(batch_info, 0, sizeof(struct batch_info));
#endif

	batch_info->bi_svc_type = req->svc[0].svc_type;
	batch_info->bi_mpool_type = mpool_type;
	batch_info->bi_pcr = pcr;
	batch_info->bi_polled_idx = 0;

	pcr->batch_info = batch_info;
out:
	return batch_info;
}

static void
destroy_batch_chain(struct batch_info *batch_info)
{
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	uint32_t idx, num_entries;

	OSAL_LOG_DEBUG("enter ...");

	PPRINT_BATCH_INFO(batch_info);
	num_entries = batch_info->bi_num_entries;
	for (idx = 0; idx < num_entries; idx++) {
		batch_page = GET_PAGE(batch_info, idx);
		page_entry = GET_PAGE_ENTRY(batch_page, idx);

		OSAL_LOG_DEBUG("destroy chain num_entries: %d idx: %d pge: 0x" PRIx64 " chain: 0x" PRIx64,
				num_entries, idx, (uint64_t) page_entry,
				(uint64_t) page_entry->bpe_chain);

		if (page_entry->bpe_chain)
			chn_destroy_chain(page_entry->bpe_chain);
	}

	OSAL_LOG_DEBUG("exit!");
}

static void
deinit_batch(struct batch_info *batch_info)
{
	struct per_core_resource *pcr;
	struct batch_page *batch_page;
	uint32_t idx, num_pages;

	OSAL_LOG_DEBUG("release pages/batch batch_info: 0x" PRIx64,
			(uint64_t) batch_info);

	if (batch_info->bi_flags & BATCH_BFLAG_CHAIN_PRESENT)
		destroy_batch_chain(batch_info);

	pcr = batch_info->bi_pcr;
	num_pages = GET_NUM_PAGES_ACTIVE(batch_info->bi_num_entries);
	for (idx = 0; idx < num_pages; idx++) {
		put_bulk_batch_desc(batch_info, idx);

		batch_page = batch_info->bi_pages[idx];
		put_mpool_batch_object(pcr, MPOOL_TYPE_BATCH_PAGE, batch_page);

		OSAL_LOG_DEBUG("released page pcr: 0x" PRIx64 " page: 0x" PRIx64 " idx: %d",
				(uint64_t) pcr, (uint64_t) batch_page, idx);
	}

#ifndef TEMP_DEBUG_TO_BE_REMOVED
	batch_info->color.color_in = batch_info->color.color_out;
#endif
	put_mpool_batch_object(pcr, MPOOL_TYPE_BATCH_INFO, batch_info);
}

void
bat_destroy_batch(void)
{
	struct per_core_resource *pcr = putil_get_per_core_resource();
	struct batch_info *batch_info;

	OSAL_LOG_DEBUG("enter ...");

	batch_info = pcr->batch_info;
	if (!batch_info) {
		OSAL_LOG_ERROR("batch not found! pcr: 0x" PRIx64,
				(uint64_t) pcr);
		goto out;
	}
	deinit_batch(batch_info);
	pcr->batch_info = NULL;

out:
	OSAL_LOG_DEBUG("exit!");
}

static pnso_error_t
add_page(struct batch_info *batch_info)
{
	pnso_error_t err = EINVAL;
	struct batch_page *batch_page;
	uint32_t page_idx;

	batch_page = (struct batch_page *)
		get_mpool_batch_object(batch_info->bi_pcr,
				MPOOL_TYPE_BATCH_PAGE);
	if (!batch_page) {
		OSAL_LOG_DEBUG("failed to obtain batch page from pool! err: %d",
				err);
		goto out;
	}
	memset(batch_page, 0, sizeof(struct batch_page));

	page_idx = batch_info->bi_num_entries / MAX_PAGE_ENTRIES;
	OSAL_ASSERT(page_idx < MAX_NUM_PAGES);
	batch_info->bi_pages[page_idx] = batch_page;

	/* get a vector of either CPDC or Crypto bulk desc */
	err = get_bulk_batch_desc(batch_info, page_idx);
	if (err)
		goto out;

	OSAL_LOG_DEBUG("added new page 0x" PRIx64 " page_idx: %d",
			(uint64_t) batch_page, page_idx);
	err = PNSO_OK;
out:
	return err;
}

static void read_write_result_all_chains(struct batch_info *batch_info);
static void read_write_error_result_all_chains(struct batch_info *batch_info,
					       pnso_error_t err);

pnso_error_t bat_poller(void *pnso_poll_ctx);

pnso_error_t
bat_poller(void *poll_ctx)
{
	pnso_error_t err = EINVAL;
	struct batch_info *batch_info = (struct batch_info *) poll_ctx;
	completion_cb_t	cb;
	void *cb_ctx;

	OSAL_LOG_DEBUG("enter ...");

	OSAL_LOG_DEBUG("core_id: %u", osal_get_coreid());

	if (!poll_ctx) {
		OSAL_LOG_ERROR("invalid poll context! err: %d", err);
		goto out;
	}
	PPRINT_BATCH_INFO(batch_info);

#ifndef TEMP_DEBUG_TO_BE_REMOVED
	if ((batch_info->color.color_in ^ batch_info->color.color_out) == 0) {
		g_osal_log_level = OSAL_LOG_LEVEL_DEBUG;
		pprint_batch_info(batch_info);
		OSAL_ASSERT(batch_info->color.color_in ^ batch_info->color.color_out);
	}
#endif

	err = poll_all_chains(batch_info);
	if (err) {
		OSAL_LOG_DEBUG("poll failed! batch_info: 0x" PRIx64 "err: %d",
			(uint64_t) batch_info, err);
		if (err == EBUSY)
			goto out;
		read_write_error_result_all_chains(batch_info, err);
	} else {
		/*
		 * on success, read/write result from first to last chain's - first
		 * to last service - of the entire batch
		 *
		 */
		read_write_result_all_chains(batch_info);
	}

	/* save caller's cb and context ahead of destroy */
	cb = batch_info->bi_req_cb;
	cb_ctx =  batch_info->bi_req_cb_ctx;

	deinit_batch(batch_info);

	if (cb) {
		OSAL_LOG_DEBUG("invoking caller's cb ctx: 0x" PRIx64 "err: %d",
				(uint64_t) cb_ctx, err);

		cb(cb_ctx, NULL);
	}

out:
	OSAL_LOG_DEBUG("exit! err: %d", err);
	return err;
}

static inline void
set_batch_mode(uint16_t mode_flags, uint16_t *flags)
{
	if (mode_flags & REQUEST_RFLAG_MODE_SYNC)
		*flags |= BATCH_BFLAG_MODE_SYNC;
	else if (mode_flags & REQUEST_RFLAG_MODE_POLL)
		*flags |= BATCH_BFLAG_MODE_POLL;
	else if (mode_flags & REQUEST_RFLAG_MODE_ASYNC)
		*flags |= BATCH_BFLAG_MODE_ASYNC;
}

static pnso_error_t
set_interrupt_params(struct batch_info *batch_info, struct service_chain *chain)
{
	struct service_chain *last_chain;
	struct chain_entry *last_ce;
	struct service_info *svc_info;

	last_chain = chn_get_last_service_chain(batch_info);
	last_ce = chn_get_last_centry(last_chain);
	svc_info = &last_ce->ce_svc_info;

	return svc_info->si_ops.enable_interrupt(svc_info, batch_info);
}

static pnso_error_t
build_batch(struct batch_info *batch_info, struct request_params *req_params)
{
	pnso_error_t err;
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	struct service_chain *chain = NULL;
	uint32_t idx, num_entries;

	OSAL_LOG_DEBUG("enter ...");

	set_batch_mode(req_params->rp_flags, &batch_info->bi_flags);

	if ((req_params->rp_flags & REQUEST_RFLAG_MODE_ASYNC) ||
			(req_params->rp_flags & REQUEST_RFLAG_MODE_POLL)) {
		batch_info->bi_req_cb = req_params->rp_cb;
		batch_info->bi_req_cb_ctx = req_params->rp_cb_ctx;

		if (req_params->rp_flags & REQUEST_RFLAG_MODE_POLL) {
			/* for caller to poll */
			*req_params->rp_poll_fn = bat_poller;
			*req_params->rp_poll_ctx = (void *) batch_info;
		}
	}
	PPRINT_BATCH_INFO(batch_info);

	req_params->rp_batch_info = batch_info;
	num_entries = batch_info->bi_num_entries;
	for (idx = 0; idx < num_entries; idx++) {
		batch_page = GET_PAGE(batch_info, idx);
		page_entry = GET_PAGE_ENTRY(batch_page, idx);

		OSAL_LOG_DEBUG("use pge to batch num_entries: %d idx: %d page: 0x" PRIx64 " pge: 0x" PRIx64,
				num_entries, idx, (uint64_t) batch_page,
				(uint64_t) page_entry);

		req_params->rp_svc_req = page_entry->bpe_req;
		req_params->rp_svc_res = page_entry->bpe_res;
		req_params->rp_page = batch_page;
		req_params->rp_batch_index = idx;

		err = chn_create_chain(req_params, &chain);
		if (err) {
			OSAL_LOG_DEBUG("failed to build batch of chains! idx: %d err: %d",
					idx, err);
			PAS_INC_NUM_CHAIN_FAILURES(batch_info->bi_pcr);
			if (idx)
				batch_info->bi_flags |=
					BATCH_BFLAG_CHAIN_PRESENT;
			goto out;
		}
		page_entry->bpe_chain = chain;
		PAS_INC_NUM_CHAINS(batch_info->bi_pcr);
	}
	batch_info->bi_flags |= BATCH_BFLAG_CHAIN_PRESENT;

	/* setup interrupt params in last chain's last service */
	if ((req_params->rp_flags & REQUEST_RFLAG_MODE_ASYNC) && chain) {
		err = set_interrupt_params(batch_info, chain);
		if (err)
			goto out;
	}

	OSAL_LOG_DEBUG("added all entries to batch! num_entries: %d",
			num_entries);
	PPRINT_BATCH_INFO(batch_info);

	err = PNSO_OK;
	OSAL_LOG_DEBUG("exit!");
	return err;

out:
	OSAL_LOG_ERROR("exit! err: %d", err);
	return err;
}

pnso_error_t
bat_add_to_batch(struct pnso_service_request *svc_req,
		struct pnso_service_result *svc_res)
{
	pnso_error_t err = EINVAL;
	struct per_core_resource *pcr = putil_get_per_core_resource();
	struct batch_info *batch_info;
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	uint16_t req_svc_type;
	bool new_batch = false;

	OSAL_LOG_DEBUG("enter ...");

	batch_info = pcr->batch_info;
	if (!batch_info) {
		batch_info = init_batch_info(svc_req);
		if (!batch_info) {
			OSAL_LOG_DEBUG("failed to init batch! err: %d",
					err);
			goto out;
		}
#ifndef TEMP_DEBUG_TO_BE_REMOVED
		batch_info->core_id = pcr->core_id;
#endif
		new_batch = true;
	}

	if (batch_info->bi_num_entries >= MAX_NUM_BATCH_ENTRIES) {
		OSAL_LOG_DEBUG("batch limit reached! num_entries: %d err: %d",
				batch_info->bi_num_entries, err);
		goto out_batch;
	}

	if ((batch_info->bi_num_entries % MAX_PAGE_ENTRIES) == 0) {
		err = add_page(batch_info);
		if (err)
			goto out_batch;
	}

	if (!new_batch) {
		req_svc_type = svc_req->svc[0].svc_type;
		if (batch_info->bi_svc_type != req_svc_type) {
			err = EINVAL;
			OSAL_LOG_DEBUG("batch service type mismatch! batch_svc_type: %d req_svc_type: %d err: %d",
					batch_info->bi_svc_type,
					req_svc_type, err);
			goto out_batch;
		}
	}

	batch_page = GET_PAGE(batch_info, batch_info->bi_num_entries);
	page_entry = GET_PAGE_ENTRY(batch_page, batch_info->bi_num_entries);

	page_entry->bpe_req = svc_req;
	page_entry->bpe_res = svc_res;
	batch_info->bi_num_entries++;

	err = PNSO_OK;
	OSAL_LOG_DEBUG("exit!");
	return err;

out_batch:
	deinit_batch(batch_info);
	pcr->batch_info = NULL;
out:
	OSAL_LOG_ERROR("exit! err: %d", err);
	return err;
}

static void
read_write_result_all_chains(struct batch_info *batch_info)
{
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	int i;

	for (i = 0; i < batch_info->bi_num_entries;  i++) {
		batch_page = GET_PAGE(batch_info, i);
		page_entry = GET_PAGE_ENTRY(batch_page, i);

		chn_read_write_result(page_entry->bpe_chain);

		chn_update_overall_result(page_entry->bpe_chain);

		chn_notify_caller(page_entry->bpe_chain);

		PPRINT_SERVICE_RESULT(page_entry->bpe_res);
	}
}

static void
read_write_error_result_all_chains(struct batch_info *batch_info, pnso_error_t err)
{
	struct batch_page *batch_page;
	struct batch_page_entry *page_entry;
	int i;

	for (i = 0; i < batch_info->bi_num_entries;  i++) {
		batch_page = GET_PAGE(batch_info, i);
		page_entry = GET_PAGE_ENTRY(batch_page, i);

		if (page_entry->bpe_chain) {
			page_entry->bpe_chain->sc_res->err = err;
			chn_notify_caller(page_entry->bpe_chain);
		}
	}
}

static void
set_batch_page_cflag(struct batch_page *bp, uint16_t cflag)
{
	struct batch_page_entry *bpe;
	uint32_t i;

	for (i = 0; i < MAX_PAGE_ENTRIES; i++) {
		bpe = GET_PAGE_ENTRY(bp, i);
		if (!bpe->bpe_chain)
			break;
		bpe->bpe_chain->sc_flags |= cflag;
	}
}

static void
clear_batch_page_cflag(struct batch_page *bp, uint16_t cflag)
{
	struct batch_page_entry *bpe;
	uint32_t i;

	for (i = 0; i < MAX_PAGE_ENTRIES; i++) {
		bpe = GET_PAGE_ENTRY(bp, i);
		if (!bpe->bpe_chain)
			break;
		bpe->bpe_chain->sc_flags &= ~cflag;
	}
}

static pnso_error_t
execute_batch(struct batch_info *batch_info)
{
	pnso_error_t err = EINVAL;
	struct service_chain *first_chain, *last_chain;
	struct chain_entry *first_ce, *last_ce;
	uint32_t idx;
	uint32_t num_entries;
	bool is_sync_mode = (batch_info->bi_flags & BATCH_BFLAG_MODE_SYNC) != 0;

	OSAL_LOG_DEBUG("enter ...");

	num_entries = batch_info->bi_num_entries;
	for (idx = 0; idx < num_entries; idx += MAX_PAGE_ENTRIES) {
		/* get first chain's first service within the mini-batch */
		first_chain = chn_get_first_service_chain(batch_info, idx);
		first_ce = chn_get_first_centry(first_chain);
		OSAL_LOG_DEBUG("ring DB batch idx: %d", idx);

		/* ring DB first chain's first service within the mini-batch  */
		set_batch_page_cflag(GET_PAGE(batch_info, idx),
				CHAIN_CFLAG_RANG_DB);
		err = first_ce->ce_svc_info.si_ops.ring_db(
				&first_ce->ce_svc_info);
		if (err) {
			clear_batch_page_cflag(GET_PAGE(batch_info, idx),
					CHAIN_CFLAG_RANG_DB);
			OSAL_LOG_DEBUG("failed to ring service door bell! svc_type: %d err: %d",
				       first_ce->ce_svc_info.si_type, err);
			goto out;
		}
	}

	if (!is_sync_mode) {
		OSAL_LOG_DEBUG("in non-sync mode ...");
		goto done;
	}

	/* get last chain's last service of the batch */
	last_chain = chn_get_last_service_chain(batch_info);
	last_ce = chn_get_last_centry(last_chain);

	/* poll on last chain's last service of the batch */
	err = last_ce->ce_svc_info.si_ops.poll(&last_ce->ce_svc_info);
	if (err) {
		/* in sync-mode, poll() will return either OK or ETIMEDOUT */
		OSAL_LOG_ERROR("service failed to poll svc_type: %d err: %d",
			       last_ce->ce_svc_info.si_type, err);
		goto out;
	}

	/* on success, loop n' poll on every chain's - every service(s) */
	err = poll_all_chains(batch_info);
	if (err) {
		OSAL_LOG_ERROR("failed to poll all services err: %d", err);
		goto out;
	}

	/*
	 * on success, read/write result from first to last chain's - first
	 * to last service - of the entire batch
	 *
	 */
	read_write_result_all_chains(batch_info);

done:
	err = PNSO_OK;
	OSAL_LOG_DEBUG("exit!");
	return err;

out:
	OSAL_LOG_ERROR("exit! err: %d", err);
	return err;
}

pnso_error_t
bat_flush_batch(struct request_params *req_params)
{
	pnso_error_t err = EINVAL;
	struct per_core_resource *pcr = putil_get_per_core_resource();
	struct batch_info *batch_info;

	OSAL_LOG_DEBUG("enter ...");

	batch_info = pcr->batch_info;
	if (!batch_info) {
		OSAL_LOG_DEBUG("invalid thread/request! err: %d", err);
		goto out;
	}

	err = build_batch(batch_info, req_params);
	if (err) {
		OSAL_LOG_DEBUG("batch/build failed! err: %d", err);
		goto out;
	}

	err = execute_batch(batch_info);
	if (err) {
		OSAL_LOG_DEBUG("batch/execute failed! err: %d", err);
		goto out;
	}

	if ((batch_info->bi_flags & BATCH_BFLAG_MODE_POLL) ||
		(batch_info->bi_flags & BATCH_BFLAG_MODE_ASYNC))
		pcr->batch_info = NULL;

	OSAL_LOG_DEBUG("exit!");
	return err;

out:
	OSAL_LOG_ERROR("exit! err: %d", err);
	return err;
}
