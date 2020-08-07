/*
 * Copyright (c) 2020 Pensando Systems. All rights reserved.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <linux/vfio.h>


/* Supply these for ionic_if.h */
typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int16_t __le16;
typedef u_int32_t u32;
typedef u_int32_t __le32;
typedef u_int64_t u64;
// typedef u_int64_t __le64;
typedef u_int64_t dma_addr_t;
typedef u_int64_t phys_addr_t;
typedef u_int64_t cpumask_t;
#define BIT(n)  (1 << (n))
#define BIT_ULL(n)  (1ULL << (n))
typedef u8 bool;
#define false 0
#define true 1
#define __iomem
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include "ionic_if.h"

#define IONIC_DEV_CMD_DONE              0x00000001
#define DEVCMD_TIMEOUT                  (5)
#define IONIC_FW_INSTALL_TIMEOUT        (25 * 60)
#define IONIC_FW_ACTIVATE_TIMEOUT       (30)

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

int is_kmod_loaded(const char* name)
{
    int ret;
    char cmd[PATH_MAX] = {0};

    ret = snprintf(cmd, PATH_MAX, "lsmod | grep -s %s > /dev/null", name);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to check if module %s is loaded\n", name);
        return -1;
    }

    ret = system(cmd);
    if (WEXITSTATUS(ret)) {
        fprintf(stderr, "Module %s is not loaded\n", name);
        return WEXITSTATUS(ret);
    }

    return 0;
}

int load_kmod(const char *name)
{
    int ret;
    char cmd[PATH_MAX] = {0};

    fprintf(stdout, "Loading module %s\n", name);

    ret = is_kmod_loaded(name);
    if (ret == 0) {
        fprintf(stdout, "Module %s is already loaded\n", name);
        return 0;
    }

    ret = snprintf(cmd, PATH_MAX, "modprobe %s", name);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create command to load module %s\n", name);
        return -1;
    }

    ret = system(cmd);
    if (WEXITSTATUS(ret)) {
        fprintf(stderr, "Failed to load module %s, err %d\n", name, ret);
        return WEXITSTATUS(ret);
    }

    return 0;
}

int unload_kmod(const char *name)
{
    int ret;
    char cmd[PATH_MAX] = {0};

    fprintf(stdout, "Unloading module %s\n", name);

    ret = is_kmod_loaded(name);
    if (ret > 0) {
        fprintf(stdout, "Module %s is not loaded\n", name);
        return 0;
    }

    ret = snprintf(cmd, PATH_MAX, "rmmod %s", name);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create command to unload module %s\n", name);
        return -1;
    }

    ret = system(cmd);
    if (WEXITSTATUS(ret)) {
        fprintf(stderr, "Failed to unload module %s, err %d\n", name, ret);
        return WEXITSTATUS(ret);
    }

    return 0;
}

int check_path(const char *path, int amode)
{
    int ret;
    struct stat status;

    ret = stat(path, &status);
    if (ret) {
        fprintf(stderr, "Failed to check status of path %s, err %d\n",
            path, errno);
        return -errno;
    }

    if ((amode & F_OK) && !(status.st_mode & F_OK)) {
        fprintf(stderr, "Path %s does not exist!\n", path);
        return -1;
    }

    if ((amode & R_OK) && !(status.st_mode & R_OK)) {
        fprintf(stderr, "Path %s is not readable\n", path);
        return -2;
    }

    if ((amode & W_OK) && !(status.st_mode & W_OK)) {
        fprintf(stderr, "Path %s is not writeable\n", path);
        return -3;
    }

    if ((amode & X_OK) && !(status.st_mode & X_OK)) {
        fprintf(stderr, "Path %s is not executable\n", path);
        return -4;
    }

    return 0;
}

