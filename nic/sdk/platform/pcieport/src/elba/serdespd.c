/*
 * Copyright (c) 2018-2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "elb_sw_glue.h"
#include "elb_pcie_sw_api.h"

#include "platform/pal/include/pal.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pcieport/include/pcieport.h"
#include "pcieportpd.h"

#define ELB_PCIE_PXC_INST_PER_PP_INST 4
#define ELB_PCIE_API_NUM_PP_INST 2
#define ELB_PCIE_S_FILE_DOWNLOAD 1

unsigned int
pciesd_sbus_rd(const unsigned int addr,
               const unsigned int reg)
{
    // XXX ELBA-TODO
    assert(0);
    return 0;
}

void
pciesd_sbus_wr(const unsigned int addr,
               const unsigned int reg,
               const unsigned int data)
{
    // XXX ELBA-TODO
    assert(0);
}

#define ELB_PCIE_PXC_INST_PER_PP_INST 4
#define ELB_PCIE_API_NUM_PP_INST 2
#define ELB_PCIE_S_FILE_DOWNLOAD 1

static uint64_t
pp_pcsd_interrupt_addr(const int port,
                       const int lane)
{
    return PP_(CFG_PP_PCSD_INTERRUPT, port) + lane * 4;
}

static uint16_t
pciesd_poll_interrupt_in_progress(const int port,
                                  const uint16_t want)
{
    const int maxpolls = 100;
    uint16_t inprog;
    int polls = 0;

    /* check once, if not done immediately then poll loop */
    inprog = pal_reg_rd32(PP_(STA_PP_PCSD_INTERRUPT_IN_PROGRESS, port));
    while (inprog != want && ++polls < maxpolls) {
        usleep(1000);
        inprog = pal_reg_rd32(PP_(STA_PP_PCSD_INTERRUPT_IN_PROGRESS, port));
    }

    if (inprog != want) {
        pciesys_loginfo("interrupt timeout (want 0x%04x, got 0x%04x)\n",
                        want, inprog);
        /* continue */
    }

    return inprog;
}

int
pciesd_core_interrupt(const int port,
                      const uint16_t lanemask,
                      const uint16_t code,
                      const uint16_t data,
                      laneinfo_t *dataout)
{
    const uint32_t codedata = ((uint32_t)data << 16) | code;
    uint16_t inprog;
    int i;

    /* set up interrupt code/data */
    for (i = 0; i < 16; i++) {
        const uint16_t lanebit = 1 << i;

        if (lanemask & lanebit) {
            pal_reg_wr32(pp_pcsd_interrupt_addr(port, i), codedata);
        }
    }

    /* issue interrupt request */
    pal_reg_wr32(PP_(CFG_PP_PCSD_INTERRUPT_REQUEST, port), lanemask);

    /* wait for interrupt-in-progress */
    inprog = pciesd_poll_interrupt_in_progress(port, lanemask);
    if (inprog != lanemask) {
        memset(dataout->w, 0xff, sizeof(dataout->w));
        return -1;
    }

    /* clear interrupt request */
    pal_reg_wr32(PP_(CFG_PP_PCSD_INTERRUPT_REQUEST, port), 0);

    /* wait for interrupt-complete */
    inprog = pciesd_poll_interrupt_in_progress(port, 0);
    if (inprog != 0) {
        memset(dataout->w, 0xff, sizeof(dataout->w));
        return -1;
    }

    /* read interrupt response data */
    pal_reg_rd32w(PP_(STA_PP_PCSD_INTERRUPT_DATA_OUT, port), dataout->w, 8);

    return 0;
}

/*
 * Read core status and figure out which pcie serdes lanes of lanemask
 * have the "ready" bit set.  See the Avago serdes documentation.
 */
