/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#ifndef __PNSO_INIT_H__
#define __PNSO_INIT_H__

#include "sonic_dev.h"

/*
 * The current intermediate buffer implementation optimizes for
 * 32K size, i.e., one single source of up to 32K, which has these
 * desired properties:
 *
 * a) The smaller size provides for a larger pool of intermediate buffers
 *    from which large batch operations may draw, and
 * b) A given transfer will not exceed a chain_sgl_pdma descriptor
 *    (which is a sequencer requirement).
 */
#define INTERM_BUF_NOMINAL_BUF_SIZE	32768
#define INTERM_BUF_NOMINAL_PAGE_SIZE	4096
#define INTERM_BUF_NOMINAL_NUM_BUFS					\
	(INTERM_BUF_NOMINAL_BUF_SIZE / INTERM_BUF_NOMINAL_PAGE_SIZE)
#define INTERM_BUF_MAX_BUF_SIZE		65536
#define INTERM_BUF_MAX_NUM_NOMINAL_BUFS					\
	(INTERM_BUF_MAX_BUF_SIZE / INTERM_BUF_NOMINAL_BUF_SIZE)
#define INTERM_BUF_MAX_NUM_BUFS						\
	(INTERM_BUF_MAX_BUF_SIZE / INTERM_BUF_NOMINAL_PAGE_SIZE)

/*
 * (3 for 32k buffers * 2 for src+dst buffers)
 */
#define MAX_SGLS_PER_REQ 6
#define MAX_CPDC_SGLS_PER_REQ MAX_SGLS_PER_REQ
#define MAX_CRYPTO_SGLS_PER_REQ MAX_SGLS_PER_REQ
/*
 * 2 (hash + chksum)
 */
#define MAX_CPDC_VEC_PER_REQ 2
#define MAX_CPDC_SGL_VEC_PER_REQ MAX_CPDC_VEC_PER_REQ
#define MAX_CPDC_DESC_VEC_PER_REQ MAX_CPDC_VEC_PER_REQ

#define MAX_CRYPTO_VEC_PER_REQ 1
#define MAX_CRYPTO_SGL_VEC_PER_REQ MAX_CRYPTO_VEC_PER_REQ
#define MAX_CRYPTO_DESC_VEC_PER_REQ MAX_CRYPTO_VEC_PER_REQ


/**
 * struct pc_res_init_params - used to initialize per core resources.
 *
 */
struct pc_res_init_params {
	struct pnso_init_params pnso_init;
	uint32_t rmem_page_size;
	uint32_t max_seq_sq_descs;
};

extern uint64_t pad_buffer;

void pnso_deinit(void);
void pnso_set_log_level(int level);

#endif /* __PNSO_INIT_H__ */