int add_devid_to_driver(const char *driver, const char *dev_id)
{
    int fd;
    int ret;
    char path[PATH_MAX] = {0};

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s/new_id", driver);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for driver new id\n");
        return -1;
    }

    if (!check_path(path, F_OK)) {
        fprintf(stdout, "Setting new id %s for driver %s\n", dev_id, driver);

        fd = open(path, O_WRONLY);
        if (fd < 0) {
            fprintf(stderr, "Failed to open syspath for new id: %s\n",
                strerror(errno));
            return -errno;
        }

        if (write(fd, dev_id, strlen(dev_id)) != strlen(dev_id)) {
            close(fd);
            fprintf(stderr, "Failed to write syspath for new id: %s\n",
                strerror(errno));
            return -errno;
        }

        close(fd);
    }

    return 0;
}

int remove_devid_from_driver(const char *driver, const char *dev_id)
{
    int fd;
    int ret;
    char path[PATH_MAX] = {0};

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s/remove_id", driver);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for driver remove id\n");
        return -1;
    }

    if (!check_path(path, F_OK)) {
        fprintf(stdout, "Setting remove id %s for driver %s\n", dev_id, driver);

        fd = open(path, O_WRONLY);
        if (fd < 0) {
            fprintf(stderr, "Failed to open syspath for remove id: %s\n",
                strerror(errno));
            return -errno;
        }

        if (write(fd, dev_id, strlen(dev_id)) != strlen(dev_id)) {
            close(fd);
            fprintf(stderr, "Failed to write syspath for remove id: %s\n",
                strerror(errno));
            return -errno;
        }

        close(fd);
    }

    return 0;
}

int set_driver_override(const char *driver, const char *sbdf)
{
    int fd;
    int ret;
    char buf[PATH_MAX] = {0};
    char path[PATH_MAX] = {0};

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/driver_override", sbdf);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for driver override\n");
        return -1;
    }

    if (!check_path(path, F_OK)) {
        fprintf(stdout, "Setting override driver %s for device %s\n", driver, sbdf);

        fd = open(path, O_RDWR);
        if (fd < 0) {
            fprintf(stderr, "Failed to open syspath for driver override: %s\n",
                strerror(errno));
            return -errno;
        }

        if (read(fd, buf, sizeof(buf)) > 0) {
            if (strcmp(buf, driver) == 0) {
                fprintf(stdout, "Driver override %s is already set for device %s\n",
                    driver, sbdf);
                return 0;
            }
        }

        if (write(fd, driver, strlen(driver)) != strlen(driver)) {
            close(fd);
            fprintf(stderr, "Failed to write syspath for driver override: %s\n",
                strerror(errno));
            return -errno;
        }

        close(fd);
    }

    return 0;
}

int bind_pcidev(const char *sbdf, const char *driver, const char *dev_id)
{
    int fd;
    int ret;
    char path[PATH_MAX] = {0};

    fprintf(stdout, "Binding device %s sbdf %s to driver %s\n",
        dev_id, sbdf, driver);

    if (set_driver_override(driver, sbdf)) {
        if (add_devid_to_driver(driver, dev_id)) {
            return -1;
        }
    }

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s/bind", driver);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for binding device\n");
        return -1;
    }

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open syspath for binding device: %s\n",
            strerror(errno));
        return -errno;
    }

    if (write(fd, sbdf, strlen(sbdf)) != strlen(sbdf)) {
        close(fd);
        fprintf(stderr, "Failed to write syspath for binding device: %s\n",
            strerror(errno));
        return -errno;
    }

    close(fd);

    return 0;
}

int
get_current_driver(const char *sbdf, char *driver)
{
    int ret;
    char buf[PATH_MAX];
    char path[PATH_MAX];

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/driver", sbdf);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for current driver\n");
        return -1;
    }

    ret = readlink(path, buf, sizeof(buf));
    if (ret >= sizeof(buf)) {
        fprintf(stderr, "Failed to readlink for current driver: %s\n",
            strerror(errno));
        return -errno;
    }

    buf[ret] = 0;
    strcpy(driver, basename(buf));
    return 0;
}

