// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2020 Pensando Systems, Inc */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/firmware.h>

#include "ionic.h"
#include "ionic_dev.h"
#include "ionic_lif.h"

static void
ionic_dev_cmd_firmware_download(struct ionic_dev *idev, uint64_t addr,
				uint32_t offset, uint32_t length)
{
	union ionic_dev_cmd cmd = {
		.fw_download.opcode = IONIC_CMD_FW_DOWNLOAD,
		.fw_download.offset = offset,
		.fw_download.addr = addr,
		.fw_download.length = length
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void
ionic_dev_cmd_firmware_install(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_INSTALL_ASYNC
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void
ionic_dev_cmd_firmware_install_status(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_INSTALL_STATUS
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void
ionic_dev_cmd_firmware_activate(struct ionic_dev *idev, uint8_t slot)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_ACTIVATE_ASYNC,
		.fw_control.slot = slot
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void
ionic_dev_cmd_firmware_activate_status(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_ACTIVATE_STATUS,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

int ionic_firmware_update(struct ionic_lif *lif, const char *fw_name)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	struct net_device *netdev = lif->netdev;
	struct ionic *ionic = lif->ionic;
	union ionic_dev_cmd_comp comp;
	u32 buf_sz, copy_sz, offset;
	const struct firmware *fw;
	struct devlink *dl;
	int err = 0;
	u8 fw_slot;

	netdev_info(netdev, "Installing firmware %s\n", fw_name);

	dl = priv_to_devlink(ionic);
	devlink_flash_update_begin_notify(dl);
	devlink_flash_update_status_notify(dl, "Preparing to flash", NULL, 0, 0);

	err = request_firmware(&fw, fw_name, ionic->dev);
	if (err)
		goto err_out;

	buf_sz = sizeof(idev->dev_cmd_regs->data);

	netdev_info(netdev,
		"downloading firmware - size %d part_sz %d nparts %lu\n",
		(int)fw->size, buf_sz, DIV_ROUND_UP(fw->size, buf_sz));

	devlink_flash_update_status_notify(dl, "Downloading", NULL, 0, fw->size);
	offset = 0;
	while (offset < fw->size) {
		copy_sz = min_t(unsigned int, buf_sz, fw->size - offset);
		mutex_lock(&ionic->dev_cmd_lock);
		memcpy_toio(&idev->dev_cmd_regs->data, fw->data + offset, copy_sz);
		ionic_dev_cmd_firmware_download(idev,
			offsetof(union ionic_dev_cmd_regs, data), offset, copy_sz);
		err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
		mutex_unlock(&ionic->dev_cmd_lock);
		if (err) {
			netdev_err(netdev,
				   "download failed offset 0x%x addr 0x%lx len 0x%x\n",
				   offset, offsetof(union ionic_dev_cmd_regs, data),
				   copy_sz);
			goto err_out;
		}
		offset += copy_sz;
	}

	netdev_info(netdev, "installing firmware\n");
	devlink_flash_update_status_notify(dl, "Installing", NULL, 0, fw->size);

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_install(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	ionic_dev_cmd_comp(idev, (union ionic_dev_cmd_comp *)&comp);
	fw_slot = comp.fw_control.slot;
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "failed to start firmware install\n");
		goto err_out;
	}

	/* The worst case wait for the install activity is about 25 minutes
	 * when installing a new CPLD.  Normal should be about 30-35 seconds.
	 */
	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_install_status(idev);
	err = ionic_dev_cmd_wait(ionic, 25 * 60 * HZ);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "firmware install failed\n");
		goto err_out;
	}

	netdev_info(netdev, "activating firmware\n");
	devlink_flash_update_status_notify(dl, "Activating", NULL, 0, fw->size);

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_activate(idev, fw_slot);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "failed to start firmware activation\n");
		goto err_out;
	}

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_activate_status(idev);
	err = ionic_dev_cmd_wait(ionic, 30 * HZ);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "firmware activation failed\n");
		goto err_out;
	}

	netdev_info(netdev, "Firmware update completed\n");

err_out:
	if (err)
		devlink_flash_update_status_notify(dl, "Flash failed", NULL, 0, fw->size);
	release_firmware(fw);
	devlink_flash_update_end_notify(dl);
	return err;
}
