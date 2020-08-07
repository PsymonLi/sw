/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#ifndef __RBT_TRANSPORT_H__
#define __RBT_TRANSPORT_H__

#include <string>
#include <sys/ioctl.h>
#include <net/ethernet.h>

//uint8_t pkt[1514];

namespace sdk {
namespace platform {
namespace ncsi {

#define NCSI_PROTOCOL_ETHERTYPE 0x88F8

class rbt_transport : public transport {
private:
    int sock_fd;
    int ifindex;
    struct ifreq ifr;
    struct sockaddr_ll sock_addr;

public:
    rbt_transport(const char* iface_name)
    {
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), iface_name);
        xport_name = std::string("RBT_") + std::string(iface_name);
    };

    int GetFd() { return sock_fd; };
    int Init() {
        int rc;
        const unsigned char ether_broadcast_addr[]=
                {0xff,0xff,0xff,0xff,0xff,0xff};

        printf("opening raw socket now...\n");
        
        //TODO: Implement DeInit() method in this class to close socket()
        sock_fd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(NCSI_PROTOCOL_ETHERTYPE));

        memset(&sock_addr, 0, sizeof(sock_addr));

        if (ioctl(sock_fd, SIOCGIFINDEX, &ifr)==-1) {
                printf("%s",strerror(errno));
        }
        ifindex=ifr.ifr_ifindex;

        sock_addr.sll_family=AF_PACKET;
        sock_addr.sll_ifindex=ifindex;
        sock_addr.sll_halen=ETHER_ADDR_LEN;
        sock_addr.sll_protocol=htons(NCSI_PROTOCOL_ETHERTYPE);
        memcpy(sock_addr.sll_addr,ether_broadcast_addr,ETHER_ADDR_LEN);

        printf("Opened RAW socket and seting socket options now\n");
        rc = setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr));

        if (rc < 0) {
            printf("socket option SO_BINDTODEVICE failed\n");
            return rc;
        }
        else {
            printf("Opened socket succesfully for interface: %s\n", ifr.ifr_name);
            //SendPkt(dummy_rsp, sizeof(dummy_rsp));
            return sock_fd;
        }
    };
    ssize_t SendPkt(const void *buf, size_t len) 
    {
        ssize_t ret;
        uint8_t ncsi_eth_hdr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02,
            0x02, 0x02, 0x02, 0x02, 0x02, 0x88, 0xF8};
        uint8_t *ncsi_eth_buf = ((uint8_t *)buf - sizeof(struct ethhdr));

        memcpy(ncsi_eth_buf, ncsi_eth_hdr, sizeof(ncsi_eth_hdr));

        ret = sendto(sock_fd, ncsi_eth_buf, len + sizeof(struct ethhdr), 0,
                (struct sockaddr*)&sock_addr,sizeof(sock_addr));
        if (ret < 0)
            printf("Error in sending packet out over RBT interface: error: %ld",
                    ret);
        
        free(ncsi_eth_buf);

        return ret;
    };

    ssize_t RecvPkt(void *buf, size_t len, size_t& ncsi_hdr_offset) 
    {
        ssize_t ret = recvfrom(sock_fd, buf, len /*ETH_FRAME_LEN*/, 0, NULL, NULL);
        ncsi_hdr_offset = sizeof(struct ethhdr);
#if 0
        uint8_t *ptr = (uint8_t *)buf;
        printf("\n pkt received on %s interface: \n", ifr.ifr_name);
        for (uint32_t i = 0; i < ret; i++)
        {
            printf("%02x ", ptr[i]);
            if (!((i+1) % 16))
                printf("\n");
        }
        printf("\n");
#endif
        return ret;
    };

    void *GetNcsiRspPkt(ssize_t sz)
    {
        void *ptr = calloc(sizeof(struct ethhdr) + sz, sizeof(uint8_t));
        return ((uint8_t *)ptr + sizeof(struct ethhdr));
    }

};

} // namespace ncsi
} // namespace platform
} // namespace sdk

#endif //__RBT_TRANSPORT_H__

