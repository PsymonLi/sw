// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	// The number of retries before dataplane last sync is declared to be successful
	defaultLastSyncRetries       = 10
	defaultMigrationPollInterval = 1 * time.Second
)

// MigrationStatus holds the enum for migration state
type MigrationStatus uint32

const (
	// NONE : No migration in progress
	NONE MigrationStatus = iota

	// MIGRATING : Start migration
	MIGRATING

	// FAILED : Dataplane failed migration
	FAILED

	// DONE : Dataplane migration successful
	DONE
)

// EndpointState is a wrapper for endpoint object
type EndpointState struct {
	smObjectTracker
	sync.Mutex
	Endpoint        *ctkit.Endpoint                `json:"-"` // embedding endpoint object
	groups          map[string]*SecurityGroupState // list of security groups
	stateMgr        *Statemgr                      // state manager
	migrationState  MigrationStatus
	markedForDelete bool
	moveEP          *netproto.Endpoint
}

// endpointKey returns endpoint key
func (eps *EndpointState) endpointKey() string {
	return eps.Endpoint.ObjectMeta.Name
}

// EndpointStateFromObj conerts from memdb object to endpoint state
func EndpointStateFromObj(obj runtime.Object) (*EndpointState, error) {
	switch obj.(type) {
	case *ctkit.Endpoint:
		epobj := obj.(*ctkit.Endpoint)
		switch epobj.HandlerCtx.(type) {
		case *EndpointState:
			eps := epobj.HandlerCtx.(*EndpointState)
			return eps, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

func convertEndpoint(eps *workload.Endpoint) *netproto.Endpoint {
	// build endpoint
	creationTime, _ := types.TimestampProto(time.Now())
	nep := netproto.Endpoint{
		TypeMeta:   eps.TypeMeta,
		ObjectMeta: agentObjectMeta(eps.ObjectMeta),
		Spec: netproto.EndpointSpec{
			NetworkName:   eps.Status.Network,
			IPv4Addresses: eps.Status.IPv4Addresses,
			IPv6Addresses: eps.Status.IPv6Addresses,
			MacAddress:    eps.Status.MacAddress,
			UsegVlan:      eps.Status.MicroSegmentVlan,
			NodeUUID:      eps.Spec.NodeUUID,
		},
	}
	nep.CreationTime = api.Timestamp{Timestamp: *creationTime}

	if eps.Status.Migration != nil && eps.Status.Migration.Status == workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String() {
		nep.Spec.Migration = netproto.MigrationState_FROM_NON_PEN_HOST.String()
	}

	return &nep
}

// AddSecurityGroup adds a security group to an endpoint
func (eps *EndpointState) AddSecurityGroup(sgs *SecurityGroupState) error {
	// lock the endpoint
	eps.Endpoint.Lock()
	defer eps.Endpoint.Unlock()

	// add security group to the list
	eps.Endpoint.Status.SecurityGroups = append(eps.Endpoint.Status.SecurityGroups, sgs.SecurityGroup.Name)
	eps.groups[sgs.SecurityGroup.Name] = sgs

	// save it to api server
	err := eps.Write()
	if err != nil {
		log.Errorf("Error writing the endpoint state to api server. Err: %v", err)
		return err
	}

	sm := MustGetStatemgr()
	return sm.UpdateObjectToMbus(eps.Endpoint.MakeKey("cluster"), eps, references(eps.Endpoint))
}

// DelSecurityGroup removes a security group from an endpoint
func (eps *EndpointState) DelSecurityGroup(sgs *SecurityGroupState) error {
	// lock the endpoint
	eps.Endpoint.Lock()
	defer eps.Endpoint.Unlock()

	// remove the security group from the list
	for i, sgname := range eps.Endpoint.Status.SecurityGroups {
		if sgname == sgs.SecurityGroup.Name {
			if i < (len(eps.Endpoint.Status.SecurityGroups) - 1) {
				eps.Endpoint.Status.SecurityGroups = append(eps.Endpoint.Status.SecurityGroups[:i], eps.Endpoint.Status.SecurityGroups[i+1:]...)
			} else {
				eps.Endpoint.Status.SecurityGroups = eps.Endpoint.Status.SecurityGroups[:i]
			}
		}
	}
	delete(eps.groups, sgs.SecurityGroup.Name)

	sm := MustGetStatemgr()
	return sm.UpdateObjectToMbus(eps.Endpoint.MakeKey("cluster"), eps, references(eps.Endpoint))
}

// attachSecurityGroups attach all security groups
func (eps *EndpointState) attachSecurityGroups() error {
	// get a list of security groups
	sgs, err := eps.stateMgr.ListSecurityGroups()
	if err != nil {
		log.Errorf("Error getting the list of security groups. Err: %v", err)
		return err
	}

	// walk all sgs and see if endpoint matches the selector
	for _, sg := range sgs {
		if sg.SecurityGroup.Spec.WorkloadSelector != nil && sg.SecurityGroup.Spec.WorkloadSelector.Matches(labels.Set(eps.Endpoint.Status.WorkloadAttributes)) {
			err = sg.AddEndpoint(eps)
			if err != nil {
				log.Errorf("Error adding ep %s to sg %s. Err: %v", eps.Endpoint.Name, sg.SecurityGroup.Name, err)
				return err
			}

			// add sg to endpoint
			eps.Endpoint.Status.SecurityGroups = append(eps.Endpoint.Status.SecurityGroups, sg.SecurityGroup.Name)
			eps.groups[sg.SecurityGroup.Name] = sg
		}
	}

	return nil
}

// Write writes the object to api server
func (eps *EndpointState) Write() error {
	return eps.Endpoint.Write()
}

// Delete deletes all state associated with the endpoint
func (eps *EndpointState) Delete() error {
	// detach the endpoint from security group
	for _, sg := range eps.groups {
		err := sg.DelEndpoint(eps)
		if err != nil {
			log.Errorf("Error removing endpoint from sg. Err: %v", err)
		}
	}

	return nil
}

// NewEndpointState returns a new endpoint object
func NewEndpointState(epinfo *ctkit.Endpoint, stateMgr *Statemgr) (*EndpointState, error) {
	// build the endpoint state
	eps := EndpointState{
		Endpoint: epinfo,
		groups:   make(map[string]*SecurityGroupState),
		stateMgr: stateMgr,
	}
	epinfo.HandlerCtx = &eps

	//move old fields to new state
	if eps.Endpoint.Status.IPv4Address != "" {
		eps.Endpoint.Status.IPv4Addresses = append(eps.Endpoint.Status.IPv4Addresses, eps.Endpoint.Status.IPv4Address)
		eps.Endpoint.Status.IPv4Address = ""
	}

	// attach security groups
	err := eps.attachSecurityGroups()
	if err != nil {
		log.Errorf("Error attaching security groups to ep %v. Err: %v", eps.Endpoint.Name, err)
		return nil, err
	}

	// save it to api server
	err = eps.Write()
	if err != nil {
		log.Errorf("Error writing the endpoint state to api server. Err: %v", err)
		return nil, err
	}

	eps.smObjectTracker.init(&eps)
	//No need to send update notification as we are not updating status yet
	eps.smObjectTracker.skipUpdateNotification()
	return &eps, nil
}

// GetKey returns the key of NetworkSecurityPolicy
func (eps *EndpointState) GetKey() string {
	return eps.Endpoint.GetKey()
}

//TrackedDSCs tracked DSCs
func (eps *EndpointState) TrackedDSCs() []string {

	dscs, _ := eps.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if eps.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) &&
			dsc.DistributedServiceCard.Status.PrimaryMAC == eps.Endpoint.Spec.NodeUUID {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

// processDSCUpdate sgpolicy update handles for DSC
func (eps *EndpointState) processDSCUpdate(dsc *cluster.DistributedServiceCard) error {

	eps.Endpoint.Lock()
	defer eps.Endpoint.Unlock()

	if dsc.Status.PrimaryMAC == eps.Endpoint.Spec.NodeUUID {
		eps.smObjectTracker.startDSCTracking(dsc.Name)
	}

	return nil
}

// processDSCUpdate sgpolicy update handles for DSC
func (eps *EndpointState) processDSCDelete(dsc *cluster.DistributedServiceCard) error {

	eps.Endpoint.Lock()
	defer eps.Endpoint.Unlock()

	eps.smObjectTracker.stopDSCTracking(dsc.Name)

	return nil
}

func (eps *EndpointState) isMarkedForDelete() bool {
	return eps.markedForDelete
}

//GetEndpointWatchOptions gets options
func (sm *Statemgr) GetEndpointWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec", "Status.Migration", "Status.NodeUUID"}
	return &opts
}

// OnEndpointCreateReq gets called when agent sends create request
func (sm *Statemgr) OnEndpointCreateReq(nodeID string, objinfo *netproto.Endpoint) error {
	return nil
}

// OnEndpointUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnEndpointUpdateReq(nodeID string, objinfo *netproto.Endpoint) error {

	return nil
}

// OnEndpointDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnEndpointDeleteReq(nodeID string, objinfo *netproto.Endpoint) error {
	return nil
}

// OnEndpointOperUpdate gets called when agent sends oper update
func (sm *Statemgr) OnEndpointOperUpdate(nodeID string, objinfo *netproto.Endpoint) error {

	eps, err := sm.FindEndpoint(objinfo.Tenant, objinfo.Name)
	if err != nil {
		return err
	}

	eps.updateNodeVersion(nodeID, objinfo.ObjectMeta.GenerationID)
	// TODO : add nimbus code to update the endpoint migration status and send it back to NPM
	if objinfo.Spec.Migration == netproto.MigrationState_DONE.String() {
		log.Infof("Setting migration status to success for EP %v", eps.Endpoint.Name)
		eps.migrationState = DONE
	}
	return nil
}

// OnEndpointOperDelete is called when agent sends oper delete
func (sm *Statemgr) OnEndpointOperDelete(nodeID string, objinfo *netproto.Endpoint) error {
	// FIXME: handle endpoint status updates from agent
	return nil
}

// OnEndpointReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnEndpointReconnect() {
	return
}

