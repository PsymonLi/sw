#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <net/ethernet.h>
#include <net/route.h>
#include <zmq.h>
#include <assert.h>
#include "nic/model_sim/include/buf_hdr.h"
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include "nic/model_sim/include/lib_model_client.h"
#include "nic/model_sim/include/buf_hdr.h"
#include "nic/proxy-e2etest/lib_driver.hpp"
#include "nic/proxy-e2etest/ntls.hpp"

#define HNTAP_HOST_TAPIF        "hntap_host0"
#define HNTAP_HOST_TAPIF_IP     "64.1.0.4"
#define HNTAP_HOST_TAPIF_IPMASK "255.255.255.0"
#define HNTAP_HOST_ROUTE_DESTIP "64.0.0.1"
#define HNTAP_HOST_ROUTE_GWIP   "64.1.0.4"

#define HNTAP_NET_TAPIF         "hntap_net0"
#define HNTAP_NET_TAPIF_IP      "64.0.0.2"
#define HNTAP_NET_TAPIF_IPMASK  "255.255.255.0"
#define HNTAP_NET_ROUTE_DESTIP  "64.1.0.3"
#define HNTAP_NET_ROUTE_GWIP    "64.0.0.2"

#define PKTBUF_LEN              2000

#define HNTAP_LIF_ID      15 
#define HNTAP_QSTATE_ADDR 0xa4512000
#define HNTAP_IOMEM_BASE  0xb39d2000

int host_tap_fd, net_tap_fd;

int hntap_dump_pkt(char *pkt, int len);
void hntap_nat(char *pkt, int len);

extern int tls_server_main(int argv, char *argc[]);

bool hntap_go_thru_model = true; // Go thru model for host<->nw talk
bool hntap_drop_rexmit = false;
bool hntap_allow_udp = false;
uint32_t hflow_seqnum = 0, nflow_seqnum = 0;
uint32_t nw_retries = 0;


#define ETH_ADDR_LEN 6

struct ether_header_t
{
    uint8_t  dmac[ETH_ADDR_LEN];      /* destination eth addr */
    uint8_t  smac[ETH_ADDR_LEN];      /* source ether addr    */
    uint16_t etype;                   /* ether type */
} __attribute__ ((__packed__));

struct vlan_header_t
{
    uint8_t  dmac[ETH_ADDR_LEN];      /* destination eth addr */
    uint8_t  smac[ETH_ADDR_LEN];      /* source ether addr    */
    uint16_t tpid;                    /* Tag protocol id*/
    uint16_t vlan_tag;                /* dot1p +cfi + vlan-id */
    uint16_t etype;                   /* ether type */
} __attribute__ ((__packed__));

struct ipv4_header_t {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t    version:4;
    uint8_t    ihl:4;
#else
    uint8_t    ihl:4;
    uint8_t    version:4;
#endif
    uint8_t    tos;
    uint16_t   tot_len;
    uint16_t   id;
    uint16_t   frag_off;
    uint8_t    ttl;
    uint8_t    protocol;
    uint16_t   check;
    uint32_t   saddr;
    uint32_t   daddr;
    /*The options start here. */
}__attribute__ ((__packed__));

struct tcp_header_t {
    uint16_t  sport;
    uint16_t  dport;
    uint32_t  seq;
    uint32_t  ack_seq;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint16_t   doff:4,
        res1:4,
        cwr:1,
        ece:1,
        urg:1,
        ack:1,
        psh:1,
        rst:1,
        syn:1,
        fin:1;
#else
    uint16_t   res1:4,
        doff:4,
        fin:1,
        syn:1,
        rst:1,
        psh:1,
        ack:1,
        urg:1,
        ece:1,
        cwr:1;
#endif
    uint16_t  window;
    uint16_t  check;
    uint16_t  urg_ptr;
}__attribute__ ((__packed__));



struct tcp_pseudo /*the tcp pseudo header*/
{
  uint32_t src_addr;
  uint32_t dst_addr;
  uint8_t zero;
  uint8_t proto;
  uint16_t length;
} pseudohead;

struct tcp_header_t hflow_tcp, nflow_tcp;

