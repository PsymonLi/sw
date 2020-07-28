/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#ifndef _LIBMCTP_H
#define _LIBMCTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define __unused __attribute__((unused))

#define EID_ACCEPTED           0
#define EID_REJECTED           (1 << 4)
#define NO_DYNAMIC_EID_POOL    0
#define SIMPLE_ENDPOINT        (0 << 4)
#define DYNAMIC_EID            0

const char * get_type_name(uint8_t type);

// MCTP control message completion codes
enum {
    MCTP_CTRL_SUCCESS         = 0,
    MCTP_CTRL_ERROR           = 1,
    MCTP_CTRL_INVALID_DATA    = 2,
    MCTP_CTRL_INVALID_LENGTH  = 3,
    MCTP_CTRL_NOT_READY       = 4,
    MCTP_CTRL_UNSUPPORTED_CMD = 5,
};

/* MCTP message types */
enum {
    MSG_TYPE_MCTP_CONTROL_MSG   = 0x00,
    MSG_TYPE_PLDM_MSG           = 0x01,
    MSG_TYPE_NCSI_OVER_MCTP_MSG = 0x02,
    MSG_TYPE_ETH_OVER_MCTP_MSG  = 0x03,
    MSG_TYPE_VENDOR_PCI_MSG     = 0x7e,
    MSG_TYPE_VENDOR_IANA_MSG    = 0x7f,
};

typedef uint8_t mctp_eid_t;

/* MCTP packet definitions */
struct mctp_hdr {
    uint8_t ver;
    uint8_t dest;
    uint8_t src;
    uint8_t flags_seq_tag;
};

/* Definitions for flags_seq_tag field */
#define MCTP_HDR_FLAG_SOM   (1<<7)
#define MCTP_HDR_FLAG_EOM   (1<<6)
#define MCTP_HDR_FLAG_TO    (1<<3)
#define MCTP_HDR_SEQ_SHIFT  (4)
#define MCTP_HDR_SEQ_MASK   (0x3)
#define MCTP_HDR_TAG_SHIFT  (0)
#define MCTP_HDR_TAG_MASK   (0x7)

/* Baseline Transmission Unit and packet size */
#define MCTP_BTU        64
#define MCTP_PACKET_SIZE(unit)  ((unit) + sizeof(struct mctp_hdr))

/* packet buffers */

struct mctp_pktbuf {
    size_t      start, end, size;
    size_t      mctp_hdr_off;
    struct mctp_pktbuf *next;
    unsigned char   data[];
};

struct mctp_binding;

struct mctp_pktbuf *mctp_pktbuf_alloc(struct mctp_binding *hw, size_t len);
void mctp_pktbuf_free(struct mctp_pktbuf *pkt);
struct mctp_hdr *mctp_pktbuf_hdr(struct mctp_pktbuf *pkt);
void *mctp_pktbuf_data(struct mctp_pktbuf *pkt);
uint8_t mctp_pktbuf_size(struct mctp_pktbuf *pkt);
void *mctp_pktbuf_alloc_start(struct mctp_pktbuf *pkt, size_t size);
void *mctp_pktbuf_alloc_end(struct mctp_pktbuf *pkt, size_t size);
int mctp_pktbuf_push(struct mctp_pktbuf *pkt, void *data, size_t len);

/* MCTP core */
struct mctp;
struct mctp_bus;

struct mctp *mctp_init(void);
void mctp_destroy(struct mctp *mctp);

/* Register a binding to the MCTP core, and creates a bus (populating
 * binding->bus).
 *
 * If this function is called, the MCTP stack is initialised as an 'endpoint',
 * and will deliver local packets to a RX callback - see `mctp_set_rx_all()`
 * below.
 */
int mctp_register_bus(struct mctp *mctp,
        struct mctp_binding *binding,
        mctp_eid_t eid);

/* Create a simple bidirectional bridge between busses.
 *
 * In this mode, the MCTP stack is initialised as a bridge. There is no EID
 * defined, so no packets are considered local. Instead, all messages from one
 * binding are forwarded to the other.
 */
