//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// common definitions needed by local and remote endpoints
///
//----------------------------------------------------------------------------

#ifndef __LEARN__LEARN_HPP__
#define __LEARN__LEARN_HPP__

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/ht/ht.hpp"
#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/include/globals.hpp"
#include "nic/apollo/core/msg.h"

namespace learn {

#define EP_MAX_MAC_ENTRY        PDS_MAX_VNIC
#define EP_MAX_IP_ENTRY         (EP_MAX_MAC_ENTRY * 32)

#define UIO_DEV_ROOT            "/sys/class/uio/"
#define UIO_DEV_SCAN_INTERVAL   1
#define UIO_DEV_SCAN_MAX_RETRY  600
#define TIMER_NOT_STARTED       0

/// \brief endpoint state
/// \remark
/// all endpoint state changes are done in the context on learn thread
/// if the state is LEARNING, UPDATING or DELETING, it indicates that an
/// event is currently modifying p4 one or more p4 table entries and a
/// subsequent event that needs to manipulate the same EP must wait
typedef enum {
    EP_STATE_LEARNING,      ///< EP is being programmed
    EP_STATE_CREATED,       ///< EP is programmed
    EP_STATE_PROBING,       ///< EP is awaiting arp probe response
    EP_STATE_UPDATING,      ///< EP is being updated
    EP_STATE_DELETING,      ///< EP is being deleted
    EP_STATE_DELETED,       ///< EP is deleted
} ep_state_t;

/// \brief key to L2 endpoint data base
typedef struct {
    mac_addr_t mac_addr;
    union {
        pds_obj_key_t subnet;
        uint16_t      lif;
    } __PACK__;

    void make_key(char *mac, pds_obj_key_t subnet_uuid) {
        MAC_ADDR_COPY(mac_addr, mac);
        subnet = subnet_uuid;
    }

    void make_key(char *mac, uint16_t lif_id) {
        MAC_ADDR_COPY(mac_addr, mac);
        lif = lif_id;
    }
} __PACK__ ep_mac_key_t;

/// \brief key to L3 endpoint data base
typedef struct ep_ip_key_s {
    ip_addr_t ip_addr;
    union {
        pds_obj_key_t vpc;
        uint16_t      lif;
    } __PACK__;

    void make_key(ip_addr_t *ip, pds_obj_key_t vpc_uuid) {
        memcpy(&ip_addr, ip, sizeof(ip_addr_t));
        vpc = vpc_uuid;
    }

    void make_key(ip_addr_t *ip, uint16_t lif_id) {
        memcpy(&ip_addr, ip, sizeof(ip_addr_t));
        lif = lif_id;
    }

    // TODO: redefine this to accomodate notify mode too
    // 1. define a char[] of size same as max size of union or
    // 2. do memcmp of struct size from ip_addr variablw
    bool operator ==(const ep_ip_key_s &key) const {
        return (key.vpc == vpc) &&
            (memcmp(&key.ip_addr, &ip_addr, sizeof(ip_addr_t)) == 0);
    }
} __PACK__ ep_ip_key_t;

/// \brief hasher class for ep_ip_key_t
class ep_ip_key_hash {
    public:
        std::size_t operator()(const ep_ip_key_t& key) const {
            return hash_algo::fnv_hash((void *)&key, sizeof(key));
        }
};

/// \brief learn type of the endpoint under process
typedef enum {
    LEARN_TYPE_INVALID = 0,      ///< learn type not available
    LEARN_TYPE_NONE,             ///< existing entry, nothing to learn
    LEARN_TYPE_COUNTER_MIN = LEARN_TYPE_NONE,
    LEARN_TYPE_NEW_LOCAL,        ///< new local learn
    LEARN_TYPE_NEW_REMOTE,       ///< new remote learn
    LEARN_TYPE_MOVE_L2L,         ///< local to local move
    LEARN_TYPE_MOVE_R2L,         ///< remote to local move
    LEARN_TYPE_MOVE_L2R,         ///< local to remote move
    LEARN_TYPE_MOVE_R2R,         ///< remote to remote move
    LEARN_TYPE_DELETE,           ///< delete
    LEARN_TYPE_COUNTER_MAX = LEARN_TYPE_DELETE,
} ep_learn_type_t;

/// \brief return number of learn types tracked by counters
static constexpr int
learn_type_ctr_sz (void)
{
    return (LEARN_TYPE_COUNTER_MAX - LEARN_TYPE_COUNTER_MIN + 1);
}
#define NUM_LEARN_TYPE_CTRS learn_type_ctr_sz()

// counter index
#define LEARN_TYPE_CTR_IDX(learn_type)     (learn_type - LEARN_TYPE_COUNTER_MIN)
#define CTR_IDX_TO_LEARN_TYPE(idx)         (idx + LEARN_TYPE_COUNTER_MIN)

static inline const char *
ep_learn_type_str (ep_learn_type_t learn_type)
{
    switch (learn_type) {
    case LEARN_TYPE_INVALID:
        return "Invalid";
    case LEARN_TYPE_NONE:
        return "None";
    case LEARN_TYPE_NEW_LOCAL:
        return "New Local";
    case LEARN_TYPE_NEW_REMOTE:
        return "New Remote";
    case LEARN_TYPE_MOVE_L2L:
        return "L2L";
    case LEARN_TYPE_MOVE_R2L:
        return "R2L";
    case LEARN_TYPE_MOVE_L2R:
        return "L2R";
    case LEARN_TYPE_MOVE_R2R:
        return "R2R";
    case LEARN_TYPE_DELETE:
        return "Delete";
    }
    return "Error";
}

/// \brief process learn packet received on learn lif
void process_learn_pkt(void *mbuf);

/// \brief process API batch received by learn thread
sdk_ret_t process_api_batch(api::api_msg_t *api_msg);

/// \brief     get time remaining for timer expiry
///            when remaining aging time is returned as '0, it can mean any of
///            the following condiitons:
///            1. aging is disabled by conifg by setting timeout to 0
///            2. aging timer has just expired, for IP entries, state indicates
//                this
///            3. aging has not yet started, for MAC entries
/// \param[in] ts    timestamp at which the timer expires
/// \return    remaining time from now in seconds
///            0 if current time is past the timestamp or timestamp is invalid
static inline uint32_t
remaining_age (uint64_t ts)
{
    uint64_t now;

    if (ts == 0) {
        return TIMER_NOT_STARTED;
    }
    now = sdk::event_thread::timestamp_now();

    // this may be called just after expiry before timer callback is run
    // or, ARP probing might be on, in which case we deem the entry expired
    // for IP entry, state indicates that we are in ARP probe
    if (now > ts) {
        return 0;
    }
    return (ts - now);
}

}    // namespace learn

using namespace learn;

typedef enum {
    LEARN_CLEAR_MSG_ID_NONE = 0,
    LEARN_CLEAR_MAC,
    LEARN_CLEAR_IP,
    LEARN_CLEAR_MAC_ALL,
    LEARN_CLEAR_IP_ALL,
    LEARN_CLEAR_MSG_ID_MAX = LEARN_CLEAR_IP_ALL,
} learn_clear_msg_id;

typedef struct learn_clear_msg_s {
    learn_clear_msg_id   id;        ///< unique id of the msg
    union {
        ep_ip_key_t ip_key;         ///< ip key
        ep_mac_key_t mac_key;       ///< mac key
    };
} learn_clear_msg_t;

typedef enum learn_msg_id_e {
    LEARN_MSG_ID_NONE        = (PDS_MSG_TYPE_MAX + 1),
    LEARN_MSG_ID_API,
    LEARN_MSG_ID_CLEAR_CMD,
} learn_msg_id_t;

sdk_ret_t learn_mac_clear(ep_mac_key_t *key);
sdk_ret_t learn_ip_clear(ep_ip_key_t *key);

#endif  // __LEARN__LEARN_HPP__
