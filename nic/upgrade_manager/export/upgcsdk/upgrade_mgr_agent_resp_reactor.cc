// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "upgrade_mgr_agent_resp_reactor.hpp"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

namespace upgrade {

using namespace std;

string UpgMgrAgentRespReact::GetRespStr(delphi::objects::UpgRespPtr resp) {
    switch (resp->upgrespval()) {
        case UpgRespPass:
            return ("Upgrade Successful");
        case UpgRespFail:
            return ("Upgrade Failed");
        case UpgRespAbort:
            return ("Upgrade Aborted");
        default:
            return ("");
    }
}

delphi::error UpgMgrAgentRespReact::DeleteUpgReqSpec(void) {
    delphi::objects::UpgReqPtr req = make_shared<delphi::objects::UpgReq>();
    req->set_key(10);

    // find the object
    delphi::BaseObjectPtr obj = sdk_->FindObject(req);

    req = static_pointer_cast<delphi::objects::UpgReq>(obj);
 
    sdk_->DeleteObject(req);
    return delphi::error::OK();
}

void UpgMgrAgentRespReact::InvokeAgentHandler(delphi::objects::UpgRespPtr resp) {
    vector<string> errStrList;
    switch (resp->upgrespval()) {
        case UpgRespPass:
            upgAgentHandler_->UpgSuccessful();
            break;
        case UpgRespFail:
            for (int i=0; i<resp->upgrespfailstr_size(); i++) {
                errStrList.push_back(resp->upgrespfailstr(i));
            }
            upgAgentHandler_->UpgFailed(errStrList);
            break;
        case UpgRespAbort:
            for (int i=0; i<resp->upgrespfailstr_size(); i++) {
                errStrList.push_back(resp->upgrespfailstr(i));
            }
            upgAgentHandler_->UpgAborted(errStrList);
            break;
        default:
            break;
    }
}

delphi::error UpgMgrAgentRespReact::OnUpgRespCreate(delphi::objects::UpgRespPtr resp) {
    UPG_LOG_DEBUG("UpgRespHdlr::OnUpgRespCreate called with status {}", GetRespStr(resp));
    InvokeAgentHandler(resp);
    if (DeleteUpgReqSpec() == delphi::error::OK()) {
        UPG_LOG_DEBUG("Upgrade Req Object deleted for next request");
    }
    return delphi::error::OK();
}

delphi::error UpgMgrAgentRespReact::OnUpgRespVal(delphi::objects::UpgRespPtr
resp) {
    if (GetRespStr(resp) != "")
        UPG_LOG_DEBUG("UpgRespHdlr::OnUpgRespVal called with status: {}", GetRespStr(resp));
    InvokeAgentHandler(resp);
    if (DeleteUpgReqSpec() == delphi::error::OK()) {
        UPG_LOG_DEBUG("Upgrade Req Object deleted for next request");
    }
    return delphi::error::OK();
}

delphi::objects::UpgRespPtr UpgMgrAgentRespReact::FindUpgRespSpec(void) {
    delphi::objects::UpgRespPtr req = make_shared<delphi::objects::UpgResp>();
    req->set_key(10);

    // find the object
    delphi::BaseObjectPtr obj = sdk_->FindObject(req);

    return static_pointer_cast<delphi::objects::UpgResp>(obj);
}

};
