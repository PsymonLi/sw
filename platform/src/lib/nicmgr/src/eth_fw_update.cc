/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include "nic/sdk/lib/thread/thread.hpp"
#include "dev.hpp"
#include "eth_fw_update.hpp"
#include "eth_if.h"
#include "nicmgr_utils.hpp"

#define FW_UPDATE_DEV_NONE  ""

enum fw_install_state {
    FW_INSTALL_NONE = 0,
    FW_INSTALL_IN_PROGRESS,
    FW_INSTALL_DONE,
    FW_INSTALL_ERR
};

struct fw_install_args {
    int seq_no;
    string dev_name;
};

static uint64_t fw_buf_addr;
static uint32_t fw_buf_size;
static uint32_t fw_buf_off;
static uint8_t *fw_buf;
static FILE *fw_file;
static uint32_t fw_file_off;
static bool init_done = false;
static string curr_dev = FW_UPDATE_DEV_NONE;
static atomic<int> fw_install_status(FW_INSTALL_NONE);
static atomic<int> fw_install_seq(0);
static sdk::lib::thread *fw_install_thread = NULL;

const char *
fwctrl_cmd_to_str(int cmd)
{
    switch (cmd) {
        CASE(IONIC_FW_RESET);
        CASE(IONIC_FW_INSTALL);
        CASE(IONIC_FW_INSTALL_ASYNC);
        CASE(IONIC_FW_ACTIVATE);
        CASE(IONIC_FW_INSTALL_STATUS);
        default:
            return "UNKNOWN";
    }
}

void
FwUpdateInit(PdClient *pd)
{
    if (init_done)
        return;

    fw_buf_addr = pd->mp_->start_addr(MEM_REGION_FWUPDATE_NAME);
    if (fw_buf_addr == INVALID_MEM_ADDRESS || fw_buf_addr == 0) {
        NIC_LOG_ERR("Failed to locate fwupdate region base");
        throw;
    }

    fw_buf_size = pd->mp_->size(MEM_REGION_FWUPDATE_NAME);
    if (fw_buf_size == 0) {
        NIC_LOG_ERR("Failed to locate fwupdate region size");
        throw;
    };

    NIC_LOG_INFO("fw_buf_addr {:#x} fw_buf_size {}", fw_buf_addr,
                 fw_buf_size);

    fw_buf = (uint8_t *)MEM_MAP(fw_buf_addr, fw_buf_size, 0);
    if (fw_buf == NULL) {
        NIC_LOG_ERR("Failed to map firmware buffer");
        throw;
    }
    init_done = true;
}

status_code_t
FwDownloadReset(string dev_name)
{
    if (fw_file)
        fclose(fw_file);

    remove(FW_FILEPATH);
    fw_buf_off = 0;
    fw_file_off = 0;
    fw_file = fopen(FW_FILEPATH, "wb+");
    if (fw_file == NULL) {
        NIC_LOG_ERR("{}: Failed to open firmware fw_file", dev_name);
        return (IONIC_RC_EIO);
    }
    return (IONIC_RC_SUCCESS);
}

void
FwInstallReset(string dev_name)
{
    if (fw_install_thread) {
        fw_install_thread->destroy(fw_install_thread);
        fw_install_thread = NULL;
    }
    curr_dev = dev_name;
    fw_install_status = FW_INSTALL_NONE;
    fw_install_seq++;
}

status_code_t
FwFlushBuffer(string dev_name, bool close)
{
    int err;

    if (fw_file == NULL) {
        if(close)
            return (IONIC_RC_SUCCESS);
        NIC_LOG_ERR("{}: Fw file not open, exiting", dev_name);
        return (IONIC_RC_EIO);
    }

    NIC_LOG_DEBUG("{}: Flushing fw buffer, buf_off {} file_off {}", dev_name,
                  fw_buf_off, fw_file_off);
    /* write the leftover data */
    if (fw_buf_off > 0) {
        err = fseek(fw_file, fw_file_off, SEEK_SET);
        if (err) {
            NIC_LOG_ERR("{}: Failed to seek offset {}, {}", dev_name, fw_file_off,
                        strerror(errno));
            return (IONIC_RC_EIO);
        }

        err = fwrite((const void *)fw_buf, sizeof(fw_buf[0]), fw_buf_off, fw_file);
        if (err != (int)fw_buf_off) {
            NIC_LOG_ERR("{}: Failed to write chunk, {}", dev_name, strerror(errno));
            return (IONIC_RC_EIO);
        }
        fw_file_off += fw_buf_off;
        fw_buf_off = 0;
    }
    if (close) {
        fclose(fw_file);
        fw_file = NULL;
    }

    return (IONIC_RC_SUCCESS);
}

