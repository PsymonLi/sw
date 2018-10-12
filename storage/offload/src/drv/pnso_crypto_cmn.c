/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#include "osal.h"
#include "pnso_api.h"
#include "sonic_api_int.h"

#include "pnso_pbuf.h"
#include "pnso_mpool.h"
#include "pnso_chain.h"
#include "pnso_crypto.h"
#include "pnso_utils.h"
#include "pnso_crypto_cmn.h"

void
crypto_pprint_aol(uint64_t aol_pa)
{
	const struct crypto_aol *aol;

	aol = (const struct crypto_aol *) sonic_phy_to_virt(aol_pa);

	OSAL_LOG_DEBUG("%30s: 0x%llx ==> 0x%llx", "", (uint64_t) aol, aol_pa);
	while (aol) {
		OSAL_LOG_DEBUG("%30s: 0x%llx/%d/%d 0x%llx/%d/%d 0x%llx/%d/%d",
				"",
				aol->ca_addr_0, aol->ca_off_0, aol->ca_len_0,
				aol->ca_addr_1, aol->ca_off_1, aol->ca_len_1,
				aol->ca_addr_2, aol->ca_off_2, aol->ca_len_2);
		OSAL_LOG_DEBUG("%30s: 0x%llx/0x%llx", "",
				aol->ca_next, aol->ca_rsvd);

		aol = aol->ca_next ? sonic_phy_to_virt(aol->ca_next) : NULL;
	}
}

static void
crypto_pprint_cmd(const struct crypto_cmd *cmd)
{
	if (!cmd)
		return;

	OSAL_LOG_DEBUG("%30s: %d", "cc_enable_crc", cmd->cc_enable_crc);
	OSAL_LOG_DEBUG("%30s: %d", "cc_bypass_aes", cmd->cc_bypass_aes);
	OSAL_LOG_DEBUG("%30s: %d", "cc_is_decrypt", cmd->cc_is_decrypt);
	OSAL_LOG_DEBUG("%30s: %d", "cc_token_3", cmd->cc_token_3);
	OSAL_LOG_DEBUG("%30s: %d", "cc_token_4", cmd->cc_token_4);
}

void
crypto_pprint_desc(const struct crypto_desc *desc)
{
	if (!desc)
		return;

	OSAL_LOG_DEBUG("%30s: 0x%llx", "crypto_desc", (uint64_t) desc);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_src", desc->cd_in_aol);
	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_dst", desc->cd_out_aol);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "=== crypto_cmd", (uint64_t)&desc->cd_cmd);
	crypto_pprint_cmd(&desc->cd_cmd);

	OSAL_LOG_DEBUG("%30s: %d", "cd_key_desc_idx", desc->cd_key_desc_idx);
	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_iv_addr", desc->cd_iv_addr);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_auth_tag", desc->cd_auth_tag);

	OSAL_LOG_DEBUG("%30s: %d", "cd_hdr_size", desc->cd_hdr_size);
	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_status_addr", desc->cd_status_addr);

	OSAL_LOG_DEBUG("%30s: %d", "cd_otag", desc->cd_otag);
	OSAL_LOG_DEBUG("%30s: %d", "cd_otag_on", desc->cd_otag_on);

	OSAL_LOG_DEBUG("%30s: %d", "cd_sector_size", desc->cd_sector_size);

	OSAL_LOG_DEBUG("%30s: %d", "cd_app_tag", desc->cd_app_tag);
	OSAL_LOG_DEBUG("%30s: %d", "cd_sector_num", desc->cd_sector_num);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_db_addr", desc->cd_db_addr);
	OSAL_LOG_DEBUG("%30s: 0x%llx", "cd_db_data", desc->cd_db_data);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "=== cd_in_aol", desc->cd_in_aol);
	crypto_pprint_aol(desc->cd_in_aol);

	OSAL_LOG_DEBUG("%30s: 0x%llx", "=== cd_out_aol", desc->cd_out_aol);
	crypto_pprint_aol(desc->cd_out_aol);
}

