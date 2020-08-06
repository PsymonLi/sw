/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include "nic/sdk/lib/thread/thread.hpp"
#include "dev.hpp"
#include "eth_fw_update.hpp"
#include "eth_if.h"
#include "nicmgr_utils.hpp"

#define FW_UPDATE_OWNER_NONE  ""

enum fw_install_state {
    FW_INSTALL_NONE = 0,
    FW_INSTALL_IN_PROGRESS,
    FW_INSTALL_DONE,
    FW_INSTALL_ERR
};

enum fw_activate_state {
    FW_ACTIVATE_NONE = 0,
    FW_ACTIVATE_IN_PROGRESS,
    FW_ACTIVATE_DONE,
    FW_ACTIVATE_ERR
};

typedef struct fw_install_args_s {
    int seq_no;
    string owner;
} fw_install_args_t;

typedef fw_install_args_t fw_activate_args_t;

static uint64_t fw_buf_addr;
static uint32_t fw_buf_size;
static uint32_t fw_buf_off;
static uint8_t *fw_buf;
static FILE *fw_file;
static uint32_t fw_file_off;
static bool init_done = false;
static string curr_owner = FW_UPDATE_OWNER_NONE;
static atomic<int> fw_install_status(FW_INSTALL_NONE);
static atomic<int> fw_activate_status(FW_ACTIVATE_NONE);
static atomic<int> fw_update_seq(0);
static sdk::lib::thread *fw_install_thread = NULL;
static sdk::lib::thread *fw_activate_thread = NULL;