uint16_t
pciesd_lanes_ready(const int port,
                   const uint16_t lanemask)
{
    uint16_t lanes_ready = 0;
    uint32_t sd_core_status[16];
    int i;

    pal_reg_rd32w(PP_(STA_PP_SD_CORE_STATUS, port), sd_core_status, 16);
    for (i = 0; i < 16; i++) {
        const uint16_t lanebit = 1 << i;

        if (lanemask & lanebit) {
            if (sd_core_status[i] & (1 << 5)) {
                lanes_ready |= lanebit;
            }
        }
    }
    return lanes_ready;
}

char * get_sbus_bin_filename() {

    char * filename = NULL;
    const char *s = getenv("PCIE_SERDES_BIN_FILE");
    if (s == NULL) {
        filename = (char *) s;
    } else {
        filename = "serdes.0x10AC_1047.bin";
    }

    return filename;
}

int elb_pcie_serdes_common_setup(int chip_id, int enable_irom,
        struct elb_pcie_sbus_bin_hdr_t * hdr_ptr) {

   PLOG_MSG("PCIE:SW FUNCTION 7 " << dec << endl);
   int cur_sbus_clk_div = elb_pcie_get_cur_sbus_clk_div(chip_id);
   if(cur_sbus_clk_div != 6) {
       PLOG_MSG("TEST:SBUS cap_top_pp_sbus_test clk divider =6 " << endl);
       elb_pcie_sbus_clk(chip_id, 6);
   }


   if(elb_pcie_sbus_idcode_check(chip_id) < 0) return -1;

   uint32_t spico_pcie_addr = ELB_ADDR_BASE_MS_SOC_OFFSET +
       ELB_SOC_CSR_SCA_SPICO_PCIE_BYTE_ADDRESS;
   u_int32_t spico_pcie_data[ELB_SOCA_CSR_SPICO_PCIE_SIZE];
   pal_reg_rd32w(spico_pcie_addr , &spico_pcie_data[0] , ELB_SOCA_CSR_SPICO_PCIE_SIZE);
   if(ELB_SOCA_CSR_SPICO_PCIE_SPICO_PCIE_1_2_I_ROM_ENABLE_PCIESBM_GET(spico_pcie_data[1]) != enable_irom) {
    spico_pcie_data[1] = ELB_SOCA_CSR_SPICO_PCIE_SPICO_PCIE_1_2_I_ROM_ENABLE_PCIESBM_MODIFY(spico_pcie_data[1], enable_irom);
    pal_reg_wr32w(spico_pcie_addr , &spico_pcie_data[0] , ELB_SOCA_CSR_SPICO_PCIE_SIZE);
   }

#if 0
   if(enable_irom) {
   } else {
       /* XXX ELBA-TODO rewrite SBUS API to use rom_info */
        if(elb_pcie_upload_sbus_bin_file(
                    chip_id, 1, 0, 32, 0, hdr_ptr->nwords, file_ptr) >= 0) {
            SW_PRINT("pcie serdes upload happened for all lanes\n");
        }

   }
#endif
   return 0;
}

int elb_pcie_serdes_port_setup(int chip_id, int enable_irom, int pp_inst,
        int phy_port, int lanes, struct elb_pcie_sbus_bin_hdr_t *hdr_ptr, void * file_ptr) {

    int px_port = (pp_inst << 2) + (phy_port & 0x3);
    if(enable_irom) {
        hdr_ptr->build_id = 0;
        hdr_ptr->rev_id  = 0;
    } else {
        if(elb_pcie_upload_sbus_bin_file(
                    chip_id, 0, px_port, lanes, px_port, hdr_ptr->nwords, file_ptr) >= 0) {
            SW_PRINT("pcie serdes upload happened for px_port %d lanes %d\n", px_port, lanes);
        }
    }

    return elb_pcie_serdes_init(chip_id, pp_inst, phy_port, lanes,
            1, 1, 10000, hdr_ptr->build_id, hdr_ptr->rev_id);
}

void *
romfile_open(const void *rom_info)
{
    pal_reg_trace("================ romfile_open\n");
    return (void *) rom_info;
}

void
romfile_close(void *ctx)
{
    pal_reg_trace("================ romfile_close\n");
}

