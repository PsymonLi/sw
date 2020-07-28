/*
 * Copyright (c) 2020, Pensando Systems Inc.
 *
 * Pensando MCTP driver utility.
 *
 */

#ifndef MCTP_IOCTL_H
#define MCTP_IOCTL_H

#include <stdint.h>
#include <linux/ioctl.h>

struct mctp_statistics {
    /* message types */
    uint32_t mctp_ctrl_msg;
    uint32_t mctp_pldm_msg;
    uint32_t mctp_ncsi_msg;
    uint32_t mctp_ethernet_msg;
    uint32_t mctp_vendor_pci_msg;
    uint32_t mctp_vendor_iana_msg;

    /* mctp message sizes */
    uint32_t mctp_pkt_64b;
    uint32_t mctp_pkt_65b_127b;
    uint32_t mctp_pkt_128b_255b;
    uint32_t mctp_pkt_256b_511b;

    /* mctp shared tx kfifo */
    uint32_t mctp_tx_bytes;
    uint32_t mctp_txfifo_max_bytes;
    uint32_t mctp_txfifo_full_reset;
    uint32_t mctp_txfifo_error_reset;

    /* mctp control message */
    uint32_t ctrl_rx_bytes;
    uint32_t ctrl_tx_bytes;
    uint32_t ctrl_msg_not_read;
    uint32_t ctrl_msg_resp_late;
    uint32_t ctrl_avg_latency_usec;
    uint32_t ctrl_max_latency_usec;
    uint32_t ctrl_rxfifo_max_bytes;
    uint32_t ctrl_rxfifo_full_reset;
    uint32_t ctrl_rxfifo_error_reset;

    /* ncsi message */
    uint32_t ncsi_rx_bytes;
    uint32_t ncsi_tx_bytes;
    uint32_t ncsi_msg_not_read;
    uint32_t ncsi_msg_resp_late;
    uint32_t ncsi_avg_latency_usec;
    uint32_t ncsi_max_latency_usec;
    uint32_t ncsi_rxfifo_max_bytes;
    uint32_t ncsi_rxfifo_full_reset;
    uint32_t ncsi_rxfifo_error_reset;

    /* pldm message */
    uint32_t pldm_rx_bytes;
    uint32_t pldm_tx_bytes;
    uint32_t pldm_msg_not_read;
    uint32_t pldm_msg_resp_late;
    uint32_t pldm_max_latency_usec;
    uint32_t pldm_avg_latency_usec;
    uint32_t pldm_rxfifo_max_bytes;
    uint32_t pldm_rxfifo_full_reset;
    uint32_t pldm_rxfifo_error_reset;

    /* ioctl and error recovery */
    uint32_t arp_reset;
    uint32_t slave_reset;
    uint32_t reset_all_fifos;

    /* i2c layer */
    uint32_t i2c_transfer_success;
    uint32_t i2c_transfer_error;
};

/* The ioctl type, 'L', 0x30 - 0x3F, unused in ioctl-number.txt */
enum {
    _SLAVE_RESET = 0x30,
    _SET_EID,
    _GET_EID,
    _SLAVE_ADDR,
    _ARP_RESET,
    _MCTP_INFO,
};

#define MCTP_IOCTL 'L'
#define SLAVE_RESET  _IO(MCTP_IOCTL, _SLAVE_RESET)
#define SET_EID      _IOW(MCTP_IOCTL, _SET_EID, unsigned int *)
#define GET_EID      _IOR(MCTP_IOCTL, _GET_EID, unsigned int *)
#define SLAVE_ADDR   _IOR(MCTP_IOCTL, _SLAVE_ADDR, unsigned int *)
#define ARP_RESET    _IO(MCTP_IOCTL, _ARP_RESET)
#define MCTP_INFO    _IOR(MCTP_IOCTL, _MCTP_INFO, struct mctp_statistics *)

#endif    /* MCTP_IOCTL_H */
