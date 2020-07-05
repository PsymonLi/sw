/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#ifndef __PAL_PCIE_H__
#define __PAL_PCIE_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

int pal_pciepreg_rd32(const uint64_t pa, uint32_t *valp);
int pal_pciepreg_wr32(const uint64_t pa, const uint32_t val);

#ifdef __cplusplus
}
#endif

#endif /* __PAL_PCIE_H__ */
