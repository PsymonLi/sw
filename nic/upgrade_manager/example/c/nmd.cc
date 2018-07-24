// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "nic/upgrade_manager/example/c/nmd.hpp"

using namespace std;
using namespace nmd;
using namespace upgrade;

int main(int argc, char **argv) {
    // Create delphi SDK
    delphi::SdkPtr sdk(make_shared<delphi::Sdk>());
    string myName = "NMD";

    // Create a service instance
    shared_ptr<NMDService> exupgsvc = make_shared<NMDService>(sdk, myName);
    assert(exupgsvc != NULL);

    sdk->RegisterService(exupgsvc);

    // start a timer to create an object
    exupgsvc->createTimer.set<NMDService, &NMDService::createTimerHandler>(exupgsvc.get());
    exupgsvc->createTimer.start(2, 0);

    exupgsvc->createTimerV2.set<NMDService, &NMDService::createTimerHandlerV2>(exupgsvc.get());
    exupgsvc->createTimerV2.start(20, 0);
    // run the main loop
    return sdk->MainLoop();
}

namespace nmd {

int count = 0;
// NMDService constructor
NMDService::NMDService(delphi::SdkPtr sk) : NMDService(sk, "NMDService") {
}

NMDService::NMDService(delphi::SdkPtr sk, string name) {
    sdk_ = sk;
    svcName_ = name;

    upgsdk_ = make_shared<UpgSdk>(sdk_, make_shared<NMDSvcHandler>(), name, AGENT);

    UPG_LOG_DEBUG("NMD service constructor got called");
}

// createTimerHandler creates a dummy code upgrade request
void NMDService::createTimerHandler(ev::timer &watcher, int revents) {
    upgsdk_->StartNonDisruptiveUpgrade();
    UPG_LOG_DEBUG("NMD: called start upgrade");
}

void NMDService::createTimerHandlerV2(ev::timer &watcher, int revents) {
    vector<string> checkHealth;
    upgsdk_->GetUpgradeStatus(checkHealth);
    for (uint i=0; i<checkHealth.size(); i++) {
        UPG_LOG_DEBUG("{}", checkHealth[i]);
    }
    //upgsdk_->AbortUpgrade();
    UPG_LOG_DEBUG("NMD: Reported Upgrade Manager health.");
}

void NMDService::OnMountComplete() {
}

}
