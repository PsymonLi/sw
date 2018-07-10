#include "offloader.h"

void eff_poller_fn(void *arg)
{
	pnso_error_t ret;
	struct spdk_poller_ctx *sp_ctx = (struct spdk_poller_ctx *)arg;

	if (arg == NULL) return;
	printf("Polling IO: %s in eff thread\n", sp_ctx->io->name);
	ret = sp_ctx->poller(sp_ctx->poll_ctx);
	if (ret != EAGAIN) {
		spdk_poller_unregister(&sp_ctx->spdk_poller_hdl);
		free(sp_ctx);
	}
}


void eff_comp_cb(void *arg1, struct pnso_service_result *svc_res)
{
	struct io_ctx *io = (struct io_ctx *)arg1;

	if (arg1 == NULL) return;
	printf("IO: %s in Eff thread completed\n", io->name);
	printf("IO: %s compressed length is %d\n", io->name,
			svc_res->svc[0].u.dst.data_len);
	free(svc_res);
	spdk_event_call(spdk_event_allocate(
			io->tchain.threads[++io->tchain.current_thread],
			offload_fn, io, NULL));
}

int exec_eff_thread(void *arg1, void *arg2)
{
	struct pnso_service_request *svc_req;
	struct pnso_service_result *svc_res;
	struct io_ctx *io = (struct io_ctx *)arg1;
	struct pnso_hash_tag *hash_tags;
	size_t alloc_sz;
	int rc;

	if (arg1 == NULL) return PNSO_INVALID_ARG;
	printf("Servicing IO: %s in eff thread\n", io->name);
	hash_tags = io->tbuf[io->tchain.current_thread].hash_tags;
	memset(hash_tags, 0, PNSO_NUM_TAGS * sizeof(struct pnso_hash_tag));

	/* Allocate request and response */
	alloc_sz = sizeof(struct pnso_service_request) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service);
	svc_req = (struct pnso_service_request *) malloc(alloc_sz);
	memset(svc_req, 0, alloc_sz);

	alloc_sz = sizeof(struct pnso_service_result) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service_status);
	svc_res = (struct pnso_service_result *) malloc(alloc_sz);
	memset(svc_res, 0, alloc_sz);

	svc_req->sgl = io->src_buflist[io->tchain.current_thread];

	/* Setup 2 services */
	svc_req->num_services = 2;
	svc_res->num_services = 2;

	/* Setup compression service */
	svc_req->svc[0].svc_type = PNSO_SVC_TYPE_COMPRESS;
	svc_req->svc[0].u.cp_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
	svc_req->svc[0].u.cp_desc.flags = PNSO_CP_DFLAG_ZERO_PAD | PNSO_CP_DFLAG_INSERT_HEADER;
	svc_req->svc[0].u.cp_desc.threshold_len = PNSO_TEST_DATA_SIZE - 8;
	svc_res->svc[0].u.dst.sgl = io->dst_buflist[io->tchain.current_thread];

	/* Setup hash service */
	svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
	svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
	svc_res->svc[1].u.hash.num_tags = 16;
	svc_res->svc[1].u.hash.tags = hash_tags;

	/* Execute asynchronously */
	struct spdk_poller_ctx *sp_ctx = (struct spdk_poller_ctx *)malloc(sizeof(*sp_ctx));

	memset(sp_ctx, 0, sizeof(*sp_ctx));
	rc = pnso_submit_request(svc_req, svc_res,
				eff_comp_cb, io,
				&sp_ctx->poller, &sp_ctx->poll_ctx);
	if (rc != 0) {
		printf("pnso_submit_request failed in eff thread with rc: %d\n", rc);
		return rc;
	}
	sp_ctx->io = io;
	sp_ctx->spdk_poller_hdl = spdk_poller_register(eff_poller_fn, sp_ctx, 10000); //Call every 10msec

	return 0;
}

