#ifndef _PAL_CPLD_H_
#define _PAL_CPLD_H_
#include <stdint.h>
#include <stdbool.h>
#include "pal_types.h"

#define CPLD_REGISTER_REVISION                0x00
#define CPLD_REGISTER_CTRL                    0x01
#define CPLD_REGISTER_QSFP_CTRL               0x02
#define CPLD_REGISTER_INTERRUPT_ENABLE        0x03
#define CPLD_REGISTER_INTERRUPT_STATUS        0x04
#define CPLD_REGISTER_QSFP_LED                0x05
#define CPLD_REGISTER_QSFP_LED_FREQUENCY      0x0F
#define CPLD_REGISTER_CTRL2                   0x12
#define CPLD_REGISTER_SYSTEM_LED              0x15
#define CPLD_REGISTER_CORE_TEMP               0x16
#define CPLD_REGISTER_HBM_TEMP                0x17
#define CPLD_REGISTER_BOARD_TEMP              0x18
#define CPLD_REGISTER_QSFP_PORT1_TEMP         0x19
#define CPLD_REGISTER_QSFP_PORT2_TEMP         0x1a
#define CPLD_REGISTER_HBM_WARNING_TEMP        0x1b
#define CPLD_REGISTER_HBM_CRITICAL_TEMP       0x1c
#define CPLD_REGISTER_HBM_FATAL_TEMP          0x1d
#define CPLD_REGISTER_EXT_INTERRUPT_ENABLE    0x1e
#define CPLD_REGISTER_EXT_INTERRUPT_STATUS    0x1f
#define CPLD_PERSISTENT_REG                   0x20
#define CPLD_REGISTER_QSFP_PORT1_ALARM_TEMP   0x34
#define CPLD_REGISTER_QSFP_PORT1_WARNING_TEMP 0x35
#define CPLD_REGISTER_QSFP_PORT2_ALARM_TEMP   0x36
#define CPLD_REGISTER_QSFP_PORT2_WARNING_TEMP 0x37
#define CPLD_REGISTER_SYSTEM_HEALTH0          0x38
#define CPLD_REGISTER_SYSTEM_HEALTH1          0x39
#define CPLD_REGISTER_MAJOR_FW_VER            0x3a
#define CPLD_REGISTER_MINOR_FW_VER            0x3b
#define CPLD_REGISTER_MAINTANENCE_FW_VER      0x3c
#define CPLD_REGISTER_PIPELINE_FW             0x3d
#define CPLD_CONF_FLASH_READ_BYTE             0x50
#define CPLD_DATA_CACHE_END_ADDR              0x5f
#define CPLD_REGISTER_ID                      0x80

#define CPLD_ALOM_PRESENT_BIT                 0x20

//CPLD ID and REV
#define CPLD_NAPLES_100_ID         0x12
#define CPLD_NAPLES_25_ID          0x13
#define CPLD_NAPLES_FORIO_ID       0x14
#define CPLD_NAPLES_VOMERO_ID      0x15
#define CPLD_ID_NAPLES25_SWM       0x17
#define CPLD_ID_NAPLES25_OCP       0x19
#define CPLD_REV_0                 0x0
#define CPLD_REV_8                 0x8
#define CPLD_NAPLES_VOMERO_2_ID    0x1e
#define CPLD_NAPLES_100_HPE_ID     0x1F
#define CPLD_NAPLES25_DELL_SWM_ID  0x20

// CPLD qsfp port number specifiers.
#define QSFP_PORT1   1
#define QSFP_PORT2   2

// CPLD bits for QSFP LED
#define QSFP_LED_PORT1_GREEN_ON   0
#define QSFP_LED_PORT1_YELLOW_ON  1
#define QSFP_LED_PORT2_GREEN_ON   2
#define QSFP_LED_PORT2_YELLOW_ON  3

// Defines used for QSFP Frequency
#define QSFP_PORT1_GREEN_BLINK_SHIFT             0
#define QSFP_PORT1_YELLOW_BLINK_SHIFT            2
#define QSFP_PORT2_GREEN_BLINK_SHIFT             4
#define QSFP_PORT2_YELLOW_BLINK_SHIFT            6

#define QSFP_PORT1_GREEN_BLINK_MASK         ~(0x3 << QSFP_PORT1_GREEN_BLINK_SHIFT)
#define QSFP_PORT1_YELLOW_BLINK_MASK        ~(0x3 << QSFP_PORT1_YELLOW_BLINK_SHIFT)
#define QSFP_PORT2_GREEN_BLINK_MASK         ~(0x3 << QSFP_PORT2_GREEN_BLINK_SHIFT)
#define QSFP_PORT2_YELLOW_BLINK_MASK        ~(0x3 << QSFP_PORT2_YELLOW_BLINK_SHIFT)

