// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "upgrade_resp.hpp"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

namespace upgrade {

using namespace std;

delphi::objects::UpgRespPtr UpgMgrResp::findUpgMgrRespObj (uint32_t id) {
    delphi::objects::UpgRespPtr resp = make_shared<delphi::objects::UpgResp>();
    resp->set_key(id);

    // find the object
    delphi::BaseObjectPtr obj = sdk_->FindObject(resp);

    return static_pointer_cast<delphi::objects::UpgResp>(obj);
}

delphi::error UpgMgrResp::createUpgMgrResp(uint32_t id, UpgRespType val, vector<string> &str) {
    // create an object
    delphi::objects::UpgRespPtr resp = make_shared<delphi::objects::UpgResp>();
    resp->set_key(id);
    resp->set_upgrespval(val);
    while (!str.empty()) {
        resp->add_upgrespfailstr(str.back());
        str.pop_back();
    }
    // add it to database
    sdk_->SetObject(resp);

    UPG_LOG_DEBUG("Created upgrade response object for id {} state {} req: {}", id, val, resp);

    return delphi::error::OK();
}

delphi::error UpgMgrResp::DeleteUpgMgrResp(void) {
    auto upgResp = findUpgMgrRespObj(10);
    if (upgResp != NULL) {
        UPG_LOG_DEBUG("UpgResp object got deleted for agent");
        sdk_->DeleteObject(upgResp);
    }
    return delphi::error::OK();
}

delphi::error UpgMgrResp::UpgradeFinish(UpgRespType respType, vector<string> &str) {
    UPG_LOG_INFO("Returning response {} to agent", respType);
    auto upgResp = findUpgMgrRespObj(10);
    if (upgResp == NULL) {
        UPG_LOG_DEBUG("Sending Following String to Agent");
        for (uint i=0; i<str.size(); i++) {
            UPG_LOG_DEBUG("{}", str[i]);
        }
        RETURN_IF_FAILED(createUpgMgrResp(10, respType, str));
    }
    UPG_LOG_DEBUG("Responded back to the agent");
    return delphi::error::OK();
}

}
