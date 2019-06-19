#include "nic/hal/pd/pd_api.hpp"
#include "nic/hal/pd/capri/capri_hbm.hpp"
#include "nic/hal/pd/capri/capri_barco_crypto.hpp"
#include "nic/hal/pd/capri/capri_barco_res.hpp"
#include "nic/hal/pd/capri/capri_barco_rings.hpp"
#include "nic/hal/src/internal/crypto_keys.hpp"
#include "nic/hal/pd/capri/capri_barco_asym_apis.hpp"
#include <openssl/async.h>

namespace hal {
namespace pd {

hal_ret_t
capri_barco_asym_poll_pend_req(uint32_t batch_size, uint32_t* id_count, uint32_t *ids)
{
    dllist_ctxt_t          *entry = NULL;
    crypto_pend_req_t      *req = NULL;

    *id_count = 0;

    if(dllist_empty(&g_pend_req_list)) {
        return HAL_RET_OK;
    }

    dllist_for_each(entry, &g_pend_req_list) {
        req = dllist_entry(entry, crypto_pend_req_t, list_ctxt);
        HAL_TRACE_DEBUG("Checking status for req: {}", req->hw_id);
        if(capri_barco_ring_poll(types::BARCO_RING_ASYM, req->hw_id) != TRUE) {
            return HAL_RET_OK;
        }
        ids[*id_count] = req->sw_id;
        HAL_TRACE_DEBUG("Request completed for count: {} hw-id: {} sw-id: {} id: {}",
                        *id_count, req->hw_id, req->sw_id, ids[*id_count]);
        capri_barco_del_pend_req_from_db(req);
        if(++(*id_count) >= batch_size)
            break;
    }
    return HAL_RET_OK;
}

hal_ret_t
capri_barco_asym_add_pend_req(uint32_t hw_id, uint32_t sw_id)
{
    return capri_barco_add_pend_req_to_db(hw_id, sw_id);
}

void
capri_barco_wait_for_resp(uint32_t req_tag, pd_capri_barco_asym_async_args_t *async_args)
{
    ASYNC_JOB           *job = NULL;
    ASYNC_WAIT_CTX      *wait_ctx = NULL;
    OSSL_ASYNC_FD       hw_id;

    if(async_args->async_en && ((job = ASYNC_get_current_job()) != NULL)) {
        HAL_TRACE_DEBUG("Async: Pausing existing job to wait for resp..");
        if((wait_ctx = ASYNC_get_wait_ctx(job)) == NULL) {
            HAL_TRACE_ERR("Failed to get wait_ctx");
            return;
        }

        hw_id = req_tag;
        if(ASYNC_WAIT_CTX_set_wait_fd(wait_ctx, async_args->unique_key,
                                      hw_id, NULL, NULL) == 0) {
            HAL_TRACE_ERR("Failed to set fd to wait_ctx");
            return;
        }

        ASYNC_pause_job();
        HAL_TRACE_DEBUG("Async: Job resumed..");
    } else {
        HAL_TRACE_DEBUG("Poll: Waiting for resp");
        /* Poll for completion */
        while (capri_barco_ring_poll(types::BARCO_RING_ASYM, req_tag) != TRUE) {
            //HAL_TRACE_DEBUG("Waiting for Barco completion...");
        }
    }
    return;
}

hal_ret_t capri_barco_asym_ecc_point_mul_p256(uint8_t *p, uint8_t *n,
        uint8_t *xg, uint8_t *yg, uint8_t *a, uint8_t *b, uint8_t *x1, uint8_t *y1,
        uint8_t *k, uint8_t *x3, uint8_t *y3)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    int32_t                     ecc_pm_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    pd_func_args_t              pd_func_args = {0};

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"p", (char *)p, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"a", (char *)a, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"b", (char *)b, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"xg", (char *)xg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"yg", (char *)yg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"x1", (char *)x1, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"y1", (char *)y1, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"k", (char *)k, 32);

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_pm_p256_key_idx;
    // ret = pd_crypto_asym_alloc_key(&ecc_pm_p256_key_idx);
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Allocated Key Descr @ {:x}", ecc_pm_p256_key_idx);