long checksum(unsigned short *addr, unsigned int count) {
  /* Compute Internet Checksum for "count" bytes
   *         beginning at location "addr".
   */
  register long sum = 0;


  while( count > 1 )  {
    /*  This is the inner loop */
    sum += * addr++;
    if(sum & 0x80000000)   /* if high order bit set, fold */
      sum = (sum & 0xFFFF) + (sum >> 16);
    count -= 2;
  }
  /*  Add left-over byte, if any */
  if( count > 0 )
    sum += * (unsigned char *) addr;

  /*  Fold 32-bit sum to 16 bits */
  while (sum>>16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}



long get_tcp_checksum(struct ipv4_header_t * myip, struct tcp_header_t * mytcp) {

  uint16_t total_len = ntohs(myip->tot_len);

  int tcpopt_len = mytcp->doff*4 - 20;
  int tcpdatalen = total_len - (mytcp->doff*4) - (myip->ihl*4);

  pseudohead.src_addr=myip->saddr;
  pseudohead.dst_addr=myip->daddr;
  pseudohead.zero=0;
  pseudohead.proto=IPPROTO_TCP;
  pseudohead.length=htons(sizeof(struct tcp_header_t) + tcpopt_len + tcpdatalen);

  int totaltcp_len = sizeof(struct tcp_pseudo) + sizeof(struct tcp_header_t) + tcpopt_len + tcpdatalen;
  unsigned short * tcp = new unsigned short[totaltcp_len];


  memcpy((unsigned char *)tcp,&pseudohead,sizeof(struct tcp_pseudo));
  memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo),(unsigned char *)mytcp,sizeof(struct tcp_header_t));
  memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo)+sizeof(struct tcp_header_t), (unsigned char *)myip+(myip->ihl*4)+(sizeof(struct tcp_header_t)), tcpopt_len);
  memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo)+sizeof(struct tcp_header_t)+tcpopt_len, (unsigned char *)mytcp+(mytcp->doff*4), tcpdatalen);


  return checksum(tcp,totaltcp_len);

}

void 
hntap_nat_worker(char *pkt, int len, bool source_ip, uint32_t orig_addr, uint32_t to_addr)
{

#if 0
  int i;
  for (i = 0; i< len; i++) {
    if (i % 16 == 0) {
      printf("\n");
    }
    printf(" 0x%02x", (unsigned char)pkt[i]);
  }
  printf("\n");
#endif

  struct ether_header_t *eth;
  struct vlan_header_t *vlan;
  struct ipv4_header_t *ip;
  struct tcp_header_t *tcp;
  uint16_t etype;
  eth = (struct ether_header_t *)pkt;
  if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
    vlan = (struct vlan_header_t*)pkt;
    TLOG(" ETH-VLAN: DMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x SMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x vlan=%d proto=0x%x\n",
           vlan->dmac[0], vlan->dmac[1], vlan->dmac[2], vlan->dmac[3], vlan->dmac[4], vlan->dmac[5],
           vlan->smac[0], vlan->smac[1], vlan->smac[2], vlan->smac[3], vlan->smac[4], vlan->smac[5],
           ntohs(vlan->vlan_tag),
           ntohs(vlan->etype));
    etype = ntohs(vlan->etype);
    ip = (ipv4_header_t *)(vlan+1);
  } else {
    TLOG(" ETH: DMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x SMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x proto=0x%x\n",
           eth->dmac[0], eth->dmac[1], eth->dmac[2], eth->dmac[3], eth->dmac[4], eth->dmac[5],
           eth->smac[0], eth->smac[1], eth->smac[2], eth->smac[3], eth->smac[4], eth->smac[5],
           ntohs(eth->etype));
    etype = ntohs(eth->etype);
    ip = (ipv4_header_t *)(eth+1);
  }

  if (etype == ETHERTYPE_IP) {
    TLOG(" IP BEFORE1: tos=%d tot_len=%d id=0x%x frag_off=0x%x ttl=%d protocol=%d check=0x%x saddr=0x%x daddr=0x%x\n",
            ip->tos, ntohs(ip->tot_len), ntohs(ip->id), ntohs(ip->frag_off), 
            ip->ttl, ip->protocol, ntohs(ip->check), ntohl(ip->saddr), ntohl(ip->daddr));

    ip->check = 0;
    ip->check = checksum((unsigned short *)ip, 20);


    TLOG(" IP BEFORE2: tos=%d tot_len=%d id=0x%x frag_off=0x%x ttl=%d protocol=%d check=0x%x saddr=0x%x daddr=0x%x\n",
            ip->tos, ntohs(ip->tot_len), ntohs(ip->id), ntohs(ip->frag_off), 
            ip->ttl, ip->protocol, ntohs(ip->check), ntohl(ip->saddr), ntohl(ip->daddr));

    if (ip->protocol == IPPROTO_TCP) {
      tcp = (struct tcp_header_t*)(ip+1);
      TLOG(" TCP BEFORE1: sp=0x%x dp=0x%x seq=0x%x ack_seq=0x%x doff=%d res1=%d %s%s%s%s%s%s%s%s wnd=0x%x check=0x%x urg_ptr=0x%x\n",
              ntohs(tcp->sport), ntohs(tcp->dport), ntohl(tcp->seq), ntohl(tcp->ack_seq),
              tcp->doff, tcp->res1,
              tcp->fin ? "F" : "",
              tcp->syn ? "S" : "",
              tcp->rst ? "R" : "",
              tcp->psh ? "P" : "",
              tcp->ack ? "A" : "",
              tcp->urg ? "U" : "",
              tcp->ece ? "E" : "",
              tcp->cwr ? "C" : "",
              ntohs(tcp->window),
              ntohs(tcp->check),
              ntohs(tcp->urg_ptr));
    }

    if (ip->protocol == IPPROTO_TCP) {
      tcp = (struct tcp_header_t*)(ip+1);
      tcp->check = 0;
      tcp->check = get_tcp_checksum(ip, tcp);

      TLOG(" TCP BEFORE2: sp=0x%x dp=0x%x seq=0x%x ack_seq=0x%x doff=%d res1=%d %s%s%s%s%s%s%s%s wnd=0x%x check=0x%x urg_ptr=0x%x\n",
              ntohs(tcp->sport), ntohs(tcp->dport), ntohl(tcp->seq), ntohl(tcp->ack_seq),
              tcp->doff, tcp->res1,
              tcp->fin ? "F" : "",
              tcp->syn ? "S" : "",
              tcp->rst ? "R" : "",
              tcp->psh ? "P" : "",
              tcp->ack ? "A" : "",
              tcp->urg ? "U" : "",
              tcp->ece ? "E" : "",
              tcp->cwr ? "C" : "",
              ntohs(tcp->window),
              ntohs(tcp->check),
              ntohs(tcp->urg_ptr));
    }

    if (source_ip) {
      if (ntohl(ip->saddr) == orig_addr) {
        TLOG("NAT IPSA: 0x%x -> 0x%x\n", orig_addr, to_addr);
        
        ip->saddr = htonl(to_addr);
        ip->check = 0;
        ip->check = checksum((unsigned short *)ip, 20);
      } else {
        TLOG("No NAT\n");
      }
    } else {

      if (ntohl(ip->daddr) == orig_addr) {
        TLOG("NAT IPDA: 0x%x -> 0x%x\n", orig_addr, to_addr);
        
        ip->daddr = htonl(to_addr);
        ip->check = 0;
        ip->check = checksum((unsigned short *)ip, 20);

      } else {
        TLOG("No NAT\n");
      }
    }
    TLOG(" IP AFTER: tos=%d tot_len=%d id=0x%x frag_off=0x%x ttl=%d protocol=%d check=0x%x saddr=0x%x daddr=0x%x\n",
            ip->tos, ntohs(ip->tot_len), ntohs(ip->id), ntohs(ip->frag_off), 
            ip->ttl, ip->protocol, ntohs(ip->check), ntohl(ip->saddr), ntohl(ip->daddr));

    if (ip->protocol == IPPROTO_TCP) {
      tcp = (struct tcp_header_t*)(ip+1);
#if 0
      TLOG("NAT DP: 80 -> 10080\n");
      tcp->dport = htons(10080);
#endif
      tcp->check = 0;
      tcp->check = get_tcp_checksum(ip, tcp);

      TLOG(" TCP AFTER: sp=0x%x dp=0x%x seq=0x%x ack_seq=0x%x doff=%d res1=%d %s%s%s%s%s%s%s%s wnd=0x%x check=0x%x urg_ptr=0x%x\n",
              ntohs(tcp->sport), ntohs(tcp->dport), ntohl(tcp->seq), ntohl(tcp->ack_seq),
              tcp->doff, tcp->res1,
              tcp->fin ? "F" : "",
              tcp->syn ? "S" : "",
              tcp->rst ? "R" : "",
              tcp->psh ? "P" : "",
              tcp->ack ? "A" : "",
              tcp->urg ? "U" : "",
              tcp->ece ? "E" : "",
              tcp->cwr ? "C" : "",
              ntohs(tcp->window),
              ntohs(tcp->check),
              ntohs(tcp->urg_ptr));
    }
  }
}