status_code_t
FwDownloadEdma(string dev_name, uint64_t addr, uint32_t offset, uint32_t length, edma_q *edmaq, bool host_dev)
{
    status_code_t status = IONIC_RC_SUCCESS;
    uint32_t transfer_off = 0, transfer_sz = 0;
    bool posted = false;
    struct edmaq_ctx ctx = {0};
    int retries;

    if (curr_dev == FW_UPDATE_DEV_NONE) {
        FwInstallReset(dev_name);
    } else if (dev_name != curr_dev) {
        NIC_LOG_ERR("Fw update in progress by device {}, exiting", curr_dev);
        status = IONIC_RC_EBUSY;
        goto err;
    }

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        status = IONIC_RC_ERROR;
        goto err;
    }

    if (addr == 0) {
        NIC_LOG_ERR("{}: Invalid chunk address {:#x}!", dev_name, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (addr & ~BIT_MASK(52)) {
        NIC_LOG_ERR("{}: bad addr {:#x}", dev_name, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (offset + length > FW_MAX_SZ) {
        NIC_LOG_ERR("{}: Invalid chunk offset {} or length {}!", dev_name, offset,
                    length);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    /* cleanup update partition before starting download */
    if (offset == 0) {
        if ((status = FwDownloadReset(dev_name)) != IONIC_RC_SUCCESS)
            goto err;
    }

    retries = 0;
    /* transfer from host buffer in chunks of max size allowed by edma */
    while (transfer_off < length) {
        transfer_sz = min(length - transfer_off, EDMAQ_MAX_TRANSFER_SZ);
        /* if the local buffer does not have enough free space, then flush it to fw_file */
        if (fw_buf_off + transfer_sz > fw_buf_size) {
            /* wait for pending edma transfers to complete before writing to file */
            edmaq->flush();
            status = FwFlushBuffer(dev_name, false);
            if (status != IONIC_RC_SUCCESS) {
                goto err;
            }
        }

        posted =
            edmaq->post(host_dev ? EDMA_OPCODE_HOST_TO_LOCAL : EDMA_OPCODE_LOCAL_TO_LOCAL,
                        addr + transfer_off, fw_buf_addr + fw_buf_off, transfer_sz, &ctx);
        if (posted) {
            NIC_LOG_INFO("{}: Queued transfer offset {:#x} size {} src {:#x} dst {:#x}",
                dev_name, transfer_off, transfer_sz,
                addr + transfer_off, fw_buf_addr + transfer_off);
            transfer_off += transfer_sz;
            fw_buf_off += transfer_sz;
        } else {
            if (++retries > 5) {
                NIC_LOG_ERR("{}: Exhausted edma transfer retries, exiting", dev_name);
                status = IONIC_RC_EIO;
                goto err;
            }
            NIC_LOG_INFO("{}: Waiting for transfers to complete ...", dev_name);
            usleep(1000);
        }
    }

    /* Make sure that edmaq transfers are done before returning */
    edmaq->flush();

err:
    if (status != IONIC_RC_SUCCESS)
        FwInstallReset(FW_UPDATE_DEV_NONE);

    return status;
}

status_code_t
FwDownload(string dev_name, uint8_t *addr, uint32_t offset, uint32_t length)
{
    status_code_t status = IONIC_RC_SUCCESS;
    uint32_t transfer_off = 0, transfer_sz = 0;

    if (curr_dev == FW_UPDATE_DEV_NONE) {
        FwInstallReset(dev_name);
    } else if (dev_name != curr_dev) {
        NIC_LOG_ERR("Fw update in progress by device {}, exiting", curr_dev);
        status = IONIC_RC_EBUSY;
        goto err;
    }

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        status = IONIC_RC_ERROR;
        goto err;
    }

    if (addr == 0) {
        NIC_LOG_ERR("{}: Invalid chunk address {:#x}!", dev_name, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (offset + length > FW_MAX_SZ) {
        NIC_LOG_ERR("{}: Invalid chunk offset {} or length {}!", dev_name, offset,
                    length);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    /* cleanup update partition before starting download */
    if (offset == 0) {
        if ((status = FwDownloadReset(dev_name)) != IONIC_RC_SUCCESS)
            goto err;
    }

    while (transfer_off < length) {
        transfer_sz = min(length - transfer_off, fw_buf_size);
        /* if the local buffer does not have enough free space, then flush it to fw_file */
        if (fw_buf_off + transfer_sz > fw_buf_size) {
            status = FwFlushBuffer(dev_name, false);
            if (status != IONIC_RC_SUCCESS) {
                goto err;
            }
        }
        memcpy((uint8_t *)fw_buf + fw_buf_off, addr, transfer_sz);
        transfer_off += transfer_sz;
        fw_buf_off += transfer_sz;
    }

err:
    if (status != IONIC_RC_SUCCESS)
        FwInstallReset(FW_UPDATE_DEV_NONE);

    return status;
}

static void*
FwInstallAsync(void *obj)
{
    int err;
    struct fw_install_args *args = (struct fw_install_args *)obj;

    NIC_LOG_INFO("{}: IONIC_FW_INSTALL_ASYNC[{}] thread started!", args->dev_name,
                     args->seq_no);
    string buf = "/nic/tools/fwupdate -p " + string(FW_FILEPATH) + " -i all";
    err = system(buf.c_str());
    if (args->seq_no != fw_install_seq) {
        NIC_LOG_WARN("{}: FW_INSTALL SEQ MISMATCH seq %d fw_install_seq %d, aborting",
                     args->dev_name, args->seq_no, fw_install_seq);
        goto err;
    }
    if (err) {
        fw_install_status = FW_INSTALL_ERR;
        NIC_LOG_ERR("{}: Fw install failed with exit status: {}", args->dev_name, err);
        goto err;
    }

    fw_install_status = FW_INSTALL_DONE;

err:
    delete args;

    return NULL;
}

status_code_t
FwInstall(string dev_name, bool is_async)
{
    int err;
    status_code_t status = IONIC_RC_SUCCESS;

    if (curr_dev != dev_name) {
            NIC_LOG_ERR("{}: Fw update in progress by {}, exiting", dev_name, curr_dev);
            return (IONIC_RC_EBUSY);
    }

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        return (IONIC_RC_ERROR);
    }

    /* Flush any leftover data in the buffer */
    status = FwFlushBuffer(dev_name, true);
    if (status != IONIC_RC_SUCCESS) {
        goto err;
    }

    if (is_async) {
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL_ASYNC[{}] starting", dev_name,
                     fw_install_seq);
        struct fw_install_args *args = new fw_install_args();
        args->dev_name = dev_name;
        args->seq_no = fw_install_seq;
        fw_install_thread = sdk::lib::thread::factory(std::string("FW_INSTALL").c_str(),
                                                      NICMGRD_THREAD_ID_FW_INSTALL,
                                                      sdk::lib::THREAD_ROLE_CONTROL,
                                                      0xD,
                                                      FwInstallAsync,
                                                      sched_get_priority_min(SCHED_OTHER),
                                                      SCHED_OTHER,
                                                      false);
        if (fw_install_thread == NULL) {
                NIC_LOG_ERR("{}: Failed to instantiate fw install thread", dev_name);
                status = IONIC_RC_ERROR;
                goto err;
        }
        if (fw_install_thread->start(args) == SDK_RET_OK) {
            fw_install_status = FW_INSTALL_IN_PROGRESS;
        } else {
            NIC_LOG_ERR("{}: Failed to start fw install thread", dev_name);
            status = IONIC_RC_ERROR;
            goto err;
        }
    } else {
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL[{}] starting", dev_name, fw_install_seq);
        fw_install_status = FW_INSTALL_IN_PROGRESS;
        string buf = "/nic/tools/fwupdate -p " + string(FW_FILEPATH) + " -i all";
        err = system(buf.c_str());
        if (err) {
            NIC_LOG_ERR("{}: Fw install failed with exit status: {}", dev_name, err);
            status = IONIC_RC_ERROR;
            goto err;
        }
        fw_install_status = FW_INSTALL_DONE;
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL[{}] done!", dev_name, fw_install_seq);
    }

    err:
    if (status != IONIC_RC_SUCCESS)
        FwInstallReset(FW_UPDATE_DEV_NONE);

    return (IONIC_RC_SUCCESS);
}

status_code_t
FwControl(string dev_name, int oper)
{
    status_code_t status = IONIC_RC_SUCCESS;
    int err;

    switch (oper) {
    case IONIC_FW_RESET:
        if (curr_dev == dev_name) {
            NIC_LOG_INFO("{}: IONIC_FW_RESET", dev_name);
            FwInstallReset(FW_UPDATE_DEV_NONE); // Clear fw install in progress
        }
        status = IONIC_RC_SUCCESS;
        break;

    case IONIC_FW_INSTALL:
        status = FwInstall(dev_name, false);
        break;

    case IONIC_FW_INSTALL_ASYNC:
        status = FwInstall(dev_name, true);
        break;

    case IONIC_FW_INSTALL_STATUS:
        if (dev_name != curr_dev) {
            status = IONIC_RC_EBUSY;
        } else if (fw_install_status == FW_INSTALL_ERR) {
            status = IONIC_RC_ERROR;
        } else if (fw_install_status == FW_INSTALL_IN_PROGRESS) {
            status = IONIC_RC_EAGAIN;
        } else {
            status = IONIC_RC_SUCCESS;
        }
        break;

    case IONIC_FW_ACTIVATE:
        NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE starting", dev_name);
        err = system("/nic/tools/fwupdate -s altfw");
        if (err) {
            // TODO: Indicate the reason for the failure
            NIC_LOG_ERR("{}: Fw activate failed with exit status: {}", dev_name, err);
            status = IONIC_RC_ERROR;
        } else {
            NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE done!", dev_name);
        }
        FwInstallReset(FW_UPDATE_DEV_NONE);
        break;

    default:
        NIC_LOG_ERR("{}: Unknown operation {}", dev_name, oper);
        status = IONIC_RC_ENOSUPP;
        break;
    }

    return status;
}