    asym_key.key_param_list = 0; /* Barco does not use key space for ECC Point MUL for now */
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(32) |
                            CAPRI_BARCO_ASYM_CMD_ECC_POINT_MUL);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_pm_p256_key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key= &w_args;
    // ret = pd_crypto_asym_write_key(ecc_pm_p256_key_idx, &asym_key);
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write key: {}", ecc_pm_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Setup key @ {:x}", ecc_pm_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG("ECC Point Mul P256: Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)p, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param p into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param n into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)a, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param a into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)b, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param b into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)x1, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param x1 into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)y1, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param y1 into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)k, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ECC param k into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (2 * 32 ); /* Outputs: x3, y3 */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_pm_p256_key_idx;
    asym_req_descr.status_addr = curr_ptr;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    /* Poll for completion */
    while (capri_barco_ring_poll(types::BARCO_RING_ASYM, req_tag) != TRUE) {
        //HAL_TRACE_DEBUG("ECC Point Mul P256: Waiting for Barco completion...");
    }


    /* Copy out the results */
    if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)x3, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to read x3 output from memory @ {:x}",
                (uint64_t) olist_mem_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    if (sdk::asic::asic_mem_read(olist_mem_addr + 32, (uint8_t*)y3, 32)) {
        HAL_TRACE_ERR("ECC Point Mul P256: Failed to read y3 output from memory @ {:x}",
                (uint64_t) (olist_mem_addr + 32));
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"x3", (char *)x3, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"y3", (char *)y3, 32);


cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_pm_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_pm_p256_key_idx;
        // ret = pd_crypto_asym_free_key(ecc_pm_p256_key_idx);
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free key descriptor");
        }
    }

    return ret;
}

hal_ret_t
capri_barco_asym_ecdsa_p256_setup_priv_key(uint8_t *p, uint8_t *n,
                                              uint8_t *xg, uint8_t *yg,
                                              uint8_t *a, uint8_t *b,
                                              uint8_t *da, int32_t *key_idx)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_dma_descriptor_t key_dma_descr;
    crypto_asym_key_t           asym_key;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "ECDSA Setup Key: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"p", (char *)p, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"a", (char *)a, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"b", (char *)b, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"xg", (char *)xg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"yg", (char *)yg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"da", (char *)da, 32);

    *key_idx = -1;

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}",
                        key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)p, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param p into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)xg, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param xg into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)yg, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param yg into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)a, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param a into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)b, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param b into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)da, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param da into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", *key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(32) |
                            CAPRI_BARCO_ASYM_ECDSA_SIG_GEN);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = *key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", *key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", *key_idx);

    return ret;

cleanup:
    if (*key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = *key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }
    return ret;
}

hal_ret_t capri_barco_asym_ecdsa_p256_sig_gen(int32_t key_idx, uint8_t *p, uint8_t *n,
                                              uint8_t *xg, uint8_t *yg, uint8_t *a, uint8_t *b, uint8_t *da,
                                              uint8_t *k, uint8_t *h, uint8_t *r, uint8_t *s,
                                              pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "ECDSA Sig Gen: "

    HAL_TRACE_DEBUG("Key_idx: {:x}", key_idx);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"k", (char *)k, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"h", (char *)h, 32);

    if(key_idx < 0) {
        ret =
            capri_barco_asym_ecdsa_p256_setup_priv_key(p, n, xg, yg,
                                                          a, b, da,
                                                          &ecc_p256_key_idx);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to setup private key");
            goto cleanup;
        }
    } else {
        ecc_p256_key_idx = key_idx;
    }

    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)k, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param k into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)h, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param h into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (2 * 32 ); /* Outputs: r, s */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = curr_ptr;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    /* Copy out the results */
    if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)r, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read r output from memory @ {:x}",
                (uint64_t) olist_mem_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    if (sdk::asic::asic_mem_read(olist_mem_addr + 32, (uint8_t*)s, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read s output from memory @ {:x}",
                (uint64_t) (olist_mem_addr + 32));
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"r", (char *)r, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"s", (char *)s, 32);


cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }
    if(status)
        ret = HAL_RET_ERR;
    return ret;
}

hal_ret_t capri_barco_asym_ecdsa_p256_sig_verify(uint8_t *p, uint8_t *n,
                                                 uint8_t *xg, uint8_t *yg, uint8_t *a, uint8_t *b, uint8_t *xq,
                                                 uint8_t *yq, uint8_t *r, uint8_t *s, uint8_t *h,
                                                 pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr;
    barco_asym_dma_descriptor_t key_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;
    pd_func_args_t              pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "ECDSA Sig Verify: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"p", (char *)p, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"a", (char *)a, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"b", (char *)b, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"xg", (char *)xg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"yg", (char *)yg, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"xq", (char *)xq, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"yq", (char *)yq, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"r", (char *)r, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"s", (char *)s, 32);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"h", (char *)h, 32);

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}", key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)p, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param p into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)xg, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param xg into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)yg, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param yg into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)a, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param a into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)b, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param b into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)xq, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param xq into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)yq, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param yq into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_p256_key_idx;
    // ret = pd_crypto_asym_alloc_key(&ecc_p256_key_idx);
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", ecc_p256_key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(32) |
                            CAPRI_BARCO_ASYM_ECDSA_SIG_VERIFY);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_p256_key_idx;
    w_args.key = &asym_key;
    // ret = pd_crypto_asym_write_key(ecc_p256_key_idx, &asym_key);
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", ecc_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)r, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param r into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)s, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param s into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)h, 32)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ECC param h into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 32;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = 0;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = curr_ptr;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Signature Verification success");
    }

    /* No output */

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_p256_key_idx;
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa2k_setup_sig_gen_priv_key(uint8_t *n, uint8_t *d,
                                                          int32_t *key_idx)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_dma_descriptor_t key_dma_descr;
    crypto_asym_key_t           asym_key;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K Key setup: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"d", (char *)d, 256);

    *key_idx = -1;

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}", key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)d, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param d into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", *key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(256) |
                            CAPRI_BARCO_ASYM_CMD_RSA_SIG_GEN);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = *key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", *key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", *key_idx);

    return ret;

cleanup:
    if (*key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = *key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }
    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }
    return ret;
}

hal_ret_t capri_barco_asym_rsa_setup_priv_key(uint16_t key_size, uint8_t *n, uint8_t *d,
                                                            int32_t* key_idx)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    key_dma_descr_addr1 = 0, key_dma_descr_addr2 = 0;
    uint64_t                    curr_ptr = 0;
    uint64_t                    key_param_addr1 = 0, key_param_addr2 = 0;
    barco_asym_dma_descriptor_t key_dma_descr;
    crypto_asym_key_t           asym_key;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA Key setup: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, key_size);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"d", (char *)d, key_size);

    *key_idx = -1;

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param1");
        goto cleanup;
    }

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}, {:x}", key_param_addr1, key_param_addr2);

    curr_ptr = key_param_addr1;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    curr_ptr = key_param_addr2;
    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)d, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param d into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr1");
        goto cleanup;
    }
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}, {:x}", key_dma_descr_addr1, key_dma_descr_addr2);

    /* Setup key DMA descriptor 1 */
    key_dma_descr.address = key_param_addr1;
    key_dma_descr.stop = 0;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = (key_dma_descr_addr2 >> 2);
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 0;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (key_size);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr1, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr1 @ {:x}",
                (uint64_t) key_dma_descr_addr1);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup key DMA descriptor 2 */
    key_dma_descr.address = key_param_addr2;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (key_size);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr2, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr2);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", *key_idx);

    asym_key.key_param_list = key_dma_descr_addr1;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(key_size) |
                            CAPRI_BARCO_ASYM_CMD_RSA_SIG_GEN);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = *key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", *key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", *key_idx);

    return ret;

cleanup:
    if (*key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = *key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }
    if (key_dma_descr_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr2);
        }
    }
    if (key_dma_descr_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr1);
        }
    }

    if (key_param_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr2);
        }
    }
    if (key_param_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr1);
        }
    }

    return ret;
}

