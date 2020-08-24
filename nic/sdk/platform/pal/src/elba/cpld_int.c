/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */
#include "pal_cpld.h"
#include "pal_locks.h"
#include "pal.h"
#ifdef __x86_64__
void
cpld_reset(void)
{
}

bool cpld_verify_idcode(void)
{
    return false;
}

int cpld_erase(void)
{
    return -1;
}

int cpld_read_flash(uint8_t *buf, uint32_t len)
{
    return -1;
}

int
cpld_write_flash(const uint8_t *buf, uint32_t len, cpld_upgrade_status_cb_t cpld_upgrade_status_cb, void *arg)
{
    return -1;
}

int cpld_reg_bit_set(int reg, int bit)
{
    return -1;
}
int cpld_reg_bits_set(int reg, int bit, int nbits, int val)
{
    return -1;
}

int cpld_reg_bit_reset(int reg, int bit)
{
    return -1;
}

int cpld_reg_rd(uint8_t addr)
{
    return -1;
}

int cpld_reg_wr(uint8_t addr, uint8_t data)
{
    return -1;
}

int cpld_mdio_rd(uint8_t addr, uint16_t* data, uint8_t phy)
{
    return -1;
}

int cpld_mdio_wr(uint8_t addr, uint16_t data, uint8_t phy)
{
    return -1;
}

int
pal_write_gpios(int gpio, uint32_t data)
{
    return -1;
}
#else
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>

struct gpiohandle_request {
        __u32 lineoffsets[GPIOHANDLES_MAX];
        __u32 flags;
        __u8 default_values[GPIOHANDLES_MAX];
        char consumer_label[32];
        __u32 lines;
        int fd;
};
struct gpiohandle_data {
        __u8 values[GPIOHANDLES_MAX];
};

#define GPIOHANDLE_GET_LINE_VALUES_IOCTL _IOWR(0xB4, 0x08, struct gpiohandle_data)
#define GPIOHANDLE_SET_LINE_VALUES_IOCTL _IOWR(0xB4, 0x09, struct gpiohandle_data)
#define GPIO_GET_LINEHANDLE_IOCTL _IOWR(0xB4, 0x03, struct gpiohandle_request)
#define GPIO_GET_LINEEVENT_IOCTL _IOWR(0xB4, 0x04, struct gpioevent_request)

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))

static const char spidev0_path[] = "/dev/spidev0.0";
//static const char spidev1_path[] = "/dev/spidev0.1";
void
cpld_upgrade_status_cb_default(pal_cpld_status_t status, int percentage, void *ctxt)
{
    return;
}

// Elba: TODO
//static cpld_upgrade_status_cb_t g_cpld_upgrade_status_cb = cpld_upgrade_status_cb_default;

static int
write_gpios(int gpio, uint32_t data)
{
    struct gpiochip_info ci;
    struct gpiohandle_request hr;
    struct gpiohandle_data hd;
    int fd;

    memset(&hr, 0, sizeof (hr));
    //control only one gpio
    if (gpio > 7) {
        if ((fd = open("/dev/gpiochip1", O_RDWR, 0)) < 0) {
            return -10;
        }
        hr.lineoffsets[0] = gpio - 7;
    } else {
        if ((fd = open("/dev/gpiochip0", O_RDWR, 0)) < 0) {
            return -11;
        }
        hr.lineoffsets[0] = gpio;
    }
    if (ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &ci) < 0) {
        close(fd);
        return -12;
    }
    hr.flags = GPIOHANDLE_REQUEST_OUTPUT;
    hr.lines = 1;
    hd.values[0] = data;
    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &hr) < 0) {
        close(fd);
        return -13;
    }
    close(fd);
    if (ioctl(hr.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &hd) < 0) {
        close(hr.fd);
        return -14;
    }
    close(hr.fd);
    return 0;
}

#if 0
static int
read_gpios(int d, uint32_t mask)
{
    struct gpiochip_info ci;
    struct gpiohandle_request hr;
    struct gpiohandle_data hd;
    char buf[32];
    int value, fd, n, i;

    snprintf(buf, sizeof (buf), "/dev/gpiochip%d", d);
    if ((fd = open(buf, O_RDWR, 0)) <  0) {
        return -1;
    }
    if (ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &ci) < 0) {
        close(fd);
        return -1;
    }
    memset(&hr, 0, sizeof (hr));
    n = 0;
    for (i = 0; i < ci.lines; i++) {
        if (mask & (1 << i)) {
            hr.lineoffsets[n++] = i;
        }
    }
    hr.flags = GPIOHANDLE_REQUEST_INPUT;
    hr.lines = n;
    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &hr) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    if (ioctl(hr.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &hd) < 0) {
        close(hr.fd);
        return -1;
    }
    close(hr.fd);
    value = 0;
    for (i = hr.lines - 1; i >= 0; i--) {
        value = (value << 1) | (hd.values[i] & 0x1);
    }
    return value;
}
#endif

static int
cpid_spi_issue(const char *path, const uint8_t *txbuf, int txlen,
        uint8_t *rxbuf, int rxlen)
{
    struct spi_ioc_transfer msg[2];
    int fd, nmsgs, r;

    memset(msg, 0, sizeof (msg));
    nmsgs = 1;
    msg[0].tx_buf = (intptr_t)txbuf;
    msg[0].len = txlen;
    if (rxlen > 0) {
        msg[1].rx_buf = (intptr_t)rxbuf;
        msg[1].len = rxlen;
        ++nmsgs;
    }
    if ((fd = open(path, O_RDWR, 0)) < 0) {
        return -30;
    }
    r = ioctl(fd, SPI_IOC_MESSAGE(nmsgs), msg);
    close(fd);
    return (r < 0) ? -31 : 0;
}

