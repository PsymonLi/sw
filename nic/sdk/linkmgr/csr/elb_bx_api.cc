#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <cstdint>
#include "include/sdk/types.hpp"
#include "lib/catalog/catalog.hpp"
#include "linkmgr/linkmgr_internal.hpp"
#include "include/sdk/asic/elba/elb_mx_common_api.h"
#include "elb_bx_api.h"

#ifndef BX_DEBUG_MSG

#define BX_DEBUG_INFO(...)
#define BX_DEBUG_MSG  SDK_LINKMGR_TRACE_DEBUG
#define BX_DEBUG_ERR  SDK_LINKMGR_TRACE_ERR

#endif

mac_profile_t bx[MAX_MAC];

void elb_bx_soft_reset(int chip_id, int inst_id) {
   elb_bx_set_soft_reset(chip_id, inst_id, 0x1);
}

int elb_bx_apb_read(int chip_id, int inst_id, int addr) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);
    bx_csr.dhs_apb.entry[addr].read();
    int rdata = bx_csr.dhs_apb.entry[addr].data().convert_to<int>();
    return rdata;
}

void elb_bx_apb_write(int chip_id, int inst_id, int addr, int data) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);
    bx_csr.dhs_apb.entry[addr].data(data);
    bx_csr.dhs_apb.entry[addr].write();
    // bx_csr.dhs_apb.entry[addr].show();
}

// reads addr, set/reset bit and writes back
static int
elb_bx_set_bit (int chip_id, int inst_id, int addr, int bit, bool set)
{
   int data = elb_bx_apb_read(chip_id, inst_id, addr);

   if (set) {
       data = data | (1 << bit);
   } else {
       data = data & ~(1 << bit);
   }
   elb_bx_apb_write(chip_id, inst_id, addr, data);
   return 0;
}

static inline uint32_t
set_bits(uint32_t data, int pos, int num_bits, uint32_t value)
{
    uint32_t mask = 0;

    for (int i = pos; i < pos + num_bits; ++i) {
        mask |= (1 << i);
    }

    // clear the bits
    data = data & ~mask;

    // set the bits
    data = data | (value << pos);

    return data;
}

static int
elb_bx_set_bits(int chip_id, int inst_id, uint32_t addr,
                int pos, int num_bits, uint32_t value)
{
    uint32_t data = elb_bx_apb_read(chip_id, inst_id, addr);

    data = set_bits(data, pos, num_bits, value);
    elb_bx_apb_write(chip_id, inst_id, addr, data);
    return 0;
}

void elb_bx_set_ch_enable(int chip_id, int inst_id, int value) { 
    BX_DEBUG_MSG("BX%d ch%d elb_bx_set_ch_enable = %d\n", inst_id, 0, value);
    int rdata = elb_bx_apb_read(chip_id, inst_id, 0x1f10);
    int wdata = (rdata & 0xfe) | (value & 0x1);
    elb_bx_apb_write(chip_id, inst_id, 0x1f10, wdata);
}

void elb_bx_set_glbl_mode(int chip_id, int inst_id, int value) { 
    BX_DEBUG_MSG("BX%d ch%d elb_bx_set_glbl_mode = %d\n", inst_id, 0, value);
    int rdata = elb_bx_apb_read(chip_id, inst_id, 0x1f10);
    int wdata = (rdata & 0x3) | ((value & 0xf) << 2);
    elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

    elb_bx_apb_write(chip_id, inst_id, 0x1f10, wdata);

    bx_csr.cfg_fixer.read();
    // set the fixer timeout to 1024
    bx_csr.cfg_fixer.timeout(1024);
    bx_csr.cfg_fixer.write();
}

void elb_bx_set_tx_rx_enable(int chip_id, int inst_id, int value) {
    uint32_t addr = 0x2100;
    int pos = 0;
    int num_bits = 2;

    elb_bx_set_bits(chip_id, inst_id, addr, pos, num_bits, value);
}

void elb_bx_set_soft_reset(int chip_id, int inst_id, int value) { 
    // Software reset for stats
    elb_bx_apb_write(chip_id, inst_id, 0x2006, value);

    // umac software reset
    int rdata = elb_bx_apb_read(chip_id, inst_id, 0x1f10);
    int wdata = (rdata & 0xfd) | ((value & 0x1) << 1);
    elb_bx_apb_write(chip_id, inst_id, 0x1f10, wdata);
}