void 
hntap_host_client_to_server_nat(char *pkt, int len)
{
  hntap_nat_worker(pkt, len, true, 0x40010004, 0x40010003);
}
void 
hntap_net_client_to_server_nat(char *pkt, int len)
{
  hntap_nat_worker(pkt, len, false, 0x40000001, 0x40000002);
}
void 
hntap_net_server_to_client_nat(char *pkt, int len)

{
  hntap_nat_worker(pkt, len, true, 0x40000002, 0x40000001);
}
void 
hntap_host_server_to_client_nat(char *pkt, int len)
{
  hntap_nat_worker(pkt, len, false, 0x40010003, 0x40010004);
}

void
hntap_net_ether_header_add(char *pkt)
{
  struct ether_header_t *eth = (struct ether_header_t *)pkt;

  eth->dmac[0] = 0x00;
  eth->dmac[1] = 0xEE;
  eth->dmac[2] = 0x00;
  eth->dmac[3] = 0x00;
  eth->dmac[4] = 0x00;
  eth->dmac[5] = 0x04;

  eth->smac[0] = 0x00;
  eth->smac[1] = 0xEE;
  eth->smac[2] = 0xFF;
  eth->smac[3] = 0x00;
  eth->smac[4] = 0x00;
  eth->smac[5] = 0x04;

  eth->etype   = htons(0x0800);
}

void
hntap_host_ether_header_add(char *pkt)
{
  struct vlan_header_t *vlan = (struct vlan_header_t *)pkt;

  vlan->dmac[0] = 0x00;
  vlan->dmac[1] = 0xEE;
  vlan->dmac[2] = 0xFF;
  vlan->dmac[3] = 0x00;
  vlan->dmac[4] = 0x00;
  vlan->dmac[5] = 0x04;

  vlan->smac[0] = 0x00;
  vlan->smac[1] = 0xEE;
  vlan->smac[2] = 0x00;
  vlan->smac[3] = 0x00;
  vlan->smac[4] = 0x00;
  vlan->smac[5] = 0x04;

  vlan->tpid    = htons(0x8100);
  vlan->vlan_tag= htons(3003);
  vlan->etype   = htons(0x0800);
}