/* FIXME: deprecated, need to cut over to capri_barco_asym_rsa_encrypt */
hal_ret_t capri_barco_asym_rsa2k_encrypt(uint8_t *n, uint8_t *e,
                                         uint8_t *m,  uint8_t *c,
                                         pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    barco_asym_dma_descriptor_t key_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K Encrypt: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"e", (char *)e, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"m", (char *)m, 256);
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "async: {}", async_args->async_en);

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}", key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)e, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param e into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_p256_key_idx;
    // ret = pd_crypto_asym_alloc_key(&ecc_p256_key_idx);
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", ecc_p256_key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(256) |
                            CAPRI_BARCO_ASYM_CMD_RSA_ENCRYPT);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_p256_key_idx;
    w_args.key = &asym_key;
    // ret = pd_crypto_asym_write_key(ecc_p256_key_idx, &asym_key);
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", ecc_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)m, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param m into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (1 * 256); /* cipher text */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = olist_mem_addr + 256;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)c, 256)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output c from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"c", (char *)c, 256);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Encrypt succeeded");
    }

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_p256_key_idx;
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa_encrypt(uint16_t key_size, uint8_t *n, uint8_t *e,
                                         uint8_t *m,  uint8_t *c,
                                         pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    key_dma_descr_addr1 = 0, key_dma_descr_addr2 = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, status_mem_addr = 0, curr_ptr = 0;
    uint64_t                    key_param_addr1 = 0, key_param_addr2 = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    barco_asym_dma_descriptor_t key_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA Encrypt: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, key_size);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"e", (char *)e, key_size);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"m", (char *)m, key_size);
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "async: {}", async_args->async_en);

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param1");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param1 @ {:x}", key_param_addr1);
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param2 @ {:x}", key_param_addr2);

    curr_ptr = key_param_addr1;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    curr_ptr = key_param_addr2;
    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)e, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param e into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr1");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr1 @ {:x}", key_dma_descr_addr1);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr2 @ {:x}", key_dma_descr_addr2);

    /* Setup key DMA descriptor 1 */
    key_dma_descr.address = key_param_addr1;
    key_dma_descr.stop = 0;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = (key_dma_descr_addr2 >> 2);
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 0;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (key_size);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr1, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr1 @ {:x}",
                (uint64_t) key_dma_descr_addr1);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup key DMA descriptor 2 */
    key_dma_descr.address = key_param_addr2;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (key_size);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr2, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr2 @ {:x}",
                (uint64_t) key_dma_descr_addr2);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_p256_key_idx;
    // ret = pd_crypto_asym_alloc_key(&ecc_p256_key_idx);
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", ecc_p256_key_idx);

    asym_key.key_param_list = key_dma_descr_addr1;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(key_size) |
                            CAPRI_BARCO_ASYM_CMD_RSA_ENCRYPT);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_p256_key_idx;
    w_args.key = &asym_key;
    // ret = pd_crypto_asym_write_key(ecc_p256_key_idx, &asym_key);
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", ecc_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &status_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for status output");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", status_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)m, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param m into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (key_size);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (key_size); /* cipher text */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = status_mem_addr;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)c, key_size)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output c from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"c", (char *)c, key_size);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Encrypt succeeded");
    }

