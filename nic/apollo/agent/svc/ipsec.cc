//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include "nic/apollo/agent/svc/ipsec_svc.hpp"

Status
IpsecSvcImpl::IpsecSAEncryptCreate(ServerContext *context,
                                   const pds::IpsecSAEncryptRequest *proto_req,
                                   pds::IpsecSAEncryptResponse *proto_rsp) {
    pds_svc_ipsec_sa_encrypt_create(proto_req, proto_rsp);
    return Status::OK;
}

Status
IpsecSvcImpl::IpsecSAEncryptDelete(ServerContext *context,
                                   const pds::IpsecSAEncryptDeleteRequest *proto_req,
                                   pds::IpsecSAEncryptDeleteResponse *proto_rsp) {
    pds_svc_ipsec_sa_encrypt_delete(proto_req, proto_rsp);
    return Status::OK;
}

Status
IpsecSvcImpl::IpsecSAEncryptGet(ServerContext *context,
                                const pds::IpsecSAEncryptGetRequest *proto_req,
                                pds::IpsecSAEncryptGetResponse *proto_rsp) {
    pds_svc_ipsec_sa_encrypt_get(proto_req, proto_rsp);
    return Status::OK;
}

Status
IpsecSvcImpl::IpsecSADecryptCreate(ServerContext *context,
                                   const pds::IpsecSADecryptRequest *proto_req,
                                   pds::IpsecSADecryptResponse *proto_rsp) {
    pds_svc_ipsec_sa_decrypt_create(proto_req, proto_rsp);
    return Status::OK;
}

Status
IpsecSvcImpl::IpsecSADecryptDelete(ServerContext *context,
                                   const pds::IpsecSADecryptDeleteRequest *proto_req,
                                   pds::IpsecSADecryptDeleteResponse *proto_rsp) {
    pds_svc_ipsec_sa_decrypt_delete(proto_req, proto_rsp);
    return Status::OK;
}

Status
IpsecSvcImpl::IpsecSADecryptGet(ServerContext *context,
                                const pds::IpsecSADecryptGetRequest *proto_req,
                                pds::IpsecSADecryptGetResponse *proto_rsp) {
    pds_svc_ipsec_sa_decrypt_get(proto_req, proto_rsp);
    return Status::OK;
}