int hntap_route_add (int sockfd, const char *dest_addr, const char *gateway_addr) {
  
  struct rtentry     route;
  struct sockaddr_in *addr;

  int err = 0;
  memset(&route, 0, sizeof(route));
  addr = (struct sockaddr_in*) &route.rt_gateway;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr(gateway_addr);
  addr = (struct sockaddr_in*) &route.rt_dst;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr(dest_addr);

  route.rt_flags = RTF_HOST;

  if ((err = ioctl(sockfd, SIOCADDRT, &route)) < 0) {
      perror("Route add failed");
      return -1;
  }
  return 1;
}                         

int hntap_create_tundev (const char *dev, const char *dev_ip, const char *dev_ipmask,
			 const char *route_dest, const char *route_gw)
{
  struct ifreq ifr;
  int	   fd, err, sock;
  const char *tapdev = "/dev/net/tun";

  if ((fd = open(tapdev, O_RDWR)) < 0 ) {
      TLOG("Failed to open Tap device %s\n", strerror(errno));
      abort();
      return fd;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  /* create the device */
  if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
      TLOG("Failed to set Tap device property %s\n", strerror(errno));
      close(fd);
      abort();
      return err;
  }

  sock = socket(PF_PACKET,SOCK_DGRAM,0);
  if (sock < 0) {
      TLOG("Failed to open socket %s\n", strerror(errno));
      close(fd);
      abort();
      return sock;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_UP | IFF_RUNNING | IFF_PROMISC; 
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	
  /* Set the device UP */
  if ((err = ioctl(sock, SIOCSIFFLAGS, (void *) &ifr)) < 0 ) {
    TLOG("Failed to bring up Tap device %s\n", strerror(errno));
    close(sock);
    close(fd);
    abort();
    return err;
  }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr((const char *)dev_ip);
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr)); 

  if ((err = ioctl(sock, SIOCSIFADDR, &ifr)) < 0) {
    perror("IP address config failed");
    close(sock);
    close(fd);
    abort();
    return err;
  }

  /*
   * Add the netmask.
   */
  memset(&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr((const char *)dev_ipmask);
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

  if ((err = ioctl(sock, SIOCSIFNETMASK, &ifr)) < 0) {
    perror("IP address mask config failed");
    close(sock);
    close(fd);
    abort();
    return err;
  }

  /*
   * Add a route to the host/nw dest reachable thru this tap device.
   */
  if ((err = hntap_route_add(sock, route_dest, route_gw)) < 0) {
      perror("IP Route add failed");
      close(sock);
      close(fd);
      abort();
      return err;
  }

  return fd;
}


