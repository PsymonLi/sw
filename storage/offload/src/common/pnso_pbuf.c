/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#ifndef __KERNEL__
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#endif

#include "osal.h"
#include "pnso_api.h"
#include "pnso_pbuf.h"

/* TODO-pbuf: move to common/util?? */
#ifndef __KERNEL__
static inline bool
is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}
#endif

static inline void
pbuf_alloc_buf(struct pnso_flat_buffer *flat_buf, uint32_t len)
{
	flat_buf->buf = (uint64_t) osal_alloc(len);
	flat_buf->len = len;
}

struct pnso_flat_buffer *__attribute__((unused))
pbuf_alloc_flat_buffer(uint32_t len)
{
	struct pnso_flat_buffer *flat_buf;

	flat_buf = osal_alloc(sizeof(struct pnso_flat_buffer));
	if (!flat_buf) {
		OSAL_ASSERT(0);
		goto out;
	}

	pbuf_alloc_buf(flat_buf, len);
	if (!flat_buf->buf) {
		OSAL_ASSERT(0);
		goto out_free;
	}

	return flat_buf;

out_free:
	osal_free(flat_buf);
out:
	return NULL;
}

static inline void
pbuf_aligned_alloc_buf(struct pnso_flat_buffer *flat_buf, uint32_t align_size,
		       uint32_t len)
{
	flat_buf->buf = (uint64_t) osal_aligned_alloc(align_size, len);
	flat_buf->len = len;
}

struct pnso_flat_buffer *__attribute__((unused))
pbuf_aligned_alloc_flat_buffer(uint32_t align_size, uint32_t len)
{
	struct pnso_flat_buffer *flat_buf;

	if (!is_power_of_2(align_size))
		goto out;

	flat_buf = osal_alloc(sizeof(struct pnso_flat_buffer));
	if (!flat_buf) {
		OSAL_ASSERT(0);
		goto out;
	}

	pbuf_aligned_alloc_buf(flat_buf, align_size, len);
	if (!flat_buf->buf) {
		OSAL_ASSERT(0);
		goto out_free;
	}

	return flat_buf;

out_free:
	osal_free(flat_buf);
out:
	return NULL;
}

void __attribute__((unused))
pbuf_free_flat_buffer(struct pnso_flat_buffer *flat_buf)
{
	void *p;

	if (!flat_buf)
		return;

	p = (void *) flat_buf->buf;
	osal_free(p);

	/* done, and let caller be responsible to free the container */
}

struct pnso_buffer_list *__attribute__((unused))
pbuf_alloc_buffer_list(uint32_t count, uint32_t len)
{
	struct pnso_buffer_list *buf_list;
	struct pnso_flat_buffer *flat_buf;
	size_t num_bytes;
	uint32_t i;

	if (count <= 0) {
		OSAL_ASSERT(0);
		goto out;
	}

	num_bytes = sizeof(struct pnso_buffer_list) +
	    count * sizeof(struct pnso_flat_buffer);

	buf_list = osal_alloc(num_bytes);
	if (!buf_list) {
		OSAL_ASSERT(0);
		goto out;
	}

	for (i = 0; i < count; i++) {
		flat_buf = &buf_list->buffers[i];
		pbuf_alloc_buf(flat_buf, len);
		if (!flat_buf->buf) {
			OSAL_ASSERT(0);
			goto out_free;
		}
	}
	buf_list->count = count;

	return buf_list;

out_free:
	buf_list->count = i;
	pbuf_free_buffer_list(buf_list);
out:
	return NULL;
}

struct pnso_buffer_list *__attribute__((unused))
pbuf_aligned_alloc_buffer_list(uint32_t count, uint32_t align_size,
		uint32_t len)
{
	struct pnso_buffer_list *buf_list;
	struct pnso_flat_buffer *flat_buf;
	size_t num_bytes;
	uint32_t i;

	if (count <= 0 || !is_power_of_2(align_size))
		goto out;

	num_bytes = sizeof(struct pnso_buffer_list) +
	    count * sizeof(struct pnso_flat_buffer);

	buf_list = osal_alloc(num_bytes);
	if (!buf_list) {
		OSAL_ASSERT(0);
		goto out;
	}

	for (i = 0; i < count; i++) {
		flat_buf = &buf_list->buffers[i];
		pbuf_aligned_alloc_buf(flat_buf, align_size, len);
		if (!flat_buf->buf) {
			OSAL_ASSERT(0);
			goto out_free;
		}
	}
	buf_list->count = count;

	return buf_list;

out_free:
	buf_list->count = i;
	pbuf_free_buffer_list(buf_list);
out:
	return NULL;
}

