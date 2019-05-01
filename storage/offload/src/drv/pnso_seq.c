/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#include "pnso_seq_ops.h"
#include "pnso_seq.h"
#include "pnso_utils.h"

/* run on model/dol or on real hardware */
#ifdef PNSO_API_ON_MODEL
const struct sequencer_ops *g_sequencer_ops = &model_seq_ops;
#else
const struct sequencer_ops *g_sequencer_ops = &hw_seq_ops;
#endif

void *
seq_setup_desc(struct service_info *svc_info, const void *src_desc,
		size_t desc_size)
{
	return g_sequencer_ops->setup_desc(svc_info, src_desc, desc_size);
}

void
seq_cleanup_desc(struct service_info *svc_info)
{
	return g_sequencer_ops->cleanup_desc(svc_info);
}

pnso_error_t
seq_ring_db(struct service_info *svc_info)
{
	pnso_error_t err;

	if (pnso_lif_reset_ctl_pending()) {
		err = PNSO_LIF_IO_ERROR;
		OSAL_LOG_ERROR("pnso pending error reset! err: %d", err);
		goto out;
	}
	err = g_sequencer_ops->ring_db(svc_info);
out:
	return err;
}

pnso_error_t
seq_setup_cp_chain_params(struct service_info *svc_info,
			struct cpdc_desc *cp_desc,
			struct cpdc_status_desc *status_desc)
{
	return g_sequencer_ops->setup_cp_chain_params(svc_info,
			cp_desc, status_desc);
}

pnso_error_t
seq_setup_cpdc_chain(struct service_info *svc_info,
		   struct cpdc_desc *cp_desc)
{
	return g_sequencer_ops->setup_cpdc_chain(svc_info, cp_desc);
}

pnso_error_t
seq_setup_cp_pad_chain_params(struct service_info *svc_info,
			struct cpdc_desc *cp_desc,
			struct cpdc_status_desc *status_desc)
{
	return g_sequencer_ops->setup_cp_pad_chain_params(svc_info,
			cp_desc, status_desc);
}

pnso_error_t
seq_setup_hash_chain_params(struct cpdc_chain_params *chain_params,
			struct service_info *svc_info,
			struct cpdc_desc *hash_desc, struct cpdc_sgl *sgl)
{
	return g_sequencer_ops->setup_hash_chain_params(chain_params, svc_info,
			hash_desc, sgl);
}

pnso_error_t
seq_setup_chksum_chain_params(struct cpdc_chain_params *chain_params,
			struct service_info *svc_info,
			struct cpdc_desc *chksum_desc, struct cpdc_sgl *sgl)
{
	return g_sequencer_ops->setup_chksum_chain_params(chain_params,
			svc_info, chksum_desc, sgl);
}
pnso_error_t
seq_setup_cpdc_chain_status_desc(struct service_info *svc_info)
{
	return g_sequencer_ops->setup_cpdc_chain_status_desc(svc_info);
}

void
seq_cleanup_cpdc_chain(struct service_info *svc_info)
{
	g_sequencer_ops->cleanup_cpdc_chain(svc_info);
}

pnso_error_t
seq_setup_crypto_chain(struct service_info *svc_info,
			struct crypto_desc *desc)
{
	return g_sequencer_ops->setup_crypto_chain(svc_info, desc);
}

void
seq_cleanup_crypto_chain(struct service_info *svc_info)
{
	g_sequencer_ops->cleanup_crypto_chain(svc_info);
}
