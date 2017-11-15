//:: import os, pdb
//:: from collections import OrderedDict
//:: pddict = _context['pddict']
//:: if pddict['p4plus']:
//::    p4prog = pddict['p4program'] + '_'
//::    hdrdir = pddict['p4program']
//::    caps_p4prog = '_' + pddict['p4program'].upper() + '_'
//::    prefix = 'p4pd_' + pddict['p4program']
//::    if pddict['p4program'] == 'common_rxdma_actions':
//::        start_table_base = 101
//::    elif pddict['p4program'] == 'common_txdma_actions':
//::	    start_table_base = 201
//::    else:
//::	    start_table_base = 301
//::    #endif
//:: elif pddict['p4program'] == 'gft':
//::    p4prog = pddict['p4program'] + '_'
//::    hdrdir = pddict['p4program']
//::    caps_p4prog = '_' + pddict['p4program'].upper() + '_'
//::    prefix = 'p4pd_' + pddict['p4program']
//::	start_table_base = 401
//:: else:
//::    p4prog = ''
//::    hdrdir = 'iris'
//::    caps_p4prog = ''
//::    prefix = 'p4pd'
//::	start_table_base = 1
//:: #endif
//:: #define here any constants needed.
//:: ACTION_PC_LEN = 8 # in bits
/*
 * p4pd.c
 * Pensando Systems
 */
#include "nic/gen/proto/hal/debug.pb.h"
#include "nic/gen/proto/hal/debug.grpc.pb.h"
#include <grpc++/grpc++.h>
#include "nic/gen/iris/include/p4pd.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using ::debug::Debug;
using ::debug::DebugRequestMsg;
using ::debug::DebugSpec;
using ::debug::DebugKeyHandle;
using ::debug::DebugResponseMsg;

//::     tabledict = OrderedDict() # key=table-name
//::     tableid = start_table_base
//::     for table in pddict['tables']:
//::        tabledict[table] = tableid
//::        tableid += 1
//::     #endfor

//::    if len(tabledict):
//::        if pddict['p4plus'] or pddict['p4program'] == 'gft':
//::            api_prefix = 'p4pd_' + pddict['p4program']
//::        else:
//::            api_prefix = 'p4pd'
//::        #endif
//::    #endif
p4pd_error_t
${api_prefix}_entry_write(uint32_t tableid,
                          uint32_t index,
                          void     *swkey,
                          void     *swkey_mask,
                          void     *actiondata)
{
    DebugRequestMsg  debug_req_msg;
    DebugResponseMsg debug_rsp_msg;
    ClientContext    context;
    DebugSpec        *debug_spec    = NULL;
    DebugKeyHandle   *key_or_handle = NULL;

    auto channel =
        grpc::CreateChannel("localhost:50054", grpc::InsecureChannelCredentials());

    auto stub    = ::debug::Debug::NewStub(channel);

    debug_spec = debug_req_msg.add_request();

    key_or_handle = debug_spec->mutable_key_or_handle();

    key_or_handle->set_table_id(tableid);

    debug_spec->set_index(index);
    debug_spec->set_opn_type(::debug::DEBUG_OP_TYPE_WRITE);

    switch (tableid) {
        //::        for table, tid in tabledict.items():
        //::            caps_tablename = table.upper()
        //::            if pddict['tables'][table]['hash_overflow'] and not pddict['tables'][table]['otcam']:
        //::                continue
        //::            #endif
        case P4${caps_p4prog}TBL_ID_${caps_tablename}: /* p4-table '${table}' */
        //::            if len(pddict['tables'][table]['hash_overflow_tbl']):
        //::                tbl_ = pddict['tables'][table]['hash_overflow_tbl']
        //::                caps_tbl_ = tbl_.upper()
        case P4${caps_p4prog}TBL_ID_${caps_tbl_}: /* p4-table '${tbl_}' */
        //::            #endif
        //::            if pddict['tables'][table]['type'] == 'Index' or pddict['tables'][table]['type'] == 'Mpu':
            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

        //::            #endif
        //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            debug_spec->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

        //::            #endif
        //::            if pddict['tables'][table]['type'] == 'Ternary':
            debug_spec->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            debug_spec->set_swkey_mask(
                        std::string ((char*)(${table}_swkey_mask_t*)swkey_mask,
                                     sizeof(${table}_swkey_mask_t)));

            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

        //::            #endif
        //::        #endfor
        default:
            // Invalid tableid
            return (P4PD_FAIL);
            break;
    }

    Status status = stub->DebugInvoke(&context, debug_req_msg, &debug_rsp_msg);

    return (P4PD_SUCCESS);
}