int hntap_create_tapdev (const char *dev, const char *dev_ip, const char *dev_ipmask)
{
  struct ifreq ifr;
  int	   fd, err, sock;
  const char *tapdev = "/dev/net/tun";

  if ((fd = open(tapdev, O_RDWR)) < 0 ) {
    abort();
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  /* create the device */
  if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
    close(fd);
    perror("2\n");
    return err;
  }

  sock = socket(PF_PACKET,SOCK_DGRAM,0);
  if (sock < 0) {
    close(fd);
    perror("3\n");
    return err;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_UP | IFF_RUNNING | IFF_PROMISC; 
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	
  /* Set the device UP */
  if ((err = ioctl(sock, SIOCSIFFLAGS, (void *) &ifr)) < 0 ) {
    close(sock);
    close(fd);
    abort();
    return err;
  }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr((const char *)dev_ip);
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr)); 

  if ((err = ioctl(sock, SIOCSIFADDR, &ifr)) < 0) {
    perror("IP address config failed");
    close(sock);
    close(fd);
    abort();
    return err;
  }

  memset(&sa, 0, sizeof(struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr((const char *)dev_ipmask);
  memcpy(&ifr.ifr_addr, &sa, sizeof(struct sockaddr));

  if ((err = ioctl(sock, SIOCSIFNETMASK, &ifr)) < 0) {
    perror("IP address mask config failed");
    close(sock);
    close(fd);
    abort();
    return err;
  }
   
  return fd;
}

void
hntap_host_tx_to_model (char *pktbuf, int size)
{
  uint16_t lif_id = (uint16_t) (HNTAP_LIF_ID & 0xffff);
  uint32_t port = 0, cos = 0, count;

  uint8_t *pkt = (uint8_t *) pktbuf;
  std::vector<uint8_t> ipkt, opkt;
  uint8_t *buf;

  ipkt.resize(size);
  opkt.resize(size);
  memcpy(ipkt.data(), pkt, size);

  if (!hntap_go_thru_model) {

      /*
       * Bypass the model and send packet to Network-tap directly. Test only
       */
      memcpy(opkt.data(), pkt, size);
      TLOG("Host-Tx: Bypassing model, packet size: %d (%d), on port: %d cos %d\n",
              opkt.size(), size, port, cos);
      goto send_to_nettap;
  }

  lib_model_connect();

  // --------------------------------------------------------------------------------------------------------//

  // Create Queues
  alloc_queue(lif_id, TX, 0, 1024);
  alloc_queue(lif_id, RX, 0, 1024);

  // --------------------------------------------------------------------------------------------------------//

  // Post tx buffer
  buf = alloc_buffer(ipkt.size());
  assert(buf != NULL);
  memcpy(buf, ipkt.data(), ipkt.size());
  TLOG("buf %p size %lu\n", buf, ipkt.size());
  hntap_dump_pkt((char *)buf, ipkt.size());

  // Transmit Packet
  TLOG("Writing packet to model! size: %d on port: %d\n", ipkt.size(), port);
  post_buffer(lif_id, TX, 0, buf, ipkt.size());

  // Wait for packet to come out of port
  count = 0;
  do {
     get_next_pkt(opkt, port, cos);
     if (opkt.size() == 0) {usleep(10000); count++;}
  } while (count < (nw_retries*100) && !opkt.size());
  if (!opkt.size()) {
      TLOG("NO packet back from model! size: %d\n", opkt.size());
  } else {
      TLOG("Got packet back from model! size: %d on port: %d cos %d\n", opkt.size(), port, cos);
  }

  lib_model_conn_close();

 send_to_nettap:

  if (opkt.size()) {
    /*
     * Now that we got the packet from the Model, lets send it out on the Net-Tap interface.
     */
    int nwrite;
    hntap_net_client_to_server_nat((char *)opkt.data(), opkt.size());
    if (hntap_go_thru_model) {
        nwrite = write(net_tap_fd, opkt.data()+sizeof(struct ether_header_t), opkt.size()-sizeof(struct ether_header_t));
    } else {

        /*
         * When not going thru model, we'll send the original packet as-is to the net-tap.
	 */
        nwrite = write(net_tap_fd, opkt.data() + sizeof(struct vlan_header_t), opkt.size() - sizeof(struct vlan_header_t));
    }

    if (nwrite < 0) {
        perror("Host-Tx: Writing data to net-tap");
    } else {
        TLOG("Wrote packet with %lu bytes (%d) to Network Tap (Tx)\n", opkt.size() - sizeof(struct ether_header_t), nwrite);
    }
  }
}

uint16_t
hntap_get_etype(struct ether_header_t *eth)
{
  struct vlan_header_t *vlan;
  if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
    vlan = (struct vlan_header_t *)eth;
    return ntohs(vlan->etype);
  }
  return ntohs(eth->etype);
}

void
hntap_handle_host_tx (char *pktbuf, int nread)
{
  struct ether_header_t *eth_header;

  TLOG("Host-Tx to Model: packet sent with %d bytes\n", nread);
  if (nread < (int) sizeof(struct ether_header)) return;

  hntap_dump_pkt(pktbuf, nread);
  eth_header = (struct ether_header_t *) pktbuf;

  if (hntap_get_etype(eth_header) == ETHERTYPE_IP) {
    TLOG("Ether-type IP, sending to Model\n");
    hntap_host_client_to_server_nat(pktbuf, nread);
    hntap_host_tx_to_model(pktbuf, nread);
  } else {
    TLOG("Ether-type 0x%x IGNORED\n", ntohs(eth_header->etype));
  }

}

