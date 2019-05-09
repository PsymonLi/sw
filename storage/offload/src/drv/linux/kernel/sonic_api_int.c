/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#ifdef __FreeBSD__
#include <linux/io.h>
#endif

#include "sonic.h"
#include "sonic_dev.h"
#include "sonic_lif.h"
#include "sonic_api_int.h"
#include "osal_mem.h"
#include "osal_logger.h"
#include "osal_assert.h"

const char *accel_ring_name_tbl[] = {
	[ACCEL_RING_CP]		= "cp",
	[ACCEL_RING_CP_HOT]	= "cp_hot",
	[ACCEL_RING_DC]		= "dc",
	[ACCEL_RING_DC_HOT]	= "dc_hot",
	[ACCEL_RING_XTS0]	= "xts0",
	[ACCEL_RING_XTS1]	= "xts1",
	[ACCEL_RING_GCM0]	= "gcm0",
	[ACCEL_RING_GCM1]	= "gcm1",
};

static identity_t *sonic_get_identity(void);

static inline uint64_t
sonic_rmem_base_pa(void)
{
	identity_t *ident = sonic_get_identity();

	return ident->dev.base.cm_base_pa;
}

static inline void __iomem *
sonic_rmem_base_iomem(void)
{
	struct sonic_dev *idev = sonic_get_idev();

	OSAL_ASSERT(idev->hbm_iomem_vaddr);
	return idev->hbm_iomem_vaddr;
}

static inline uint64_t
sonic_rmem_pgid_to_offset(uint32_t pgid)
{
	return (uint64_t)pgid * PAGE_SIZE;
}

static inline uint32_t
sonic_rmem_offset_to_pgid(uint64_t offset)
{
	return offset / PAGE_SIZE;
}

static inline int
sonic_rmem_size_to_pages(size_t size)
{
	return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}

static inline uint64_t
sonic_rmem_pgid_to_pgaddr(uint32_t pgid)
{
	uint64_t offset = sonic_rmem_pgid_to_offset(pgid);

	OSAL_ASSERT((sonic_rmem_base_pa() & (offset + PAGE_SIZE - 1)) == 0);
	return sonic_rmem_base_pa() + offset;
}

static inline void __iomem *
sonic_rmem_pgid_to_iomem(uint32_t pgid)
{
	uint64_t offset = sonic_rmem_pgid_to_offset(pgid);

	OSAL_ASSERT(((uint64_t)sonic_rmem_base_iomem() &
		(PAGE_SIZE - 1)) == 0);
	return sonic_rmem_base_iomem() + offset;
}

static inline uint32_t
sonic_rmem_pgaddr_to_pgid(uint64_t pgaddr)
{
	OSAL_ASSERT(pgaddr >= sonic_rmem_base_pa());
	return sonic_rmem_offset_to_pgid(pgaddr - sonic_rmem_base_pa());
}

static inline uint64_t
sonic_rmem_pgaddr_to_pgoffset(uint64_t pgaddr)
{
	return pgaddr & (PAGE_SIZE - 1);
}

static inline void __iomem *
sonic_rmem_pgaddr_to_iomem(uint64_t pgaddr)
{
	return sonic_rmem_pgid_to_iomem(sonic_rmem_pgaddr_to_pgid(pgaddr));
}

static uint64_t sonic_api_get_rmem(int npages)
{
	struct sonic_dev *idev = sonic_get_idev();
	uint64_t pgaddr = SONIC_RMEM_ADDR_INVALID;

	OSAL_ASSERT(npages);
	spin_lock(&idev->hbm_inuse_lock);

	if (idev->hbm_nfrees) {
		OSAL_LOG_ERROR("rmem alloc cannot be used after free");
		goto out;
	}
	if (npages > sonic_rmem_avail_pages_get()) {
		OSAL_LOG_ERROR("rmem alloc npages %d exceeds avail %d",
				npages, sonic_rmem_avail_pages_get());
		goto out;
	}

	pgaddr = sonic_rmem_pgid_to_pgaddr(idev->hbm_nallocs);
	OSAL_ASSERT(sonic_rmem_addr_valid(pgaddr));
	idev->hbm_nallocs += npages;
out:
	spin_unlock(&idev->hbm_inuse_lock);
	return pgaddr;
}

static void sonic_api_put_rmem(uint32_t pgid, int npages)
{
	struct sonic_dev *idev = sonic_get_idev();

	OSAL_ASSERT((pgid + npages) <= idev->hbm_npages);
	spin_lock(&idev->hbm_inuse_lock);
	if ((idev->hbm_nfrees + npages) > idev->hbm_nallocs) {
		OSAL_LOG_ERROR("rmem free amount failure: nfrees %u npages %u nallocs %u",
				idev->hbm_nfrees, npages, idev->hbm_nallocs);
	} else
		idev->hbm_nfrees += npages;
	spin_unlock(&idev->hbm_inuse_lock);
}