/* Unbinds pci device from it's current driver */
int unbind_pcidev(const char *sbdf, const char *dev_id)
{
    int fd;
    int ret;
    char driver[PATH_MAX];
    char path[PATH_MAX];

    ret = get_current_driver(sbdf, driver);
    if (ret) {
        fprintf(stderr, "Failed to get current driver name\n");
        return -1;
    }

    fprintf(stdout, "Unbinding device %s from driver %s\n", sbdf, driver);

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s/unbind", driver);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Failed to create syspath for unbinding device\n");
        return -1;
    }

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open syspath for unbinding device: %s\n",
            strerror(errno));
        return -errno;
    }

    if (write(fd, sbdf, strlen(sbdf)) != strlen(sbdf)) {
        close(fd);
        fprintf(stderr, "Failed to write syspath for unbinding device: %s\n",
            strerror(errno));
        return -errno;
    }

    close(fd);

    if (set_driver_override("", sbdf)) {
        if (remove_devid_from_driver(driver, dev_id)) {
            /* ignore errors */
            return 0;
        }
    }

    return 0;
}

bool is_iommu_enabled()
{
    bool intel_iommu = !check_path("/sys/firmware/acpi/tables/DMAR", F_OK);
    bool amd_iommu = !check_path("/sys/firmware/acpi/tables/IVRS", F_OK);

    if (!intel_iommu && !amd_iommu) {
        fprintf(stderr, "No ACPI iommu table found\n");
        return false;
    }

    DIR *dir = opendir("/sys/class/iommu");
    if (dir == NULL)
        return false;

    int n = 0;
    while (readdir(dir) != NULL) {
        if (++n > 2)
            break;
    }
    closedir(dir);

    return (n > 2);
}

int get_iommu_group(const char *sbdf)
{
    int ret;
    int group_id = 0;
    char *group_name;
    char path[PATH_MAX] = {0};
    char iommu_group_path[PATH_MAX] = {0};

    ret = snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/iommu_group",
        sbdf);
    if (ret >= sizeof(path)) {
        fprintf(stderr, "Failed to create syspath for iommu group\n");
        return -1;
    }

    ret = readlink(path, iommu_group_path, PATH_MAX);
    if (ret <= 0) {
        fprintf(stderr, "No iommu group for device %s\n",
            sbdf);
        return -errno;
    }

    iommu_group_path[ret] = 0;
    group_name = basename(iommu_group_path);

    if (sscanf(group_name, "%d", &group_id) != 1) {
        fprintf(stderr, "Failed to decode id from group %s", group_name);
        return -errno;
    }

    return group_id;
}

struct vfio_dev {
    int fd;
    int group_fd;
    int container_fd;
    bool iommu_enabled;
    struct vfio_iommu_type1_dma_map regions[VFIO_PCI_NUM_REGIONS];
};