#if defined(ELB_PCIE_S_FILE_DOWNLOAD)
#define SERDESFW_GEN(mac) \
    mac(0x10AC_1047)

static uint8_t *
serdes_lookup(const char *name)
{
    /* generate extern declaration of serdesfw symbol */
#define ext_decl(n) \
    extern uint8_t sbus_pcie_rom_ ## n ## _start[];
    SERDESFW_GEN(ext_decl);

    /* generate serdesfw table */
    static struct serdes_entry {
        const char *name;
        uint8_t *start;
    } serdes_table[] = {
#define serdes_table_ent(n) \
        { #n, sbus_pcie_rom_ ## n ## _start },
        SERDESFW_GEN(serdes_table_ent)
        { NULL, 0 }
    };
    struct serdes_entry *ent;

    /* search serdesfw table for "name" */
    for (ent = serdes_table; ent->name; ent++) {
        if (strcmp(name, ent->name) == 0) {
            return ent->start;
        }
    }
    return NULL;
}

#endif // if ELB_PCIE_S_FILE_DOWNLOAD

int
pcieportpd_serdes_init(pcieport_t *p)
{

    int irom_enable = 0;
    int r=0;
    int phy_port = p->port & 0x3;
    int lanes = p->cap_width;
    int pp_inst = (p->port >> 2) & 1;

#if defined(ELB_PCIE_S_FILE_DOWNLOAD)
    extern uint8_t sbus_pcie_rom_start[];
    const char *s = getenv("PCIE_SERDESFW");
    struct elb_pcie_sbus_bin_hdr_t * hdr;
    void * f_ptr = NULL;

    if (s == NULL) {
        hdr = (struct elb_pcie_sbus_bin_hdr_t *)sbus_pcie_rom_start;
    } else if ((hdr = (struct elb_pcie_sbus_bin_hdr_t *)serdes_lookup(s)) != 0) {
        pciesys_loginfo("$PCIE_SERDESFW selects %s\n", s);
    } else {
        pciesys_loginfo("$PCIE_SERDESFW bad value: %s (using default)\n", s);
        hdr = (struct elb_pcie_sbus_bin_hdr_t *)sbus_pcie_rom_start;
    }
    pciesys_loginfo("pcie sbus ROM @ %p", hdr);
    if (hdr->magic != SBUS_ROM_MAGIC) {
        pciesys_loginfo(", bad magic got 0x%x want 0x%x\n",
                        hdr->magic, SBUS_ROM_MAGIC);
        return -1;
    }
    pciesys_loginfo(", good magic.  %d words\n", hdr->nwords);

    f_ptr = (uint32_t *) (hdr+1);


#else //ELB_PCIE_S_FILE_DOWNLOAD
    struct elb_pcie_sbus_bin_hdr_t hdr_info;
    struct elb_pcie_sbus_bin_hdr_t *hdr = &hdr_info;
    char * filename = NULL;
    void * f_ptr = NULL;
    if(irom_enable ==0) {
        filename = get_sbus_bin_filename();
        f_ptr = elb_pcie_sbus_bin_info(hdr, filename);
        if(f_ptr== NULL) return -1;
    }

#endif // ELB_PCIE_S_FILE_DOWNLOAD

    pal_reg_trace("================ elb_pcie_serdes_setup start\n");
    pciesys_sbus_lock();
    if(r >= 0) {
        r = elb_pcie_serdes_common_setup(0, irom_enable, hdr);
        if(r >= 0) {
            if(elb_pcie_serdes_port_setup(0, irom_enable, pp_inst, phy_port, lanes, hdr, f_ptr) < 0) {
                SW_PRINT("elb_pcie_serdes_port_setup failed for pp_inst %d\n", pp_inst);
                r = -1;
                //break;
            }
        }
    }
    pciesys_sbus_unlock();

    pal_reg_trace("================ elb_pcie_serdes_setup end %d\n", r);

    return r;
}

int
pcieportpd_serdes_reset(void)
{
    assert(0); /* XXX ELBA-TODO */
    return 0;
}
