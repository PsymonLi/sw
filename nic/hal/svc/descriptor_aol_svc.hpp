#ifndef __DESCRIPTOR_AOL_SVC_HPP__
#define __DESCRIPTOR_AOL_SVC_HPP__

#include "nic/include/base.h"
#include "grpc++/grpc++.h"
#include "nic/gen/proto/hal/types.pb.h"
#include "nic/gen/proto/hal/descriptor_aol.grpc.pb.h"

using grpc::ServerContext;
using grpc::Status;

using descraol::DescrAol;
using descraol::DescrAolSpec;
using descraol::DescrAolRequest;
using descraol::DescrAolRequestMsg;
using descraol::DescrAolResponseMsg;

class DescrAolServiceImpl final : public DescrAol::Service {
public:
    Status DescrAolGet(ServerContext* context,
                        const DescrAolRequestMsg* request,
                        DescrAolResponseMsg* response) override;
};

#endif /* __DESCRIPTOR_AOL_SVC_HPP__ */