struct vfio_dev *
setup_vfio_device(const char *sbdf, const char *dev_id)
{
    int ret;
    int container_fd;
    int group_fd;
    int group_id;
    int device_fd;
    int i;
    bool iommu_enabled;
    char path[PATH_MAX] = {0};
    struct vfio_dev *dev = NULL;
    struct vfio_group_status group_status = { .argsz = sizeof(group_status) };
    struct vfio_iommu_type1_info iommu_info = { .argsz = sizeof(iommu_info) };
    struct vfio_device_info dev_info = { .argsz = sizeof(dev_info) };
    struct vfio_region_info reg = { .argsz = sizeof(reg) };

    /* Create a new container */
    container_fd = open("/dev/vfio/vfio", O_RDWR);
    if (container_fd < 0) {
        fprintf(stderr, "Failed to open /dev/vfio/vfio\n");
        return NULL;
    }

    if (ioctl(container_fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
        fprintf(stderr, "Unknown API version\n");
        return NULL;
    }

    /* Check if iommu is enabled */
    iommu_enabled = is_iommu_enabled();

    if (iommu_enabled) {
        fprintf(stdout, "IOMMU is enabled\n");

        /* Check if vfio supports the iommu we want */
        if (!ioctl(container_fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
            fprintf(stderr, "Doesn't support the IOMMU driver we want.\n");
            return NULL;
        }

        /* Find the device's iommu group */
        group_id = get_iommu_group(sbdf);
        if (group_id <= 0) {
            fprintf(stderr, "Failed to get iommu group for device!\n");
            return NULL;
        }

        fprintf(stdout, "Device %s is in iommu group %d\n", sbdf, group_id);

        /* Unbind pci device from it's current driver */
        unbind_pcidev(sbdf, dev_id);

        /* Bind pci device to vfio-pci */
        if (bind_pcidev(sbdf, "vfio-pci", dev_id)) {
            fprintf(stderr, "Failed to bind device to vfio-pci!\n");
            return NULL;
        }

        /* Set the vfio iommu group path */
        ret = snprintf(path, sizeof(path), "/dev/vfio/%d", group_id);
        if (ret >= sizeof(path)) {
            fprintf(stderr, "Failed to create syspath for iommu group\n");
            return NULL;
        }

    } else {
        fprintf(stdout, "IOMMU is not enabled\n");

        /* Enable vfio no-iommu mode */
        int fd = open("/sys/module/vfio/parameters/enable_unsafe_noiommu_mode",
                    O_WRONLY);
        if (fd < 0) {
            fprintf(stderr, "Failed to open vfio no-iommu mode param syspath\n");
            return NULL;
        }

        if (write(fd, "Y", 1) != 1) {
            close(fd);
            fprintf(stderr, "Failed to enable VFIO no-iommu mode%s\n", path);
            return NULL;
        }
        close(fd);

        /* Unbind pci device from it's current driver */
        unbind_pcidev(sbdf, dev_id);

        /* Bind pci device to vfio-pci */
        if (bind_pcidev(sbdf, "vfio-pci", dev_id)) {
            fprintf(stderr, "Failed to bind device to vfio-pci!\n");
            return NULL;
        }

        /* Set the vfio no-iommu group path */
        ret = snprintf(path, sizeof(path), "/dev/vfio/noiommu-0");
        if (ret >= sizeof(path)) {
            fprintf(stderr, "Failed to create syspath for iommu group\n");
            return NULL;
        }
    }

    group_fd = open(path, O_RDWR);
    if (group_fd < 0) {
        fprintf(stderr, "Failed to open %s\n", path);
        return NULL;
    }

    if (ioctl(group_fd, VFIO_GROUP_GET_STATUS, &group_status)) {
        fprintf(stderr, "Failed to get group status\n");
        return NULL;
    }

    if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
        fprintf(stderr, "Group is not viable\n");
        return NULL;
    }

    if (ioctl(group_fd, VFIO_GROUP_SET_CONTAINER, &container_fd)) {
        fprintf(stderr, "Failed to add group to container\n");
        return NULL;
    }

    if (iommu_enabled) {
        if (ioctl(container_fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU)) {
            fprintf(stderr, "Failed to set iommu for container\n");
            return NULL;
        }

        if (ioctl(container_fd, VFIO_IOMMU_GET_INFO, &iommu_info)) {
            fprintf(stderr, "Failed to get iommu information\n");
            return NULL;
        }
    } else {
        if (ioctl(container_fd, VFIO_SET_IOMMU, VFIO_NOIOMMU_IOMMU)) {
            fprintf(stderr, "Failed to set iommu for container\n");
            return NULL;
        }
    }

    /* Get a file descriptor for the device */
    device_fd = ioctl(group_fd, VFIO_GROUP_GET_DEVICE_FD, sbdf);
    if (device_fd < 0) {
        fprintf(stderr, "Failed to open device\n");
        return NULL;
    }

    /* Test and setup the device */
    if (ioctl(device_fd, VFIO_DEVICE_GET_INFO, &dev_info)) {
        fprintf(stderr, "Failed to get device info\n");
        return NULL;
    }

    if (!(dev_info.flags & VFIO_DEVICE_FLAGS_PCI)) {
        fprintf(stderr, "This is not a pci device\n");
        return NULL;
    }

    dev = calloc(1, sizeof(struct vfio_dev));
    if (dev == NULL) {
        fprintf(stderr, "Failed to allocate device struct\n");
        return NULL;
    }

    dev->fd = device_fd;
    dev->group_fd = group_fd;
    dev->container_fd = container_fd;
    dev->iommu_enabled = iommu_enabled;

    for (i = VFIO_PCI_BAR0_REGION_INDEX; i < VFIO_PCI_NUM_REGIONS; i++) {
        reg.index = i;

        if (ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, &reg)) {
            fprintf(stdout, "Found %i regions\n", reg.index);
            break;
        }

        fprintf(stdout, "Region: index %d size 0x%llx offset 0x%llx flags %s%s%s%s\n",
            reg.index, reg.size, reg.offset,
            (reg.flags & VFIO_REGION_INFO_FLAG_READ) ? "R" : "",
            (reg.flags & VFIO_REGION_INFO_FLAG_WRITE) ? "W" : "",
            (reg.flags & VFIO_REGION_INFO_FLAG_MMAP) ? "M" : "",
            (reg.flags & VFIO_REGION_INFO_FLAG_CAPS) ? "C" : "");


        if (reg.flags & VFIO_REGION_INFO_FLAG_MMAP) {
            int prot = 0;
            if (reg.flags & VFIO_REGION_INFO_FLAG_READ) {
                prot |= PROT_READ;
            }
            if (reg.flags & VFIO_REGION_INFO_FLAG_WRITE) {
                prot |= PROT_WRITE;
            }

            void *map = mmap(NULL, min(reg.size, 4096), prot, MAP_SHARED,
                device_fd, reg.offset);
            if (map == MAP_FAILED) {
                fprintf(stderr, "Failed to map region %d: %s\n",
                    reg.index, strerror(errno));
                return NULL;
            }

            struct vfio_iommu_type1_dma_map *region = &dev->regions[reg.index];
            region->argsz = sizeof(struct vfio_iommu_type1_dma_map);
            region->flags = reg.flags;
            region->size = reg.size;
            region->iova = 0;
            region->vaddr = (__u64)map;

            if (!ioctl(container_fd, VFIO_IOMMU_MAP_DMA, region)) {
                fprintf(stderr, "Failed to DMA map region\n");
                return NULL;
            }

            fprintf(stdout, "Mapped: index %d vaddr 0x%llx len 0x%llx\n",
                reg.index,
                region->vaddr, region->size);
        }
    }

    return dev;
}

