/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "ionic_os.h"

/*
 * User mode ionic driver.
 */

/* Supply these for ionic_if.h */
typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int16_t __le16;
typedef u_int32_t u32;
typedef u_int32_t __le32;
typedef u_int64_t u64;
typedef u_int64_t __le64;
typedef u_int64_t dma_addr_t;
typedef u_int64_t phys_addr_t;
typedef u_int64_t cpumask_t;
#define BIT(n)  (1 << (n))
#define BIT_ULL(n)  (1ULL << (n))
typedef u8 bool;
#define false 0
#define true 1
#define __iomem

#include "ionic_if.h"

#define IONIC_DEV_CMD_DONE              0x00000001

/* Polling delay in micro seconds. */
#define IONIC_DEVCMD_WAIT_USECS		500
/* Total wait time of 5 seconds. */
#define IONIC_DEVCMD_MAX_RETRY		(5000000 / IONIC_DEVCMD_WAIT_USECS)


static int
devcmd_go(FILE *fstream, union ionic_dev_cmd_regs *cmd_regs, union ionic_dev_cmd *cmd,
	union ionic_dev_cmd_comp *comp)
{
	uint32_t i, retry;
	/* Wait for 5 seconds. */
	uint32_t max_retry = IONIC_DEVCMD_MAX_RETRY;
	for (i = 0; i < ARRAY_SIZE(cmd->words); i++)
		cmd_regs->cmd.words[i] = cmd->words[i];

	cmd_regs->done = 0;
	cmd_regs->doorbell = 1;

	/*
	 * For firmware install, we can't abort in between.
	 * if it includes  CPLD image.
	 */
	if (cmd->cmd.opcode == IONIC_CMD_FW_CONTROL) {
		max_retry = 0xFFFFFFF;
	}

	for (retry = 0;retry < max_retry; retry++) {
		/* Sleep for 1 ms. */
		usleep(IONIC_DEVCMD_WAIT_USECS);

		if (cmd_regs->done & IONIC_DEV_CMD_DONE) {
			break;
		}
	}

	if (!(cmd_regs->done & IONIC_DEV_CMD_DONE)) {
		ionic_print_error(fstream, "", "Done bit not set after retries: %d for opcode: %d.\n",
			retry, cmd->cmd.opcode);
		return (ETIMEDOUT);
	}

	for (i = 0; i < ARRAY_SIZE(comp->words); i++)
		comp->words[i] = cmd_regs->comp.words[i];

	return (comp->status);
}

static int
ionic_read_identify(FILE *fstream, union ionic_dev_info_regs *info_regs, char *fw_vers, char *asic_type)
{

	if (info_regs->signature != IONIC_DEV_INFO_SIGNATURE) {
		ionic_print_error(fstream, "", "Signature mismatch. expected 0x%08x != 0x%08x\n",
			IONIC_DEV_INFO_SIGNATURE, info_regs->signature);
		return (ENXIO);
	}

	strcpy(fw_vers, info_regs->fw_version);

	switch (info_regs->asic_type) {
	case IONIC_ASIC_TYPE_CAPRI:
		strcpy(asic_type, "CAPRI");
		break;
	default:
		strcpy(asic_type, "Unknown");;
		break;
	}

	ionic_print_info(fstream, "", "signature: 0x%x asic_type: %s asic_rev: 0x%x serial_num: %s "
		"fw_version: %s fw_status: 0x%x fw_heartbeat: 0x%x\n", info_regs->signature,
		asic_type, info_regs->asic_rev, info_regs->serial_num,
		info_regs->fw_version, info_regs->fw_status, info_regs->fw_heartbeat);

	return (0);
}

static int
ionic_dev_identify_cmd(FILE *fstream, union ionic_dev_cmd_regs *cmd_regs)
{
	union ionic_dev_cmd cmd = {
		.identify.opcode = IONIC_CMD_IDENTIFY,
		.identify.ver = IONIC_IDENTITY_VERSION_1,
	};
	union ionic_dev_cmd_comp comp;
	union ionic_dev_identity *dev_ident;

	if (devcmd_go(fstream, cmd_regs, &cmd, &comp))
		return (EIO);

	dev_ident = (union ionic_dev_identity *)cmd_regs->data;
	ionic_print_info(fstream, "", "version: %d type: %d nports: %d nlifs: %d nintrs: %d eth_eq_count: %d "
		"ndbpgs_per_lif: %d intr_coal_mult: %d intr_coal_div: %d\n", dev_ident->version,
		dev_ident->type, dev_ident->nports, dev_ident->nlifs, dev_ident->nintrs,
		dev_ident->eq_count, dev_ident->ndbpgs_per_lif, dev_ident->intr_coal_mult,
		dev_ident->intr_coal_div);

	return (0);
}

/*
 * Copy the firmware image to the Naples card.
 */