static int
cpld_read(uint8_t addr)
{
    uint8_t txbuf[] = { 0x0b, addr, 0x00 };
    uint8_t rxbuf[1];
    int r;

    r = cpid_spi_issue(spidev0_path, txbuf, sizeof (txbuf),
            rxbuf, sizeof (rxbuf));
    return (r < 0) ? r : rxbuf[0];
}

static int
cpld_write(uint8_t addr, uint8_t data)
{
    uint8_t txbuf[] = { 0x02, addr, data, 0x00 };
    return cpid_spi_issue(spidev0_path, txbuf, sizeof (txbuf), NULL, 0);
}

/* Public APIs */
int
cpld_reg_bit_set(int reg, int bit)
{
    int cpld_data = 0;
    int mask = 0x01 << bit;
    int rc = -70;
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -71;
    }
    cpld_data = cpld_read(reg);
    if (cpld_data == -1) {
        if (!pal_wr_unlock(CPLDLOCK)) {
            pal_mem_trace("Failed to unlock.\n");
            return -72;
        }
        return cpld_data;
    }
    cpld_data |= mask;
    rc = cpld_write(reg, cpld_data);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -73;
    }
    return rc;
}

int
cpld_reg_bits_set(int reg, int bit, int nbits, int val)
{
    int cpld_data = 0;
    int mask = 0;
    int rc = -80;

    if (val >= (1 << nbits)) {
        pal_mem_trace("Incompatible value and mask\n");
        return -81;
    }

    mask = (1 << nbits) - 1;
    mask = mask << bit;

    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -82;
    }
    cpld_data = cpld_read(reg);
    if (cpld_data == -1) {
        if (!pal_wr_unlock(CPLDLOCK)) {
            pal_mem_trace("Failed to unlock.\n");
            return -83;
        }
        return cpld_data;
    }
    cpld_data = (cpld_data & ((0xff)^(mask))) | (val << bit);
    rc = cpld_write(reg, cpld_data);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -84;
    }
    return rc;
}

int
cpld_reg_bit_reset(int reg, int bit)
{
    int cpld_data = 0;
    int mask = 0x01 << bit;
    int rc = -90;
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -91;
    }
    cpld_data = cpld_read(reg);
    if (cpld_data == -1) {
        if (!pal_wr_unlock(CPLDLOCK)) {
            pal_mem_trace("Failed to unlock.\n");
            return -92;
        }
        return cpld_data;
    }
    cpld_data &= ~mask;
    rc = cpld_write(reg, cpld_data);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -93;
    }
    return rc;
}

int
cpld_reg_rd(uint8_t reg)
{
    int value = 0;
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -100;
    }
    value = cpld_read(reg);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -101;
    }
    return value;
}

int
cpld_reg_wr(uint8_t reg, uint8_t data)
{
    int rc = -110;
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -111;
    }
    rc = cpld_write(reg, data);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -112;
    }
    return rc;
}

int cpld_mdio_rd(uint8_t addr, uint16_t* data, uint8_t phy)
{
    uint8_t data_lo, data_hi;
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -120;
    }
    cpld_write(MDIO_CRTL_HI_REG, addr);
    cpld_write(MDIO_CRTL_LO_REG, (phy << 3) | MDIO_RD_ENA | MDIO_ACC_ENA);
    usleep(1000);
    cpld_write(MDIO_CRTL_LO_REG, 0);
    usleep(1000);
    data_lo = cpld_read(MDIO_DATA_LO_REG);
    data_hi = cpld_read(MDIO_DATA_HI_REG);
    *data = (data_hi << 8) | data_lo;
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
    }
    return 0;
}

int cpld_mdio_wr(uint8_t addr, uint16_t data, uint8_t phy)
{
    if (!pal_wr_lock(CPLDLOCK)) {
        pal_mem_trace("Could not lock pal.lck\n");
        return -130;
    }
    cpld_write(MDIO_CRTL_HI_REG, addr);
    cpld_write(MDIO_DATA_LO_REG, (data & 0xFF));
    cpld_write(MDIO_DATA_HI_REG, ((data >> 8) & 0xFF));
    cpld_write(MDIO_CRTL_LO_REG, (phy << 3) | MDIO_WR_ENA | MDIO_ACC_ENA);
    usleep(1000);
    cpld_write(MDIO_CRTL_LO_REG, 0);
    if (!pal_wr_unlock(CPLDLOCK)) {
        pal_mem_trace("Failed to unlock.\n");
        return -131;
    }
    return 0;
}

int
pal_write_gpios(int gpio, uint32_t data)
{
    return (write_gpios(gpio, data));
}

void
cpld_reload_reset(void)
{
    // TODO for Elba FPGA update
}

bool
cpld_verify_idcode(void)
{
    // TODO for Elba FPGA update
    return false;
}

int
cpld_erase(void)
{
    // TODO for Elba FPGA update
    return -1;
}

int
cpld_read_flash(uint8_t *buf, uint32_t len)
{
    // TODO for Elba FPGA update
    return -1;
}

int
cpld_write_flash(const uint8_t *buf, uint32_t len, cpld_upgrade_status_cb_t cpld_upgrade_status_cb, void *arg)
{
    // TODO for Elba FPGA update
    return -1;
}
#endif