// FindEndpoint finds endpoint by name
func (sm *Statemgr) FindEndpoint(tenant, name string) (*EndpointState, error) {
	// find the object
	obj, err := sm.FindObject("Endpoint", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return EndpointStateFromObj(obj)
}

// EndpointIsPending finds endpointy by name
func (sm *Statemgr) EndpointIsPending(tenant, name string) (bool, error) {
	// find the object
	return sm.IsPending("Endpoint", tenant, "default", name)
}

// ListEndpoints lists all endpoints
func (sm *Statemgr) ListEndpoints() ([]*EndpointState, error) {
	objs := sm.ListObjects("Endpoint")

	var eps []*EndpointState
	for _, obj := range objs {
		ep, err := EndpointStateFromObj(obj)
		if err != nil {
			return eps, err
		}

		eps = append(eps, ep)
	}

	return eps, nil
}

//GetDBObject get db object
func (eps *EndpointState) GetDBObject() memdb.Object {
	if eps.moveEP != nil {
		return eps.moveEP
	}
	return convertEndpoint(&eps.Endpoint.Endpoint)
}

// OnEndpointCreate creates an endpoint
func (sm *Statemgr) OnEndpointCreate(epinfo *ctkit.Endpoint) error {
	log.Infof("Creating endpoint: %#v", epinfo)

	// find network
	ns, err := sm.FindNetwork(epinfo.Tenant, epinfo.Status.Network)
	if err != nil {
		//Retry again, Create network may be lagging.
		time.Sleep(20 * time.Millisecond)
		ns, err = sm.FindNetwork(epinfo.Tenant, epinfo.Status.Network)
		if err != nil {
			log.Errorf("could not find the network %s for endpoint %+v. Err: %v", epinfo.Status.Network, epinfo.ObjectMeta, err)
			return kvstore.NewKeyNotFoundError(epinfo.Status.Network, 0)
		}
	}

	// create a new endpoint instance
	eps, err := NewEndpointState(epinfo, sm)
	if err != nil {
		log.Errorf("Error creating endpoint state from spec{%+v}, Err: %v", epinfo, err)
		return err
	}

	// save the endpoint in the database
	ns.AddEndpoint(eps)
	err = sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return nil
	}
	return nil
}

