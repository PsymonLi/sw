#ifndef _LKLSHIM_HPP_
#define _LKLSHIM_HPP_

#include "nic/include/base.hpp"
#include "sdk/slab.hpp"
#include "sdk/list.hpp"
#include "nic/hal/hal.hpp"
#include "sdk/ht.hpp"
#include "nic/include/cpupkt_headers.hpp"
#include "nic/hal/src/nw/session.hpp"
#include "nic/hal/src/internal/proxy.hpp"

#include <netinet/in.h>
#include <memory>
#include <map>

extern "C" {
#include <netinet/in.h>
}
using sdk::lib::slab;

namespace hal {

#define PACKED __attribute__((__packed__))

typedef enum {
    FLOW_STATE_INVALID = 0,
    FLOW_STATE_SYN_RCVD = 1,
    FLOW_STATE_CONNECT_PENDING = 2,
    FLOW_STATE_ESTABLISHED = 3,
    FLOW_STATE_SSL_HANDSHAKE_IN_PROGRESS = 4,
    FLOW_STATE_SSL_HANDSHAKE_DONE = 5
} lklshim_flow_state_e;

typedef struct lklshim_flow_key_t_ {
    ipvx_addr_t src_ip;
    ipvx_addr_t dst_ip;
    uint16_t    src_port;
    uint16_t    dst_port;
    uint8_t     type;
} PACKED lklshim_flow_key_t;

#define MAC_SIZE 6
#define VLAN_SIZE 2

typedef struct lklshim_flow_ns_t_ {
    int                  sockfd;
    lklshim_flow_state_e state;
    void                 *skbuff;
    //tcpcb_t              *tcpcb;
    //tlscb_t              *tlscb;
    char                 dev[16];
    uint8_t   src_mac[MAC_SIZE];
    uint8_t   dst_mac[MAC_SIZE];
    uint8_t   vlan[VLAN_SIZE];
} lklshim_flow_ns_t;

typedef struct lklshim_flow_encap_s {
    uint16_t                i_src_lif;      // LIF for initiator
    uint16_t                i_src_vlan_id;  // VLAN for initiator
    uint16_t                r_src_lif;      // LIF for responder
    uint16_t                r_src_vlan_id;  // VLAN for responder
    uint32_t                encrypt_qid;    // QID for encrypt flow 
    uint32_t                decrypt_qid;    // QID for decrypt flow 
    bool                    is_server_ctxt; // is this a TLS server ctxt
} lklshim_flow_encap_t;

typedef struct lklshim_flow_t_ {
    lklshim_flow_key_t      key;
    lklshim_flow_ns_t       hostns;
    lklshim_flow_ns_t       netns;

    hal::flow_direction_t   itor_dir;
    uint32_t                iqid;
    uint32_t                rqid;
    lklshim_flow_encap_t    flow_encap;
    proxy_flow_info_t       *pfi;
} lklshim_flow_t;

typedef struct lklshim_listen_sockets_t_ {
    int       tcp_portnum;
    int       ipv4_sockfd;
    int       ipv6_sockfd;
} PACKED lklshim_listen_sockets_t;

#define MAX_PROXY_FLOWS 32768

/*
 * Extern definitions.
 */
extern slab *lklshim_flowdb_slab;
extern lklshim_flow_t           *lklshim_flow_by_qid[MAX_PROXY_FLOWS];

static inline void
lklshim_make_flow_v4key (lklshim_flow_key_t *key, in_addr_t src, in_addr_t dst,
			 uint16_t sport, uint16_t dport)
{
    memset(key, 0, sizeof(lklshim_flow_key_t));
    key->src_ip.v4_addr = src;
    key->dst_ip.v4_addr = dst;
    key->src_port = sport;
    key->dst_port = dport;
    key->type = hal::FLOW_TYPE_V4;
}

static inline void
lklshim_make_flow_v6key (lklshim_flow_key_t *key,
                         uint8_t *src, uint8_t *dst,
                         uint16_t sport, uint16_t dport)
{
    memset(key, 0, sizeof(lklshim_flow_key_t));
    memcpy(key->src_ip.v6_addr.addr8, src, sizeof(key->src_ip.v6_addr.addr8));
    memcpy(key->dst_ip.v6_addr.addr8, dst, sizeof(key->dst_ip.v6_addr.addr8));
    key->src_port = sport;
    key->dst_port = dport;
    key->type = hal::FLOW_TYPE_V6;
}

bool lklshim_process_flow_miss_rx_packet (void *pkt_skb, hal::flow_direction_t dir, uint32_t iqid, uint32_t rqid, proxy_flow_info_t *pfi, lklshim_flow_encap_t *flow_encap);
bool lklshim_process_v6_flow_miss_rx_packet (void *pkt_skb, hal::flow_direction_t dir, uint32_t iqid, uint32_t rqid, proxy_flow_info_t *pfi, lklshim_flow_encap_t *flow_encap);
bool lklshim_process_flow_hit_rx_packet (void *pkt_skb, hal::flow_direction_t dir, const hal::pd::p4_to_p4plus_cpu_pkt_t* rxhdr);
bool lklshim_process_flow_hit_rx_header (void *pkt_skb, hal::flow_direction_t dir, const hal::pd::p4_to_p4plus_cpu_pkt_t* rxhdr);
bool lklshim_process_v6_flow_hit_rx_packet (void *pkt_skb, hal::flow_direction_t dir, const hal::pd::p4_to_p4plus_cpu_pkt_t* rxhdr);
bool lklshim_process_v6_flow_hit_rx_header (void *pkt_skb, hal::flow_direction_t dir, const hal::pd::p4_to_p4plus_cpu_pkt_t* rxhdr);
void lklshim_process_tx_packet(unsigned char* pkt, unsigned int len, void* flow, bool is_connect_req, void *tcpcb, bool tx_pkt);

void lklshim_flowdb_init(void);
void lklshim_update_tcpcb(void *tcpcb, uint32_t qid, uint32_t src_lif);
hal::flow_direction_t lklshim_get_flow_hit_pkt_direction(uint16_t qid);

lklshim_flow_t *
lklshim_flow_entry_alloc (lklshim_flow_key_t *flow_key);
} //namespace hal

#endif // _LKLSHIM_HPP_