uint32_t sonic_rmem_total_pages_get(void)
{
	return sonic_get_idev()->hbm_npages;
}

uint32_t sonic_rmem_avail_pages_get(void)
{
	struct sonic_dev *idev = sonic_get_idev();

	OSAL_ASSERT(idev->hbm_npages >= idev->hbm_nallocs);
	return idev->hbm_npages - idev->hbm_nallocs;
}

uint32_t sonic_rmem_page_size_get(void)
{
	return PAGE_SIZE;
}

uint64_t sonic_rmem_alloc(size_t size)
{
	if (!size)
		return SONIC_RMEM_ADDR_INVALID;
	return sonic_api_get_rmem(sonic_rmem_size_to_pages(size));
}

uint64_t sonic_rmem_calloc(size_t size)
{
	uint64_t pgaddr = sonic_rmem_alloc(size);

	if (sonic_rmem_addr_valid(pgaddr))
		sonic_rmem_set(pgaddr, 0, size);
	return pgaddr;
}

void sonic_rmem_free(uint64_t pgaddr, size_t size)
{
	if (sonic_rmem_addr_valid(pgaddr) && size)
		sonic_api_put_rmem(sonic_rmem_pgaddr_to_pgid(pgaddr),
				   sonic_rmem_size_to_pages(size));
}

void sonic_rmem_set(uint64_t pgaddr, uint8_t val, size_t size)
{
	OSAL_ASSERT(sonic_rmem_addr_valid(pgaddr));
	memset_io(sonic_rmem_pgaddr_to_iomem(pgaddr) +
		  sonic_rmem_pgaddr_to_pgoffset(pgaddr), val, size);
}

void sonic_rmem_read(void *dst, uint64_t pgaddr, size_t size)
{
	OSAL_ASSERT(sonic_rmem_addr_valid(pgaddr));
	memcpy_fromio(dst, sonic_rmem_pgaddr_to_iomem(pgaddr) +
		      sonic_rmem_pgaddr_to_pgoffset(pgaddr), size);
}

void sonic_rmem_write(uint64_t pgaddr, const void *src, size_t size)
{
	OSAL_ASSERT(sonic_rmem_addr_valid(pgaddr));
	memcpy_toio(sonic_rmem_pgaddr_to_iomem(pgaddr) +
		    sonic_rmem_pgaddr_to_pgoffset(pgaddr), src, size);
}

static identity_t *sonic_get_identity(void)
{
	return &sonic_get_lif()->sonic->ident;
}

unsigned int
sonic_get_lif_id(struct sonic *sonic, uint32_t idx)
{
	identity_t *ident = &sonic->ident;

	OSAL_ASSERT(ident && (idx < ident->dev.base.nlifs));
	return ident->lif.base.hw_index;
}

uint64_t
sonic_get_lif_local_dbaddr(void)
{
	identity_t *ident = sonic_get_identity();

	return ident->lif.base.hw_lif_local_dbaddr;
}

bool
sonic_validate_crypto_key_idx(uint32_t user_key_idx, uint32_t *ret_keys_max)
{
	identity_t *ident = sonic_get_identity();
	*ret_keys_max = ident->lif.base.num_crypto_keys_max;
	return user_key_idx < *ret_keys_max;
}

uint32_t
sonic_get_crypto_key_idx(uint32_t user_key_idx)
{
	identity_t *ident = sonic_get_identity();

	return ident->lif.base.crypto_key_idx_base + user_key_idx;
}

uint64_t
sonic_get_intr_assert_addr(uint32_t intr_idx)
{
	identity_t *ident = sonic_get_identity();

	OSAL_LOG_DEBUG("intr_assert_addr=0x%llx, intr_idx=0x%llx, intr_assert_stride=0x%llx",
		       (unsigned long long) ident->dev.base.intr_assert_addr,
		       (unsigned long long) intr_idx,
		       (unsigned long long) ident->dev.base.intr_assert_stride);
	return ident->dev.base.intr_assert_addr +
		((uint64_t)intr_idx * ident->dev.base.intr_assert_stride);
}

uint32_t
sonic_get_intr_assert_data(void)
{
	identity_t *ident = sonic_get_identity();

	return ident->dev.base.intr_assert_data;
}

uint64_t
sonic_get_per_core_intr_assert_addr(struct per_core_resource *pcr)
{
	return sonic_get_intr_assert_addr(pcr->intr.index);
}


#ifdef NDEBUG
#define DBG_CHK_RING_ID(r)	PNSO_OK
#else
#define DBG_CHK_RING_ID(r)	dbg_check_ring_id(r)
#endif

static inline int __attribute__ ((unused))
dbg_check_ring_id(uint32_t accel_ring_id)
{
	return (accel_ring_id >= ACCEL_RING_ID_MAX) ? -EINVAL : PNSO_OK;
}