void elb_bx_set_mtu(int chip_id , int inst_id, int max_value, int jabber_value) {
   BX_DEBUG_MSG("BX%d ch%d elb_bx_set_mtu max_value = %d, jabber_value=%d\n", inst_id, 0, max_value, jabber_value);
   elb_bx_apb_write(chip_id, inst_id, 0x2103, max_value);
   elb_bx_apb_write(chip_id, inst_id, 0x2104, jabber_value);
   elb_bx_apb_write(chip_id, inst_id, 0x2105, jabber_value);
}

void elb_bx_set_pause(int chip_id , int inst_id, int pri_vec, int legacy) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

   int txfcxoff_enable = 0;
   int txpfcxoff_enable = 0;
   int addr1 = 0x2101;
   int addr2 = 0x2102;
   int addr3 = 0x2106;

   if (pri_vec == 0) {
      elb_bx_apb_write(chip_id, inst_id, addr1, 0x030);
      elb_bx_apb_write(chip_id, inst_id, addr2, 0x010);  // bit4 Promiscuous Mode (1: disable MAC address check)
      elb_bx_apb_write(chip_id, inst_id, addr3, 0x00);   // MAC Tx Priority Pause Vector
   } else {
      if (legacy) {
         txfcxoff_enable = 1;
         elb_bx_apb_write(chip_id, inst_id, addr1, 0x130);   // Bit8: Control Frame Generation Enable
         elb_bx_apb_write(chip_id, inst_id, addr2, 0x130);   // bit5: Enable Rx Flow Control Decode, bit8 filter pause frame
         elb_bx_apb_write(chip_id, inst_id, addr3, 0x00);    // MAC Tx Priority Pause Vector
      } else {
         txpfcxoff_enable = 0xff;
         elb_bx_apb_write(chip_id, inst_id, addr1, 0x230);   // bit9: Priority Flow Control Generation Enable
         elb_bx_apb_write(chip_id, inst_id, addr2, 0x130);   // bit5: Enable Rx Flow Control Decode, bit8 filter pause frame
         elb_bx_apb_write(chip_id, inst_id, addr3, pri_vec); // MAC Tx Priority Pause Vector
      }
   }

   bx_csr.cfg_mac_xoff.ff_txfcxoff_i(txfcxoff_enable);
   bx_csr.cfg_mac_xoff.ff_txpfcxoff_i(txpfcxoff_enable);
   bx_csr.cfg_mac_xoff.write();
   // bx_csr.cfg_mac_xoff.show();
}

void elb_bx_init_start(int chip_id, int inst_id) {
 BX_DEBUG_MSG("BX%d ch%d inside init start\n", inst_id, 0);

 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

 elb_bx_set_ch_enable(chip_id, inst_id, 0x0);
 BX_DEBUG_MSG("BX%d ch%d Set BX fixer timeout to 1024\n", inst_id, 0);
 bx_csr.cfg_fixer.timeout(1024);
 bx_csr.cfg_fixer.write();
}

void elb_bx_init_done(int chip_id, int inst_id) {
 BX_DEBUG_MSG("BX%d ch%d inside init done\n", inst_id, 0);
}

// unsupported ASIC verification functions
void elb_bx_show_mac_mode(int chip_id, int inst_id) {
}
void elb_bx_set_mac_mode(int chip_id, int inst_id, string new_mode) {
}
int elb_bx_get_port_enable_state(int chip_id, int inst_id, int port) {
   return -1;
}
int elb_bx_get_mac_detail(int chip_id, int inst_id, string field) {
   return -1;
}
void elb_bx_load_from_cfg(int chip_id, int inst_id) {
}

void elb_bx_eos(int chip_id, int inst_id) {
   elb_bx_eos_cnt(chip_id,inst_id);
   elb_bx_eos_int(chip_id,inst_id);
   elb_bx_eos_sta(chip_id,inst_id);
}

