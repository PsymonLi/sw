/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#include "osal.h"
#include "pnso_api.h"

#include "pnso_chain.h"

static pnso_error_t
decompact_setup(struct service_info *svc_info,
		const struct service_params *svc_params)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_chain(struct chain_entry *centry)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_sub_chain_from_cpdc(struct service_info *svc_info,
			      struct cpdc_chain_params *cpdc_chain)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_sub_chain_from_crypto(struct service_info *svc_info,
				struct crypto_chain_params *crypto_chain)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_enable_interrupt(struct service_info *svc_info, void *poll_ctx)
{
	return EOPNOTSUPP;
}

static void
decompact_disable_interrupt(struct service_info *svc_info)
{
	/* EOPNOTSUPP */
}

static pnso_error_t
decompact_ring_db(struct service_info *svc_info)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_poll(struct service_info *svc_info)
{
	return EOPNOTSUPP;
}

static pnso_error_t
decompact_write_result(struct service_info *svc_info)
{
	return EOPNOTSUPP;
}

static void
decompact_teardown(struct service_info *svc_info)
{
	/* EOPNOTSUPP */
}

struct service_ops decompact_ops = {
	.setup = decompact_setup,
	.chain = decompact_chain,
	.sub_chain_from_cpdc = decompact_sub_chain_from_cpdc,
	.sub_chain_from_crypto = decompact_sub_chain_from_crypto,
	.enable_interrupt = decompact_enable_interrupt,
	.disable_interrupt = decompact_disable_interrupt,
	.ring_db = decompact_ring_db,
	.poll = decompact_poll,
	.write_result  = decompact_write_result,
	.teardown = decompact_teardown
};