int mctp_bridge_busses(struct mctp *mctp,
        struct mctp_binding *b1, struct mctp_binding *b2);

typedef void (*mctp_rx_fn)(uint8_t src_eid, void *data,
        void *msg, size_t len, uint8_t tag);

int mctp_set_rx_all(struct mctp *mctp, mctp_rx_fn fn, void *data);

int mctp_message_tx(struct mctp *mctp, mctp_eid_t eid,
        void *msg, size_t msg_len, uint8_t tag);

int mctp_set_bus_eid(struct mctp *mctp, mctp_eid_t eid);

/* hardware bindings */
struct mctp_binding {
    const char  *name;
    uint8_t     version;
    struct mctp_bus *bus;
    struct mctp *mctp;
    int     pkt_size;
    int     pkt_pad;
    int     (*start)(struct mctp_binding *binding);
    int     (*tx)(struct mctp_binding *binding,
                struct mctp_pktbuf *pkt);
};

void mctp_binding_set_tx_enabled(struct mctp_binding *binding, bool enable);

/*
 * Receive a packet from binding to core. Takes ownership of pkt, free()-ing it
 * after use.
 */
void mctp_bus_rx(struct mctp_binding *binding, struct mctp_pktbuf *pkt);

/* environment-specific allocation */
void mctp_set_alloc_ops(void *(*alloc)(size_t),
        void (*free)(void *),
        void *(realloc)(void *, size_t));

/* environment-specific logging */

void mctp_set_log_stdio(int level);
void mctp_set_log_syslog(void);
void mctp_set_log_custom(void (*fn)(int, const char *, va_list));

/* MCTP control commands */
enum {
    MCTP_CTRL_RSVD                  = 0x00,
    MCTP_SET_EID                    = 0x01,     // mandatory
    MCTP_GET_EID                    = 0x02,     // mandatory
    MCTP_GET_UUID                   = 0x03,     // conditional
    MCTP_GET_VERSION_SUPPORT        = 0x04,     // mandatory
    MCTP_GET_MSG_SUPPORT            = 0x05,     // mandatory
    MCTP_GET_VENDOR_MSG_SUPPORT     = 0x06,     // optional
    MCTP_RESOLVE_EID                = 0x07,     // endpoint not applicable
    MCTP_ALLOCATE_EIDS              = 0x08,     // endpoint not applicable
    MCTP_ROUTING_INFO_UPDATE        = 0x09,     // optional
    MCTP_GET_ROUTING_TABLE          = 0x0a,     // endpoint not applicable
    MCTP_PREPARE_ENDPOINT_DISCOVERY = 0x0b,     // conditional
    MCTP_ENDPOINT_DISCOVERY         = 0x0c,     // conditional
    MCTP_DISCOVERY_NOTIFY           = 0x0d,     // endpoint not applicable
    MCTP_GET_NETWORK_ID             = 0x0e,     // conditional
    MCTP_QUERY_HOP                  = 0x0f,     // endpoint not applicable
    MCTP_RESOLVE_UUID               = 0x10,     // endpoint not applicable
    MCTP_QUERY_RATE_LIMIT           = 0x11,     // optional
    MCTP_RQST_TX_RATE_LIMIT         = 0x12,     // optional
    MCTP_UPDATE_RATE_LIMIT          = 0x13,     // optional
    MCTP_QUERY_SUPPORTED_INTERFACES = 0x14,     // optional
    //MCTP_TRANSPORT_SPECIFIC         = 0x0f,
};

/* NCSI commands */
enum {
    NCSI_CLEAR_INITIAL_STATE = 0x00,
    NCSI_ENABLE_CHANNEL      = 0x01,
};

/* these should match the syslog-standard LOG_* definitions, for
 * easier use with syslog */
#define MCTP_LOG_ERR        3
#define MCTP_LOG_WARNING    4
#define MCTP_LOG_NOTICE     5
#define MCTP_LOG_INFO       6
#define MCTP_LOG_DEBUG      7

#ifdef __cplusplus
}
#endif

#endif /* _LIBMCTP_H */