void
hntap_net_rx_to_model (char *pktbuf, int size)
{
  uint16_t lif_id = (uint16_t)(HNTAP_LIF_ID & 0xffff);
  uint32_t port = 0;
  uint16_t rsize = 0;
  uint16_t prev_cindex = 0xFFFF;

  uint8_t *pkt = (uint8_t *) pktbuf;
  std::vector<uint8_t> ipkt,opkt;
  uint8_t *buf;
  bool got_nw_pkt = false;


  ipkt.resize(size);
  opkt.resize(size);

  memcpy(ipkt.data(), pkt, size);

  if (!hntap_go_thru_model) {

      /*
       * Bypass the model and send packet to Host-tap directly. Test only.
       */
      buf = (uint8_t *) pktbuf;
      rsize = size;
      TLOG("Net-Rx: Bypassing model, packet size %d!\n", rsize);
      goto send_to_hosttap;

  }

  lib_model_connect();

  // --------------------------------------------------------------------------------------------------------//

  // Create Queues
  alloc_queue(lif_id, TX, 0, 1024);
  alloc_queue(lif_id, RX, 0, 1024);

  // --------------------------------------------------------------------------------------------------------//

  // Post buffer
  buf = alloc_buffer(9126);
  assert(buf != NULL);
  memset(buf, 0, 9126);
  post_buffer(lif_id, RX, 0, buf, 9126);

  // Send packet to Model
  TLOG("Sending packet to model! size: %d on port: %d\n", ipkt.size(), port);
  step_network_pkt(ipkt, port);

#define POLL_RETRIES 4

  if (poll_queue(lif_id, RX, 0, POLL_RETRIES, &prev_cindex)) {
    TLOG("Got some packet\n");
    // Receive Packet
    consume_buffer(lif_id, RX, 0, buf, &rsize);
    if (!rsize) {
        TLOG("Did not get packet back from host side of model!\n");
    } else {
        TLOG("Got packet of size %d bytes back from host side of model!\n", rsize);
    }
  } else {
    uint32_t port = 0, cos = 0;
    uint32_t count = 0;
    do {
       get_next_pkt(opkt, port, cos);
       if (opkt.size() == 0) {usleep(10000); count++;}
    } while (count < (nw_retries*100) && !opkt.size());
    if (!opkt.size()) {
        TLOG("NO packet back from nw side of model! size: %d\n", opkt.size());
    } else {
        TLOG("Got packet back from nw side of model! size: %d on port: %d cos %d\n", opkt.size(), port, cos);
	got_nw_pkt = true;
    }
    
  }

  if (got_nw_pkt) {

    /*
     * Now that we got the packet from the Model, lets send it out on the Net-Tap interface.
     */
    int nwrite;

    hntap_net_client_to_server_nat((char *)opkt.data(), opkt.size());

    if ((nwrite = write(net_tap_fd, opkt.data()+sizeof(struct ether_header_t), opkt.size()-sizeof(struct ether_header_t))) < 0){
      perror("Net-Rx: Writing data to net-tap");
    } else {
      TLOG("Wrote packet with %lu bytes to Network Tap (Rx)\n", opkt.size() - sizeof(struct ether_header_t));
    }
  }

  if (!rsize && poll_queue(lif_id, RX, 0, POLL_RETRIES, &prev_cindex)) {
      TLOG("Got some packet\n");
    // Receive Packet                                                                                                                                            
    consume_buffer(lif_id, RX, 0, buf, &rsize);
    if (!rsize) {
        TLOG("Did not get packet back from host side of model!\n");
    } else {
        TLOG("Got packet of size %d bytes back from host side of model!\n", rsize);
    }
  }


 send_to_hosttap:

  if (rsize) {
    hntap_dump_pkt((char *)buf, rsize);
    /*
     * Now that we got the packet from the Model, lets send it out on the Host-Tap interface.
     */
    int nwrite;

    hntap_host_server_to_client_nat((char*)buf,rsize);
    if (hntap_go_thru_model) {

        nwrite = write(host_tap_fd, buf + sizeof(struct vlan_header_t), 
		       rsize - sizeof(struct vlan_header_t));
    } else {

        /*
         * When not going thru model, we'll send the original packet as-is to the host-tap.
	 */
        nwrite = write(host_tap_fd, buf + sizeof(struct ether_header_t), rsize - sizeof(struct ether_header_t));
    }

    if (nwrite < 0) {
      perror("Net-Rx: Writing data to host-tap");
    } else {
      TLOG("Wrote packet with %lu bytes to Host Tap (Tx)\n", rsize - sizeof(struct vlan_header_t));
    }
  }

  lib_model_conn_close();
}

void
hntap_handle_net_rx (char *pktbuf, int nread)
{
  struct ether_header_t *eth_header;

  TLOG("Net-Rx to Model: packet sent with %d bytes\n", nread);
  if (nread < (int) sizeof(struct ether_header)) return;

  hntap_dump_pkt(pktbuf, nread);
  eth_header = (struct ether_header_t *) pktbuf;
  if ((ntohs(eth_header->etype) == ETHERTYPE_IP) ||
      (ntohs(eth_header->etype) == ETHERTYPE_VLAN)) {
      TLOG("Ether-type IP, sending to Model\n");
      hntap_net_server_to_client_nat(pktbuf, nread);
      hntap_net_rx_to_model(pktbuf, nread);
  } else {
    TLOG("Ether-type 0x%x IGNORED\n", ntohs(eth_header->etype));
  }

}

