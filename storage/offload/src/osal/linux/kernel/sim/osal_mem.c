#include <linux/slab.h>
#include "osal_mem.h"

/*
 * TODO:
 * 1. Using GFP_NOIO flag for now to get sim going - this will need to change when we have real driver
 * 2. Kernel doesnt provide an elegant way to allocate aligned memory. This may need to be moved to upper layer 
 *    or serviced via memory manager within osal . Sim is not using any aligned access right now
 * 3. kmalloc has a limitation that allocated size should be less than KMALLOC_MAXSIZE. This again may need to 
 *    be moved to upper layer or serviced via memory manager within osal . 
 */

void* osal_alloc(size_t size) 
{
	return kmalloc(size, GFP_KERNEL|GFP_NOIO);
}

void* osal_aligned_alloc(size_t alignment, size_t size) 
{
	return kmalloc(size, GFP_KERNEL|GFP_NOIO);
}

void osal_free(void* ptr) 
{
	return kfree(ptr);
}

uint64_t osal_rmem_alloc(size_t size) 
{
	return (uint64_t) kmalloc(size, GFP_KERNEL|GFP_NOIO);
}

uint64_t osal_rmem_aligned_alloc(size_t alignment, size_t size) 
{
	return (uint64_t) kmalloc(size, GFP_KERNEL|GFP_NOIO);
}

void osal_rmem_free(uint64_t ptr, size_t size) 
{
	return kfree((void*)ptr);
}

uint64_t osal_virt_to_phy(void *ptr)
{
	return (uint64_t) ptr;	
}

void *osal_phy_to_virt(uint64_t phy)
{
	return (void*)phy;
}

uint64_t osal_hostpa_to_devpa(uint64_t hostpa)
{
	return hostpa;
}

uint64_t osal_devpa_to_hostpa(uint64_t devpa)
{
	return devpa;
}
