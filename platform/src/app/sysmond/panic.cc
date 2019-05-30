/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#include "sysmond.h"
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#define PROC_MTD "/proc/mtd"
#define PANIC_BUF "panicbuf"
#define PANIC_KDUMP_FILE "/obfl/kdump.log"
#define PANIC_KDUMP_OLD "/obfl/kdump.log.1"
#define PANIC_SIGNATURE 0x9D7A7318

struct panicbuf_header {
    uint32_t magic;
    uint32_t len;
};

static void 
getpanicmtd(string& mtdname)
{
    FILE *fptr = NULL;
    char mtdline[100];

    fptr = fopen(PROC_MTD, "r");
    if (fptr != NULL) {
        while (fgets(mtdline, sizeof(mtdline), fptr) != NULL) {
            if (strstr(mtdline, PANIC_BUF) == NULL) {
                continue;
            }
            mtdname = strtok(mtdline, ":");
            break;
        }
        fclose(fptr);
    }
}

void
checkpanicdump(void)
{
    int fd;
    FILE *kfptr;
    uint8_t *kdump;
    string mtdname;
    char filename[32];
    struct panicbuf_header hdr;
    mtd_info_t mtd_info;
    erase_info_t mtd_erase;

    getpanicmtd(mtdname);
    if (mtdname.empty()) {
        TRACE_INFO(GetLogger(), "Could not find MTD name");
        return;
    }

    snprintf(filename, sizeof(filename), "/dev/%s", mtdname.c_str());
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        TRACE_INFO(GetLogger(), "Error opening {}", filename);
        return;
    }

    // read the header
    if (read(fd, &hdr, sizeof(panicbuf_header))  < 0) {
        TRACE_INFO(GetLogger(), "Error reading {}", filename);
        goto exit;
    }
    if (hdr.magic == PANIC_SIGNATURE) {
        memset(&mtd_info, 0, sizeof(mtd_info));
        // get the MTD info and validate length
        ioctl(fd, MEMGETINFO, &mtd_info);
        // validate size
        if (hdr.len + sizeof(hdr) > mtd_info.size) {
            TRACE_INFO(GetLogger(), "Invalid length {}", hdr.len);
            goto exit;
        }

        // allocate memory for the data and read the data
        kdump = (uint8_t *)malloc(hdr.len);
        if (kdump == NULL) {
            TRACE_INFO(GetLogger(), "Error allocating {} bytes", hdr.len);
            goto exit;
        }
        if (read(fd, kdump, hdr.len) < 0) {
            TRACE_INFO(GetLogger(), "Error reading data from {}", filename);
            goto exit;
        }

        // rename kdump.log to kdump.log.1
        rename(PANIC_KDUMP_FILE, PANIC_KDUMP_OLD);

        // write data to the file
        kfptr = fopen(PANIC_KDUMP_FILE, "w");
        if (kfptr == NULL) {
            TRACE_INFO(GetLogger(), "Error opening {}", PANIC_KDUMP_FILE);
            goto exit;
        }
        if (fwrite(kdump, sizeof(uint8_t), hdr.len, kfptr) != hdr.len) {
            TRACE_INFO(GetLogger(), "Error writing to {}", PANIC_KDUMP_FILE);
            fclose(kfptr);
            goto exit;
        }
        fclose(kfptr);

        // erase the flash part
        mtd_erase.length = mtd_info.erasesize;
        for (mtd_erase.start = 0; mtd_erase.start < mtd_info.size;
            mtd_erase.start += mtd_erase.length) {
                ioctl(fd, MEMUNLOCK, &mtd_erase);
                ioctl(fd, MEMERASE, &mtd_erase);
        }
        TRACE_INFO(GetLogger(), "{} flash erased and {} created",
                   PANIC_BUF, PANIC_KDUMP_FILE);
    }
exit:
    close(fd);
}
