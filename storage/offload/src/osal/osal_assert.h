/*
 * {C} Copyright 2018 Pensando Systems Inc.
 * All rights reserved.
 *
 */
#ifndef __OSAL_ASSERT_H__
#define __OSAL_ASSERT_H__

#ifndef __KERNEL__
#include <assert.h>
#define OSAL_ASSERT(x) assert(x)
#else
#ifndef __FreeBSD__
#include <linux/bug.h>
#else
#include <linux/kernel.h>
#endif
#define OSAL_ASSERT(x) BUG_ON(!(x))
#endif

#define OSAL_STATIC_ASSERT(x) BUILD_BUG_ON(!(x))

#endif	/* __OSAL_ASSERT_H__ */