int
teardown_vfio_device(struct vfio_dev *dev, const char *sbdf, const char *dev_id)
{
    int i;
    struct vfio_iommu_type1_dma_unmap dma_unmap;

    for (i = VFIO_PCI_BAR0_REGION_INDEX; i < VFIO_PCI_NUM_REGIONS; i++) {
        struct vfio_iommu_type1_dma_map *region = &dev->regions[i];
        if (region->vaddr == (unsigned long long)MAP_FAILED ||
            region->size == 0)
            continue;

        if (dev->iommu_enabled) {
            dma_unmap.argsz = sizeof(struct vfio_iommu_type1_dma_unmap);
            dma_unmap.size = region->size;
            dma_unmap.iova = region->iova;
            if (ioctl(dev->container_fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)) {
                fprintf(stderr, "Failed to DMA unmap region\n");
            }
        }

        munmap((void *)region->vaddr, min(region->size, 4096));
        fprintf(stderr, "Unmapped: index %d vaddr 0x%llx len 0x%llx\n",
            i, region->vaddr, region->size);

        memset(region, 0, sizeof(*region));
    }

    close(dev->fd);
    close(dev->group_fd);
    close(dev->container_fd);

    /* Unbind pci device from it's current driver */
    unbind_pcidev(sbdf, dev_id);

    return 0;
}