const char *
fwctrl_cmd_to_str(int cmd)
{
    switch (cmd) {
        CASE(IONIC_FW_RESET);
        CASE(IONIC_FW_INSTALL);
        CASE(IONIC_FW_INSTALL_ASYNC);
        CASE(IONIC_FW_INSTALL_STATUS);
        CASE(IONIC_FW_ACTIVATE);
        CASE(IONIC_FW_ACTIVATE_ASYNC);
        CASE(IONIC_FW_ACTIVATE_STATUS);
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
FwDownloadReset(string owner)
{
    if (fw_file)
        fclose(fw_file);

    remove(FW_FILEPATH);
    fw_buf_off = 0;
    fw_file_off = 0;
    fw_file = fopen(FW_FILEPATH, "wb+");
    if (fw_file == NULL) {
        NIC_LOG_ERR("{}: Failed to open firmware fw_file", owner);
        return (IONIC_RC_EIO);
    }
    return (IONIC_RC_SUCCESS);
}

void
FwUpdateReset(string owner)
{
    if (fw_install_thread) {
        fw_install_thread->destroy(fw_install_thread);
        fw_install_thread = NULL;
    }
    if (fw_activate_thread) {
        fw_activate_thread->destroy(fw_activate_thread);
        fw_activate_thread = NULL;
    }
    curr_owner = owner;
    fw_install_status = FW_INSTALL_NONE;
    fw_activate_status = FW_ACTIVATE_NONE;
    fw_update_seq++;
}

status_code_t
FwFlushBuffer(string owner, bool close)
{
    int err;

    if (fw_file == NULL) {
        if(close)
            return (IONIC_RC_SUCCESS);
        NIC_LOG_ERR("{}: Fw file not open, exiting", owner);
        return (IONIC_RC_EIO);
    }

    NIC_LOG_DEBUG("{}: Flushing fw buffer, buf_off {} file_off {}", owner,
                  fw_buf_off, fw_file_off);
    /* write the leftover data */
    if (fw_buf_off > 0) {
        err = fseek(fw_file, fw_file_off, SEEK_SET);
        if (err) {
            NIC_LOG_ERR("{}: Failed to seek offset {}, {}", owner, fw_file_off,
                        strerror(errno));
            return (IONIC_RC_EIO);
        }

        err = fwrite((const void *)fw_buf, sizeof(fw_buf[0]), fw_buf_off, fw_file);
        if (err != (int)fw_buf_off) {
            NIC_LOG_ERR("{}: Failed to write chunk, {}", owner, strerror(errno));
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
FwDownloadEdma(string owner, uint64_t addr, uint32_t offset, uint32_t length, edma_q *edmaq, bool host_dev)
{
    status_code_t status = IONIC_RC_SUCCESS;
    uint32_t transfer_off = 0, transfer_sz = 0;
    bool posted = false;
    struct edmaq_ctx ctx = {0};
    int retries;

    if (curr_owner == FW_UPDATE_OWNER_NONE) {
        FwUpdateReset(owner);
    } else if (owner != curr_owner) {
        NIC_LOG_ERR("Fw update in progress by {}, exiting", curr_owner);
        return (IONIC_RC_EBUSY);
    }

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        status = IONIC_RC_ERROR;
        goto err;
    }

    if (addr == 0) {
        NIC_LOG_ERR("{}: Invalid chunk address {:#x}!", owner, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (addr & ~BIT_MASK(52)) {
        NIC_LOG_ERR("{}: bad addr {:#x}", owner, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (offset + length > FW_MAX_SZ) {
        NIC_LOG_ERR("{}: Invalid chunk offset {} or length {}!", owner, offset,
                    length);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    /* cleanup update partition before starting download */
    if (offset == 0) {
        if ((status = FwDownloadReset(owner)) != IONIC_RC_SUCCESS)
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
            status = FwFlushBuffer(owner, false);
            if (status != IONIC_RC_SUCCESS) {
                goto err;
            }
        }

        posted =
            edmaq->post(host_dev ? EDMA_OPCODE_HOST_TO_LOCAL : EDMA_OPCODE_LOCAL_TO_LOCAL,
                        addr + transfer_off, fw_buf_addr + fw_buf_off, transfer_sz, &ctx);
        if (posted) {
            NIC_LOG_INFO("{}: Queued transfer offset {:#x} size {} src {:#x} dst {:#x}",
                owner, transfer_off, transfer_sz,
                addr + transfer_off, fw_buf_addr + transfer_off);
            transfer_off += transfer_sz;
            fw_buf_off += transfer_sz;
        } else {
            if (++retries > 5) {
                NIC_LOG_ERR("{}: Exhausted edma transfer retries, exiting", owner);
                status = IONIC_RC_EIO;
                goto err;
            }
            NIC_LOG_INFO("{}: Waiting for transfers to complete ...", owner);
            usleep(1000);
        }
    }

    /* Make sure that edmaq transfers are done before returning */
    edmaq->flush();

err:
    if (status != IONIC_RC_SUCCESS)
        FwUpdateReset(FW_UPDATE_OWNER_NONE);

    return status;
}

status_code_t
FwDownload(string owner, uint8_t *addr, uint32_t offset, uint32_t length)
{
    status_code_t status = IONIC_RC_SUCCESS;
    uint32_t transfer_off = 0, transfer_sz = 0;

    if (curr_owner == FW_UPDATE_OWNER_NONE) {
        FwUpdateReset(owner);
    } else if (owner != curr_owner) {
        NIC_LOG_ERR("Fw update in progress by {}, exiting", curr_owner);
        return (IONIC_RC_EBUSY);
    }

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        status = IONIC_RC_ERROR;
        goto err;
    }

    if (addr == 0) {
        NIC_LOG_ERR("{}: Invalid chunk address {:#x}!", owner, addr);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    if (offset + length > FW_MAX_SZ) {
        NIC_LOG_ERR("{}: Invalid chunk offset {} or length {}!", owner, offset,
                    length);
        status = IONIC_RC_EINVAL;
        goto err;
    }

    /* cleanup update partition before starting download */
    if (offset == 0) {
        if ((status = FwDownloadReset(owner)) != IONIC_RC_SUCCESS)
            goto err;
    }

    while (transfer_off < length) {
        transfer_sz = min(length - transfer_off, fw_buf_size);
        /* if the local buffer does not have enough free space, then flush it to fw_file */
        if (fw_buf_off + transfer_sz > fw_buf_size) {
            status = FwFlushBuffer(owner, false);
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
        FwUpdateReset(FW_UPDATE_OWNER_NONE);

    return status;
}

static void*
FwInstallAsync(void *obj)
{
    int err;
    fw_install_args_t *args = (fw_install_args_t *)obj;

    NIC_LOG_INFO("{}: IONIC_FW_INSTALL_ASYNC[{}] thread started!", args->owner,
                 args->seq_no);
    string buf = "/nic/tools/fwupdate -p " + string(FW_FILEPATH) + " -i all";
    err = system(buf.c_str());
    if (err) {
        fw_install_status = FW_INSTALL_ERR;
        NIC_LOG_ERR("{}: Fw install failed with exit status: {}", args->owner, err);
        goto err;
    }
    fw_install_status = FW_INSTALL_DONE;

err:
    delete args;

    return NULL;
}

status_code_t
FwInstall(string owner, bool is_async)
{
    int err;
    status_code_t status = IONIC_RC_SUCCESS;

    if (!init_done) {
        NIC_LOG_ERR("Fw buffer not initialized, exiting");
        return (IONIC_RC_ERROR);
    }

    /* Flush any leftover data in the buffer */
    status = FwFlushBuffer(owner, true);
    if (status != IONIC_RC_SUCCESS) {
        goto err;
    }

    if (is_async) {
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL_ASYNC[{}] starting", owner,
                     fw_update_seq);
        fw_install_args_t *args = new fw_install_args_t();
        args->owner = owner;
        args->seq_no = fw_update_seq;
        fw_install_thread = sdk::lib::thread::factory("FW_INSTALL",
                                                      NICMGRD_THREAD_ID_FW_INSTALL,
                                                      sdk::lib::THREAD_ROLE_CONTROL,
                                                      0xD,
                                                      FwInstallAsync,
                                                      sched_get_priority_min(SCHED_OTHER),
                                                      SCHED_OTHER);
        if (fw_install_thread == NULL) {
                NIC_LOG_ERR("{}: Failed to instantiate fw install thread", owner);
                status = IONIC_RC_ERROR;
                goto err;
        }
        fw_install_status = FW_INSTALL_IN_PROGRESS;
        if (fw_install_thread->start(args) != SDK_RET_OK){
            NIC_LOG_ERR("{}: Failed to start fw install thread", owner);
            status = IONIC_RC_ERROR;
            goto err;
        }
    } else {
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL[{}] starting", owner, fw_update_seq);
        string buf = "/nic/tools/fwupdate -p " + string(FW_FILEPATH) + " -i all";
        err = system(buf.c_str());
        if (err) {
            NIC_LOG_ERR("{}: Fw install failed with exit status: {}", owner, err);
            status = IONIC_RC_ERROR;
            goto err;
        }
        fw_install_status = FW_INSTALL_DONE;
        NIC_LOG_INFO("{}: IONIC_FW_INSTALL[{}] done!", owner, fw_update_seq);
    }

err:
    if (status != IONIC_RC_SUCCESS)
        FwUpdateReset(FW_UPDATE_OWNER_NONE);

    return status;
}

static void*
FwActivateAsync(void *obj)
{
    int err;
    fw_activate_args_t *args = (fw_activate_args_t *)obj;

    NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE_ASYNC[{}] thread started!", args->owner,
                 args->seq_no);
    err = system("/nic/tools/fwupdate -s altfw");
    if (err) {
        fw_activate_status = FW_ACTIVATE_ERR;
        NIC_LOG_ERR("{}: Fw activate failed with exit status: {}", args->owner, err);
        goto err;
    }
    fw_activate_status = FW_ACTIVATE_DONE;

err:
    delete args;

    return NULL;
}

status_code_t
FwActivate(string owner, bool async)
{
    status_code_t status = IONIC_RC_SUCCESS;
    int err;

    if (async) {
        NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE_ASYNC[{}] starting", owner,
                     fw_update_seq);
        fw_activate_args_t *args = new fw_activate_args_t();
        args->owner = owner;
        args->seq_no = fw_update_seq;
        fw_activate_thread = sdk::lib::thread::factory("FW_ACTIVATE",
                                                       NICMGRD_THREAD_ID_FW_ACTIVATE,
                                                       sdk::lib::THREAD_ROLE_CONTROL,
                                                       0xD,
                                                       FwActivateAsync,
                                                       sched_get_priority_min(SCHED_OTHER),
                                                       SCHED_OTHER);
        if (fw_activate_thread == NULL) {
                NIC_LOG_ERR("{}: Failed to instantiate fw install thread", owner);
                status = IONIC_RC_ERROR;
                goto err;
        }
        fw_activate_status = FW_ACTIVATE_IN_PROGRESS;
        if (fw_activate_thread->start(args) != SDK_RET_OK) {
            NIC_LOG_ERR("{}: Failed to start fw activate thread", owner);
            status = IONIC_RC_ERROR;
            goto err;
        }
    } else {
        NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE starting", owner);
        err = system("/nic/tools/fwupdate -s altfw");
        if (err) {
            NIC_LOG_ERR("{}: Fw activate failed with exit status: {}", owner, err);
            status = IONIC_RC_ERROR;
            goto err;
        }
        NIC_LOG_INFO("{}: IONIC_FW_ACTIVATE done!", owner);
        FwUpdateReset(FW_UPDATE_OWNER_NONE);
    }

err:
    if (status != IONIC_RC_SUCCESS)
        FwUpdateReset(FW_UPDATE_OWNER_NONE);

    return status;
}


status_code_t
FwControl(string owner, int oper)
{
    status_code_t status = IONIC_RC_SUCCESS;

    if (curr_owner != owner) {
            NIC_LOG_ERR("{}: Fw update in progress by {}, exiting", owner, curr_owner);
            return (IONIC_RC_EBUSY);
    }

    switch (oper) {
    case IONIC_FW_RESET:
        NIC_LOG_INFO("{}: IONIC_FW_RESET", owner);
        FwUpdateReset(FW_UPDATE_OWNER_NONE);
        status = IONIC_RC_SUCCESS;
        break;
    case IONIC_FW_INSTALL:
        status = FwInstall(owner, false);
        break;
    case IONIC_FW_INSTALL_ASYNC:
        status = FwInstall(owner, true);
        break;
    case IONIC_FW_INSTALL_STATUS:
        if (fw_install_status == FW_INSTALL_ERR) {
            status = IONIC_RC_ERROR;
            // Revoke the ownership and cleanup the state
            FwUpdateReset(FW_UPDATE_OWNER_NONE);
        } else if (fw_install_status == FW_INSTALL_IN_PROGRESS) {
            status = IONIC_RC_EAGAIN;
        } else {
            status = IONIC_RC_SUCCESS;
        }
        break;
    case IONIC_FW_ACTIVATE:
        status = FwActivate(owner, false);
        break;
    case IONIC_FW_ACTIVATE_ASYNC:
        status = FwActivate(owner, true);
        break;
    case IONIC_FW_ACTIVATE_STATUS:
        if (fw_activate_status == FW_ACTIVATE_ERR) {
            status = IONIC_RC_ERROR;
        } else if (fw_activate_status == FW_ACTIVATE_IN_PROGRESS) {
            status = IONIC_RC_EAGAIN;
        } else {
            status = IONIC_RC_SUCCESS;
        }
        if (status == IONIC_RC_ERROR || status == IONIC_RC_SUCCESS) {
            // Revoke the ownership and cleanup the state
            FwUpdateReset(FW_UPDATE_OWNER_NONE);
        }
        break;
    default:
        NIC_LOG_ERR("{}: Unknown operation {}", owner, oper);
        status = IONIC_RC_ENOSUPP;
        break;
    }

    return status;
}
