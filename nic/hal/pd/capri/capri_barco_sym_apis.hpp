#ifndef __CAPRI_BARCO_SYM_APIS_H__
#define __CAPRI_BARCO_SYM_APIS_H__

#include "nic/hal/pd/capri/capri_barco_crypto.hpp"
#include "gen/proto/crypto_apis.pb.h"

namespace hal {
namespace pd {

typedef enum {
    CAPRI_SYMM_ENCTYPE_NONE = 0,
    CAPRI_SYMM_ENCTYPE_AES_GCM = 1,
    CAPRI_SYMM_ENCTYPE_AES_CCM = 2,
    CAPRI_SYMM_ENCTYPE_AES_SHA256_CBC = 3,
    CAPRI_SYMM_ENCTYPE_AES_SHA384_CBC = 4,
    CAPRI_SYMM_ENCTYPE_AES_CBC_SHA256 = 5,
    CAPRI_SYMM_ENCTYPE_AES_CBC_SHA384 = 6,
    CAPRI_SYMM_ENCTYPE_AES_HMAC_SHA256_CBC = 7,
    CAPRI_SYMM_ENCTYPE_AES_HMAC_SHA384_CBC = 8,
    CAPRI_SYMM_ENCTYPE_AES_CBC_HMAC_SHA256 = 9,
    CAPRI_SYMM_ENCTYPE_AES_CBC_HMAC_SHA384 = 10
} capri_barco_symm_enctype_e;

#define TLS_AES_CCM_AUTH_TAG_SIZE             16
#define TLS_AES_CCM_LENGTH_FIELD_SIZE          3
#define TLS_AES_CCM_AAD_PRESENT                1
#define TLS_AES_CCM_NONCE_SIZE                12
#define TLS_AES_CCM_NONCE_SALT_SIZE            4
#define TLS_AES_CCM_NONCE_EXPLICIT_SIZE        8
#define TLS_AES_CCM_AAD_SIZE                  13
#define TLS_AES_CCM_HEADER_BLOCK_SIZE         16
#define TLS_AES_CCM_HEADER_SIZE               (TLS_AES_CCM_HEADER_BLOCK_SIZE * 2)
#define TLS_AES_CCM_HEADER_AAD_OFFSET         18 // 3rd byte of B_1

#define TLS_AES_CCM_HDR_B0_FLAGS  \
    (TLS_AES_CCM_AAD_PRESENT << 6 | \
    (((TLS_AES_CCM_AUTH_TAG_SIZE - 2)/2) << 3) | \
    (TLS_AES_CCM_LENGTH_FIELD_SIZE -1))

static inline const char *
capri_barco_symm_enctype_name (capri_barco_symm_enctype_e encaptype)
{
  switch(encaptype) {
  case CAPRI_SYMM_ENCTYPE_NONE:
    return("None");
  case  CAPRI_SYMM_ENCTYPE_AES_GCM:
    return("AES-GCM");
  case CAPRI_SYMM_ENCTYPE_AES_CCM:
    return("AES-CCM");
  case CAPRI_SYMM_ENCTYPE_AES_SHA256_CBC:
    return("AES-HASH-SHA256-CBC");
  case CAPRI_SYMM_ENCTYPE_AES_SHA384_CBC:
    return("AES-HASH-SHA384-CBC");
  case CAPRI_SYMM_ENCTYPE_AES_CBC_SHA256:
    return("AES-HASH-CBC-SHA256");
  case CAPRI_SYMM_ENCTYPE_AES_CBC_SHA384:
    return("AES-HASH-CBC-SHA384");
  case CAPRI_SYMM_ENCTYPE_AES_HMAC_SHA256_CBC:
    return("AES-HASH-HMAC-SHA256-CBC");
  case CAPRI_SYMM_ENCTYPE_AES_HMAC_SHA384_CBC:
    return("AES-HASH-HMAC-SHA384-CBC");
  case CAPRI_SYMM_ENCTYPE_AES_CBC_HMAC_SHA256:
    return("AES-HASH-CBC-HMAC-SHA256");
  case CAPRI_SYMM_ENCTYPE_AES_CBC_HMAC_SHA384:
    return("AES-HASH-CBC-HMAC-SHA384");
  default:
    return("Unknown");
  }
  return("Unknown");
}

static inline const char *
barco_symm_err_status_str (uint64_t status)
{
  uint32_t err = (status & (CAPRI_BARCO_SYM_ERR_BUS_ERR |
			    CAPRI_BARCO_SYM_ERR_GEN_PUSH_ERR |
			    CAPRI_BARCO_SYM_ERR_GEN_FETCH_ERR |
			    CAPRI_BARCO_SYM_ERR_BUS_UNSUP_MODE |
			    CAPRI_BARCO_SYM_ERR_BUS_RSVD |
			    CAPRI_BARCO_SYM_ERR_BUS_BAD_CMD |
			    CAPRI_BARCO_SYM_ERR_BUS_UNK_STATE |
			    CAPRI_BARCO_SYM_ERR_BUS_AXI_BUS_RESP |
			    CAPRI_BARCO_SYM_ERR_BUS_WRONG_KEYTYPE |
			    CAPRI_BARCO_SYM_ERR_BUS_KEYTYPE_RANGE));


  switch (err) {
  case  CAPRI_BARCO_SYM_ERR_BUS_ERR:
    return("Bus error when writing status");
  case  CAPRI_BARCO_SYM_ERR_GEN_PUSH_ERR:
    return("Generic Pusher error");
  case  CAPRI_BARCO_SYM_ERR_GEN_FETCH_ERR:
    return("Generic Fetcher error");
  case  CAPRI_BARCO_SYM_ERR_BUS_UNSUP_MODE:
    return("Unsupported mode in this Hw");
  case  CAPRI_BARCO_SYM_ERR_BUS_RSVD:
    return("Reserved");
  case  CAPRI_BARCO_SYM_ERR_BUS_BAD_CMD:
    return("Bad command");
  case  CAPRI_BARCO_SYM_ERR_BUS_UNK_STATE:
    return("Unknown state reached");
  case  CAPRI_BARCO_SYM_ERR_BUS_AXI_BUS_RESP:
    return("AXI bus resp not ok");
  case  CAPRI_BARCO_SYM_ERR_BUS_WRONG_KEYTYPE:
    return("Wrong key type");
  case  CAPRI_BARCO_SYM_ERR_BUS_KEYTYPE_RANGE:
    return("Index key out of range");
  default:
    return(status == 0 ? "success" : "Unknown error");
  }
}

hal_ret_t capri_barco_sym_hash_process_request(cryptoapis::CryptoApiHashType hash_type, bool generate,
					       unsigned char *key, int key_len,
					       unsigned char *data, int data_len,
					       uint8_t *output_digest, int digest_len);

hal_ret_t capri_barco_sym_aes_encrypt_process_request(capri_barco_symm_enctype_e enc_type, bool encrypt,
						      uint8_t *key, int key_len,
						      uint8_t *header, int header_len,
						      uint8_t *plaintext, int plaintext_len,
						      uint8_t *iv, int iv_len,
						      uint8_t *ciphertext, int ciphertext_len,
						      uint8_t *auth_tag, int auth_tag_len, bool schedule_barco);

void capri_barco_init_drbg(void);

}    // namespace pd
}    // namespace hal

#endif /* __CAPRI_BARCO_SYM_APIS_H__ */
