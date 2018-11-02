// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#include <ev++.h>
#include "delphi_sdk.hpp"

namespace delphi {

// Sdk constructor
Sdk::Sdk() {
    client_ = make_shared<DelphiClient>();
    stopAsync_.set<Sdk, &Sdk::asyncStop>(this);
    stopAsync_.start();
}

// MainLoop runs the main event loop
int Sdk::MainLoop() {
    // first connect to delphi server
    error err = client_->Connect();
    if (err.IsNotOK()) {
        LogError("Could not connect to the server. exiting..");
        exit(1);
    }

    // run event loop
    loop_.run(0);

    return 0;
}

// Connect connects to delphi hub
error Sdk::Connect() {
    return client_->Connect();
}

// RegisterService registers the service with delphi client
error Sdk::RegisterService(ServicePtr service) {
    return client_->RegisterService(service);
}

error Sdk::MountKind(string kind, MountMode mode) {
    return client_->MountKind(kind, mode);
}

error Sdk::MountKey(string kind, string key, MountMode mode) {
    return client_->MountKey(kind, key, mode);
}

error Sdk::WatchKind(string kind, BaseReactorPtr rctr) {
    return client_->WatchKind(kind, rctr);
}

error Sdk::WatchMountComplete(BaseReactorPtr rctr) {
    return client_->WatchMountComplete(rctr);
}

BaseObjectPtr Sdk::FindObject(BaseObjectPtr objinfo) {
    return client_->FindObject(objinfo);
}

error Sdk::SetObject(BaseObjectPtr objinfo) {
    return client_->SetObject(objinfo);
}

error Sdk::DeleteObject(BaseObjectPtr objinfo) {
    return client_->DeleteObject(objinfo);
}

error Sdk::QueueUpdate(BaseObjectPtr objinfo) {
    return client_->QueueUpdate(objinfo);
}

error Sdk::QueueDelete(BaseObjectPtr objinfo) {
    return client_->QueueDelete(objinfo);
}

vector<BaseObjectPtr> Sdk::ListKind(string kind) {
    return client_->ListKind(kind);
}

map<string, BaseObjectPtr> Sdk::GetSubtree(string kind) {
    return client_->GetSubtree(kind);
}

// TestLoop runs the event loop for test runs
void Sdk::TestLoop() {
    // fake a connection to delphi hub
    client_->MockConnect(1);

    // run event loop without connecting to delphi hub
    loop_.run(0);
}

// Stop stops the sdk event loop and closes delphi hub client
error Sdk::Stop() {
    this->stopAsync_.send();
    return error::OK();
}

// asyncStop is the async handler to stop the event loop and close delphi client
void Sdk::asyncStop(ev::async &watcher, int revents) {
    // break the loop
    loop_.break_loop(ev::ALL);
    stopAsync_.stop();

    // close delphi hub client
    client_->Close();
}
} // namespace delphi
