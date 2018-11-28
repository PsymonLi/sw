// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

#ifndef _DELPHI_SERVER_H_
#define _DELPHI_SERVER_H_

#include <ev++.h>

#include "nic/delphi/utils/utils.hpp"
#include "nic/delphi/messenger/messenger_server.hpp"
#include "nic/delphi/shm/shm.hpp"

namespace delphi {

using namespace std;
using namespace delphi::messenger;

// an object sync is triggered at sync period
#define SYNC_PERIOD 0.005

// DbSubtree is a sub tree of delphi database
class DbSubtree {
public:
    map<string, ObjectDataPtr>   objects;
};
typedef std::shared_ptr<DbSubtree> DbSubtreePtr;

// MountInfo contains info about each service for a mount point
class MountInfo {
public:
    string       MountPath;    // path being mounted
    string       ServiceName;  // service mounting the path
    MountMode    Mode;         // mount mode (read-only or read-write)
};

// ServiceInfo is information about a service
class ServiceInfo {
public:
    string                 ServiceName;   // unique service name
    uint16_t               ServiceID;     // service id allocated for this service
    int                    SockCtx;       // socket id
    vector<MountInfo>      Mounts;        // mount points requested by this service
    bool                   HasMounted(string kind, string key);
};
typedef std::shared_ptr<ServiceInfo> ServiceInfoPtr;


// MountPoint contains list of services mounting each object kind
class MountPoint {
public:
    string                   MountPath;   // mount path (kind + key)
    map<string, MountInfo>   Services;    // list of services
};
typedef std::shared_ptr<MountPoint> MountPointPtr;

// DelphiServer is the server instance
class DelphiServer : public messenger::ServerHandler, public std::enable_shared_from_this<DelphiServer> {
public:
        DelphiServer();                         // constructor
        error Start();                          // start delphi server
        error Stop();                           // stop delphi server
        DbSubtreePtr GetSubtree(string kind);   // get a subtree of objects for a kind
        vector<ServiceInfoPtr> ListService();   // list all registered service

        // required by messenger::ServerHandler
        error HandleChangeReq(int sockCtx, vector<ObjectData> req, vector<ObjectData *> *resp);
        error HandleMountReq(int sockCtx, MountReqMsgPtr req, MountRespMsgPtr resp);
        error HandleSocketClosed(int sockCtx);

private:
    error          addObject(string kind,   string key, ObjectDataPtr obj);
    error          delObject(string kind,   string key);
    ServiceInfoPtr addService(string svcName, int sockCtx);
    ServiceInfoPtr findServiceName(string svcName);
    ServiceInfoPtr findServiceID(uint16_t svcID);
    uint16_t       getNewServiceID();
    ServiceInfoPtr findSock(int sockCtx);
    error          delService(ServiceInfoPtr svc, int sockCtx);
    MountPointPtr  addMountPoint(string kind);
    MountPointPtr  findMountPoint(string kind);
    error          deleteMountPoint(string kind);
    error          requestMount(string kind, string key, string svcName, MountMode mode);
    error          releaseMount(string mountPath, string svcName);

    MessangerServerPtr             msgServer;      // messenger server
    map<string, DbSubtreePtr>      subtrees;       // subtree per object kind
    vector<ObjectDataPtr>          syncQueue;      // sync queue of pending objects
    ev::timer                      syncTimer;      // sync timer
    uint32_t                       currServiceId;  // service id we are currently allocating
    map<string, ServiceInfoPtr>    services;       // map of services indexed by name
    map<uint16_t, ServiceInfoPtr>  serviceIds;     // map of services by service id
    map<int, ServiceInfoPtr>       sockets;        // map of socket id to service
    map<string, MountPointPtr>     mountPoints;    // map of object kind to mount point
    delphi::shm::DelphiShmPtr      srv_shm_;       // shared memory instance

protected:
    void syncTimerHandler(ev::timer &watcher, int revents);
};
typedef std::shared_ptr<DelphiServer> DelphiServerPtr;

// internal functions
static inline string getMountPath(string kind, string key) {
    return kind + "|" + key;
}

} // namespace delphi

#endif // _DELPHI_SERVER_H_
