// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef __EXAMPLE_H__
#define __EXAMPLE_H__

#include "nic/delphi/sdk/delphi_sdk.hpp"
#include "nic/upgrade_manager/export/upgrade.hpp"

namespace example {

using namespace std;
using namespace upgrade;

// ExUpgSvc is the service object for example upgrade service 
class ExUpgSvc : public delphi::Service, public enable_shared_from_this<ExUpgSvc> {
private:
    UpgSdkPtr          upgsdk_;
    delphi::SdkPtr     sdk_;
    string             svcName_;
public:
    // ExUpgSvc constructor
    ExUpgSvc(delphi::SdkPtr sk);
    ExUpgSvc(delphi::SdkPtr sk, string name);

    // override service name method
    virtual string Name() { return svcName_; }

    // timer for creating a dummy object
    ev::timer          createTimer;
    void createTimerHandler(ev::timer &watcher, int revents);

    void OnMountComplete(void) {
        LogInfo("ExUpgSvc OnMountComplete called!!");
        this->upgsdk_->OnMountComplete();
    }
};
typedef std::shared_ptr<ExUpgSvc> ExUpgSvcPtr;

class ExSvcHandler : public upgrade::UpgHandler {
public:
    ExSvcHandler(){}
    void UpgReqStatusCreate(delphi::objects::UpgReqStatusPtr req) {
         LogInfo("UpgHandler UpgReqStatusCreate called for the SVC!!");
    }
};

} // namespace example

#endif // __EXAMPLE_H__
