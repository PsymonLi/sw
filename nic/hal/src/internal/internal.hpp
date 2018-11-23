//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __INTERNAL_HPP__
#define __INTERNAL_HPP__

#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/list.hpp"
#include "nic/sdk/include/sdk/ht.hpp"
#include "nic/include/bitmap.hpp"
#include "gen/proto/internal.pb.h"
#include "nic/hal/src/internal/internal.hpp"
#include "gen/proto/fwlog.pb.h"
#include "gen/proto/types.pb.h"

namespace hal {

void getprogram_address(const internal::ProgramAddressReq& req,
                        internal::ProgramAddressResponseMsg *rsp);

void allochbm_address(const internal::HbmAddressReq &req,
                      internal::HbmAddressResp *resp);

void configurelif_bdf(const internal::LifBdfReq &req,
                      internal::LifBdfResp *resp);

hal_ret_t software_phv_get (internal::SoftwarePhvGetRequest &req, internal::SoftwarePhvGetResponseMsg *rsp);
hal_ret_t software_phv_inject (internal::SoftwarePhvInject &req, internal::SoftwarePhvResponse *rsp);
hal_ret_t log_flow (fwlog::FWEvent &req, internal::LogFlowResponse *rsp); 
hal_ret_t quiesce_msg_snd(const types::Empty &request, types::Empty* rsp);
hal_ret_t quiesce_start(const types::Empty &request, types::Empty* rsp);
hal_ret_t quiesce_stop(const types::Empty &request, types::Empty* rsp);
hal_ret_t ipseccb_create(internal::IpsecCbSpec& spec,
                       internal::IpsecCbResponse *rsp);

hal_ret_t ipseccb_update(internal::IpsecCbSpec& spec,
                       internal::IpsecCbResponse *rsp);

hal_ret_t ipseccb_delete(internal::IpsecCbDeleteRequest& req,
                       internal::IpsecCbDeleteResponseMsg *rsp);

hal_ret_t ipseccb_get(internal::IpsecCbGetRequest& req,
                    internal::IpsecCbGetResponseMsg *rsp);

hal_ret_t tcpcb_create(internal::TcpCbSpec& spec,
                       internal::TcpCbResponse *rsp);

hal_ret_t tcpcb_update(internal::TcpCbSpec& spec,
                       internal::TcpCbResponse *rsp);

hal_ret_t tcpcb_delete(internal::TcpCbDeleteRequest& req,
                       internal::TcpCbDeleteResponseMsg *rsp);

hal_ret_t tcpcb_get(internal::TcpCbGetRequest& req,
                    internal::TcpCbGetResponseMsg *rsp);

hal_ret_t tlscb_create(internal::TlsCbSpec& spec,
                       internal::TlsCbResponse *rsp);

hal_ret_t tlscb_update(internal::TlsCbSpec& spec,
                       internal::TlsCbResponse *rsp);

hal_ret_t tlscb_delete(internal::TlsCbDeleteRequest& req,
                       internal::TlsCbDeleteResponseMsg *rsp);

hal_ret_t tlscb_get(internal::TlsCbGetRequest& req,
                    internal::TlsCbGetResponseMsg *rsp);

hal_ret_t wring_create(internal::WRingSpec& spec,
                       internal::WRingResponse *rsp);

hal_ret_t wring_update(internal::WRingSpec& spec,
                       internal::WRingResponse *rsp);

hal_ret_t wring_get_entries(internal::WRingGetEntriesRequest& req, internal::WRingGetEntriesResponseMsg *rsp);

hal_ret_t wring_get_meta(internal::WRingSpec& spec, internal::WRingGetMetaResponseMsg *rsp);

hal_ret_t wring_set_meta (internal::WRingSpec& spec, internal::WRingSetMetaResponse *rsp);


}    // namespace hal

#endif    // __INTERNAL_HPP__

