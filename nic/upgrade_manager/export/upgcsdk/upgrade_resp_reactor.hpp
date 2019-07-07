// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef __UPGRAGE_RESP_REACTOR_H__
#define __UPGRAGE_RESP_REACTOR_H__

#include "nic/delphi/sdk/delphi_sdk.hpp"
#include "gen/proto/upgrade.delphi.hpp"
#include "upgrade_app_resp_reactor.hpp"

namespace upgrade {

using namespace std;

class UpgRespReact : public delphi::objects::UpgRespReactor {
    delphi::SdkPtr           sdk_;
    UpgAgentHandlerPtr       upgAgentHandler_;

    delphi::error DeleteUpgReqSpec(void);
public:
    UpgRespReact() {}

    UpgRespReact(delphi::SdkPtr sk, UpgAgentHandlerPtr ptr) {
        upgAgentHandler_ = ptr;
        sdk_ = sk;
    }

    UpgAgentHandlerPtr GetUpgAgentHandlerPtr() {
        return upgAgentHandler_;
    }

    // OnUpgRespCreate gets called when UpgResp object is created
    virtual delphi::error OnUpgRespCreate(delphi::objects::UpgRespPtr resp);

    // OnUpgRespVal gets called when UpgRespVal attribute changes
    virtual delphi::error OnUpgRespVal(delphi::objects::UpgRespPtr resp);

    string GetRespStr(delphi::objects::UpgRespPtr resp);
    void InvokeAgentHandler(delphi::objects::UpgRespPtr resp);

    //FindUpgRespSpec returns UpgResp Object
    delphi::objects::UpgRespPtr FindUpgRespSpec(void);

    virtual void OnMountComplete(void);
};
typedef std::shared_ptr<UpgRespReact> UpgRespReactPtr;

} //namespace upgrade
#endif // __UPGRAGE_RESP_REACTOR_H__