struct crypto_aol *
crypto_aol_packed_get(const struct per_core_resource *pc_res,
		      const struct service_buf_list *svc_blist,
		      enum mem_pool_type *ret_mpool_type,
		      uint32_t *ret_total_len)
{
	struct buffer_list_iter buffer_list_iter;
	struct buffer_list_iter *iter;
	struct crypto_aol *aol_head = NULL;
	struct crypto_aol *aol_prev = NULL;
	struct crypto_aol *aol;

	iter = buffer_list_iter_init(&buffer_list_iter, svc_blist);
	*ret_mpool_type = MPOOL_TYPE_CRYPTO_AOL;
	*ret_total_len = 0;
	while (iter) {
		aol = pc_res_mpool_object_get(pc_res, MPOOL_TYPE_CRYPTO_AOL);
		if (!aol) {
			OSAL_LOG_ERROR("cannot obtain crypto aol_vec from pool: "
				       "current len %u", *ret_total_len);
			goto out;
		}
		memset(aol, 0, sizeof(*aol));
		iter = buffer_list_iter_addr_len_get(iter,
					CRYPTO_AOL_TUPLE_LEN_MAX,
					&aol->ca_addr_0, &aol->ca_len_0);
		if (iter)
			iter = buffer_list_iter_addr_len_get(iter,
					CRYPTO_AOL_TUPLE_LEN_MAX,
					&aol->ca_addr_1, &aol->ca_len_1);
		if (iter)
			iter = buffer_list_iter_addr_len_get(iter,
					CRYPTO_AOL_TUPLE_LEN_MAX,
					&aol->ca_addr_2, &aol->ca_len_2);
		*ret_total_len += aol->ca_len_0 + aol->ca_len_1 + aol->ca_len_2;

		if (!aol_head)
			aol_head = aol;
		else
			aol_prev->ca_next = sonic_virt_to_phy(aol);
		aol_prev = aol;
	}

	return aol_head;
out:
	crypto_aol_put(pc_res, MPOOL_TYPE_CRYPTO_AOL, aol_head);
	return NULL;
}

struct crypto_aol *
crypto_aol_vec_sparse_get(const struct per_core_resource *pc_res,
			  uint32_t block_size,
			  const struct service_buf_list *svc_blist,
			  enum mem_pool_type *ret_mpool_type,
			  uint32_t *ret_total_len)
{
	struct buffer_list_iter buffer_list_iter;
	struct buffer_list_iter *iter;
	struct crypto_aol *aol_vec_head;
	struct crypto_aol *aol_vec;
	uint32_t vec_count;
	uint32_t cur_count;

	OSAL_ASSERT(is_power_of_2(block_size));
	*ret_mpool_type = MPOOL_TYPE_CRYPTO_AOL_VECTOR;
	*ret_total_len = 0;
	aol_vec_head = pc_res_mpool_object_get_with_count(pc_res,
				MPOOL_TYPE_CRYPTO_AOL_VECTOR, &vec_count);
	if (!aol_vec_head) {
		OSAL_LOG_ERROR("cannot obtain crypto aol_vec from pool");
		goto out;
	}

	iter = buffer_list_iter_init(&buffer_list_iter, svc_blist);
	aol_vec = aol_vec_head;
	cur_count = 0;
	while (iter && (cur_count < vec_count)) {
		memset(aol_vec, 0, sizeof(*aol_vec));
		iter = buffer_list_iter_addr_len_get(iter, block_size,
					&aol_vec->ca_addr_0, &aol_vec->ca_len_0);
		*ret_total_len += aol_vec->ca_len_0;

		/*
		 * For padding purposes, every element of a sparse AOL
		 * must be a full block size.
		 */
		if (aol_vec->ca_len_0 != block_size) {
			OSAL_LOG_ERROR("Sparse AOL fails to make full block_size "
				       "%u, current_len %u", block_size, *ret_total_len);
			goto out;
                }
		aol_vec++;
		cur_count++;
	}

	if (iter) {
		OSAL_LOG_ERROR("buffer_list total length exceeds AOL vector, "
			       "current_len %u", *ret_total_len);
		goto out;
        }

	return aol_vec_head;
out:
	crypto_aol_put(pc_res, MPOOL_TYPE_CRYPTO_AOL_VECTOR, aol_vec_head);
	return NULL;
}

void
crypto_aol_put(const struct per_core_resource *pc_res,
	       enum mem_pool_type mpool_type,
	       struct crypto_aol *aol)
{
	struct crypto_aol *aol_next;

	while (aol) {
		aol_next = aol->ca_next ? sonic_phy_to_virt(aol->ca_next) :
					  NULL;
		pc_res_mpool_object_put(pc_res, mpool_type, (void *)aol);
		aol = aol_next;
	}
}

pnso_error_t
crypto_desc_status_convert(uint64_t status)
{
        pnso_error_t err = PNSO_OK;

	if (status & CRYPTO_LEN_NOT_MULTI_SECTORS)
		err = PNSO_ERR_CRYPTO_LEN_NOT_MULTI_SECTORS;
	else if (status)
		err = PNSO_ERR_CRYPTO_GENERAL_ERROR;

	return err;
}

