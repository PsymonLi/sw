/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#ifndef __ETH_FW_UPDATE_HPP__
#define __ETH_FW_UPDATE_HPP__

using namespace std;

#include "gen/platform/mem_regions.hpp"
#include "pd_client.hpp"
#include "edmaq.hpp"
#include "pal_compat.hpp"
#include "logger.hpp"
#include "eth_lif.hpp"

#define FW_MAX_SZ ((900 << 20)) // 900 MiB
#define FW_FILEPATH "/update/firmware.tar"

const char *fwctrl_cmd_to_str(int cmd);
void FwUpdateInit(PdClient *pd);
status_code_t FwDownloadEdma(string owner, uint64_t addr, uint32_t offset,
                             uint32_t length, edma_q *edmaq, bool host_dev);
status_code_t FwDownload(string owner, uint8_t *addr, uint32_t offset,
                         uint32_t length);
status_code_t FwControl(string owner, int oper);

#endif /* __ETH_FW_UPDATE_HPP__ */
