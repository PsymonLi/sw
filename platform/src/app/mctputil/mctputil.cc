/*
 * Copyright (C) 2020, Pensando Systems Inc.
 *
 * Pensando MCTP driver utility.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "mctp_ioctl.h"

static void usage(void)
{
    fprintf(stderr,
"Usage: mctp [options]\n"
"    -i, --info         show mctp info\n"
"    -e, --eid          show endpoint id\n"
"    -s, --slaveaddr    show i2c slave 8-bit address\n"
"    -a, --arp          arp reset (clear ar/av flags, slave addr 0xc2)\n"
"    -r, --reset        i2c-3 reset, slave addr retained\n"
           );
}

int
get_info(int fd)
{
    struct mctp_statistics stats;
    int rc = 0;

    rc = ioctl(fd, MCTP_INFO, &stats);
    if (rc) {
        printf("ioctl error - %d\n", rc);
        return rc; 
    }

    printf("# mctp message types\n");
    printf("mctp_ctrl_msg: %u\n", stats.mctp_ctrl_msg);
    printf("mctp_pldm_msg: %u\n", stats.mctp_pldm_msg);
    printf("mctp_ncsi_msg: %u\n", stats.mctp_ncsi_msg);
    printf("mctp_ethernet_msg: %u\n", stats.mctp_ethernet_msg);
    printf("mctp_vendor_pci_msg: %u\n", stats.mctp_vendor_pci_msg);
    printf("mctp_vendor_iana_msg: %u\n", stats.mctp_vendor_iana_msg);

    printf("\n# mctp message sizes\n");
    printf("mctp_pkt_64b: %u\n", stats.mctp_pkt_64b);
    printf("mctp_pkt_65b_127b: %u\n", stats.mctp_pkt_65b_127b);
    printf("mctp_pkt_128b_255b: %u\n", stats.mctp_pkt_128b_255b);
    printf("mctp_pkt_256b_511b: %u\n", stats.mctp_pkt_256b_511b);

    printf("\n# mctp shared tx kfifo\n");
    printf("mctp_tx_bytes: %u\n", stats.mctp_tx_bytes);
    printf("mctp_txfifo_max_bytes: %u\n", stats.mctp_txfifo_max_bytes);
    printf("mctp_txfifo_full_reset: %u\n", stats.mctp_txfifo_full_reset);
    printf("mctp_txfifo_error_reset: %u\n", stats.mctp_txfifo_error_reset);

    printf("\n# mctp control message\n");
    printf("ctrl_rx_bytes: %u\n", stats.ctrl_rx_bytes);
    printf("ctrl_tx_bytes: %u\n", stats.ctrl_tx_bytes);
    printf("ctrl_msg_not_read: %u\n", stats.ctrl_msg_not_read);
    printf("ctrl_msg_resp_late: %u\n", stats.ctrl_msg_resp_late);
    printf("ctrl_avg_latency_usec: %u\n", stats.ctrl_avg_latency_usec);
    printf("ctrl_max_latency_usec: %u\n", stats.ctrl_max_latency_usec);
    printf("ctrl_rxfifo_max_bytes: %u\n", stats.ctrl_rxfifo_max_bytes);
    printf("ctrl_rxfifo_full_reset: %u\n", stats.ctrl_rxfifo_full_reset);
    printf("ctrl_rxfifo_error_reset: %u\n", stats.ctrl_rxfifo_error_reset);

    printf("\n# ncsi message\n");
    printf("ncsi_rx_bytes: %u\n", stats.ncsi_rx_bytes);
    printf("ncsi_tx_bytes: %u\n", stats.ncsi_tx_bytes);
    printf("ncsi_msg_not_read: %u\n", stats.ncsi_msg_not_read);
    printf("ncsi_msg_resp_late: %u\n", stats.ncsi_msg_resp_late);
    printf("ncsi_avg_latency_usec: %u\n", stats.ncsi_avg_latency_usec);
    printf("ncsi_max_latency_usec: %u\n", stats.ncsi_max_latency_usec);
    printf("ncsi_rxfifo_max_bytes: %u\n", stats.ncsi_rxfifo_max_bytes);
    printf("ncsi_rxfifo_full_reset: %u\n", stats.ncsi_rxfifo_full_reset);
    printf("ncsi_rxfifo_error_reset: %u\n", stats.ncsi_rxfifo_error_reset);

    printf("\n# pldm message\n");
    printf("pldm_rx_bytes: %u\n", stats.pldm_rx_bytes);
    printf("pldm_tx_bytes: %u\n", stats.pldm_tx_bytes);
    printf("pldm_msg_not_read: %u\n", stats.pldm_msg_not_read);
    printf("pldm_msg_resp_late: %u\n", stats.pldm_msg_resp_late);
    printf("pldm_avg_latency_usec: %u\n", stats.pldm_avg_latency_usec);
    printf("pldm_max_latency_usec: %u\n", stats.pldm_max_latency_usec);
    printf("pldm_rxfifo_max_bytes: %u\n", stats.pldm_rxfifo_max_bytes);
    printf("pldm_rxfifo_full_reset: %u\n", stats.pldm_rxfifo_full_reset);
    printf("pldm_rxfifo_error_reset: %u\n", stats.pldm_rxfifo_error_reset);

    printf("\n# ioctl and error recovery\n");
    printf("arp_reset: %u\n", stats.arp_reset);
    printf("slave_reset: %u\n", stats.slave_reset);
    printf("reset_all_fifos: %u\n", stats.reset_all_fifos);

    printf("\n# i2c layer\n");
    printf("i2c_transfer_success: %u\n", stats.i2c_transfer_success);
    printf("i2c_transfer_error: %u\n", stats.i2c_transfer_error);

    return rc;
}

int
main(int argc, char *argv[])
{
    unsigned int option;
    unsigned int data;
    int rc = 0;
    int fd;

    if (argc == 1) {
        usage();
        return -1;
    } else if (strcmp(argv[1], "-r") == 0) {
        option = SLAVE_RESET;

    } else if (strcmp(argv[1], "-e") == 0) {
        option = GET_EID;

    } else if (strcmp(argv[1], "-s") == 0) {
        option = SLAVE_ADDR;

    } else if (strcmp(argv[1], "-a") == 0) {
        option = ARP_RESET;

    } else if (strcmp(argv[1], "-i") == 0) {
        option = MCTP_INFO;

    } else {
        usage();
        return -2;
    }

    fd = open("/dev/mctp", O_RDWR);
    if (fd == -1)
    {
        perror("mctp open");
        return -3;
    }

    switch (option) {
    case SLAVE_RESET:
        rc = ioctl(fd, SLAVE_RESET, &data);
        if (rc)
            perror("mctp reset ioctl error");
        break;

    case GET_EID:
        rc = ioctl(fd, GET_EID, &data);
        if (rc)
            perror("mctp eid ioctl error");
        else
            fprintf(stdout, "0x%x\n", data);
        break;

    case SLAVE_ADDR:
        rc = ioctl(fd, SLAVE_ADDR, &data);
        if (rc)
            perror("mctp slave addr ioctl error");
        else
            fprintf(stdout, "0x%x\n", data);
        break;

    case ARP_RESET:
        rc = ioctl(fd, ARP_RESET, &data);
        if (rc)
            perror("smbus arp reset ioctl error");
        else
            fprintf(stdout, "success\n");
        break;

    case MCTP_INFO:
        rc = get_info(fd);
        if (rc)
            printf("ioctl error - %d\n", rc);
        break;

    default:
        usage();
        break;
    }
    close (fd);

    return rc;
}