cleanup:

    if (status_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, status_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for status content:{:x}",
                    status_mem_addr);
        }
    }

    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_p256_key_idx;
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr2: {:x}",
                    key_dma_descr_addr2);
        }
    }

    if (key_dma_descr_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr1: {:x}",
                    key_dma_descr_addr1);
        }
    }

    if (key_param_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param2 :{:x}",
                    key_param_addr2);
        }
    }

    if (key_param_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param1 :{:x}",
                    key_param_addr1);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa2k_decrypt(uint8_t *n, uint8_t *d,
        uint8_t *c,  uint8_t *m)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    barco_asym_dma_descriptor_t key_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K Decrypt: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"d", (char *)d, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"c", (char *)c, 256);

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}", key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)d, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param d into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_p256_key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    // ret = pd_crypto_asym_alloc_key(&ecc_p256_key_idx);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", ecc_p256_key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(256) |
                            CAPRI_BARCO_ASYM_CMD_RSA_DECRYPT);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_p256_key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    // ret = pd_crypto_asym_write_key(ecc_p256_key_idx, &asym_key);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", ecc_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)c, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param c into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (1 * 256); /* plain text */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = olist_mem_addr + 256;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    /* Poll for completion */
    while (capri_barco_ring_poll(types::BARCO_RING_ASYM, req_tag) != TRUE) {
        //HAL_TRACE_DEBUG("ECC Point Mul P256: Waiting for Barco completion...");
    }
    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)m, 256)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output m from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"m", (char *)m, 256);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Decrypt succeeded");
    }

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_p256_key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t
capri_barco_asym_rsa2k_crt_setup_decrypt_priv_key(uint8_t *p, uint8_t *q, uint8_t *dp,
                                                     uint8_t *dq, uint8_t *qinv, int32_t* key_idx)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    key_dma_descr_addr1 = 0, key_dma_descr_addr2 = 0, key_dma_descr_addr3 = 0;
    uint64_t                    curr_ptr = 0;
    uint64_t                    key_param_addr1 = 0, key_param_addr2 = 0, key_param_addr3 = 0;
    barco_asym_dma_descriptor_t key_dma_descr;
    crypto_asym_key_t           asym_key;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K CRT Key setup: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"p", (char *)p, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"q", (char *)q, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"dp", (char *)dp, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"dq", (char *)dq, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"qinv", (char *)qinv, 256);

    *key_idx = -1;

    /* Key Param fragment 1 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param 1");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param 1 @ {:x}", key_param_addr1);
    /* Key Param fragment 2 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param 2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param 2 @ {:x}", key_param_addr2);
    /* Key Param fragment 3 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr3);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param 3");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param 3 @ {:x}", key_param_addr3);

    /* Allocate key DMA descriptor 1 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr1");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr1 @ {:x}", key_dma_descr_addr1);

    /* Allocate key DMA descriptor 2 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr2);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr2");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr2 @ {:x}", key_dma_descr_addr2);

    /* Allocate key DMA descriptor 3 */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr3);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr3");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr3 @ {:x}", key_dma_descr_addr3);

    /* Setup params in the key memory 1 */
    curr_ptr = key_param_addr1;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)p, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param p into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)q, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param q into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    key_dma_descr.address = key_param_addr1;
    key_dma_descr.stop = 0;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = (key_dma_descr_addr2 >> 2);
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 0;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr1);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr1, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr1);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup params in the key memory 2 */
    curr_ptr = key_param_addr2;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)dp, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param dp into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)dq, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param dq into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup key DMA descriptor 2 */
    key_dma_descr.address = key_param_addr2;
    key_dma_descr.stop = 0;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = (key_dma_descr_addr3 >> 2);
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 0;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr2);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr2, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr2);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup params in the key memory 3 */
    curr_ptr = key_param_addr3;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)qinv, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param qinv into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup key DMA descriptor 3 */
    key_dma_descr.address = key_param_addr3;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr3);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr3, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr3);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", *key_idx);

    asym_key.key_param_list = key_dma_descr_addr1;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(256) |
                            CAPRI_BARCO_ASYM_CMD_RSA_CRT_DECRYPT);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = *key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", *key_idx);
        goto cleanup;
    }

    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", *key_idx);
    return ret;

cleanup:
    if (*key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = *key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr3) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr3);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr3);
        }
    }

    if (key_dma_descr_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr2);
        }
    }

    if (key_dma_descr_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr1);
        }
    }

    if (key_param_addr3) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr3);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr3);
        }
    }

    if (key_param_addr2) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr2);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr2);
        }
    }

    if (key_param_addr1) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr1);
        }
    }

    return ret;
}

