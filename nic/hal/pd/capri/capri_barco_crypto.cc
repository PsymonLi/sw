#include <unistd.h>
#include <string>
#include <sstream>
#include <ostream>
#include <iomanip>
/* Barco Crypto initialization support */
#include "nic/asic/capri/model/cap_top/cap_top_csr.h"
#include "nic/hal/pd/capri/capri_hbm.hpp"
#include "nic/hal/pd/capri/capri_barco_crypto.hpp"
#include "nic/hal/pd/capri/capri_barco_sym_apis.hpp"
#include "nic/include/hal_pd.hpp"

namespace hal {

namespace pd {

char key_desc_array[] = CAPRI_BARCO_KEY_DESC;

uint64_t    key_desc_array_base = 0;

// byte array to hex string for logging
std::string barco_hex_dump(const uint8_t *buf, size_t sz)
{
    std::ostringstream result;

    for(size_t i = 0; i < sz; i+=8) {
        result << " 0x";
        for (size_t j = i ; j < sz && j < i+8; j++) {
            result << std::setw(2) << std::setfill('0') << std::hex << (int)buf[j];
        }
    }

    return result.str();
}

hal_ret_t capri_barco_crypto_init(void)
{
    hal_ret_t                           ret = HAL_RET_OK;
    hal_cfg_t *hal_cfg =
        (hal::hal_cfg_t *)hal::hal_get_current_thread()->data();
    HAL_ASSERT(hal_cfg);
    cap_top_csr_t &                     cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_hens_csr_t &                    hens = cap0.md.hens;

    /* Barco reset */
    hens.cfg_he_ctl.sw_rst(0xff);
    hens.cfg_he_ctl.write();
    hens.cfg_he_ctl.sw_rst(0);
    hens.cfg_he_ctl.write();

    key_desc_array_base = get_start_offset(key_desc_array);
    if (key_desc_array_base == INVALID_MEM_ADDRESS) {
        /* For non IRIS scenarios, the region may not be defined
         * in that case bail out silently
         */
        return ret;
    }
    /* All regions in hbm_mem.json are in multiples of 1kb and hence should already be aligned to 16byte
     * but confirm
     */
    assert((key_desc_array_base & (BARCO_CRYPTO_KEY_DESC_ALIGN_BYTES - 1)) == 0);

    ret = capri_barco_res_allocator_init();
    if (ret != HAL_RET_OK) {
        return ret;
    }

    ret = capri_barco_rings_init();
    if (ret != HAL_RET_OK) {
        return ret;
    }

    /*
     * Initialize the padding pattern table for use with TLS-proxy.
     */
    ret = capri_barco_crypto_init_tls_pad_table();
    if (ret != HAL_RET_OK) {
        return ret;
    }

#if 0
    hal_ret_t capri_barco_asym_run_tests(void);
    ret = capri_barco_asym_run_tests();
#endif

#if 0
    hal_ret_t capri_barco_sym_hash_run_tests(void);
    ret = capri_barco_sym_hash_run_tests();
#endif

#if 0
    hal_ret_t capri_barco_sym_run_aes_gcm_1K_test(void);
    ret = capri_barco_sym_run_aes_gcm_1K_test();
#endif

    /*
     * Initialize the barco DRBG random number generator.
     */
    if (hal_cfg->platform != hal::HAL_PLATFORM_HAPS) {
        /* HAPS does not contain the DRBG block */
        capri_barco_init_drbg();
    }

    return ret;
}

hal_ret_t capri_barco_init_key(uint32_t key_idx, uint64_t key_addr)
{
    capri_barco_key_desc_t  key_desc;
    uint64_t                key_desc_addr;

    key_desc_addr = key_desc_array_base + (key_idx * BARCO_CRYPTO_KEY_DESC_SZ);
    memset(&key_desc, 0, sizeof(capri_barco_key_desc_t));
    key_desc.key_address = key_addr;
    if (capri_hbm_write_mem(key_desc_addr, (uint8_t*)&key_desc, sizeof(key_desc))) {
        HAL_TRACE_ERR("Failed to write Barco descriptor @ {:x}", (uint64_t) key_desc_addr); 
        return HAL_RET_INVALID_ARG;
    }
    return HAL_RET_OK;
}


hal_ret_t capri_barco_setup_key(uint32_t key_idx, types::CryptoKeyType key_type, uint8_t *key,
        uint32_t key_size)
{
    capri_barco_key_desc_t  key_desc;
    uint64_t                key_desc_addr = 0;
    uint64_t                key_addr = 0;
    uint32_t                cbkey_type;

    key_desc_addr = key_desc_array_base + (key_idx * BARCO_CRYPTO_KEY_DESC_SZ);
    HAL_TRACE_DEBUG("capri_barco_setup_key: key_desc_addr={} key_idx={}", key_desc_addr, key_idx);
    if (capri_hbm_read_mem(key_desc_addr, (uint8_t*)&key_desc, sizeof(key_desc))) {
        HAL_TRACE_ERR("Failed to read Barco descriptor @ {:x}", (uint64_t) key_desc_addr); 
        return HAL_RET_INVALID_ARG;
    }

    switch (key_type) {
        case types::CRYPTO_KEY_TYPE_AES128:
            cbkey_type = CAPRI_BARCO_KEYTYPE_AES128;
            break;
        case types::CRYPTO_KEY_TYPE_AES192:
            cbkey_type = CAPRI_BARCO_KEYTYPE_AES192;
            break;
        case types::CRYPTO_KEY_TYPE_AES256:
            cbkey_type = CAPRI_BARCO_KEYTYPE_AES256;
            break;
        case types::CRYPTO_KEY_TYPE_DES:
            cbkey_type = CAPRI_BARCO_KEYTYPE_DES;
            break;
        case types::CRYPTO_KEY_TYPE_CHACHA20:
            cbkey_type = CAPRI_BARCO_KEYTYPE_CHACHA20;
            break;
        case types::CRYPTO_KEY_TYPE_POLY1305:
            cbkey_type = CAPRI_BARCO_KEYTYPE_POY1305;
            break;
        case types::CRYPTO_KEY_TYPE_HMAC:
            cbkey_type = CAPRI_BARCO_KEYTYPE_HMAC(key_size);
            break;
        default:
            return HAL_RET_INVALID_ARG;
    }

    key_desc.key_type = cbkey_type;
    key_addr = key_desc.key_address;
    HAL_TRACE_DEBUG("capri_barco_setup_key key_addr={:x}", (uint64_t)key_addr);
    /* Write back key descriptor */
    if (capri_hbm_write_mem(key_desc_addr, (uint8_t*)&key_desc, sizeof(key_desc))) {
        HAL_TRACE_ERR("Failed to write Barco descriptor @ {:x}", (uint64_t) key_desc_addr); 
        return HAL_RET_INVALID_ARG;
    }
    HAL_TRACE_DEBUG("capri_barco_setup_key key={}", barco_hex_dump(key,key_size));
    /* Write key memory */
    if (capri_hbm_write_mem(key_addr, key, key_size)) {
        HAL_TRACE_ERR("Failed to write key @ {:x}", (uint64_t) key_addr); 
        return HAL_RET_INVALID_ARG;
    }
    return HAL_RET_OK;
}

hal_ret_t capri_barco_read_key(uint32_t key_idx, types::CryptoKeyType *key_type,
        uint8_t *key, uint32_t *key_size)
{
    capri_barco_key_desc_t  key_desc;
    uint64_t                key_desc_addr = 0;
    uint64_t                key_addr = 0;
    uint32_t                cbkey_type;

    key_desc_addr = key_desc_array_base + (key_idx * BARCO_CRYPTO_KEY_DESC_SZ);
    if (capri_hbm_read_mem(key_desc_addr, (uint8_t*)&key_desc, sizeof(key_desc))) {
        HAL_TRACE_ERR("Failed to read Barco descriptor @ {:x}", (uint64_t) key_desc_addr); 
        return HAL_RET_INVALID_ARG;
    }

    cbkey_type = ntohl(key_desc.key_type);
    switch (cbkey_type) {
        case CAPRI_BARCO_KEYTYPE_AES128:
            *key_type = types::CRYPTO_KEY_TYPE_AES128;
            *key_size = 16;
            break;
        case CAPRI_BARCO_KEYTYPE_AES192:
            *key_type = types::CRYPTO_KEY_TYPE_AES192;
            *key_size = 24;
            break;
        case CAPRI_BARCO_KEYTYPE_AES256:
            *key_type = types::CRYPTO_KEY_TYPE_AES256;
            *key_size = 32;
            break;
        case CAPRI_BARCO_KEYTYPE_DES:
            *key_type = types::CRYPTO_KEY_TYPE_DES;
            *key_size = 16; /* FIXME */
            break;
        case CAPRI_BARCO_KEYTYPE_CHACHA20:
            *key_type = types::CRYPTO_KEY_TYPE_CHACHA20;
            *key_size = 16; /* FIXME */
            break;
        case CAPRI_BARCO_KEYTYPE_POY1305:
            *key_type = types::CRYPTO_KEY_TYPE_POLY1305;
            *key_size = 16; /* FIXME */
            break;
        default:
            if (cbkey_type & CAPRI_BARCO_KEYTYPE_HMAC_TYPE) {
                *key_type = types::CRYPTO_KEY_TYPE_HMAC;
                *key_size = (cbkey_type & CAPRI_BARCO_KEYTYPE_HMAC_LEN_MASK);
            } 
            else {
                return HAL_RET_INVALID_ARG;
            }
            break;
    }

    key_addr = ntohll(key_desc.key_address);
    if (capri_hbm_read_mem(key_addr, key, *key_size)) {
        HAL_TRACE_ERR("Failed to read key @ {:x}", (uint64_t) key_addr); 
        return HAL_RET_INVALID_ARG;
    }

    return HAL_RET_OK;
}

hal_ret_t
capri_barco_crypto_init_tls_pad_table(void)
{
    uint8_t  tls_pad_bytes[get_size_kb(CAPRI_HBM_REG_TLS_PROXY_PAD_TABLE) * 1024], i, j;
    uint64_t tls_pad_base_addr = 0;

    HAL_TRACE_DEBUG("Initializing TLS-proxy Pad Bytes table of size {:x}", sizeof(tls_pad_bytes));
    bzero(tls_pad_bytes, sizeof(tls_pad_bytes));

    /*
     * Block-size 16.
     * Pad bytes pattern:
     *   - 16 contiguous blocks with 1st block all 0s, 2nd block all 1s etc.
     */
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            tls_pad_bytes[16*(i) + j] = i;
        }
    }

    tls_pad_base_addr = get_start_offset(CAPRI_HBM_REG_TLS_PROXY_PAD_TABLE);
    if (tls_pad_base_addr) {
        capri_hbm_write_mem(tls_pad_base_addr, tls_pad_bytes, sizeof(tls_pad_bytes));
    }
    return HAL_RET_OK;
}

} // namespace pd

} // namespace hal