// OnEndpointUpdate handles update event
func (sm *Statemgr) OnEndpointUpdate(epinfo *ctkit.Endpoint, nep *workload.Endpoint) error {
	log.Infof("Got EP update. %v", nep)
	epinfo.ObjectMeta = nep.ObjectMeta

	eps, err := sm.FindEndpoint(epinfo.Tenant, epinfo.Name)
	if err != nil {
		return err
	}

	if epinfo.Endpoint.Status.Network != nep.Status.Network {
		log.Infof("Network updated for EP %v from %v to %v", nep.Name, epinfo.Endpoint.Status.Network, nep.Status.Network)
		sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
		epinfo.Endpoint.Spec = nep.Spec
		epinfo.Endpoint.Status = nep.Status
		sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	} else {
		sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	}

	if nep.Status.Migration == nil || nep.Status.Migration.Status == workload.EndpointMigrationStatus_DONE.String() {
		return nil
	}

	ws, err := sm.FindWorkload(nep.Tenant, getWorkloadNameFromEPName(nep.Name))
	if err != nil {
		log.Errorf("Failed to get workload for EP [%v]. Err : %v", nep.Name, err)
		return err
	}

	if ws.isMigrating() {
		return sm.handleMigration(epinfo, nep)
	}

	return nil
}