hal_ret_t capri_barco_asym_rsa2k_crt_decrypt(int32_t key_idx, uint8_t *p, uint8_t *q, uint8_t *dp,
                                             uint8_t *dq, uint8_t *qinv, uint8_t *c, uint8_t *m,
                                             pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K CRT Decrypt: "

    HAL_TRACE_DEBUG("key_idx: {}", key_idx);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"c", (char *)c, 256);

    if(key_idx < 0) {
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"p", (char *)p, 256);
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"q", (char *)q, 256);
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"dp", (char *)dp, 256);
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"dq", (char *)dq, 256);
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"qinv", (char *)qinv, 256);

        ret = capri_barco_asym_rsa2k_crt_setup_decrypt_priv_key(p, q, dp, dq, qinv, &ecc_p256_key_idx);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to setup private key");
            goto cleanup;
        }
    } else {
        ecc_p256_key_idx = key_idx;
    }

    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "key @ {:x}", ecc_p256_key_idx);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)c, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param c into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (1 * 256); /* plain text */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = olist_mem_addr + 256;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)m, 256)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output m from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"m", (char *)m, 256);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA CRT Decrypt succeeded");
    }

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa2k_sig_gen(int32_t key_idx, uint8_t *n, uint8_t *d,
                                         uint8_t *h, uint8_t *s,
                                         pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K Sig Gen: "
    HAL_TRACE_DEBUG("key_idx: {}", key_idx);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"h", (char *)h, 256);

    if(key_idx < 0) {
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"d", (char *)d, 256);
        ret = capri_barco_asym_rsa2k_setup_sig_gen_priv_key(n, d, &ecc_p256_key_idx);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to setup private key");
            goto cleanup;
        }
    } else {
        ecc_p256_key_idx = key_idx;
    }

    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)h, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param h into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (1 * 256); /* sig */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = olist_mem_addr + 256;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)s, 256)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output s from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"s", (char *)s, 256);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Sig Gen succeeded");
    }

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa_sig_gen(uint16_t key_size, int32_t key_idx,
        uint8_t *n, uint8_t *d, uint8_t *h, uint8_t *s,
        pd_capri_barco_asym_async_args_t *async_args)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0, olist_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0, status_mem_addr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr, olist_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA Sig Gen: "
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, key_size);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"h", (char *)h, key_size);

    if(key_idx < 0) {
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"d", (char *)d, key_size);
        ret = capri_barco_asym_rsa_setup_priv_key(key_size, n, d, &ecc_p256_key_idx);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to setup private key");
            goto cleanup;
        }
    } else {
        ecc_p256_key_idx = key_idx;
    }

    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &olist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for olist DMA Descr @ {:x}", olist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &status_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for status content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for status mem @ {:x}", status_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)h, key_size)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param h into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += key_size;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup olist DMA descriptor */
    olist_dma_descr.address = olist_mem_addr;
    olist_dma_descr.stop = 1;
    olist_dma_descr.rsvd0 = 1;
    olist_dma_descr.next = 0;
    olist_dma_descr.int_en = 0;
    olist_dma_descr.discard = 0;
    olist_dma_descr.realign = 1;
    olist_dma_descr.cst_addr = 0;
    olist_dma_descr.length = (1 * key_size); /* sig */
    if (sdk::asic::asic_mem_write(olist_dma_descr_addr, (uint8_t*)&olist_dma_descr,
                sizeof(olist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write olist DMA Descr @ {:x}",
                (uint64_t) olist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = olist_dma_descr_addr;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = status_mem_addr;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }

    // Wait for operation to be completed
    capri_barco_wait_for_resp(req_tag, async_args);

    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        /* Copy out the results */
        if (sdk::asic::asic_mem_read(olist_mem_addr, (uint8_t*)s, key_size)) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to read output s from memory @ {:x}",
                    (uint64_t) olist_mem_addr);
            ret = HAL_RET_INVALID_ARG;
            goto cleanup;
        }
        CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"s", (char *)s, key_size);
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Sig Gen succeeded");
    }

cleanup:

    if (status_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, status_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for status content:{:x}",
                    status_mem_addr);
        }
    }

    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }

    if (olist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, olist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist DMA Descr: {:x}",
                    olist_dma_descr_addr);
        }
    }

    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

