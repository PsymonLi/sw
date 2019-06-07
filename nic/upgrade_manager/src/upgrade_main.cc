// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <stdio.h>
#include <iostream>

#include "nic/upgrade_manager/lib/upgrade.hpp"
#include "nic/upgrade_manager/utils/upgrade_log.hpp"

using namespace std;
using namespace upgrade;

int main(int argc, char **argv) {
    initializeLogger();
    // Create delphi SDK
    delphi::SdkPtr sdk(make_shared<delphi::Sdk>());
    string myName = "upgrade";

    // if name is specified, use it
    if (argc > 1) {
        myName = argv[1];
    }

    // Create a service instance
    shared_ptr<UpgradeService> upgsvc = make_shared<UpgradeService>(sdk, myName);
    assert(upgsvc != NULL);
    sdk->RegisterService(upgsvc);

    // run the main loop
    return sdk->MainLoop();
}
