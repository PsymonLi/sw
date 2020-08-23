
/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __UBOOT_H__
#define __UBOOT_H__
    
/*
 * U-Boot Magic
 */
#define UBOOT_SIZE_MAGIC            0xfb89090a
#define UBOOT_CRC32_MAGIC           0xd8569817
#define UBOOT_PART_SIZE             (4 << 20)

#define BOOT0_MAGIC                 0x30f29e8b

#ifndef __ASSEMBLY__
struct uboot_header {
    uint32_t    inst;
    uint32_t    size_magic;
    uint32_t    size;
    uint32_t    reserved;
    uint32_t    crc_magic;
    uint32_t    crc;
};
#endif

#endif