p4pd_error_t
${api_prefix}_entry_read(uint32_t  tableid,
                         uint32_t  index,
                         void      *swkey, 
                         void      *swkey_mask,
                         void      *actiondata)
{
    DebugRequestMsg  debug_req_msg;
    DebugResponseMsg debug_rsp_msg;
    ClientContext    context;
    DebugSpec        *debug_spec    = NULL;
    DebugKeyHandle   *key_or_handle = NULL;

    debug_spec = debug_req_msg.add_request();

    key_or_handle = debug_spec->mutable_key_or_handle();

    key_or_handle->set_table_id(tableid);

    debug_spec->set_opn_type(::debug::DEBUG_OP_TYPE_READ);
    debug_spec->set_index(index);

    auto channel =
        grpc::CreateChannel("localhost:50054", grpc::InsecureChannelCredentials());

    auto stub    = ::debug::Debug::NewStub(channel);

    switch (tableid) {
        //::        for table, tid in tabledict.items():
        //::            caps_tablename = table.upper()
        //::            if pddict['tables'][table]['hash_overflow'] and not pddict['tables'][table]['otcam']:
        //::                continue
        //::            #endif
        case P4${caps_p4prog}TBL_ID_${caps_tablename}: /* p4-table '${table}' */
            //::            if len(pddict['tables'][table]['hash_overflow_tbl']):
            //::                tbl_ = pddict['tables'][table]['hash_overflow_tbl']
            //::                caps_tbl_ = tbl_.upper()
        case P4${caps_p4prog}TBL_ID_${caps_tbl_}: /* p4-table '${tbl_}' */
            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Index' or pddict['tables'][table]['type'] == 'Mpu':
            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Ternary':
            debug_spec->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;
            //::            #endif
            //::        #endfor
        default:
            // Invalid tableid
            return (P4PD_FAIL);
            break;
    }

    Status status = stub->DebugInvoke(&context, debug_req_msg, &debug_rsp_msg);

    switch (tableid) {
        //::        for table, tid in tabledict.items():
        //::            caps_tablename = table.upper()
        //::            if pddict['tables'][table]['hash_overflow'] and not pddict['tables'][table]['otcam']:
        //::                continue
        //::            #endif
        case P4${caps_p4prog}TBL_ID_${caps_tablename}: /* p4-table '${table}' */
            //::            if len(pddict['tables'][table]['hash_overflow_tbl']):
            //::                tbl_ = pddict['tables'][table]['hash_overflow_tbl']
            //::                caps_tbl_ = tbl_.upper()
        case P4${caps_p4prog}TBL_ID_${caps_tbl_}: /* p4-table '${tbl_}' */
            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Index' or pddict['tables'][table]['type'] == 'Mpu':
            memcpy(actiondata, (void*)debug_rsp_msg.response(0).rsp_data().actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            memcpy(swkey, (void*)debug_rsp_msg.response(0).rsp_data().swkey().c_str(),
                   sizeof(${table}_swkey_t));

            memcpy(actiondata, (void*)debug_rsp_msg.response(0).rsp_data().actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Ternary':
            memcpy(swkey, (void*)debug_rsp_msg.response(0).rsp_data().swkey().c_str(),
                   sizeof(${table}_swkey_t));

            memcpy(swkey_mask, (void*)debug_rsp_msg.response(0).rsp_data().swkey_mask().c_str(),
                   sizeof(${table}_swkey_mask_t));

            memcpy(actiondata, (void*)debug_rsp_msg.response(0).rsp_data().actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;
            //
            //::            #endif
            //::        #endfor
        default:
            // Invalid tableid
            return (P4PD_FAIL);
            break;
    }
    return (P4PD_SUCCESS);
}
