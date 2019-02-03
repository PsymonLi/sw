/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */

#ifndef __PNSO_TEST_DESC_H__
#define __PNSO_TEST_DESC_H__

#include "pnso_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TEST_NODE_LIST \
	TEST_NODE(ROOT), \
	TEST_NODE(ALIAS), \
	TEST_NODE(SVC_CHAIN), \
	TEST_NODE(SVC), \
	TEST_NODE(CRYPTO_KEY), \
	TEST_NODE(TESTCASE), \
	TEST_NODE(VALIDATION), \
	TEST_NODE(CP_HDR), \
	TEST_NODE(CP_HDR_MAPPING), \
	TEST_NODE(FILE) \
  
#undef TEST_NODE
#define TEST_NODE(name) NODE_##name
typedef enum {
	TEST_NODE_LIST,

	/* Must be last */
	NODE_MAX
} node_type_t;

struct test_node_list {
	struct test_node *head; /* prev */
	struct test_node *tail; /* next */
};

struct test_node {
	node_type_t type;
	uint32_t idx;
	struct test_node *parent;
	struct test_node_list siblings;
};

#define TEST_TABLE_BUCKET_COUNT 101
struct test_node_table {
	struct test_node_list buckets[TEST_TABLE_BUCKET_COUNT];
};

#define FOR_EACH_NODE(list) for (node = (list).head; node; node = node->siblings.tail)
#define FOR_EACH_NODE_SAFE(list) for (node = (list).head; \
		node && ((next_node = node->siblings.tail) != node); \
		node = next_node)
#define NODE_FIND_ID(list, id)  for (node = (list).head; node; \
				     node = node->siblings.tail) { \
					if (node->idx == id) break; }
#define FOR_EACH_TABLE_BUCKET(table) \
	for (bucket = 0, list = &(table).buckets[bucket]; \
	     bucket < TEST_TABLE_BUCKET_COUNT; \
	     list = &(table).buckets[++bucket])

static inline uint32_t test_count_nodes(const struct test_node_list *list)
{
	uint32_t count = 0;
	struct test_node *node;

	if (!list)
		return 0;

	FOR_EACH_NODE(*list) {
		count++;
	}
	return count;
}

static inline uint32_t test_count_ancestor_nodes(const struct test_node *node)
{
	uint32_t count = 0;

	if (!node)
		return 0;

	while ((node = node->parent))
		count++;

	return count;
}

#define TEST_CRYPTO_MAX_KEY_LEN 128

struct test_crypto_key {
	struct test_node node;
	uint32_t key1_len;
	uint32_t key2_len;
	uint8_t key1[TEST_CRYPTO_MAX_KEY_LEN];
	uint8_t key2[TEST_CRYPTO_MAX_KEY_LEN];
};

#define TEST_MAX_NAME_LEN 64
#define TEST_MAX_PATTERN_LEN 8196
#define TEST_MAX_PATH_LEN 128
#define TEST_MAX_FILE_PREFIX_LEN 32
#define TEST_MAX_FULL_PATH_LEN (TEST_MAX_PATH_LEN+(2*TEST_MAX_FILE_PREFIX_LEN))

#define TEST_MAX_CORE_COUNT 32
#define TEST_MAX_BATCH_CONCURRENCY 32
#define TEST_MAX_BATCH_DEPTH 512

enum {
	FILE_FORMAT_RANDOM,
	FILE_FORMAT_FILE,
	FILE_FORMAT_PATTERN,
};

struct test_input_desc {
	uint16_t format; /* FILE_FORMAT_* */
	uint32_t offset;
	uint32_t len;
	uint32_t min_block_size;
	uint32_t max_block_size;
	uint32_t block_count;
	uint32_t random_seed;
	uint32_t random_len;
	char pattern[TEST_MAX_PATTERN_LEN];
	char pathname[TEST_MAX_PATH_LEN];
};

struct test_svc {
	struct test_node node;
	uint32_t output_flags;
	char output_path[TEST_MAX_PATH_LEN];
	struct pnso_service svc;
	union {
		struct {
			uint16_t threshold_delta;
		} cpdc;
		struct {
			uint32_t iv_data_len;
			uint8_t *iv_data;
		} crypto;
	} u;

	/* runtime data */
	uint32_t padded_len;
};