static int
ionic_pmd_fw_copy(FILE *fstream, union ionic_dev_cmd_regs *cmd_regs, char *fw_path)
{
	struct ionic_fw_download_cmd cmd = {
		.opcode = IONIC_CMD_FW_DOWNLOAD,
	};
	union ionic_dev_cmd_comp comp;
	struct timeval start, end;
	struct stat sb;
	uint64_t offset, file_len;
	int fd, len, status;
	/* BLOCK_SIZE has to be less than IONIC_DEVCMD_DATA_SIZE */
	const int BLOCK_SIZE = 0x700;
	uint64_t *src, *dst;

	gettimeofday(&start, NULL);

	fd = open(fw_path, O_RDONLY);
	if (fd == -1) {
		perror(fw_path);
		return (ENXIO);
	}

	status = fstat(fd, &sb);
	if (status) {
		ionic_print_error(fstream, "", "Failed to get size of: %s\n", fw_path);
		perror(fw_path);
		close(fd);
		return (ENXIO);
	}
	file_len = sb.st_size;

	ionic_print_info(fstream, "", "Firmware: %s length: %ld bytes, copy done:     ", fw_path, file_len);

	src = mmap(NULL, file_len, PROT_READ, MAP_SHARED, fd, 0);
	if (src == MAP_FAILED) {
		ionic_print_error(fstream, "", "Failed to open: %s\n", fw_path);
		perror(fw_path);
		close(fd);
		return (ENXIO);
	}

	for (offset = 0; offset < file_len; offset += BLOCK_SIZE) {
		len = file_len - offset;
		len = BLOCK_SIZE > len ? len : BLOCK_SIZE;
		cmd.offset = offset;
		cmd.addr = offsetof(union ionic_dev_cmd_regs, data);;
		cmd.length = len;
		dst = (uint64_t *)cmd_regs->data;
		memcpy(cmd_regs->data, (uint8_t *)src + offset, len);
		fprintf(fstream, "\b\b\b%02lu%%", (100 * offset) / file_len);
		status = devcmd_go(fstream, cmd_regs, (union ionic_dev_cmd *)&cmd, &comp);
		if (status) {
			ionic_print_error(fstream, "", "Firmware: %s program failed at"
				" offset: %ld length: %d status: %d\n", fw_path, offset, len, status);
			break;
		}
	}

	gettimeofday(&end, NULL);
	fprintf(fstream, "\n");
	ionic_print_info(fstream, "", "Firmware copy took %ld seconds\n", end.tv_sec - start.tv_sec);

	munmap(src, file_len);
	close(fd);

	return (0);
}

static int
ionic_pmd_fw_install(FILE *fstream, union ionic_dev_cmd_regs *cmd_regs)
{
	struct ionic_fw_control_cmd cmd = {
		.opcode = IONIC_CMD_FW_CONTROL,
		.oper = IONIC_FW_INSTALL
	};
	union ionic_dev_cmd_comp comp;
	struct ionic_fw_control_comp *fw_control_comp;
	struct timeval start, end;
	int slot, status;

	gettimeofday(&start, NULL);
	/* Install the new firmware image. */
	status = devcmd_go(fstream, cmd_regs, (union ionic_dev_cmd *)&cmd, &comp);
	if (status) {
		ionic_print_error(fstream, "", "Firmware: install failed status: %d, is it valid Naples image?\n", status);
		return (status);
	}

	fw_control_comp = (struct ionic_fw_control_comp *)&comp.comp;
	slot = fw_control_comp->slot;

	/* Activate the new firmware image. */
	cmd.opcode = IONIC_CMD_FW_CONTROL;
	cmd.oper = IONIC_FW_ACTIVATE;
	cmd.slot = slot;

	status = devcmd_go(fstream, cmd_regs, (union ionic_dev_cmd *)&cmd, &comp);
	if (status) {
		ionic_print_error(fstream, "", "Firmware: activate failed status: %d\n", status);
		return (status);
	}

	gettimeofday(&end, NULL);

	ionic_print_info(fstream, "", "Firmware install took %ld seconds\n", end.tv_sec - start.tv_sec);
	return (status);
}

int
ionic_pmd_flash_firmware(FILE *fstream, void *bar0_ptr, char *fw_file)
{
	union ionic_dev_cmd_regs *cmd_regs;
	int status;

	if (bar0_ptr == NULL) {
		ionic_print_error(fstream, "", "address is NULL\n");
		return (ENXIO);
	}

	cmd_regs = bar0_ptr + IONIC_BAR0_DEV_CMD_REGS_OFFSET;

	status = ionic_pmd_fw_copy(fstream, cmd_regs, fw_file);
	if (status) {
		return (status);
	}

	status = ionic_pmd_fw_install(fstream, cmd_regs);
	if (status) {
		return (status);
	}

	return (status);
}

int
ionic_pmd_identify(FILE *fstream, void *bar0_ptr, char *fw_vers, char *asic_type)
{

	union ionic_dev_cmd_regs *cmd_regs;
	union ionic_dev_info_regs *info_regs;
	int status;

	if (bar0_ptr == NULL) {
		ionic_print_error(fstream, "", "address is NULL\n");
		return (ENXIO);
	}

	info_regs = bar0_ptr + IONIC_BAR0_DEV_INFO_REGS_OFFSET;
	status = ionic_read_identify(fstream, info_regs, fw_vers, asic_type);
	if (status) {
		ionic_print_error(fstream, "", "identify failed, error: %d\n", status);
		return (ENXIO);
	}

	cmd_regs = bar0_ptr + IONIC_BAR0_DEV_CMD_REGS_OFFSET;

	status = ionic_dev_identify_cmd(fstream, cmd_regs);
	if (status) {
		ionic_print_error(fstream, "", "device identify failed, error: %d\n", status);
		return (ENXIO);
	}

	return (status);
}