hal_ret_t capri_barco_asym_rsa2k_sig_verify(uint8_t *n, uint8_t *e,
        uint8_t *h, uint8_t *s)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint64_t                    ilist_dma_descr_addr = 0;
    uint64_t                    key_dma_descr_addr = 0;
    uint64_t                    ilist_mem_addr = 0, olist_mem_addr = 0, curr_ptr = 0;
    uint64_t                    key_param_addr = 0;
    barco_asym_descriptor_t     asym_req_descr;
    barco_asym_dma_descriptor_t ilist_dma_descr;
    barco_asym_dma_descriptor_t key_dma_descr;
    int32_t                     ecc_p256_key_idx = -1;
    crypto_asym_key_t           asym_key;
    uint32_t                    req_tag = 0;
    uint32_t                    status = 0;
    pd_func_args_t pd_func_args = {0};

#undef CAPRI_BARCO_API_NAME
#define CAPRI_BARCO_API_NAME "RSA 2K Sig Verify: "

    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"n", (char *)n, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"e", (char *)e, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"h", (char *)h, 256);
    CAPRI_BARCO_API_PARAM_HEXDUMP((char *)"s", (char *)s, 256);

    /* Setup params in the key memory */
    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &key_param_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key param");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key param @ {:x}", key_param_addr);

    curr_ptr = key_param_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)n, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param n into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)e, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param e into key memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &key_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for key DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for key DMA Descr @ {:x}", key_dma_descr_addr);

    /* Setup key DMA descriptor */
    key_dma_descr.address = key_param_addr;
    key_dma_descr.stop = 1;
    key_dma_descr.rsvd0 = 1;
    key_dma_descr.next = 0;
    key_dma_descr.int_en = 0;
    key_dma_descr.discard = 0;
    key_dma_descr.realign = 1;
    key_dma_descr.cst_addr = 0;
    key_dma_descr.length = (curr_ptr - key_param_addr);
    if (sdk::asic::asic_mem_write(key_dma_descr_addr, (uint8_t*)&key_dma_descr,
                sizeof(key_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key DMA Descr @ {:x}",
                (uint64_t) key_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    pd_crypto_asym_alloc_key_args_t args;
    args.key_idx = &ecc_p256_key_idx;
    pd_func_args.pd_crypto_asym_alloc_key = &args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_ALLOC_KEY, &pd_func_args);
    // ret = pd_crypto_asym_alloc_key(&ecc_p256_key_idx);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate key descriptor");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated Key Descr @ {:x}", ecc_p256_key_idx);

    asym_key.key_param_list = key_dma_descr_addr;
    asym_key.command_reg = (CAPRI_BARCO_ASYM_CMD_SWAP_BYTES |
                            CAPRI_BARCO_ASYM_CMD_SIZE_OF_OP(256) |
                            CAPRI_BARCO_ASYM_CMD_RSA_SIG_VERIFY);

    pd_crypto_asym_write_key_args_t w_args;
    w_args.key_idx = ecc_p256_key_idx;
    w_args.key = &asym_key;
    pd_func_args.pd_crypto_asym_write_key = &w_args;
    ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_WRITE_KEY, &pd_func_args);
    // ret = pd_crypto_asym_write_key(ecc_p256_key_idx, &asym_key);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write key: {}", ecc_p256_key_idx);
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Setup key @ {:x}", ecc_p256_key_idx);


    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_ASYM_DMA_DESCR,
            NULL, &ilist_dma_descr_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist DMA Descr");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for ilist DMA Descr @ {:x}", ilist_dma_descr_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &ilist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for ilist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for input mem @ {:x}", ilist_mem_addr);

    ret = capri_barco_res_alloc(CRYPTO_BARCO_RES_HBM_MEM_512B,
            NULL, &olist_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to allocate memory for olist content");
        goto cleanup;
    }
    HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "Allocated memory for output mem @ {:x}", olist_mem_addr);

    /* Copy the input to the ilist memory */
    curr_ptr = ilist_mem_addr;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)s, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param s into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    if (sdk::asic::asic_mem_write(curr_ptr, (uint8_t*)h, 256)) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write RSA param h into ilist memory @ {:x}", (uint64_t) curr_ptr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }
    curr_ptr += 256;

    /* Setup ilist DMA descriptor */
    ilist_dma_descr.address = ilist_mem_addr;
    ilist_dma_descr.stop = 1;
    ilist_dma_descr.rsvd0 = 1;
    ilist_dma_descr.next = 0;
    ilist_dma_descr.int_en = 0;
    ilist_dma_descr.discard = 0;
    ilist_dma_descr.realign = 1;
    ilist_dma_descr.cst_addr = 0;
    ilist_dma_descr.length = (curr_ptr - ilist_mem_addr);
    if (sdk::asic::asic_mem_write(ilist_dma_descr_addr, (uint8_t*)&ilist_dma_descr,
                sizeof(ilist_dma_descr))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to write ilist DMA Descr @ {:x}",
                (uint64_t) ilist_dma_descr_addr);
        ret = HAL_RET_INVALID_ARG;
        goto cleanup;
    }

    /* Setup Asymmetric Request Descriptor */
    asym_req_descr.input_list_addr = ilist_dma_descr_addr;
    asym_req_descr.output_list_addr = 0;
    asym_req_descr.key_descr_idx = ecc_p256_key_idx;
    asym_req_descr.status_addr = olist_mem_addr + 256;
    asym_req_descr.opaque_tag_value = 0;
    asym_req_descr.opage_tag_wr_en = 0;
    asym_req_descr.flag_a = 0;
    asym_req_descr.flag_b = 0;

    ret = capri_barco_ring_queue_request(types::BARCO_RING_ASYM, (void *)&asym_req_descr, &req_tag, true);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to enqueue request");
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    /* Poll for completion */
    while (capri_barco_ring_poll(types::BARCO_RING_ASYM, req_tag) != TRUE) {
        //HAL_TRACE_DEBUG("ECC Point Mul P256: Waiting for Barco completion...");
    }
    if (sdk::asic::asic_mem_read(asym_req_descr.status_addr, (uint8_t*)&status, sizeof(status))) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to retrieve operation status @ {:x}",
                (uint64_t) asym_req_descr.status_addr);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    if (status != 0) {
        HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Operation failed with status {:x}",
                status);
        ret = HAL_RET_ERR;
        goto cleanup;
    }
    else {
        HAL_TRACE_DEBUG(CAPRI_BARCO_API_NAME "RSA Sig Verify succeeded");
    }