void ionic_dev_cmd_go(union ionic_dev_regs *dev_regs,
    union ionic_dev_cmd *cmd)
{
    unsigned int i;
    union ionic_dev_cmd_regs *cmd_regs = &dev_regs->devcmd;

    for (i = 0; i < ARRAY_SIZE(cmd->words); i++)
        cmd_regs->cmd.words[i] = cmd->words[i];

    cmd_regs->done = 0;
    cmd_regs->doorbell = 1;
}

int ionic_dev_cmd_wait(union ionic_dev_regs *dev_regs, int timeo_s,
    union ionic_dev_cmd_comp *comp)
{
    unsigned int i;
    unsigned int max_retries = timeo_s * 1e6;
    union ionic_dev_cmd_regs *cmd_regs = &dev_regs->devcmd;

retry_wait:
    i = 0;
    while (!(cmd_regs->done & IONIC_DEV_CMD_DONE) && i < max_retries) {
        usleep(100);
        i++;
    }

    if (cmd_regs->done == 0) {
        fprintf(stderr, "Devcmd timeout!\n");
        return -ETIMEDOUT;
    }

    for (i = 0; i < ARRAY_SIZE(comp->words); i++)
        comp->words[i] = cmd_regs->comp.words[i];

    if (comp->status == IONIC_RC_EAGAIN) {
        cmd_regs->done = 0;
        cmd_regs->doorbell = 1;
        goto retry_wait;
    }

    return comp->status;
}

static void
ionic_dev_cmd_firmware_download(union ionic_dev_regs *dev_regs, uint64_t addr,
                uint32_t offset, uint32_t length)
{
    union ionic_dev_cmd cmd = {
        .fw_download.opcode = IONIC_CMD_FW_DOWNLOAD,
        .fw_download.offset = offset,
        .fw_download.addr = addr,
        .fw_download.length = length
    };

    ionic_dev_cmd_go(dev_regs, &cmd);
}

static void
ionic_dev_cmd_firmware_install(union ionic_dev_regs *dev_regs)
{
    union ionic_dev_cmd cmd = {
        .fw_control.opcode = IONIC_CMD_FW_CONTROL,
        .fw_control.oper = IONIC_FW_INSTALL_ASYNC
    };

    ionic_dev_cmd_go(dev_regs, &cmd);
}

static void
ionic_dev_cmd_firmware_status(union ionic_dev_regs *dev_regs)
{
    union ionic_dev_cmd cmd = {
        .fw_control.opcode = IONIC_CMD_FW_CONTROL,
        .fw_control.oper = IONIC_FW_INSTALL_STATUS
    };

    ionic_dev_cmd_go(dev_regs, &cmd);
}

static void
ionic_dev_cmd_firmware_activate(union ionic_dev_regs *dev_regs, uint8_t slot)
{
    union ionic_dev_cmd cmd = {
        .fw_control.opcode = IONIC_CMD_FW_CONTROL,
        .fw_control.oper = IONIC_FW_ACTIVATE,
        .fw_control.slot = slot
    };

    ionic_dev_cmd_go(dev_regs, &cmd);
}

