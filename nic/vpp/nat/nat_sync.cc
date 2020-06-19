//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains functions that use the shm ipc transport to
// sync objects from VPP in domain 'A' to 'B' and functions to read
// the objects from IPC and program internal + p4 state. Uses helpers
// defined by implementations (apulu etc)

#include <stdbool.h>
#include <arpa/inet.h>
#include <nat_sync.pb.h>
#include <nic/sdk/include/sdk/ip.hpp>
#include <nic/vpp/infra/shm_ipc/shm_ipc.h>
#include <nic/vpp/infra/upgrade/repl_state_tp_common.h>
#include <nic/vpp/impl/nat.h>
#include "nat_api.h"

static bool shm_ipc_inited = false;

#ifdef __cplusplus
extern "C" {
#endif

sess_sync::NatAddrType
pds_encode_nat_addr_type (nat_addr_type_t addr_type)
{
    if (addr_type == NAT_ADDR_TYPE_INTERNET) {
        return sess_sync::NAT_ADDR_TYPE_INTERNET;
    } else {
        return sess_sync::NAT_ADDR_TYPE_INFRA;
    }
}

nat_addr_type_t
pds_decode_nat_addr_type (sess_sync::NatAddrType addr_type)
{
    if (addr_type == sess_sync::NAT_ADDR_TYPE_INTERNET) {
        return NAT_ADDR_TYPE_INTERNET;
    } else {
        return NAT_ADDR_TYPE_INFRA;
    }
}

void
pds_nat44_encode_one_session (uint8_t *data, uint8_t *len,
                              nat44_sync_info_t *sync)
{
    static thread_local ::sess_sync::Nat44Info info;

    // reset all internal state
    info.Clear();

    info.set_vpcid(sync->vpc_id);
    info.set_publicsrcip(sync->public_sip);
    info.set_dstip(sync->dip);
    info.set_publicsrcport(sync->public_sport);
    info.set_dstport(sync->dport);
    info.set_proto(sync->proto);
    info.set_privatesrcip(sync->pvt_sip);
    info.set_privatesrcport(sync->pvt_sport);
    info.set_rxxlateid(sync->rx_xlate_id);
    info.set_txxlateid(sync->tx_xlate_id);
    info.set_addrtype(pds_encode_nat_addr_type(sync->nat_addr_type));

    // Encode the protobuf into the passed buffer
    size_t req_sz = info.ByteSizeLong();
    *len = req_sz;
    info.SerializeWithCachedSizesToArray(data);
}

void
pds_nat44_decode_one_session (const uint8_t *data, uint8_t len,
                              nat44_sync_info_t *sync)
{
    static thread_local ::sess_sync::Nat44Info info;

    // reset all internal state
    info.Clear();
    info.ParseFromArray(data, len);

    sync->vpc_id = info.vpcid();
    sync->public_sip = info.publicsrcip();
    sync->dip = info.dstip();
    sync->public_sport = info.publicsrcport();
    sync->dport = info.dstport();
    sync->proto = info.proto();
    sync->pvt_sip = info.privatesrcip();
    sync->pvt_sport = info.privatesrcport();
    sync->rx_xlate_id = info.rxxlateid();
    sync->tx_xlate_id = info.txxlateid();
    sync->nat_addr_type = pds_decode_nat_addr_type(info.addrtype());
}


typedef struct nat_sync_ctxt_s {
    struct shm_ipcq *sess_q;
    uint32_t vpc_id;
    nat_flow_key_t *key;
    uint64_t val;
} nat_sync_ctxt_t;

static bool
nat_session_send_cb(uint8_t *data, uint8_t *len, void *opaq)
{
    uint32_t hw_index;
    nat_proto_t nat_proto;
    nat_addr_type_t nat_addr_type;
    uint32_t pvt_ip;
    uint16_t pvt_port;
    nat_sync_ctxt_t *ctxt = (nat_sync_ctxt_t *)opaq;
    nat44_sync_info_t sync;

    nat_flow_ht_get_fields(ctxt->val, &hw_index, &nat_addr_type, &nat_proto);

    pds_snat_tbl_read_ip4(hw_index, &pvt_ip, &pvt_port);

    sync.vpc_id = ctxt->vpc_id;
    sync.public_sip = ctxt->key->sip;
    sync.dip = ctxt->key->dip;
    sync.public_sport = ctxt->key->sport;
    sync.dport = ctxt->key->dport;
    sync.pvt_sip = pvt_ip;
    sync.pvt_sport = pvt_port;
    sync.proto = get_proto_from_nat_proto(nat_proto);
    sync.rx_xlate_id = hw_index;
    sync.tx_xlate_id = nat_get_tx_hw_index_pub_ip_port(ctxt->vpc_id,
                                                       nat_addr_type,
                                                       nat_proto, ctxt->key->sip,
                                                       ctxt->key->sport);
    sync.nat_addr_type = nat_addr_type;

    pds_nat44_encode_one_session(data, len, &sync);

    return false;
}

static void *
nat_sync_cb (void *ctxt, uint32_t vpc_id, nat_flow_key_t *key, uint64_t val)
{
    nat_sync_ctxt_t *sync = (nat_sync_ctxt_t *)ctxt;
    bool ret;

    if (key->protocol == IP_PROTO_ICMP) {
        // Don't sync ICMP flows
        return sync;
    }

    sync->vpc_id = vpc_id;
    sync->key = key;
    sync->val = val;

    do {
        ret = shm_ipc_try_send(sync->sess_q, nat_session_send_cb, ctxt);
    } while (!ret);

    return sync;
}

void
nat_sync (const char *q_name)
{
    nat_sync_ctxt_t ctxt;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    ctxt.sess_q = shm_ipc_create(q_name);
    
    nat_sync_start();
    nat_flow_iterate((void *)&ctxt, &nat_sync_cb);
    while (shm_ipc_send_eor(ctxt.sess_q) == false);
    nat_sync_stop();
}

static bool
nat_recv_cb (const uint8_t *data, const uint8_t len, void *opaq)
{
    nat44_sync_info_t sync;

    pds_nat44_decode_one_session(data, len, &sync);

    nat_sync_restore(&sync);
    
    return true;
}

void
nat_restore (const char *q_name)
{
    int ret;
    struct shm_ipcq *sess_q;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    sess_q = shm_ipc_attach(q_name);
    nat_sync_start();
    while ((ret = shm_ipc_recv_start(sess_q, nat_recv_cb, NULL)) != -1);
    nat_sync_stop();
}

void
nat_sync_init (void)
{
    repl_state_tp_cb_t cb = {
        .id = REPL_STATE_OBJ_ID_NAT,
        .sync_cb = nat_sync,
        .restore_cb = nat_restore,
    };
    repl_state_tp_callbacks_register(&cb);
}

#ifdef __cplusplus
} // extern "C"
#endif