void __attribute__((unused))
pbuf_free_buffer_list(struct pnso_buffer_list *buf_list)
{
	struct pnso_flat_buffer *flat_buf;
	uint32_t i;

	if (!buf_list)
		return;

	for (i = 0; i < buf_list->count; i++) {
		flat_buf = &buf_list->buffers[i];
		pbuf_free_flat_buffer(flat_buf);
	}

	osal_free(buf_list);
}

struct pnso_buffer_list *__attribute__((unused))
pbuf_clone_buffer_list(const struct pnso_buffer_list *src_buf_list)
{
	struct pnso_buffer_list *buf_list;
	size_t num_bytes;
	uint32_t i, count;

	if (!src_buf_list || src_buf_list->count <= 0) {
		OSAL_ASSERT(0);
		goto out;
	}

	count = src_buf_list->count;
	num_bytes = sizeof(struct pnso_buffer_list) +
	    count * sizeof(struct pnso_flat_buffer);

	buf_list = osal_alloc(num_bytes);
	if (!buf_list) {
		OSAL_ASSERT(0);
		goto out;
	}

	for (i = 0; i < count; i++)
		memcpy(&buf_list->buffers[i], &src_buf_list->buffers[i],
		       sizeof(struct pnso_flat_buffer));
	buf_list->count = count;

	return buf_list;

out:
	return NULL;
}

uint32_t
pbuf_copy_buffer_list(const struct pnso_buffer_list *src_buf_list,
		      uint32_t src_offset,
		      struct pnso_buffer_list *dst_buf_list)
{
	uint32_t ret;
	uint32_t copy_len;
	uint32_t src_buf_offset;
	uint32_t dst_buf_offset;
	size_t src_i;
	size_t dst_i;
	const struct pnso_flat_buffer *src_buf;
	struct pnso_flat_buffer *dst_buf;

	if (!src_buf_list || !src_buf_list->count)
		return 0;

	if (!dst_buf_list || !dst_buf_list->count)
		return 0;

	ret = 0;
	dst_i = 0;
	dst_buf = &dst_buf_list->buffers[dst_i++];
	dst_buf_offset = 0;
	for (src_i = 0; src_i < src_buf_list->count; src_i++) {
		src_buf = &src_buf_list->buffers[src_i];
		src_buf_offset = 0;
		if (src_offset) {
			/* Skip offset bytes */
			if (src_offset >= src_buf->len) {
				src_offset -= src_buf->len;
				continue;
			}
			src_buf_offset += src_offset;
			src_offset = 0;
		}
		while (src_buf_offset < src_buf->len) {
			copy_len = src_buf->len - src_buf_offset;
			if (copy_len > (dst_buf->len - dst_buf_offset))
				copy_len = dst_buf->len - dst_buf_offset;
			if (copy_len) {
				memcpy((uint8_t *)dst_buf->buf+dst_buf_offset,
				       (uint8_t *)src_buf->buf+src_buf_offset,
				       copy_len);
				ret += copy_len;
				src_buf_offset += copy_len;
				dst_buf_offset += copy_len;
				if (dst_buf_offset >= dst_buf->len) {
					dst_buf = &dst_buf_list->buffers[dst_i++];
					dst_buf_offset = 0;
					if (dst_i > dst_buf_list->count ||
					    dst_buf->len == 0) {
						/* no more space in dst_buf_list */
						goto done;
					}
				}
			} else {
				goto done;
			}
		}
	}

	/* truncate dst length to match src */
	while (dst_i <= dst_buf_list->count) {
		if (dst_buf_offset < dst_buf->len) {
			dst_buf->len = dst_buf_offset;
		}
		dst_buf = &dst_buf_list->buffers[dst_i++];
		dst_buf_offset = 0;
	}

done:
	return ret;
}