struct test_svc_chain {
	struct test_node node;
	char name[TEST_MAX_NAME_LEN];
	struct test_input_desc input;
	uint32_t num_services;
	uint16_t batch_weight;
	//struct pnso_service svcs[PNSO_SVC_TYPE_MAX];
	struct test_node_list svcs;

	/* runtime data */
	struct test_validation *rt_retcode_validation;
};

static inline struct pnso_service *get_cur_svc(struct test_node *node)
{
	return &((struct test_svc *) node)->svc;
}

#define MAX_SVC_CHAINS_PER_TESTCASE 64

#define VALIDATION_TYPE_LIST \
	VALIDATION_TYPE(UNKNOWN), \
	VALIDATION_TYPE(DATA_COMPARE), \
	VALIDATION_TYPE(SIZE_COMPARE), \
	VALIDATION_TYPE(RETCODE_COMPARE), \
	VALIDATION_TYPE(DATA_LEN_COMPARE)

#undef VALIDATION_TYPE
#define VALIDATION_TYPE(name) VALIDATION_##name
enum {
	VALIDATION_TYPE_LIST,

	/* Must be last */
	VALIDATION_TYPE_MAX
};

static inline bool validation_is_per_req(uint16_t type)
{
	return (type == VALIDATION_RETCODE_COMPARE) ||
		(type == VALIDATION_DATA_LEN_COMPARE);
}

enum {
	COMPARE_TYPE_EQ, /* default */
	COMPARE_TYPE_NE,
	COMPARE_TYPE_LT,
	COMPARE_TYPE_LE,
	COMPARE_TYPE_GT,
	COMPARE_TYPE_GE,

	/* Must be last */
	COMPARE_TYPE_MAX
};

enum {
	DYN_OFFSET_EOF = 0xffffffff,
	DYN_OFFSET_EOB = 0xfffffffe,

	DYN_OFFSET_START = DYN_OFFSET_EOB,
};

struct test_validation {
	struct test_node node;
	char name[TEST_MAX_NAME_LEN];
	uint16_t type; /* VALIDATION_* */
	uint16_t cmp_type; /* COMPARE_TYPE_* */
	char file1[TEST_MAX_PATH_LEN];
	char file2[TEST_MAX_PATH_LEN];
	char pattern[TEST_MAX_PATTERN_LEN];
	uint32_t offset;
	uint32_t len;
	uint32_t svc_chain_idx;
	pnso_error_t retcode;
	pnso_error_t req_retcode;
	uint32_t svc_count;
	pnso_error_t svc_retcodes[PNSO_SVC_TYPE_MAX];

	/* runtime stats, protect with lock */
	uint64_t rt_success_count;
	uint64_t rt_failure_count;
};

struct test_cp_header {
	struct test_node node;
	struct pnso_compression_header_format fmt;
};

struct test_cp_hdr_mapping {
	struct test_node node;
	uint32_t pnso_algo;
	uint32_t hdr_algo;
};

enum {
	SYNC_MODE_SYNC,
	SYNC_MODE_ASYNC,
	SYNC_MODE_POLL,

	/* Must be last */
	SYNC_MODE_MAX
};

struct test_testcase {
	struct test_node node;
	char name[TEST_MAX_NAME_LEN];
	bool turbo;
	uint64_t repeat;
	uint16_t batch_depth;
	uint16_t batch_concurrency;
	uint16_t sync_mode;
	uint32_t svc_chain_count;
	uint32_t svc_chains[MAX_SVC_CHAINS_PER_TESTCASE];
	struct test_node_list req_validations;
	struct test_node_list batch_validations;
};

#define TEST_ALIAS_MAX_NAME_LEN 32
#define TEST_ALIAS_MAX_VAL_LEN 80
struct test_alias {
	struct test_node node;
	char name[TEST_ALIAS_MAX_NAME_LEN];
	char val[TEST_ALIAS_MAX_VAL_LEN];
};

struct test_desc {
	struct test_node node;
	struct pnso_init_params init_params;
	uint64_t cpu_mask;
	uint64_t limit_rate;
	uint32_t status_interval;
	char output_file_prefix[TEST_MAX_FILE_PREFIX_LEN];
	char output_file_suffix[TEST_MAX_FILE_PREFIX_LEN];
	bool store_output_files;
	struct test_node_list aliases;
	struct test_node_list svc_chains;
	struct test_node_list crypto_keys;
	struct test_node_list cp_hdrs;
	struct test_node_list cp_hdr_map;
	struct test_node_list tests;
};


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __PNSO_TEST_DESC_H__ */