// Defines used for MDIO IO operations
#define MDIO_CRTL_LO_REG        0x6
#define MDIO_CRTL_HI_REG        0x7
#define MDIO_DATA_LO_REG        0x8
#define MDIO_DATA_HI_REG        0x9

#define MDIO_ACC_ENA            0x1
#define MDIO_RD_ENA             0x2
#define MDIO_WR_ENA             0x4

// Others
#define CPLD_MAX_RETRY              600
#define CPLD_HWLOCK_MASK            0x07
#define CPLD_READ_STATUS_MASK       0x31
#define CPLD_COMPLETION_STATUS_BIT1 0x80
#define CPLD_AUX_FAN_ON             0x10

#define CPLD_CFG_FLASH_SIZE         (16 * 2175)
#define CPLD_FLASH_TRANS_SIZE       16

// GPIO Definitions
#define GPIO_3_POWER_CYCLE 0x3
#define GPIO_EN 0x1

#define GPIOHANDLES_MAX 64
#define GPIOHANDLE_REQUEST_INPUT        (1UL << 0)
#define GPIOHANDLE_REQUEST_OUTPUT       (1UL << 1)
#define GPIOHANDLE_REQUEST_ACTIVE_LOW   (1UL << 2)
#define GPIOHANDLE_REQUEST_OPEN_DRAIN   (1UL << 3)
#define GPIOHANDLE_REQUEST_OPEN_SOURCE  (1UL << 4)

// CPLD Health0 Register Definitions 
#define SYSTEM_HEALTH0_CARD_STATUS      0
#define SYSTEM_HEALTH0_LIVENESS         1
#define SYSTEM_HEALTH0_PORT0_LINK       3
#define SYSTEM_HEALTH0_PORT1_LINK       4

// CPLD Link Status
#define PORT_LINK_DOWN     0
#define PORT_LINK_UP       1

typedef enum {
   SYSMOND_HEALTH_NOT_OK = 0,
   SYSMOND_HEALTH_OK = 1
} pal_cpld_health_t;

typedef void (*cpld_upgrade_status_cb_t)(pal_cpld_status_t, int, void *ctxt);

int cpld_reg_bit_set(int reg, int bit);
int cpld_reg_bits_set(int reg, int bit, int nbits, int val);
int cpld_reg_bit_reset(int reg, int bit);
int cpld_reg_rd(uint8_t addr);
int cpld_reg_wr(uint8_t addr, uint8_t data);
int cpld_mdio_rd(uint8_t addr, uint16_t* data, uint8_t phy);
int cpld_mdio_wr(uint8_t addr, uint16_t data, uint8_t phy);
int pal_write_gpios(int gpio, uint32_t data);
void cpld_reload_reset(void);
bool cpld_verify_idcode(void);
int cpld_erase(void);
int cpld_read_flash(uint8_t *buf, uint32_t len);
int cpld_write_flash(const uint8_t *buf, uint32_t len, cpld_upgrade_status_cb_t cpld_upgrade_status_cb, void *arg);
/* CPLD APIs */
int pal_is_qsfp_port_psnt(int port_no);
int pal_qsfp_set_port(int port);
int pal_qsfp_reset_port(int port);
int pal_qsfp_set_low_power_mode(int port);
int pal_qsfp_reset_low_power_mode(int port);
int pal_qsfp_set_led(int port, pal_led_color_t led, pal_led_frequency_t frequency);
int pal_system_set_led(pal_led_color_t led, pal_led_frequency_t frequency);
int pal_get_cpld_rev(void);
int pal_get_cpld_id(void);
void pal_write_core_temp(int data);
void pal_write_hbm_temp(int data);
void pal_write_board_temp(int data);
void pal_cpld_reload_reset(void);
bool pal_cpld_verify_idcode(void);
int pal_cpld_erase(void);
int pal_cpld_read_flash(uint8_t *buf, uint32_t size);
int pal_cpld_write_flash(const uint8_t *buf, uint32_t size, cpld_upgrade_status_cb_t cpld_upgrade_status_cb, void *arg);
void pal_cpld_set_card_status(uint8_t status);
void pal_cpld_increment_liveness(void);
void pal_cpld_set_port_link_status(uint32_t phy_port, uint8_t status);
void pal_power_cycle(void);
bool pal_cpld_hwlock_enabled(void);
int pal_write_qsfp_temp(int data, int port);
int pal_write_qsfp_alarm_temp(int data, int port);
int pal_write_qsfp_warning_temp(int data, int port);
void pal_write_hbmwarning_temp(int data);
void pal_write_hbmcritical_temp(int data);
void pal_write_hbmfatal_temp(int data);
int pal_enable_auxiliary_fan(void);
int pal_disable_auxiliary_fan(void);
int pal_write_fw_ver_to_cpld(uint32_t major_ver, uint32_t minor_ver,
                             uint32_t maint_ver, uint32_t pipeline);
#endif