struct sonic_accel_ring *sonic_get_accel_ring(uint32_t accel_ring_id)
{
	int err;
	struct sonic_dev *idev;

	err = DBG_CHK_RING_ID(accel_ring_id);
	if (err)
		return NULL;

	idev = sonic_get_idev();
	if (!idev)
		return NULL;

	return &idev->ring_tbl[accel_ring_id];
}

const char *sonic_accel_ring_name_get(uint32_t accel_ring_id)
{
	if ((accel_ring_id < ARRAY_SIZE(accel_ring_name_tbl)) &&
	    accel_ring_name_tbl[accel_ring_id])
		return accel_ring_name_tbl[accel_ring_id];

	return "unknown";
}

void sonic_accel_rings_reinit(struct sonic_dev *idev)
{
	struct sonic_accel_ring *ring;
	int i;

	for (i = 0, ring = idev->ring_tbl;
	     i < ACCEL_RING_ID_MAX;
	     i++, ring++) {
		osal_atomic_init(&ring->descs_inuse, 0);
	}
}

/*
 * Resource accounting with atomic counter
 */
int sonic_accounting_atomic_take(osal_atomic_int_t *atomic_c,
				 uint32_t count,
				 uint32_t high_water)
{
	if (unlikely((count > high_water) || (high_water == 0))) {
		OSAL_LOG_ERROR("Accounting take error: count %u, high_water %u",
			       count, high_water);
		return EAGAIN;
	}

	if (unlikely(count == 0))
		return PNSO_OK;

	if (likely(osal_atomic_fetch_add(atomic_c, count) < (high_water - count)))
		return PNSO_OK;

	osal_atomic_fetch_sub(atomic_c, count);
	return EAGAIN;
}

int sonic_accounting_atomic_give(osal_atomic_int_t *atomic_c,
				 uint32_t count)
{
	if (count == 0)
		return PNSO_OK;

	if (unlikely(osal_atomic_fetch_sub(atomic_c, count) < count)) {
		osal_atomic_fetch_add(atomic_c, count);
		OSAL_LOG_ERROR("Accounting counter underflow on sub count %u",
			       count);
		OSAL_ASSERT(0);
		return EAGAIN;
	}
	return PNSO_OK;
}

/*
 * Ring accounting
 */
int sonic_accel_ring_take(struct sonic_accel_ring *ring,
			  uint32_t count)
{
	return sonic_accounting_atomic_take(&ring->descs_inuse, count,
					    ring->accel_ring.ring_size);
}

int sonic_accel_ring_give(struct sonic_accel_ring *ring, uint32_t count)
{
	return sonic_accounting_atomic_give(&ring->descs_inuse, count);
}

int sonic_accel_rings_sanity_check(void)
{
	struct sonic_dev *idev;
	struct sonic_accel_ring *ring;
	int count;
	int err = PNSO_OK;

	idev = sonic_get_idev();
	if (idev) {
		for (ring = &idev->ring_tbl[0];
		     ring < &idev->ring_tbl[ACCEL_RING_ID_MAX];
		     ring++) {

			/*
			 * Ring would have been initialized if name is valid.
			 * Print out all rings with resources still outstanding.
			 */
			if (ring->name) {
				count = osal_atomic_read(&ring->descs_inuse);
				if (count) {
					OSAL_LOG_WARN("HW ring %s descs_inuse %d",
						      ring->name, count);
					err = EBUSY;
				}
			}
		}
	}

	if (err == PNSO_OK)
		OSAL_LOG_DEBUG("HW rings all descs returned");

	return err;
}

uint64_t sonic_hostpa_to_devpa(uint64_t hostpa)
{
	identity_t *ident = sonic_get_identity();

	OSAL_ASSERT((hostpa & ident->lif.base.hw_host_mask) == 0);
	return hostpa | ident->lif.base.hw_host_prefix;
}

uint64_t sonic_devpa_to_hostpa(uint64_t devpa)
{
	identity_t *ident = sonic_get_identity();

	return devpa & ~ident->lif.base.hw_host_mask;
}

uint64_t
sonic_virt_to_phy(void *ptr)
{
	uint64_t pa;

	OSAL_ASSERT(ptr);

	pa = osal_virt_to_phy(ptr);

	return sonic_hostpa_to_devpa(pa);
}

void *
sonic_phy_to_virt(uint64_t phy)
{
	uint64_t pa;

	OSAL_ASSERT(phy);

	pa = sonic_devpa_to_hostpa(phy);

	return osal_phy_to_virt(pa);
}

bool
sonic_is_accel_dev_ready()
{
	struct lif *glif = sonic_get_lif();

	return (glif && (glif->flags & LIF_F_INITED)) ? true : false;
}
