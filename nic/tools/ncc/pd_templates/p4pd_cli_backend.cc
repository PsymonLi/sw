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
//:: else:
//::    p4prog = ''
//::    hdrdir = pddict['p4program']
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
#include "nic/gen/${hdrdir}/include/${p4prog}p4pd.h"
#include "nic/include/hal_pd_error.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using ::debug::Debug;
using ::debug::MemoryRequestMsg;
using ::debug::MemoryRequest;
using ::debug::MemoryResponseMsg;
using ::debug::RegisterRequestMsg;
using ::debug::RegisterRequest;
using ::debug::RegisterResponseMsg;
using ::debug::MemoryRawRequestMsg;
using ::debug::MemoryRawRequest;
using ::debug::MemoryRawResponseMsg;
using ::debug::MemoryRawResponse;
using ::debug::MemoryRawUpdateRequestMsg;
using ::debug::MemoryRawUpdateRequest;
using ::debug::MemoryRawUpdateResponseMsg;
using ::debug::MemoryRawUpdateResponse;
//::
//::     tabledict = OrderedDict() # key=table-name
//::     tableid = start_table_base
//::     for table in pddict['tables']:
//::        tabledict[table] = tableid
//::        tableid += 1
//::     #endfor
//::
//::     if pddict['p4plus']:
//::         api_prefix = 'p4pd_' + pddict['p4program']
//::     else:
//::         api_prefix = 'p4pd'
//::     #endif

static std::shared_ptr<Channel>     channel;
static std::unique_ptr<Debug::Stub> stub;
static bool grpc_inited = 0;

static int
p4pd_grpc_init(void)
{
    if (grpc_inited != 0) {
        return 0;
    }

    std::string grpc_server_port = "localhost:50054";

    if (getenv("HAL_GRPC_PORT")) {
        grpc_server_port = getenv("HAL_GRPC_PORT");
    }

    grpc_init();
    channel =
        grpc::CreateChannel(grpc_server_port, grpc::InsecureChannelCredentials());

    stub    = ::debug::Debug::NewStub(channel);

    grpc_inited = 1;

    return 0;
}

p4pd_error_t
${api_prefix}_entry_write(uint32_t tableid,
                          uint32_t index,
                          void     *swkey,
                          void     *swkey_mask,
                          void     *actiondata)
{
    MemoryRequestMsg  req_msg;
    MemoryResponseMsg rsp_msg;
    ClientContext     context;
    MemoryRequest     *req  = NULL;

    p4pd_grpc_init();

    req = req_msg.add_request();

    req->set_table_id(tableid);
    req->set_index(index);

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
            req->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

        //::            #endif
        //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            req->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            req->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

        //::            #endif
        //::            if pddict['tables'][table]['type'] == 'Ternary':
            req->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            req->set_swkey_mask(
                        std::string ((char*)(${table}_swkey_mask_t*)swkey_mask,
                                     sizeof(${table}_swkey_mask_t)));

            req->set_actiondata(
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

    Status status = stub->MemoryUpdate(&context, req_msg, &rsp_msg);

    return (P4PD_SUCCESS);
}

void*
allocate_debug_response_msg (void)
{
    MemoryResponseMsg *rsp_msg = new MemoryResponseMsg;
    return rsp_msg;
}

void
free_debug_response_msg (void *mem_rsp_msg)
{
    MemoryResponseMsg *rsp_msg = (MemoryResponseMsg*)mem_rsp_msg;
    delete rsp_msg;
}

/* read the table info from HAL */
p4pd_error_t
${api_prefix}_entry_read(uint32_t  tableid,
                         uint32_t  index,
                         void      *swkey,
                         void      *swkey_mask,
                         void      *actiondata,
                         void      *mem_rsp_msg,
                         int       *size)
{
    MemoryRequestMsg  req_msg;
    MemoryResponseMsg *rsp_msg = (MemoryResponseMsg*)mem_rsp_msg;
    ClientContext     context;
    MemoryRequest     *req    = NULL;

    p4pd_grpc_init();

    req = req_msg.add_request();

    req->set_table_id(tableid);
    req->set_index(index);

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
            req->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            req->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            req->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Ternary':
            req->set_swkey(
                        std::string ((char*)(${table}_swkey_t*)swkey,
                                     sizeof(${table}_swkey_t)));

            req->set_swkey_mask(
                        std::string ((char*)(${table}_swkey_mask_t*)swkey_mask,
                                     sizeof(${table}_swkey_mask_t)));

            req->set_actiondata(
                        std::string ((char*)(${table}_actiondata*)actiondata,
                                     sizeof(${table}_actiondata)));
            break;
            //::            #endif
            //::        #endfor
        default:
            // Invalid tableid
            return (P4PD_FAIL);
    }

    Status status = stub->MemoryGet(&context, req_msg, rsp_msg);
    if (status.ok()) {
        *size = rsp_msg->response_size();
        return (P4PD_SUCCESS);
    }

    return P4PD_FAIL;
}