cleanup:


    if (olist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, olist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for olist content:{:x}",
                    olist_mem_addr);
        }
    }

    if (ilist_mem_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, ilist_mem_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("ECC Point Mul P256: Failed to free memory for ilist content:{:x}",
                    ilist_mem_addr);
        }
    }


    if (ilist_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, ilist_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for ilist DMA Descr: {:x}",
                    ilist_dma_descr_addr);
        }
    }

    if (ecc_p256_key_idx != -1) {
        pd_crypto_asym_free_key_args_t f_args;
        f_args.key_idx = ecc_p256_key_idx;
        pd_func_args.pd_crypto_asym_free_key = &f_args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_CRYPTO_ASYM_FREE_KEY, &pd_func_args);
        // ret = pd_crypto_asym_free_key(ecc_p256_key_idx);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME"Failed to free key descriptor");
        }
    }

    if (key_dma_descr_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_ASYM_DMA_DESCR, key_dma_descr_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key DMA Descr: {:x}",
                    key_dma_descr_addr);
        }
    }

    if (key_param_addr) {
        ret = capri_barco_res_free(CRYPTO_BARCO_RES_HBM_MEM_512B, key_param_addr);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR(CAPRI_BARCO_API_NAME "Failed to free memory for key param :{:x}",
                    key_param_addr);
        }
    }

    if (status)
        ret = HAL_RET_ERR;

    return ret;
}

}    // namespace pd
}    // namespace hal
