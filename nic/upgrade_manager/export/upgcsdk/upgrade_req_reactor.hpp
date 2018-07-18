// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef __UPGRADE_REQ_REACTOR_H__
#define __UPGRADE_REQ_REACTOR_H__

#include "nic/delphi/sdk/delphi_sdk.hpp"
#include "nic/upgrade_manager/upgrade/upgrade.delphi.hpp"
#include "upgrade_handler.hpp"
#include "upgrade_app_resp_hdlr.hpp"

namespace upgrade {

using namespace std;

// UpgReqReactor is the reactor for the UpgStateReq object
class UpgReqReactor : public delphi::objects::UpgStateReqReactor {
    delphi::SdkPtr sdk_;
    string svcName_;
    UpgHandlerPtr upgHdlrPtr_;
    UpgAppRespHdlrPtr upgAppRespPtr_;
    delphi::objects::UpgStateReqPtr upgReqStatus_;

    delphi::objects::UpgAppPtr FindUpgAppPtr(void);
    delphi::objects::UpgAppPtr CreateUpgAppObj(void);
    void RegisterUpgApp(void);
public:
    //This constructor is used only for upgrade_sdkclib_test
    UpgReqReactor(delphi::SdkPtr sk, string name = "test") {
        sdk_ = sk;
        svcName_ = name;
        upgHdlrPtr_ = make_shared<UpgHandler>();
        upgAppRespPtr_ = make_shared<UpgAppRespHdlr>(sk, name);
        InitStateMachineVector();
    }

    UpgReqReactor(delphi::SdkPtr sk, string name, UpgAppRespHdlrPtr ptr) {
        sdk_ = sk;
        svcName_ = name;
        upgHdlrPtr_ = make_shared<UpgHandler>();
        upgAppRespPtr_ = ptr;
    }

    UpgReqReactor(delphi::SdkPtr sk, UpgHandlerPtr uh, string name, UpgAppRespHdlrPtr ptr) {
        sdk_ = sk;
        svcName_ = name;
        upgHdlrPtr_ = uh;
        upgAppRespPtr_ = ptr;
    }

    // OnUpgStateReqCreate gets called when UpgStateReq object is created
    virtual delphi::error OnUpgStateReqCreate(delphi::objects::UpgStateReqPtr req);

    // OnUpgStateReqDelete gets called when UpgStateReq object is deleted
    virtual delphi::error OnUpgStateReqDelete(delphi::objects::UpgStateReqPtr req);

    // OnUpgReqState gets called when UpgReqState attribute changes
    virtual delphi::error OnUpgReqState(delphi::objects::UpgStateReqPtr req);

    void InvokeAppHdlr(UpgReqStateType type, HdlrResp &hdlrResp);

    void GetUpgCtx(delphi::objects::UpgStateReqPtr req);

    virtual void OnMountComplete(void);
};
typedef std::shared_ptr<UpgReqReactor> UpgReqReactorPtr;

} // namespace upgrade

#endif // __UPGRADE_REQ_REACTOR_H__
