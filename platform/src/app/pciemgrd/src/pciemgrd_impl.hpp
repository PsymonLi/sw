/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#ifndef __PCIEMGRD_IMPL_H__
#define __PCIEMGRD_IMPL_H__

typedef struct pciemgrenv_s {
    u_int8_t interactive:1;
    u_int8_t gold:1;
    u_int8_t reboot_on_hostdn:1;
    u_int8_t poll_port:1;
    u_int8_t poll_dev:1;
    u_int8_t enabled_ports;
    pciemgr_params_t params;
} pciemgrenv_t;

pciemgrenv_t *pciemgrenv_get(void);
void pciemgrd_params(pciemgrenv_t *pme);
int cli_loop(void);
int server_loop(void);
int gold_loop(void);
void logger_init(void);
int open_hostports(void);
void close_hostports(void);
int intr_init(void);

int upgrade_state_save(void);
int upgrade_state_restore(void);
int upgrade_in_progress(void);

#endif /* __PCIEMGRD_IMPL_H__ */
