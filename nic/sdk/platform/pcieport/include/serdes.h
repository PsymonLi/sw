/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#ifndef __SERDES_H__
#define __SERDES_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef union laneinfo_u {
    uint16_t lane[16];
    uint32_t w[8];
} laneinfo_t;

uint16_t pciesd_lanes_ready(const int port,
                            const uint16_t lanemask);
int pciesd_core_interrupt(const int port,
                          const uint16_t lanemask,
                          const uint16_t code,
                          const uint16_t data,
                          laneinfo_t *dataout);

unsigned int pciesd_sbus_rd(const unsigned int addr,
                            const unsigned int reg);
void pciesd_sbus_wr(const unsigned int addr,
                    const unsigned int reg,
                    const unsigned int data);

#ifdef __cplusplus
}
#endif

#endif /* __SERDES_H__ */