p4pd_error_t
${api_prefix}_entry_populate(uint32_t  tableid,
                             void      *swkey,
                             void      *swkey_mask,
                             void      *actiondata,
                             void      *mem_rsp_msg,
                             int       response_index)
{
    if (mem_rsp_msg == NULL) {
        return P4PD_FAIL;
    }

    MemoryResponseMsg *rsp_msg = (MemoryResponseMsg*)mem_rsp_msg;

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
            memcpy(actiondata, (void*)rsp_msg->response(response_index).actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Hash' or pddict['tables'][table]['type'] == 'Hash_OTcam':
            memcpy(swkey, (void*)rsp_msg->response(response_index).swkey().c_str(),
                   sizeof(${table}_swkey_t));

            memcpy(actiondata, (void*)rsp_msg->response(response_index).actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;

            //::            #endif
            //::            if pddict['tables'][table]['type'] == 'Ternary':
            memcpy(swkey, (void*)rsp_msg->response(response_index).swkey().c_str(),
                   sizeof(${table}_swkey_t));

            memcpy(swkey_mask, (void*)rsp_msg->response(response_index).swkey_mask().c_str(),
                   sizeof(${table}_swkey_mask_t));

            memcpy(actiondata, (void*)rsp_msg->response(response_index).actiondata().c_str(),
                   sizeof(${table}_actiondata));
            break;
            //
            //::            #endif
            //::        #endfor
        default:
            // Invalid tableid
            return (P4PD_FAIL);
    }

    return (P4PD_SUCCESS);
}

void
${api_prefix}_register_entry_read(std::string block_name, std::string   reg_name, std::string filename)
{
    RegisterRequestMsg    req_msg;
    RegisterResponseMsg   rsp_msg;
    ClientContext         context;
    RegisterRequest       *req = NULL;

    req = req_msg.add_request();

    req->set_reg_name(reg_name);
    req->set_block_name(block_name);

    auto channel =
        grpc::CreateChannel("localhost:50054", grpc::InsecureChannelCredentials());

    auto stub = ::debug::Debug::NewStub(channel);

    Status status = stub->RegisterGet(&context, req_msg, &rsp_msg);

    int reg_data_sz  = rsp_msg.response_size();
    //*len = reg_data_sz;
    FILE *reg_fd = 0;
    if (!filename.empty()) {
        reg_fd = fopen(filename.c_str(), "wb+");
        if (!reg_fd) {
            std::cout <<"Null file descriptor";
            return;
        }
    }

    for (uint32_t i = 0; i  < reg_data_sz; i++) {
        //data[i].reg_name = rsp_msg.response(i).data().reg_name();
        //data[i].offset = rsp_msg.response(i).data().address();
        //data[i].value = rsp_msg.response(i).data().value();
        if (filename.empty()) {
            std::cout << "RegName: " << rsp_msg.response(i).data().reg_name()
                      << "Address(Offset): " <<  rsp_msg.response(i).data().address()
                      << "Value: "<< rsp_msg.response(i).data().value() << "\n";
        } else {

            std::fprintf(reg_fd, "RegName:  %s Address: %s Value: %s \n",
                         rsp_msg.response(i).data().reg_name().c_str(),
                         rsp_msg.response(i).data().address().c_str(),
                         rsp_msg.response(i).data().value().c_str());
            std::fflush(reg_fd);
        }
    }
}

void
${api_prefix}_register_list(std::string block_name, std::string   reg_name, std::string filename)
{
    RegisterRequestMsg   req_msg;
    RegisterResponseMsg  rsp_msg;
    ClientContext        context;
    RegisterRequest      *req = NULL;

    req = req_msg.add_request();

    req->set_reg_name(reg_name);
    req->set_block_name(block_name);

    auto channel =
        grpc::CreateChannel("localhost:50054", grpc::InsecureChannelCredentials());

    auto stub = ::debug::Debug::NewStub(channel);

    Status status = stub->RegisterGet(&context, req_msg, &rsp_msg);

    int reg_data_sz  = rsp_msg.response_size();
    FILE *reg_fd = 0;
    if (!filename.empty()) {
        reg_fd = fopen(filename.c_str(), "wb+");
        if (!reg_fd) {
            std::cout <<"Null file descriptor";
            return;
        }
    }

    for (uint32_t i = 0; i  < reg_data_sz; i++) {
        if (filename.empty()) {
            std::cout << "RegName: " << rsp_msg.response(i).data().reg_name() << "\n";
        } else {

            std::fprintf(reg_fd, "RegName:  %s \n",
                         rsp_msg.response(i).data().reg_name().c_str());
            std::fflush(reg_fd);
        }
    }
}

//::    if pddict['p4plus']:

p4pd_error_t
${api_prefix}_raw_table_entry_read(uint32_t tableid,
        uint8_t     actionid,
        void        *actiondata,
        uint64_t    address)
{
    MemoryRawRequestMsg     req_msg;
    MemoryRawResponseMsg    rsp_msg;
    ClientContext           context;
    MemoryRawRequest        *req = NULL;
    uint32_t                len = 0;
    p4pd_error_t            ret = P4PD_FAIL;

    p4pd_grpc_init();

    ${api_prefix}_raw_table_hwentry_query(tableid, actionid,
            &len);
    assert((len % 8) == 0);

    len /= 8;

    req = req_msg.add_request();
    req->set_address(address);
    req->set_len(len);

    Status status = stub->MemoryRawGet(&context, req_msg, &rsp_msg);

    if (status.ok()) {
        assert(rsp_msg.response_size() == 1);

        memcpy(actiondata, rsp_msg.response(0).actiondata().c_str(), len);
        ret = P4PD_SUCCESS;
    }

    return ret;
}

void
${api_prefix}_raw_table_entry_write(uint32_t tableid,
        uint8_t     actionid,
        void        *actiondata,
        uint16_t    actiondata_sz,
        uint64_t    address)
{


}
//::    #endif