void elb_bx_mac_stat(int chip_id, int inst_id, int port, int short_report,
                     uint64_t *stats_data) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

   string stats[64] = {
   "A", "Frames Received OK",
   "A", "Frames Received All (Good/Bad Frames)",
   "A", "Frames Received with Bad FCS",
   "A", "Frames with any bad (CRC, Length, Align)",
   "A", "Octets Received in Good Frames",
   "A", "Octets Received (Good/Bad Frames)",
   "L", "Frames Received with Unicast Address",
   "L", "Frames Received with Multicast Address",
   "L", "Frames Received with Broadcast Address",
   "L", "Frames Received of type PAUSE",
   "L", "Frames Received with Bad Length",
   "L", "Frames Received Undersized",
   "L", "Frames Received Oversized",
   "L", "Fragments Received",
   "L", "Jabber Received",
   "L", "Frames Received Length=64",
   "L", "Frames Received Length=65~127",
   "L", "Frames Received Length=128~255",
   "L", "Frames Received Length=256~511",
   "L", "Frames Received Length=512~1023",
   "L", "Frames Received Length=1024~1518",
   "L", "Frames Received Length>=1518",
   "A", "Frames RX FIFO Full",
   "A", "Frames Transmitted OK",
   "A", "Frames Transmitted All (Good/Bad Frames)",
   "A", "Frames Transmitted Bad",
   "A", "Octets Transmitted Good",
   "A", "Octets Transmitted Total (Good/Bad)",
   "L", "Frames Transmitted with Unicast Address",
   "L", "Frames Transmitted with Multicast Address",
   "L", "Frames Transmitted with Broadcast Address",
   "L", "Frames Transmitted of type PAUSE"
   };
  
   for (int i = 0; i < 32; i += 1) {
      string report_type = stats[i*2];
      if (short_report == 1 && (report_type.compare("L") == 0)) continue;

      int addr = ((port&0x3) << 7) | i;
      bx_csr.dhs_mac_stats.entry[addr].read();
      cpp_int rdata = bx_csr.dhs_mac_stats.entry[addr].value();
      if (stats_data != NULL) {
          stats_data[i] = bx_csr.dhs_mac_stats.entry[addr].value().convert_to<uint64_t>();
      }
      string counter_name = stats[i*2+1];
      BX_DEBUG_MSG("BX%d ch%d %50s : %" PRIu64 "\n", inst_id, 0, counter_name.c_str(), (uint64_t) rdata);

   }
}

void elb_bx_eos_cnt(int chip_id, int inst_id) {
   elb_bx_mac_stat(chip_id,inst_id, 0, 1, NULL);  // port 0
}

void elb_bx_eos_int(int chip_id, int inst_id) {
}

void elb_bx_eos_sta(int chip_id, int inst_id) {
}

void elb_bx_set_an_ability(int chip_id, int inst_id, int value) {
 BX_DEBUG_MSG("BX%d ch%d BX: set an ability\n", inst_id, 0);
   elb_bx_apb_write(chip_id, inst_id, 0x4c01, value);
}

int elb_bx_rd_rx_an_ability(int chip_id, int inst_id) {
   BX_DEBUG_MSG("BX%d ch%d BX: read rx an_ability\n", inst_id, 0);
   int addr = 0x4c02;
   int value = elb_bx_apb_read(chip_id, inst_id, addr);
   return value;
}

void elb_bx_start_an(int chip_id, int inst_id) {
   BX_DEBUG_MSG("BX%d ch%d BX: start an\n", inst_id, 0);
   elb_bx_apb_write(chip_id, inst_id, 0x4c00, 0x3);
}

void elb_bx_stop_an(int chip_id, int inst_id) {
   BX_DEBUG_MSG("BX%d ch%d BX: stop an\n", inst_id, 0);
   elb_bx_apb_write(chip_id, inst_id, 0x4c00, 0x0);
}

void elb_bx_wait_an_done(int chip_id, int inst_id) {
   BX_DEBUG_MSG("BX%d ch%d BX: wait an done\n", inst_id, 0);
   int data;
   do {
     data = elb_bx_apb_read(chip_id, inst_id, 0x4c06);
   } while ((data & 0x1) == 0);
}

void elb_bx_set_an_link_timer(int chip_id, int inst_id, int value) {
   BX_DEBUG_MSG("BX%d ch%d BX: set an_ability = %d\n", inst_id, 0, value);
   int addr = 0x4c03;
   elb_bx_apb_write(chip_id, inst_id, addr, value & 0xffff);
   addr = 0x4c13;
   elb_bx_apb_write(chip_id, inst_id, addr, (value >> 16));
}

int elb_bx_check_tx_idle(int chip_id, int inst_id) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);
   bx_csr.sta_mac.read();
   bool tx_idle;
   tx_idle = (bx_csr.sta_mac.ff_txidle_o().convert_to<int>() == 1);

   return tx_idle ? 1 : 0;
}

int elb_bx_check_sync(int chip_id, int inst_id) {
 elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);
   bx_csr.sta_mac.read();
   bool rx_sync;
   rx_sync = (bx_csr.sta_mac.ff_rxsync_o().convert_to<int>() == 1);

   return rx_sync ? 1 : 0;
}

void elb_bx_set_tx_rx_enable(int chip_id, int inst_id, int tx_enable, int rx_enable) {

   BX_DEBUG_MSG("BX%d ch%d Set tx_enable = %d, rx_enable = %d\n", inst_id, 0, tx_enable, rx_enable);
   int addr = 0x2100;
   int wdata = (rx_enable & 0x1) << 1 | (tx_enable & 0x1);
   elb_bx_apb_write(chip_id, inst_id, addr, wdata);
}