int 
hntap_dump_pkt(char *pkt, int len)
{
  int i;
  for (i = 0; i< len; i++) {
    if (i % 16 == 0) {
      printf("\n");
    }
    printf(" 0x%02x", (unsigned char)pkt[i]);
  }
  printf("\n");

  struct ether_header_t *eth;
  struct vlan_header_t *vlan;
  struct ipv4_header_t *ip;
  struct tcp_header_t *tcp;
  uint16_t etype;
  eth = (struct ether_header_t *)pkt;
  if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
    vlan = (struct vlan_header_t*)pkt;
    TLOG(" ETH-VLAN: DMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x SMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x vlan=%d proto=0x%x\n",
           vlan->dmac[0], vlan->dmac[1], vlan->dmac[2], vlan->dmac[3], vlan->dmac[4], vlan->dmac[5],
           vlan->smac[0], vlan->smac[1], vlan->smac[2], vlan->smac[3], vlan->smac[4], vlan->smac[5],
           ntohs(vlan->vlan_tag),
           ntohs(vlan->etype));
    etype = ntohs(vlan->etype);
    ip = (ipv4_header_t *)(vlan+1);
  } else {
    TLOG(" ETH: DMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x SMAC=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x proto=0x%x\n",
           eth->dmac[0], eth->dmac[1], eth->dmac[2], eth->dmac[3], eth->dmac[4], eth->dmac[5],
           eth->smac[0], eth->smac[1], eth->smac[2], eth->smac[3], eth->smac[4], eth->smac[5],
           ntohs(eth->etype));
    etype = ntohs(eth->etype);
    ip = (ipv4_header_t *)(eth+1);
  }

  if (etype == ETHERTYPE_IP) {

    TLOG(" IP: tos=%d tot_len=%d id=0x%x frag_off=0x%x ttl=%d protocol=%d check=0x%x saddr=0x%x daddr=0x%x\n",
            ip->tos, ntohs(ip->tot_len), ntohs(ip->id), ntohs(ip->frag_off), 
            ip->ttl, ip->protocol, ntohs(ip->check), ntohl(ip->saddr), ntohl(ip->daddr));

    if (ip->protocol == IPPROTO_TCP) {
      tcp = (struct tcp_header_t*)(ip+1);
      TLOG(" TCP: sp=0x%x dp=0x%x seq=0x%x ack_seq=0x%x doff=%d res1=%d %s%s%s%s%s%s%s%s wnd=0x%x check=0x%x urg_ptr=0x%x\n",
              ntohs(tcp->sport), ntohs(tcp->dport), ntohl(tcp->seq), ntohl(tcp->ack_seq),
              tcp->doff, tcp->res1,
              tcp->fin ? "F" : "",
              tcp->syn ? "S" : "",
              tcp->rst ? "R" : "",
              tcp->psh ? "P" : "",
              tcp->ack ? "A" : "",
              tcp->urg ? "U" : "",
              tcp->ece ? "E" : "",
              tcp->cwr ? "C" : "",
              ntohs(tcp->window),
              ntohs(tcp->check),
              ntohs(tcp->urg_ptr));
    } else if (!hntap_allow_udp || (ip->protocol != IPPROTO_UDP)) return -1;
  }
  return 0;
}

int
hntap_do_drop_rexmit(char *pkt, int len, uint32_t *seqnum, struct tcp_header_t *flowtcp)
{
  struct ether_header_t *eth;
  struct vlan_header_t *vlan;
  struct ipv4_header_t *ip;
  struct tcp_header_t *tcp;
  uint16_t etype;

  if (!hntap_drop_rexmit) return(0);

  eth = (struct ether_header_t *)pkt;
  if (ntohs(eth->etype) == ETHERTYPE_VLAN) {
    vlan = (struct vlan_header_t*)pkt;
    etype = ntohs(vlan->etype);
    ip = (ipv4_header_t *)(vlan+1);
  } else {
    etype = ntohs(eth->etype);
    ip = (ipv4_header_t *)(eth+1);
  }

  if (etype == ETHERTYPE_IP) {

    if (ip->protocol == IPPROTO_TCP) {
      tcp = (struct tcp_header_t*)(ip+1);
#if 0
      if (ntohl(tcp->seq) == *seqnum) {
          TLOG("Same sequence-number 0x%x\n", *seqnum);
          return(1);
      }
      *seqnum = ntohl(tcp->seq);
#endif
      if(tcp->rst)
          return(1);	
      
      *seqnum = ntohl(tcp->seq);
      if (tcp->seq == flowtcp->seq &&
          tcp->ack_seq == flowtcp->ack_seq &&
          tcp->fin == flowtcp->fin &&
          tcp->syn == flowtcp->syn &&
          tcp->rst == flowtcp->rst &&
          tcp->psh == flowtcp->psh &&
          tcp->ack == flowtcp->ack &&
          tcp->urg == flowtcp->urg &&
          tcp->ece == flowtcp->ece &&
          tcp->cwr == flowtcp->cwr) {
          TLOG("Same tcp header fields\n");
          return(1);
      }
      memcpy(flowtcp, tcp, sizeof(tcp_header_t));
    }
  }
  return(0);
}

