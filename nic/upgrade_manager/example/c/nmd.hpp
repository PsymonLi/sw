// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef __NMD_SERVICE_H__
#define __NMD_SERVICE_H__

#include "nic/delphi/sdk/delphi_sdk.hpp"
#include "nic/upgrade_manager/export/upgcsdk/upgrade.hpp"

namespace nmd {

using namespace upgrade;
using namespace std;

// NMDService is the service object for NMD manager 
class NMDService : public delphi::Service, public enable_shared_from_this<NMDService> {
private:
    delphi::SdkPtr        sdk_;
    string                svcName_;
    UpgSdkPtr             upgsdk_;
public:
    // NMDService constructor
    NMDService(delphi::SdkPtr sk);
    NMDService(delphi::SdkPtr sk, string name);

    // createUpgReqSpec creates a dummy Upgrade Request
    void createUpgReqSpec();
    void updateUpgReqSpec();
    // override service name method
    virtual string Name() { return svcName_; }
    void OnMountComplete(void);

    // timer for creating a dummy object
    ev::timer          createTimer;
    ev::timer          createTimerV2;
    void createTimerHandler(ev::timer &watcher, int revents);
    void createTimerHandlerV2(ev::timer &watcher, int revents);
    void createTimerUpdHandler(ev::timer &watcher, int revents);
};
typedef std::shared_ptr<NMDService> NMDServicePtr;

class NMDSvcHandler : public UpgHandler {
public:
    NMDSvcHandler() {}

    HdlrResp UpgStateReqCreate(UpgCtx& upgCtx) {
        HdlrResp resp={.resp=SUCCESS, .errStr=""};
        LogInfo("UpgHandler UpgStateReqCreate called for the NMD");
        return resp;
    }

    HdlrResp UpgStateReqDelete(UpgCtx& upgCtx) {
        HdlrResp resp={.resp=SUCCESS, .errStr=""};
        LogInfo("UpgHandler UpgStateReqDelete called for the NMD");
        return resp;
    }

    HdlrResp HandleUpgStateDataplaneDowntimePhase1(UpgCtx& upgCtx) {
        HdlrResp resp = {.resp=SUCCESS, .errStr=""};
        //HdlrResp resp = {.resp=FAIL, .errStr="BABABABA: NMD could not do HandleUpgStateDataplaneDowntimePhase1"};
        LogInfo("UpgHandler HandleUpgStateDataplaneDowntimePhase1 called for the SVC!!");
        return resp;
    }
};
} // namespace example

#endif // __NMD_SERVICE_H__