size_t
pbuf_get_buffer_list_len(const struct pnso_buffer_list *buf_list)
{
	size_t num_bytes = 0;
	uint32_t i;

	if (buf_list)
		for (i = 0; i < buf_list->count; i++)
			num_bytes += buf_list->buffers[i].len;

	return num_bytes;
}

bool __attribute__((unused))
pbuf_is_buffer_list_sgl(const struct pnso_buffer_list *buf_list)
{
	if (!buf_list || buf_list->count == 0)
		return false;

	if (buf_list->count == 1)
		return false;

	return true;
}

void __attribute__((unused))
pbuf_convert_buffer_list_v2p(struct pnso_buffer_list *buf_list)
{
	void *ptr;
	uint32_t i;

	if (!buf_list)
		return;

	for (i = 0; i < buf_list->count; i++) {
		ptr = (void *) buf_list->buffers[i].buf;
		buf_list->buffers[i].buf = osal_virt_to_phy(ptr);
	}
}

void __attribute__((unused))
pbuf_convert_flat_buffer_v2p(struct pnso_flat_buffer *flat_buf)
{
	void *ptr;

	if (!flat_buf)
		return;

	ptr = (void *) flat_buf->buf;
	flat_buf->buf = osal_virt_to_phy(ptr);
}

void __attribute__((unused))
pbuf_convert_buffer_list_p2v(struct pnso_buffer_list *buf_list)
{
	void *ptr;
	uint32_t i;

	if (!buf_list)
		return;

	for (i = 0; i < buf_list->count; i++) {
		ptr = osal_phy_to_virt(buf_list->buffers[i].buf);
		buf_list->buffers[i].buf = (uint64_t) ptr;
	}
}

void __attribute__((unused))
pbuf_convert_flat_buffer_p2v(struct pnso_flat_buffer *flat_buf)
{
	void *ptr;

	if (!flat_buf)
		return;

	ptr = osal_phy_to_virt(flat_buf->buf);
	flat_buf->buf = (uint64_t) ptr;
}

inline uint32_t
pbuf_get_flat_buffer_block_count(const struct pnso_flat_buffer *flat_buf,
		uint32_t block_size)
{
	return (flat_buf->len + (block_size - 1)) / block_size;
}

inline uint32_t __attribute__((unused))
pbuf_get_flat_buffer_block_len(const struct pnso_flat_buffer *flat_buf,
		uint32_t block_idx, uint32_t block_size)
{
	uint32_t len;

	if (flat_buf->len >= (block_size * (block_idx + 1)))
		len = block_size;
	else if (flat_buf->len >= (block_size * block_idx))
		len = flat_buf->len % block_size;
	else
		len = 0;

	return len;
}

uint32_t __attribute__((unused))
pbuf_pad_flat_buffer_with_zeros(struct pnso_flat_buffer *flat_buf,
		uint32_t block_size)
{
	uint32_t block_count, pad_len;

	block_count = pbuf_get_flat_buffer_block_count(flat_buf, block_size);

	pad_len = block_size -
		pbuf_get_flat_buffer_block_len(flat_buf, block_count - 1,
				block_size);

	if (pad_len) {
		memset((uint8_t *) (flat_buf->buf + flat_buf->len), 0, pad_len);
		flat_buf->len += pad_len;
	}

	return pad_len;
}

void __attribute__((unused))
pbuf_pprint_buffer_list(const struct pnso_buffer_list *buf_list)
{
	struct pnso_flat_buffer *flat_buf;
	char *p;
	uint32_t i;

	if (!buf_list)
		return;

	OSAL_LOG_INFO("buf_list: 0x" PRIx64 " count: %d\n",
			(uint64_t) buf_list, buf_list->count);

	for (i = 0; i < buf_list->count; i++) {
		flat_buf = (struct pnso_flat_buffer *) &buf_list->buffers[i];
		p = (char *)flat_buf->buf;

		/* print only limited number of characters */
		OSAL_LOG_INFO("#%2d: flat_buf: 0x" PRIx64 " len: %d buf: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			      i, (uint64_t)flat_buf, flat_buf->len,
			      p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

		/*
		 * note: break here is intentional to emit just 0th fbuf.
		 * if needed, for all items in the buf_list, comment out
		 * the break.
		 *
		 */
		break;
	}
}