int
ionic_firmware_update(union ionic_dev_regs *dev_regs, const void *const fw_data, u32 fw_sz)
{
    union ionic_dev_cmd_comp comp;
    u32 buf_sz, copy_sz, offset;
    int err = 0;
    u8 fw_slot;

    buf_sz = sizeof(dev_regs->devcmd.data);

    fprintf(stdout,
        "uploading firmware - size %d part_sz %d nparts %d\n",
        fw_sz, buf_sz, DIV_ROUND_UP(fw_sz, buf_sz));

    offset = 0;
    while (offset < fw_sz) {
        copy_sz = min(buf_sz, fw_sz - offset);
        memcpy(&dev_regs->devcmd.data, fw_data + offset, copy_sz);
        ionic_dev_cmd_firmware_download(dev_regs,
            offsetof(union ionic_dev_cmd_regs, data), offset, copy_sz);
        err = ionic_dev_cmd_wait(dev_regs, DEVCMD_TIMEOUT, &comp);
        if (err) {
            fprintf(stderr,
                "download failed offset 0x%x addr 0x%lx len 0x%x\n",
                offset, offsetof(union ionic_dev_cmd_regs, data), copy_sz);
            goto err_out;
        }
        offset += copy_sz;
    }

    fprintf(stdout, "installing firmware\n");

    ionic_dev_cmd_firmware_install(dev_regs);
    err = ionic_dev_cmd_wait(dev_regs, DEVCMD_TIMEOUT, &comp);
    fw_slot = comp.fw_control.slot;
    if (err) {
        fprintf(stderr, "failed to start firmware install\n");
        goto err_out;
    }

    ionic_dev_cmd_firmware_status(dev_regs);
    err = ionic_dev_cmd_wait(dev_regs, IONIC_FW_INSTALL_TIMEOUT, &comp);
    if (err) {
        fprintf(stderr, "firmware install failed\n");
        goto err_out;
    }

    fprintf(stdout, "activating firmware - slot %d\n", fw_slot);

    ionic_dev_cmd_firmware_activate(dev_regs, fw_slot);
    err = ionic_dev_cmd_wait(dev_regs, IONIC_FW_ACTIVATE_TIMEOUT, &comp);
    if (err) {
        fprintf(stderr, "firmware activation failed\n");
        goto err_out;
    }

err_out:
    return err;
}

void usage()
{
    fprintf(stderr, "Usage: <mgmt_dev_bdf> <fw_image>\n");
}

int main(int argc, const char *argv[])
{
    const char *sbdf;
    const char *fw_fpath;
    struct vfio_dev *dev;

    if (argc < 2) {
        usage();
        return -1;
    }

    sbdf = argv[1];
    fw_fpath = argv[2];

    if (getuid() != 0) {
        fprintf(stderr, "Please run this program as root!\n");
        return -2;
    }

    if (load_kmod("vfio_pci")) {
        fprintf(stderr, "Cannot continue, aborting!\n");
        return -3;
    }

    if (check_path("/dev/vfio/vfio", F_OK | R_OK | W_OK)) {
        fprintf(stderr, "Cannot continue, aborting!\n");
        return -4;
    }

    dev = setup_vfio_device(sbdf, "1dd8:1004");
    if (!dev) {
        fprintf(stderr, "Cannot continue, aborting!\n");
        return -6;
    }

    union ionic_dev_regs *dev_regs = (union ionic_dev_regs *)dev->regions[VFIO_PCI_BAR0_REGION_INDEX].vaddr;
    if (dev_regs->info.signature != IONIC_DEV_INFO_SIGNATURE) {
        fprintf(stderr,
            "Signature mismatch. Expected 0x%08x, got 0x%08x\n",
            IONIC_DEV_INFO_SIGNATURE, dev_regs->info.signature);
        return -7;
    }

    int fw_fd = open(fw_fpath, O_RDONLY);
    if (fw_fd < 0) {
        fprintf(stderr, "Failed to open firmware file: %s\n", strerror(errno));
        return -8;
    }
    // size_t pagesize = getpagesize();

    struct stat status;
    if (fstat(fw_fd, &status)) {
        fprintf(stderr, "Failed to stat firmware file: %s\n", strerror(errno));
        return -9;
    }

    char *fw_data = mmap(
        /*(void *)(pagesize * (1 << 20))*/
        NULL, status.st_size,
        PROT_READ, MAP_FILE | MAP_PRIVATE,
        fw_fd, 0);

    ionic_firmware_update(dev_regs, fw_data, status.st_size);

    munmap(fw_data, status.st_size);
    close(fw_fd);

    teardown_vfio_device(dev, sbdf, "1dd8:1004");

    return 0;
}