func (sm *Statemgr) handleMigration(epinfo *ctkit.Endpoint, nep *workload.Endpoint) error {
	log.Infof("Handling EP migration for %v", nep)
	eps, err := EndpointStateFromObj(epinfo)
	if err != nil {
		log.Errorf("failed to find the endpoint state for EP [%v]. Err : %v", epinfo.Name, err)
		return ErrEndpointNotFound
	}

	eps.Lock()
	defer eps.Unlock()

	ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
	if err != nil {
		log.Errorf("Failed to get workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
		return err
	}

	// Start Last Sync of migration
	if nep.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {

		if eps.migrationState != MIGRATING {
			// We get here if it's after a restart, start the moveEndpoint go routine
			eps.migrationState = MIGRATING

			ws.epMoveWg.Add(1)
			go sm.moveEndpoint(epinfo, nep, ws)
		}
		log.Infof("Starting last sync for EP [%v] to move to [%v].", epinfo.Endpoint.Name, epinfo.Endpoint.Spec.NodeUUID)
		eps.Endpoint.Status = nep.Status

		// Send DONE to the new host, so that DSC can start its terminal synch
		newEP := convertEndpoint(&epinfo.Endpoint)
		newEP.Spec.Migration = netproto.MigrationState_DONE.String()
		sm.mbus.AddObjectWithReferences(epinfo.MakeKey("cluster"), newEP, references(epinfo))
		eps.Endpoint.Write()

		return nil
	}

	if nep.Status.Migration.Status == workload.EndpointMigrationStatus_DONE.String() {
		log.Infof("EP [%v] successfully moved to [%v]", nep.Name, nep.Spec.NodeUUID)
		return nil
	}

	// If it's an EP update operation with Spec and Status NodeUUID same, it would mean that the migration was aborted or failed
	if nep.Spec.NodeUUID == nep.Status.NodeUUID && (nep.Status.Migration.Status == workload.EndpointMigrationStatus_FAILED.String()) {
		// Set appropriate state of the endpoint object and call cancel of move
		eps.Endpoint.Spec = epinfo.Spec
		eps.Endpoint.Status.Migration.Status = epinfo.Status.Migration.Status

		//eps.Endpoint.Write()
		return nil
	}

	// If we reached here, it would mean it is start of new migration.
	eps.migrationState = MIGRATING

	ws.epMoveWg.Add(1)
	go sm.moveEndpoint(epinfo, nep, ws)

	return nil
}

// OnEndpointDelete deletes an endpoint
func (sm *Statemgr) OnEndpointDelete(epinfo *ctkit.Endpoint) error {
	log.Infof("Deleting Endpoint: %#v", epinfo)

	// see if we have the endpoint
	eps, err := EndpointStateFromObj(epinfo)
	if err != nil {
		log.Errorf("could not find the endpoint %+v", epinfo.ObjectMeta)
		return ErrEndpointNotFound
	}

	// find network
	ns, err := sm.FindNetwork(epinfo.Tenant, eps.Endpoint.Status.Network)
	if err != nil {
		log.Errorf("could not find the network %s for endpoint %+v. Err: %v", epinfo.Status.Network, epinfo.ObjectMeta, err)
	} else {

		// free the IPv4 address
		for _, ipv4add := range eps.Endpoint.Status.IPv4Addresses {
			ns.Lock()
			err = ns.freeIPv4Addr(ipv4add)
			ns.Unlock()
			if err != nil {
				log.Errorf("Error freeing the endpoint address. Err: %v", err)
			}

			// write the modified network state to api server
			ns.Lock()
			err = ns.Network.Write()
			ns.Unlock()
			if err != nil {
				log.Errorf("Error writing the network object. Err: %v", err)
			}
		}
		// remove it from the database
		ns.RemoveEndpoint(eps)
	}

	// delete the endpoint
	err = eps.Delete()
	if err != nil {
		log.Errorf("Error deleting the endpoint{%+v}. Err: %v", eps, err)
	}

	sm.DeleteObjectToMbus(epinfo.Endpoint.MakeKey("cluster"), eps, references(epinfo))
	log.Infof("Deleted endpoint: %+v", eps)

	return nil
}

func (sm *Statemgr) moveEndpoint(epinfo *ctkit.Endpoint, nep *workload.Endpoint, ws *WorkloadState) {
	log.Infof("Moving Endpoint. %v from DSC %v to DSC %v", nep.Name, nep.Status.NodeUUID, nep.Spec.NodeUUID)
	defer ws.epMoveWg.Done()

	lastSyncRetryCount := 0
	newEP := netproto.Endpoint{
		TypeMeta:   nep.TypeMeta,
		ObjectMeta: agentObjectMeta(nep.ObjectMeta),
		Spec: netproto.EndpointSpec{
			NetworkName: nep.Status.Network,
			MacAddress:  nep.Status.MacAddress,
			UsegVlan:    nep.Spec.MicroSegmentVlan,
			NodeUUID:    nep.Spec.NodeUUID,

			// For netproto, the homing host address is the host where the EP currently is.
			// We send the source homing host IP as part of the spec
			HomingHostAddr: nep.Status.HomingHostAddr,
			Migration:      netproto.MigrationState_START.String(),
		},
		Status: netproto.EndpointStatus{
			NodeUUID: nep.Status.NodeUUID,
		},
	}

	oldEP := netproto.Endpoint{
		TypeMeta:   nep.TypeMeta,
		ObjectMeta: agentObjectMeta(nep.ObjectMeta),
		Spec: netproto.EndpointSpec{
			NetworkName: nep.Status.Network,
			MacAddress:  nep.Status.MacAddress,
			UsegVlan:    nep.Status.MicroSegmentVlan,
			NodeUUID:    nep.Status.NodeUUID,
		},
		Status: netproto.EndpointStatus{
			NodeUUID: nep.Status.NodeUUID,
		},
	}

	log.Infof("EP object for the new host is : %v", newEP)
	log.Infof("EP object for the old host is : %v", oldEP)

	sm.mbus.AddObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))
	checkDataplaneMigration := time.NewTicker(defaultMigrationPollInterval)
	// The generationID for the new netproto object
	generationID := 1
	for {
		select {
		case <-ws.moveCtx.Done():
			log.Infof("EP [%v] move context cancelled.", newEP.Name)
			// We only get here if it's an error case - The migration my have been aborted/failed
			eps, err := EndpointStateFromObj(epinfo)
			if err != nil {
				log.Errorf("Failed to get endpoint state. Err : %v", err)
				return
			}
			eps.Lock()

			// Get WorkloadState object
			ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
			if err != nil {
				log.Errorf("Failed to get the workload state for object %v. Err : %v", eps.Endpoint.Name, err)
				return
			}

			if ws.Workload.Status.MigrationStatus == nil {
				log.Errorf("Workload [%v] has not migration status.", ws.Workload.Name)
				return
			}

			// We perform a negative check here, to avoid any race condition in setting of the Status in workload
			if ws.Workload.Status.MigrationStatus.Stage != workload.WorkloadMigrationStatus_MIGRATION_ABORT.String() {
				log.Infof("Move timed out. Sending delete EP to old DSC. Traffic flows might be affected. [%v]", oldEP.Spec.NodeUUID)

				newEP.Spec.Migration = netproto.MigrationState_DONE.String()

				sm.mbus.AddObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))

				// FIXME replace the sleep with polling from the dataplane
				log.Infof("waiting for dataplane to finish last sync")
				time.Sleep(3 * time.Second)

				oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
				oldEP.Spec.HomingHostAddr = ""
				eps.moveEP = &oldEP
				sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
				sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
				//sm.mbus.UpdateObjectWithReferences(epinfo.MakeKey("cluster"), &oldEP, references(epinfo))
				//sm.mbus.DeleteObjectWithReferences(epinfo.MakeKey("cluster"), &oldEP, references(epinfo))

				newEP.Spec.Migration = netproto.MigrationState_NONE.String()
				newEP.Spec.HomingHostAddr = ""
				eps.moveEP = &newEP
				sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
				//sm.mbus.AddObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))

				eps.Endpoint.Status.NodeUUID = eps.Endpoint.Spec.NodeUUID
				eps.Endpoint.Status.MicroSegmentVlan = eps.Endpoint.Spec.MicroSegmentVlan
				eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_DONE.String()
				eps.Endpoint.Status.HomingHostName = ws.Workload.Status.HostName
				eps.Endpoint.Status.HomingHostAddr = eps.Endpoint.Spec.HomingHostAddr

				eps.stateMgr.ctrler.Endpoint().SyncUpdate(&eps.Endpoint.Endpoint)
				log.Infof("EP [%v] migrated to [%v].", eps.Endpoint.Name, eps.Endpoint.Status.NodeUUID)

				oldDSC, err := sm.FindDistributedServiceCard(eps.Endpoint.Tenant, oldEP.Spec.NodeUUID)
				if err == nil {
					eps.stopDSCTracking(oldDSC.DistributedServiceCard.DistributedServiceCard.Name)
				}
				eps.moveEP = nil
				return
			}

			log.Errorf("Error while moving Endpoint [%v]. Expected [done] state, got [%v].", eps.Endpoint.Name, eps.Endpoint.Status.Migration.Status)
			newEP.Spec.Migration = netproto.MigrationState_ABORT.String()
			newEP.Spec.HomingHostAddr = ""

			// There can only be one object reference in the memdb for a particular object name, kind
			// To use nimbus channel, update the existingObj in memdb to point to the desired object
			eps.moveEP = &newEP
			sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
			//sm.mbus.UpdateObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))

			// Send delete to the appropriate host
			//sm.mbus.DeleteObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))
			sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

			// Clean up all the migration state in the netproto object
			oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
			oldEP.Spec.HomingHostAddr = ""

			eps.Endpoint.Spec.NodeUUID = eps.Endpoint.Status.NodeUUID
			eps.Endpoint.Spec.MicroSegmentVlan = eps.Endpoint.Status.MicroSegmentVlan
			eps.Endpoint.Spec.HomingHostAddr = eps.Endpoint.Status.HomingHostAddr

			// Update the in-memory reference for a particular object name, kind
			eps.moveEP = &oldEP
			eps.Endpoint.Spec.NodeUUID = oldEP.Spec.NodeUUID
			//sm.mbus.AddObjectWithReferences(epinfo.MakeKey("cluster"), &oldEP, references(epinfo))
			sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

			eps.stateMgr.ctrler.Endpoint().SyncUpdate(&eps.Endpoint.Endpoint)
			eps.moveEP = nil
			eps.Unlock()
			return
		case <-checkDataplaneMigration.C:
			eps, _ := EndpointStateFromObj(epinfo)
			if eps != nil {
				eps.Lock()
				if eps.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
					log.Infof("Waiting for last sync to finish for EP [%v].", eps.Endpoint.Name)
					lastSyncRetryCount = lastSyncRetryCount + 1

					// Query the new host netagent if the move was successful in the dataplane
					// Update the generation ID so that netagent does not ignore the update call
					generationID = generationID + 1
					newEP.ObjectMeta.GenerationID = fmt.Sprintf("%d", generationID)

					newEP.Spec.Migration = netproto.MigrationState_NONE.String()
					eps.moveEP = &newEP
					sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
					//sm.mbus.UpdateObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))
					// TODO : use the DONE instead of just using the lastsync timer
					//if eps.migrationState == DONE || lastSyncRetryCount > defaultLastSyncRetries

					// If the EP has already moved or has finished last sync retries
					if lastSyncRetryCount > defaultLastSyncRetries || eps.Endpoint.Status.NodeUUID == eps.Endpoint.Spec.NodeUUID {
						log.Infof("Move was successful. Sending delete EP to old DSC. [%v]", oldEP.Spec.NodeUUID)
						oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
						oldEP.Spec.HomingHostAddr = ""
						//sm.mbus.UpdateObjectWithReferences(epinfo.MakeKey("cluster"), &oldEP, references(epinfo))
						eps.moveEP = &oldEP
						sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
						//sm.mbus.DeleteObjectWithReferences(epinfo.MakeKey("cluster"), &oldEP, references(epinfo))
						sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

						newEP.Spec.Migration = netproto.MigrationState_NONE.String()
						newEP.Spec.HomingHostAddr = ""
						eps.moveEP = &newEP
						eps.Endpoint.Spec.NodeUUID = newEP.Spec.NodeUUID
						sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

						ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
						if err != nil {
							log.Errorf("Could not find workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
							eps.moveEP = nil
							eps.Unlock()
							return
						}

						eps.Endpoint.Status.NodeUUID = eps.Endpoint.Spec.NodeUUID
						eps.Endpoint.Status.MicroSegmentVlan = eps.Endpoint.Spec.MicroSegmentVlan
						eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_DONE.String()
						eps.Endpoint.Status.HomingHostName = ws.Workload.Status.HostName
						eps.Endpoint.Status.HomingHostAddr = eps.Endpoint.Spec.HomingHostAddr

						eps.stateMgr.ctrler.Endpoint().SyncUpdate(&eps.Endpoint.Endpoint)
						log.Infof("EP [%v] migrated to [%v] successfully. EP [%v]", eps.Endpoint.Name, eps.Endpoint.Status.NodeUUID, eps)

						oldDSC, err := sm.FindDistributedServiceCard(eps.Endpoint.Tenant, oldEP.Spec.NodeUUID)
						if err == nil {
							eps.stopDSCTracking(oldDSC.DistributedServiceCard.DistributedServiceCard.Name)
						}

						ws.incrMigrationSuccess()
						eps.migrationState = NONE
						eps.moveEP = nil
						eps.Unlock()
						return
					}

					// TODO : Move the last sync retry check here instead of for success once the feedback from netagent is built
					// We would want to declare dataplane success only if the dataplane declares success, it should be failed otherwise
					if eps.migrationState == FAILED {
						log.Infof("Migration of of EP [%v] failed in Dataplane. Traffic to the EP will be affected.", eps.Endpoint.Name)
						// TODO : Generate an event here

						// Set the migration of workload to failed
						ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
						if err != nil {
							log.Errorf("Could not find workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
							eps.moveEP = nil
							eps.Unlock()
							return
						}

						// Since the EP is in a bad state at this point, we do not update all spec and status fields
						// This is to ensure we have enough information for debugging and communicate what went wrong
						// during migration
						eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_FAILED.String()
						eps.stateMgr.ctrler.Endpoint().SyncUpdate(&eps.Endpoint.Endpoint)

						ws.incrMigrationFailure()
						eps.migrationState = NONE
						eps.moveEP = nil
						eps.Unlock()
						return
					}
				}
				// Unlock if the EP sync is still not complete or failed
				eps.moveEP = nil
				eps.Unlock()
			}
		}
	}
}