void
hntap_do_select_loop (void)
{
  int		  maxfd;
  uint16_t  nread;
  char	  pktbuf[PKTBUF_LEN];
  char    *p = pktbuf;
  unsigned long int num_hosttap_pkts = 0, num_nettap_pkts = 0;

  /* use select() to handle two descriptors at once */
  maxfd = (host_tap_fd > net_tap_fd) ? host_tap_fd : net_tap_fd;

  while (1) {

    int	   ret;
	fd_set rd_set;

	FD_ZERO(&rd_set);
	FD_SET(host_tap_fd, &rd_set);
	FD_SET(net_tap_fd, &rd_set);

	TLOG("Select on host_tap_fd %d, net_tap_fd %d\n", host_tap_fd, net_tap_fd);
	ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

	if (ret < 0 && errno == EINTR){
            TLOG("Ret = %d, errno %d\n", ret, errno);
            continue;
	}

	if (ret < 0) {
	  TLOG("select() failed: %s\n", strerror(errno));
	  abort();
	}
    
        TLOG("Got something\n");
	if (FD_ISSET(host_tap_fd, &rd_set)) {
            TLOG("Got stuff on host_tap_fd\n");

	    /*
	     *	Data received from host-tap. We'll read it and write to the Host-mem for model to read (Host-Tx path).
	     */
	    if ((nread = read(host_tap_fd, p+sizeof(struct vlan_header_t), PKTBUF_LEN)) < 0) {
	      TLOG("Read from host-tap failed - %s\n", strerror(errno));
	      abort();
	    }

	    if (p[sizeof(struct vlan_header_t)] != 0x45) {
	      TLOG("Not an IP packet 0x%x\n", p[sizeof(struct vlan_header_t)]);
	      continue;
	    }

	    num_hosttap_pkts++;
	    TLOG("Host-Tap %d: Read %d bytes from host tap interface\n", num_hosttap_pkts, nread);
	    hntap_host_ether_header_add(pktbuf);
	    int ret = hntap_dump_pkt(pktbuf, nread + sizeof(struct vlan_header_t));
            if (ret == -1) {
	      TLOG("Not an TCP packet\n");
	      continue;
            }

	    if (hntap_do_drop_rexmit(pktbuf, nread + sizeof(struct vlan_header_t), &hflow_seqnum, &hflow_tcp)) {
                TLOG("Retransmitted TCP packet, seqno: 0x%x, dropping\n", hflow_seqnum);
  	        continue;
	    }

	    /*
	     * Setup and write to the Host-memory interface for model to read.
	     */
	    hntap_handle_host_tx(pktbuf, nread + sizeof(struct vlan_header_t));
	}

	if (FD_ISSET(net_tap_fd, &rd_set)) {
        TLOG("Got stuff on net_tap_fd\n");
        /*
        * Data received from net-tap. We'll read it and write to the ZMQ to teh model (Net Rx path).
        */
        if ((nread = read(net_tap_fd, p+sizeof(struct ether_header_t), PKTBUF_LEN)) < 0) {
        perror("Read from net-tap failed!");
        abort();
        }

        if (p[sizeof(struct ether_header_t)] != 0x45) {
        TLOG("Not an IP packet 0x%x\n", p[sizeof(struct ether_header_t)]);
        continue;
        }

        num_nettap_pkts++;
           


        TLOG("Net-Tap %d: Read %d bytes from network tap interface\n", num_nettap_pkts, nread);
        hntap_net_ether_header_add(pktbuf);
        TLOG("Added ether header\n");
        int ret = hntap_dump_pkt(pktbuf, nread + sizeof(struct ether_header_t));
        if (ret == -1) {
          TLOG("Not an TCP packet \n");
	        continue;
        }

       if (hntap_do_drop_rexmit(pktbuf, nread + sizeof(struct vlan_header_t), &nflow_seqnum, &nflow_tcp)) {
           TLOG("Retransmitted TCP packet, seqno: 0x%x, dropping\n", nflow_seqnum);
           continue;
       }

      /*
       * Write to the ZMQ interface to modeal for Network Rx packet path.
       */
      hntap_handle_net_rx(pktbuf, nread + sizeof(struct ether_header_t));
	}
  }
}

int main(int argv, char *argc[])
{
  setlinebuf(stdout);
  setlinebuf(stderr);

  int opt = 0;
  while ((opt = getopt(argv, argc, "n:x")) != -1) {
    switch (opt) {
    case 'n':
        nw_retries = atoi(optarg);
	TLOG( "NW Retries=%d\n", nw_retries);
	break;
    case 'x':
	hntap_drop_rexmit = true;
        break;
    case '?':
    default:
        TLOG( "usage: ./hntap [-n <NW Retries>] [-x] \n");
	exit(-1);
        break;
    }

  }
  TLOG("Starting Host/network Tapper..\n");

  /* Create tap interface for Host-tap */
  if ((host_tap_fd = hntap_create_tundev(HNTAP_HOST_TAPIF,
                                         HNTAP_HOST_TAPIF_IP,
                                         HNTAP_HOST_TAPIF_IPMASK,
					 HNTAP_HOST_ROUTE_DESTIP,
					 HNTAP_HOST_ROUTE_GWIP)) < 0 ) {
    TLOG("Error creating tap interface %s!\n", HNTAP_HOST_TAPIF);
    abort();
  }

  /* Create tap interface for Network-tap */
  if ((net_tap_fd = hntap_create_tundev(HNTAP_NET_TAPIF,
                                        HNTAP_NET_TAPIF_IP,
                                        HNTAP_NET_TAPIF_IPMASK,
					HNTAP_NET_ROUTE_DESTIP,
					HNTAP_NET_ROUTE_GWIP)) < 0 ) {
    TLOG("Error creating tap interface %s!\n", HNTAP_NET_TAPIF);
    abort();
  }

  /*
   * Setup a tcp server on the specified port.
   */
  //tls_server_main(argv, argc);

  TLOG("  Setup done, listening on tap devices..\n");
  hntap_do_select_loop();

  return(0);
}