void elb_bx_bist_run(int chip_id, int inst_id, int enable) {
  elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

  BX_DEBUG_MSG("BX%d ch%d elb_bx_bist_run, enable = %d\n", inst_id, 0, enable);

  bx_csr.cfg_ff_txfifo.read();
  bx_csr.cfg_ff_txfifo.bist_run(enable);
  bx_csr.cfg_ff_txfifo.write();
  bx_csr.cfg_ff_rxfifo.read();
  bx_csr.cfg_ff_rxfifo.bist_run(enable);
  bx_csr.cfg_ff_rxfifo.write();
  bx_csr.cfg_stats_mem.read();
  bx_csr.cfg_stats_mem.bist_run(enable);
  bx_csr.cfg_stats_mem.write();
}

void elb_bx_bist_chk(int chip_id, int inst_id) {
  elb_bx_csr_t & bx_csr = PEN_BLK_REG_MODEL_ACCESS(elb_bx_csr_t, chip_id, inst_id);

  BX_DEBUG_MSG("BX%d ch%d elb_bx_bist_chk\n", inst_id, 0);

  int loop_cnt = 0;
  do {
     bx_csr.sta_ff_txfifo.read();
     bx_csr.sta_ff_rxfifo.read();
     bx_csr.sta_stats_mem.read();
  } while((loop_cnt < 10000) &&
          ((bx_csr.sta_ff_txfifo.all() == 0) ||
           (bx_csr.sta_ff_rxfifo.all() == 0) ||
           (bx_csr.sta_stats_mem.all() == 0)));

  bool err = false;
  if (bx_csr.sta_ff_txfifo.bist_done_pass() != 0x1) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_ff_txfifo.bist_done_pass()\n", inst_id, 0);
    err = true;
  }
  if (bx_csr.sta_ff_txfifo.bist_done_fail() != 0x0) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_ff_txfifo.bist_done_fail()\n", inst_id, 0);
    err = true;
  }
  if (bx_csr.sta_ff_rxfifo.bist_done_pass() != 0x1) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_ff_rxfifo.bist_done_pass()\n", inst_id, 0);
    err = true;
  }
  if (bx_csr.sta_ff_rxfifo.bist_done_fail() != 0x0) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_ff_rxfifo.bist_done_fail()\n", inst_id, 0);
    err = true;
  }
  if (bx_csr.sta_stats_mem.bist_done_pass() != 0x1) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_stats_mem.bist_done_pass()\n", inst_id, 0);
    err = true;
  }
  if (bx_csr.sta_stats_mem.bist_done_fail() != 0x0) {
    BX_DEBUG_ERR("BX%d ch%d bx_csr.sta_stats_mem.bist_done_fail()\n", inst_id, 0);
    err = true;
  }
  if (!err) {
     BX_DEBUG_MSG("BX%d ch%d PASSED: elb_bx_bist_chk\n", inst_id, 0);
  }
}

int
elb_bx_tx_drain (int chip_id, int inst_id, int mac_ch, bool drain)
{
    int addr = 0;

    // MAC control bit 5
    addr = 0x2100;
    elb_bx_set_bit(chip_id, inst_id, addr, 5, drain);
    elb_bx_apb_read(chip_id, inst_id, addr);

    elb_bx_set_bit(chip_id, inst_id, addr, 0, drain == true? false : true);
    elb_bx_apb_read(chip_id, inst_id, addr);

    elb_bx_set_bit(chip_id, inst_id, addr, 1, drain == true? false : true);
    elb_bx_apb_read(chip_id, inst_id, addr);

    // MAC transmit config  bit 15
    addr = 0x2101;
    elb_bx_set_bit(chip_id, inst_id, addr, 15, drain);
    elb_bx_apb_read(chip_id, inst_id, addr);

    // MAC txdebug config  bit 6
    addr = 0x2191;
    elb_bx_set_bit(chip_id, inst_id, addr, 6, drain);
    elb_bx_apb_read(chip_id, inst_id, addr);
    return 0;
}

void
elb_bx_stats_reset (int chip_id, int inst_id, int ch, int value)
{
    int rdata;
    int wdata;
    int mask = ((1 << ch) ^ 0xf) & 0xf;
    int addr = 0x2000;

    rdata = elb_bx_apb_read(chip_id, inst_id, addr);
    wdata = (rdata & mask) | ((value << ch) & 0xf);
    elb_bx_apb_write(chip_id, inst_id, addr, wdata);
}
