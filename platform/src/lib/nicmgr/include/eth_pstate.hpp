//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines persistent state for Nicmgr for upgrade and Restart
///
//----------------------------------------------------------------------------

#ifndef __ETH_PSTATE_HPP__
#define __ETH_PSTATE_HPP__

#include "nicmgr_shm.hpp"

#define IONIC_IFNAMSIZ  16

#define RSS_HASH_KEY_SIZE   40
#define RSS_IND_TBL_SIZE    128

/// \brief eth lif states
/// Transistion states during the config operations
enum eth_lif_state {
    LIF_STATE_NONE = 0,
    LIF_STATE_RESETTING,
    LIF_STATE_RESET,
    LIF_STATE_CREATING,
    LIF_STATE_CREATED,
    LIF_STATE_INITING,
    LIF_STATE_INIT,
    // LIF LINK STATES
    LIF_STATE_UP,
    LIF_STATE_DOWN,
};


/// \brief eth lif upgrade state persisted
/// saved in shared memory to access after process restart.
/// as this is used across upgrades, all the modifications should be
/// done to the end
typedef struct ethlif_pstate_v1_s {

    /// always assign the default valuse for the structure
    ethlif_pstate_v1_s () {
        version_magic = 1;
        lif_id = 0;
        memset(name, 0, sizeof(name));
        state = LIF_STATE_NONE;
        // memset(lif_config, 0, sizeof(lif_config));
        host_lif_config_addr = 0;
        // memset(lif_status, 0, sizeof(lif_status));
        host_lif_status_addr = 0;
        host_lif_stats_addr = 0;
        rss_type = LIF_RSS_TYPE_NONE;
        memset(rss_key, 0x00, RSS_HASH_KEY_SIZE);
        memset(rss_indir, 0x00, RSS_IND_TBL_SIZE);
        mtu = 0;
        tx_sched_table_offset = INVALID_INDEXER_INDEX;
        tx_sched_num_table_entries = 0;
        qstate_mem_address = 0;
        qstate_mem_size = 0;
        active_q_ref_cnt = 0;
        admin_state = 0;
        proxy_admin_state = 0;
        provider_admin_state = 0;
        port_status = 0;
    }

    /// Verions magic for the strucure
    uint64_t version_magic;

    /// lif id - used as shm segment identifier
    uint32_t lif_id;

    /// lif Info and state
    char name[IONIC_IFNAMSIZ];
    enum eth_lif_state state;

    /// lif config
    // union ionic_lif_config lif_config;
    uint64_t host_lif_config_addr;

    /// lif status
    // struct ionic_lif_status lif_status;
    uint64_t host_lif_status_addr;

    /// lif stats
    uint64_t host_lif_stats_addr;

    /// txdma schedular offset
    uint32_t    tx_sched_table_offset;

    /// txdma table entries
    uint32_t    tx_sched_num_table_entries;

    /// qstate memory address
    uint64_t qstate_mem_address;

    /// qstate memory size
    uint32_t qstate_mem_size;

    /// RSS config
    uint16_t rss_type;
    uint8_t rss_key[RSS_HASH_KEY_SIZE];  // 40B
    uint8_t rss_indir[RSS_IND_TBL_SIZE]; // 128B

    /// MTU info
    uint32_t mtu;

    /// queue reference count
    uint32_t active_q_ref_cnt;

    /// lif admin state
    uint8_t admin_state;

    /// lif proxy state
    uint8_t proxy_admin_state;

    /// lif proxy state
    uint8_t provider_admin_state;

    /// lif port status
    uint8_t port_status;
} __PACK__ ethlif_pstate_v1_t;

/// \brief eth dev upgrade state persisted
/// saved in shared memory to access after process restart.
/// as this is used across upgrades, all the modifications should be
/// done to the end
typedef struct ethdev_pstate_v1_s {

    // always assign the default values in the constructor
    ethdev_pstate_v1_s () {
        version_magic = 1;
        memset(name, 0, sizeof(name));
        memset(active_lif_set, 0, sizeof(active_lif_set));
        n_active_lif = 0;
        host_port_info_addr = 0;
        host_port_config_addr = 0;
        host_port_status_addr = 0;
        host_port_mac_stats_addr = 0;
        host_port_pb_stats_addr = 0;
    }

    /// Verions magic for the strucure
    uint64_t version_magic;

    /// dev name - used as shm segment identifier
    char name[IONIC_IFNAMSIZ];

    /// Active lifs
    uint16_t active_lif_set[MAX_LIFS];
    uint16_t n_active_lif;

    /// Port Info
    uint64_t host_port_info_addr;

    /// Port Config
    //  union ionic_port_config port_config;
    uint64_t host_port_config_addr;

    /// Port Status
    //  struct ionic_port_status port_status;
    uint64_t host_port_status_addr;

    /// Port MAC Stats
    uint64_t host_port_mac_stats_addr;

    /// Port PacketBuffer (PB) Stats
    uint64_t host_port_pb_stats_addr;
} __PACK__ ethdev_pstate_v1_t;

/// \brief typdefs for persistent state
/// these typedefs should always be latest version so no need to changes inside
/// code for every version bump.
typedef ethdev_pstate_v1_t ethdev_pstate_t;
typedef ethlif_pstate_v1_t ethlif_pstate_t;

#endif    // __HITLESS_UPGRADE_HPP__
