#include <gtest/gtest.h>
#include <vector>
#include <unistd.h>

#include "storage/offload/src/osal/osal_sys.h"
#include "storage/offload/src/sim/sim.h"
#include "storage/offload/include/pnso_wafl.h"

using namespace std;

class pnso_sim_test : public ::testing::Test {
public:

protected:

    pnso_sim_test() {
    }

    virtual ~pnso_sim_test() {
    }

    // will be called immediately after the constructor before each test
    virtual void SetUp() {
    }

    // will be called immediately after each test before the destructor
    virtual void TearDown() {
    }
};

/* single block request */
static uint8_t g_data1[1000];

/* multi-block request */
static uint8_t g_data2[8000];

static const char g_fill_src[] = "<html><body>The quick brown fox jumps over the lazy dog</body></html>\n";

static inline void fill_data(uint8_t* data, uint32_t data_len)
{
    uint32_t copy_len;

    while (data_len > 0) {
        copy_len = data_len >= sizeof(g_fill_src) ? sizeof(g_fill_src) : data_len;
        memcpy(data, g_fill_src, copy_len);
        data += copy_len;
        data_len -= copy_len;
    }    
}

#define TEST_CP_HDR_ALGO_VER 1234

TEST_F(pnso_sim_test, sync_request) {
    int rc;
    size_t alloc_sz;
    struct pnso_init_params init_params;
    struct pnso_service_request *svc_req;
    struct pnso_service_result *svc_res;
    struct pnso_buffer_list *src_buflist, *dst_buflist;
    uint32_t buflist_sz;
    struct pnso_flat_buffer *buf;
    struct pnso_hash_tag hash_tags[16];
    uint8_t output[16 * 4096];
    char xts_iv[16] = "";
    struct pnso_compression_header_format cp_hdr_fmt = { 3, {
        {PNSO_HDR_FIELD_TYPE_INDATA_CHKSUM, 0, 4, 0},
        {PNSO_HDR_FIELD_TYPE_OUTDATA_LENGTH, 4, 2, 0},
        {PNSO_HDR_FIELD_TYPE_ALGO, 6, 2, 0}
    } };

    memset(&init_params, 0, sizeof(init_params));
    memset(hash_tags, 0, sizeof(hash_tags));

    /* Initialize input */
    fill_data(g_data1, sizeof(g_data1));
    fill_data(g_data2, sizeof(g_data2));

    /* Initialize session */
    init_params.per_core_qdepth = 16;
    init_params.block_size = 4096;
    rc = pnso_init(&init_params);
    EXPECT_EQ(rc, 0);
    rc = pnso_sim_thread_init(osal_get_coreid());
    EXPECT_EQ(rc, 0);

    /* Initialize key store */
    char *tmp_key = nullptr;
    uint32_t tmp_key_size = 0;
    char key1[32] = "abcdefghijklmnopqrstuvwxyz78901";
    pnso_set_key_desc_idx(key1, key1, sizeof(key1), 1);
    sim_get_key_desc_idx((void**)&tmp_key, (void**)&tmp_key, &tmp_key_size, 1);
    EXPECT_EQ(tmp_key_size, sizeof(key1));
    EXPECT_EQ(0, memcmp(tmp_key, "abcd", 4));

    /* Initialize compression header format */
    rc = pnso_register_compression_header_format(&cp_hdr_fmt, 1);
    EXPECT_EQ(rc, 0);
    rc = pnso_add_compression_algo_mapping(PNSO_COMPRESSION_TYPE_LZRW1A, TEST_CP_HDR_ALGO_VER);
    EXPECT_EQ(rc, 0);

    /* Allocate request and response */
    alloc_sz = sizeof(struct pnso_service_request) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service);
    svc_req = (struct pnso_service_request*) malloc(alloc_sz);
    EXPECT_NE(svc_req, nullptr);
    memset(svc_req, 0, alloc_sz);

    alloc_sz = sizeof(struct pnso_service_result) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service_status);
    svc_res = (struct pnso_service_result*) malloc(alloc_sz);
    EXPECT_NE(svc_res, nullptr);
    memset(svc_res, 0, alloc_sz);

    /* Allocate buffer descriptors */
    buflist_sz = sizeof(struct pnso_buffer_list) + 16*sizeof(struct pnso_flat_buffer);
    src_buflist = (struct pnso_buffer_list*) malloc(buflist_sz);
    EXPECT_NE(src_buflist, nullptr);
    dst_buflist = (struct pnso_buffer_list*) malloc(buflist_sz);
    EXPECT_NE(dst_buflist, nullptr);

    /* -------------- Test 1: Compression + Hash, single block -------------- */

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data1;
    svc_req->sgl->buffers[0].len = sizeof(g_data1);

    /* Setup 2 services */
    svc_req->num_services = 2;
    svc_res->num_services = 2;

    /* Setup compression service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_COMPRESS;
    svc_req->svc[0].u.cp_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
    svc_req->svc[0].u.cp_desc.flags = PNSO_CP_DFLAG_ZERO_PAD | PNSO_CP_DFLAG_INSERT_HEADER;
    svc_req->svc[0].u.cp_desc.threshold_len = sizeof(g_data1) - 8;
    svc_req->svc[0].u.cp_desc.hdr_fmt_idx = 1;
    svc_req->svc[0].u.cp_desc.hdr_algo = TEST_CP_HDR_ALGO_VER;

    svc_res->svc[0].u.dst.sgl = dst_buflist;
    svc_res->svc[0].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[0].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[0].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Setup hash service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
    svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
    svc_res->svc[1].u.hash.num_tags = 16;
    svc_res->svc[1].u.hash.tags = hash_tags;

    /* Execute synchronously */
    rc = pnso_submit_request(svc_req, svc_res, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(svc_res->err, 0);

    /* Check compression+pad status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_COMPRESS);
    EXPECT_GT(svc_res->svc[0].u.dst.data_len, 8);
    EXPECT_LT(svc_res->svc[0].u.dst.data_len, 4000);
    EXPECT_EQ(svc_res->svc[0].u.dst.sgl->count, 16);
    buf = &svc_res->svc[0].u.dst.sgl->buffers[0];
    EXPECT_EQ(buf->len, 4096);
    EXPECT_EQ(((uint8_t*)buf->buf)[4095], 0);

    /* Check hash status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_HASH);
    EXPECT_EQ(svc_res->svc[1].u.hash.num_tags, 1);
    EXPECT_NE(svc_res->svc[1].u.hash.tags->hash[0], 0);


    /* -------------- Test 2: Encryption + Hash, single block -------------- */

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data1;
    svc_req->sgl->buffers[0].len = sizeof(g_data1);

    /* Setup 2 services */
    svc_req->num_services = 2;
    svc_res->num_services = 2;

    /* Setup encryption service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_ENCRYPT;
    svc_req->svc[0].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[0].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[0].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[0].u.dst.sgl = dst_buflist;
    svc_res->svc[0].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[0].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[0].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Setup hash service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
    svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
    svc_res->svc[1].u.hash.num_tags = 16;
    svc_res->svc[1].u.hash.tags = hash_tags;

    /* Execute synchronously */
    rc = pnso_submit_request(svc_req, svc_res, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(svc_res->err, 0);

    /* Check encryption status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_ENCRYPT);
    EXPECT_EQ(svc_res->svc[0].u.dst.data_len, sizeof(g_data1));
    EXPECT_EQ(svc_res->svc[0].u.dst.sgl->count, 16);

    /* Check hash status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_HASH);
    EXPECT_EQ(svc_res->svc[1].u.hash.num_tags, 1);
    EXPECT_NE(svc_res->svc[1].u.hash.tags->hash[0], 0);

    /* -------------- Test 3: Compression + Encryption, single block -------------- */

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data1;
    svc_req->sgl->buffers[0].len = sizeof(g_data1);

   /* Setup 2 services */
    svc_req->num_services = 2;
    svc_res->num_services = 2;

    /* Setup compression service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_COMPRESS;
    svc_req->svc[0].u.cp_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
    svc_req->svc[0].u.cp_desc.flags = PNSO_CP_DFLAG_ZERO_PAD | PNSO_CP_DFLAG_INSERT_HEADER;
    svc_req->svc[0].u.cp_desc.threshold_len = sizeof(g_data1) - 8;
    svc_req->svc[0].u.cp_desc.hdr_fmt_idx = 1;
    svc_req->svc[0].u.cp_desc.hdr_algo = TEST_CP_HDR_ALGO_VER;

    svc_res->svc[0].u.dst.sgl = dst_buflist;
    svc_res->svc[0].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[0].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[0].u.dst.sgl->buffers[i].len = 4096;
    }
 
    /* Setup encryption service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_ENCRYPT;
    svc_req->svc[1].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[1].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[1].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[1].u.dst.sgl = NULL;

    /* Execute synchronously */
    rc = pnso_submit_request(svc_req, svc_res, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(svc_res->err, 0);

    /* Check compression+pad status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_COMPRESS);
    EXPECT_GT(svc_res->svc[0].u.dst.data_len, 8);
    EXPECT_LT(svc_res->svc[0].u.dst.data_len, 4000);
    buf = &svc_res->svc[0].u.dst.sgl->buffers[0];
    EXPECT_EQ(buf->len, 4096);

    /* Check encryption status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_ENCRYPT);
    EXPECT_EQ(svc_res->svc[1].u.dst.data_len, 4096);

    /* -------------- Test 4: Compression + Hash + Encryption, multiple blocks -------------- */

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 2;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data2;
    svc_req->sgl->buffers[0].len = 4096;
    svc_req->sgl->buffers[1].buf = (uint64_t) g_data2 + 4096;
    svc_req->sgl->buffers[1].len = sizeof(g_data2) - 4096;

    /* Setup 3 services */
    svc_req->num_services = 3;
    svc_res->num_services = 3;

    /* Setup compression service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_COMPRESS;
    svc_req->svc[0].u.cp_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
    svc_req->svc[0].u.cp_desc.flags = PNSO_CP_DFLAG_ZERO_PAD | PNSO_CP_DFLAG_INSERT_HEADER;
    svc_req->svc[0].u.cp_desc.threshold_len = sizeof(g_data2) - 8;
    svc_req->svc[0].u.cp_desc.hdr_fmt_idx = 1;
    svc_req->svc[0].u.cp_desc.hdr_algo = TEST_CP_HDR_ALGO_VER;

    svc_res->svc[0].u.dst.sgl = NULL;

    /* Setup hash service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
    svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
    svc_res->svc[1].u.hash.num_tags = 16;
    svc_res->svc[1].u.hash.tags = hash_tags;

    /* Setup encryption service */
    svc_req->svc[2].svc_type = PNSO_SVC_TYPE_ENCRYPT;
    svc_req->svc[2].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[2].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[2].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[2].u.dst.sgl = dst_buflist;
    svc_res->svc[2].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[2].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[2].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Execute synchronously */
    rc = pnso_submit_request(svc_req, svc_res, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(svc_res->err, 0);

    /* Check compression+pad status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_COMPRESS);
    EXPECT_GT(svc_res->svc[0].u.dst.data_len, 8);
    EXPECT_LT(svc_res->svc[0].u.dst.data_len, 4000);

    /* Check hash status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_HASH);
    EXPECT_LE(svc_res->svc[1].u.hash.num_tags, 2);
    EXPECT_NE(svc_res->svc[1].u.hash.tags->hash[0], 0);

    /* Check encryption status */
    EXPECT_EQ(svc_res->svc[2].err, 0);
    EXPECT_EQ(svc_res->svc[2].svc_type, PNSO_SVC_TYPE_ENCRYPT);
    EXPECT_EQ(svc_res->svc[2].u.dst.data_len, 4096);
    buf = &svc_res->svc[2].u.dst.sgl->buffers[0];
    EXPECT_EQ(buf->len, 4096);

    /* -------------- Test 5: Decryption + Hash + Decompression, multiple blocks -------------- */

    /* Use previous test data as input to new test */
    memcpy(g_data2, (uint8_t*) buf->buf, 4096);

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data2;
    svc_req->sgl->buffers[0].len = 4096;

    /* Setup 3 services */
    svc_req->num_services = 3;
    svc_res->num_services = 3;

    /* Setup decryption service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_DECRYPT;
    svc_req->svc[0].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[0].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[0].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[0].u.dst.sgl = NULL;

    /* Setup hash service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
    svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
    svc_res->svc[1].u.hash.num_tags = 16;
    svc_res->svc[1].u.hash.tags = hash_tags;

    /* Setup decompression service */
    svc_req->svc[2].svc_type = PNSO_SVC_TYPE_DECOMPRESS;
    svc_req->svc[2].u.dc_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
    svc_req->svc[2].u.dc_desc.flags = PNSO_DC_DFLAG_HEADER_PRESENT;
    svc_req->svc[2].u.dc_desc.hdr_fmt_idx = 1;

    svc_res->svc[2].u.dst.sgl = dst_buflist;
    svc_res->svc[2].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[2].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[2].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Execute synchronously */
    rc = pnso_submit_request(svc_req, svc_res, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(svc_res->err, 0);

    /* Check decryption status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_DECRYPT);
    EXPECT_EQ(svc_res->svc[0].u.dst.data_len, 4096);

    /* Check hash status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_HASH);
    EXPECT_LE(svc_res->svc[1].u.hash.num_tags, 2);
    EXPECT_NE(svc_res->svc[1].u.hash.tags->hash[0], 0);

    /* Check decompression status */
    EXPECT_EQ(svc_res->svc[2].err, 0);
    EXPECT_EQ(svc_res->svc[2].svc_type, PNSO_SVC_TYPE_DECOMPRESS);
    EXPECT_EQ(svc_res->svc[2].u.dst.data_len, sizeof(g_data2));
    EXPECT_EQ(svc_res->svc[2].u.dst.sgl->count, 16);
    EXPECT_EQ(svc_res->svc[2].u.dst.sgl->buffers[0].len, 4096);
    EXPECT_EQ(svc_res->svc[2].u.dst.sgl->buffers[1].len, 4096);
    EXPECT_EQ(memcmp((uint8_t*)svc_res->svc[2].u.dst.sgl->buffers[0].buf, g_fill_src, sizeof(g_fill_src)), 0);

    /* Restore original g_data2 for next test */
    fill_data(g_data2, sizeof(g_data2));


    /* Cleanup */
    pnso_sim_thread_finit(osal_get_coreid());
    pnso_sim_finit();
}

static void completion_cb(void* cb_ctx, struct pnso_service_result *svc_res)
{
    int *cb_count = (int*) cb_ctx;
    (*cb_count)++;
}

TEST_F(pnso_sim_test, async_request) {
    int rc;
    int sleep_count = 0;
    int cb_count = 0;
    size_t alloc_sz;
    struct pnso_init_params init_params;
    struct pnso_service_request *svc_req;
    struct pnso_service_result *svc_res;
    struct pnso_buffer_list *src_buflist, *dst_buflist;
    uint32_t buflist_sz;
    struct pnso_flat_buffer *buf;
    struct pnso_hash_tag hash_tags[16];
    uint8_t output[16 * 4096];
    char xts_iv[16] = "";
    struct pnso_compression_header_format cp_hdr_fmt = { 3, {
        {PNSO_HDR_FIELD_TYPE_INDATA_CHKSUM, 0, 4, 0},
        {PNSO_HDR_FIELD_TYPE_OUTDATA_LENGTH, 4, 2, 0},
        {PNSO_HDR_FIELD_TYPE_ALGO, 6, 2, 0}
    } };

    memset(&init_params, 0, sizeof(init_params));
    memset(hash_tags, 0, sizeof(hash_tags));

    /* Initialize input */
    fill_data(g_data1, sizeof(g_data1));
    fill_data(g_data2, sizeof(g_data2));

    /* Initialize session */
    init_params.per_core_qdepth = 16;
    init_params.block_size = 4096;
    rc = pnso_init(&init_params);
    EXPECT_EQ(rc, 0);
    rc = pnso_sim_thread_init(osal_get_coreid());
    EXPECT_EQ(rc, 0);

    /* Allocate request and response */
    alloc_sz = sizeof(struct pnso_service_request) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service);
    svc_req = (struct pnso_service_request*) malloc(alloc_sz);
    EXPECT_NE(svc_req, nullptr);
    memset(svc_req, 0, alloc_sz);

    alloc_sz = sizeof(struct pnso_service_result) + PNSO_SVC_TYPE_MAX*sizeof(struct pnso_service_status);
    svc_res = (struct pnso_service_result*) malloc(alloc_sz);
    EXPECT_NE(svc_res, nullptr);
    memset(svc_res, 0, alloc_sz);

    /* Allocate buffer descriptors */
    buflist_sz = sizeof(struct pnso_buffer_list) + 16*sizeof(struct pnso_flat_buffer);
    src_buflist = (struct pnso_buffer_list*) malloc(buflist_sz);
    EXPECT_NE(src_buflist, nullptr);
    dst_buflist = (struct pnso_buffer_list*) malloc(buflist_sz);
    EXPECT_NE(dst_buflist, nullptr);

    /* Initialize key store */
    char *tmp_key = nullptr;
    uint32_t tmp_key_size = 0;
    char key1[32] = "abcdefghijklmnopqrstuvwxyz78901";
    pnso_set_key_desc_idx(key1, key1, sizeof(key1), 1);
    sim_get_key_desc_idx((void**)&tmp_key, (void**)&tmp_key, &tmp_key_size, 1);
    EXPECT_EQ(tmp_key_size, sizeof(key1));
    EXPECT_EQ(0, memcmp(tmp_key, "abcd", 4));

    /* Initialize compression header format */
    rc = pnso_register_compression_header_format(&cp_hdr_fmt, 1);
    EXPECT_EQ(rc, 0);
    rc = pnso_add_compression_algo_mapping(PNSO_COMPRESSION_TYPE_LZRW1A, TEST_CP_HDR_ALGO_VER);
    EXPECT_EQ(rc, 0);


    /* -------------- Async Test 1: Compression + Hash, single block -------------- */

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data1;
    svc_req->sgl->buffers[0].len = sizeof(g_data1);

    /* Setup 2 services */
    svc_req->num_services = 2;
    svc_res->num_services = 2;

    /* Setup compression service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_COMPRESS;
    svc_req->svc[0].u.cp_desc.algo_type = PNSO_COMPRESSION_TYPE_LZRW1A;
    svc_req->svc[0].u.cp_desc.flags = PNSO_CP_DFLAG_ZERO_PAD | PNSO_CP_DFLAG_INSERT_HEADER;
    svc_req->svc[0].u.cp_desc.threshold_len = sizeof(g_data1) - 8;
    svc_req->svc[0].u.cp_desc.hdr_fmt_idx = 1;
    svc_req->svc[0].u.cp_desc.hdr_algo = TEST_CP_HDR_ALGO_VER;

    svc_res->svc[0].u.dst.sgl = dst_buflist;
    svc_res->svc[0].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[0].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[0].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Setup hash service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_HASH;
    svc_req->svc[1].u.hash_desc.algo_type = PNSO_HASH_TYPE_SHA2_512;
    svc_res->svc[1].u.hash.num_tags = 16;
    svc_res->svc[1].u.hash.tags = hash_tags;

    /* Submit async request */
    void* poll_ctx;
    pnso_poll_fn_t poller;
    cb_count = 0;
    rc = pnso_submit_request(svc_req, svc_res, completion_cb, &cb_count, &poller, &poll_ctx);
    EXPECT_EQ(rc, 0);

    /* TODO: add gtest for batching + async */

    /* Poll for completion */
    sleep_count = 0;
    while (EAGAIN == (rc = poller(poll_ctx))) {
        usleep(1);
        sleep_count++;
    }
    EXPECT_EQ(rc, 0);
    EXPECT_GE(sleep_count, 1);
    EXPECT_LT(sleep_count, 1000);

    /* Check that callback was called exactly once */
    EXPECT_EQ(cb_count, 1);

    /* Check compression+pad status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_COMPRESS);
    EXPECT_GT(svc_res->svc[0].u.dst.data_len, 8);
    EXPECT_LT(svc_res->svc[0].u.dst.data_len, 4000);
    EXPECT_EQ(svc_res->svc[0].u.dst.sgl->count, 16);
    buf = &svc_res->svc[0].u.dst.sgl->buffers[0];
    EXPECT_EQ(buf->len, 4096);
    EXPECT_EQ(((uint8_t*)buf->buf)[4095], 0);

    /* Check hash status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_HASH);
    EXPECT_EQ(svc_res->svc[1].u.hash.num_tags, 1);
    EXPECT_NE(svc_res->svc[1].u.hash.tags->hash[0], 0);


    /* -------- Async Test 2: Encryption (of compacted block) -------- */

    /* Initialize compacted block */
    wafl_packed_blk_t *wafl = (wafl_packed_blk_t*) g_data2;
    memset(wafl, 0, sizeof(*wafl));
    wafl->wpb_hdr.wpbh_magic = WAFL_WPBH_MAGIC;
    wafl->wpb_hdr.wpbh_version = WAFL_WPBH_VERSION;
    wafl->wpb_hdr.wpbh_num_objs = 4;
    uint16_t wafl_data_offset = 1024;
    for (uint16_t i = 0; i < wafl->wpb_hdr.wpbh_num_objs; i++) {
        wafl_packed_data_info_t *wafl_data = &wafl->wpb_data_info[i];
        memset(wafl_data, 0, sizeof(*wafl_data));
        wafl_data->wpd_vvbn = 0xabc0 | i;
        wafl_data->wpd_off = wafl_data_offset + (i * 512);
        wafl_data->wpd_len = 512;
    }

    /* Initialize request buffers */
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) wafl;
    svc_req->sgl->buffers[0].len = 4096;

    /* Setup 1 services */
    svc_req->num_services = 1;
    svc_res->num_services = 1;

    /* Setup encryption service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_ENCRYPT;
    svc_req->svc[0].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[0].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[0].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[0].u.dst.sgl = dst_buflist;
    svc_res->svc[0].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[0].u.dst.sgl->buffers[i].buf = (uint64_t) (output + (4096 * i));
        svc_res->svc[0].u.dst.sgl->buffers[i].len = 4096;
    }
 
    /* Submit async request */
    cb_count = 0;
    rc = pnso_submit_request(svc_req, svc_res, completion_cb, &cb_count, &poller, &poll_ctx);
    EXPECT_EQ(rc, 0);

    /* Poll for completion */
    sleep_count = 0;
    while (EAGAIN == (rc = poller(poll_ctx))) {
        usleep(1);
        sleep_count++;
    }
    EXPECT_EQ(rc, 0);
    EXPECT_GT(sleep_count, 0);
    EXPECT_LT(sleep_count, 1000);

    /* Check that callback was called exactly once */
    EXPECT_EQ(cb_count, 1);

    /* Check encryption status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_ENCRYPT);
    EXPECT_EQ(svc_res->svc[0].u.dst.data_len, 4096);
    EXPECT_EQ(svc_res->svc[0].u.dst.sgl->count, 16);
    EXPECT_EQ(svc_res->svc[0].u.dst.sgl->buffers[0].len, 4096);


    /* -------- Async Test 3: Decryption + decompaction -------- */

    /* Initialize request buffers, using previous output */
    memcpy(g_data2, output, 4096);
    memset(src_buflist, 0, buflist_sz);
    memset(dst_buflist, 0, buflist_sz);

    svc_req->sgl = src_buflist;
    svc_req->sgl->count = 1;
    svc_req->sgl->buffers[0].buf = (uint64_t) g_data2;
    svc_req->sgl->buffers[0].len = 4096;
 
    /* Setup 2 services */
    svc_req->num_services = 2;
    svc_res->num_services = 2;

    /* Setup decryption service */
    svc_req->svc[0].svc_type = PNSO_SVC_TYPE_DECRYPT;
    svc_req->svc[0].u.crypto_desc.algo_type = PNSO_CRYPTO_TYPE_XTS;
    svc_req->svc[0].u.crypto_desc.key_desc_idx = 1;
    svc_req->svc[0].u.crypto_desc.iv_addr = (uint64_t) xts_iv;

    svc_res->svc[0].u.dst.sgl = NULL;

    /* Setup decompaction service */
    svc_req->svc[1].svc_type = PNSO_SVC_TYPE_DECOMPACT;
    svc_req->svc[1].u.decompact_desc.vvbn = 0xabc2;
    /*svc_req->svc[1].u.decompact_desc.is_uncompressed = 1;*/

    svc_res->svc[1].u.dst.sgl = dst_buflist;
    svc_res->svc[1].u.dst.sgl->count = 16;
    for (int i = 0; i < 16; i++) {
        svc_res->svc[1].u.dst.sgl->buffers[i].buf =
		(uint64_t) (output + (4096 * i));
        svc_res->svc[1].u.dst.sgl->buffers[i].len = 4096;
    }

    /* Submit async request */
    cb_count = 0;
    rc = pnso_submit_request(svc_req, svc_res,
		    completion_cb, &cb_count, &poller, &poll_ctx);
    EXPECT_EQ(rc, 0);

    /* Poll for completion */
    sleep_count = 0;
    while (EAGAIN == (rc = poller(poll_ctx))) {
        usleep(1);
        sleep_count++;
    }
    EXPECT_EQ(rc, 0);
    EXPECT_GT(sleep_count, 0);
    EXPECT_LT(sleep_count, 1000);

    /* Check that callback was called exactly once */
    EXPECT_EQ(cb_count, 1);

    /* Check decryption status */
    EXPECT_EQ(svc_res->svc[0].err, 0);
    EXPECT_EQ(svc_res->svc[0].svc_type, PNSO_SVC_TYPE_DECRYPT);
    EXPECT_EQ(svc_res->svc[0].u.dst.data_len, 4096);

    /* Check decompaction status */
    EXPECT_EQ(svc_res->svc[1].err, 0);
    EXPECT_EQ(svc_res->svc[1].svc_type, PNSO_SVC_TYPE_DECOMPACT);
    EXPECT_EQ(svc_res->svc[1].u.dst.data_len, 512);
    EXPECT_EQ(svc_res->svc[1].u.dst.sgl->count, 16);
    EXPECT_EQ(svc_res->svc[1].u.dst.sgl->buffers[0].len, 4096);


    /* ---- Cleanup ----- */
    pnso_sim_thread_finit(osal_get_coreid());
    pnso_sim_finit();
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
