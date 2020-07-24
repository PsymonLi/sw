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
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (
	// The number of retries before dataplane last sync is declared to be successful
	defaultLastSyncRetries       = 15
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
	Endpoint        *ctkit.Endpoint                `json:"-"` // embedding endpoint object
	groups          map[string]*SecurityGroupState // list of security groups
	stateMgr        *SmEndpoint
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
			log.Errorf("Incorrect type %#v, %#v", obj, epobj.HandlerCtx)
			return nil, ErrIncorrectObjectType
		}
	default:
		log.Errorf("Incorrect type %#v", obj)
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
	if eps.Spec.NodeUUID == "" {
		nep.Spec.NodeUUID = eps.Status.NodeUUID
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

	return eps.stateMgr.sm.UpdateObjectToMbus(eps.Endpoint.MakeKey("cluster"), eps, references(eps.Endpoint))
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

	return eps.stateMgr.sm.UpdateObjectToMbus(eps.Endpoint.MakeKey("cluster"), eps, references(eps.Endpoint))
}

// attachSecurityGroups attach all security groups
func (eps *EndpointState) attachSecurityGroups() error {
	// get a list of security groups
	sgs, err := eps.stateMgr.sm.ListSecurityGroups()
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
func NewEndpointState(epinfo *ctkit.Endpoint) (*EndpointState, error) {
	// build the endpoint state
	eps := EndpointState{
		Endpoint: epinfo,
		groups:   make(map[string]*SecurityGroupState),
		stateMgr: smgrEndpoint,
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
	go func() {
		err := eps.Write()
		if err != nil {
			log.Errorf("Error writing the endpoint state to api server. Err: %v", err)
		}
	}()

	eps.smObjectTracker.init(&eps)
	//No need to send update notification as we are not updating status yet
	eps.smObjectTracker.skipUpdateNotification()
	return &eps, nil
}

// GetKey returns the key of NetworkSecurityPolicy
func (eps *EndpointState) GetKey() string {
	return eps.Endpoint.GetKey()
}

// GetKind returns the kind
func (eps *EndpointState) GetKind() string {
	return eps.Endpoint.GetKind()
}

//GetGenerationID get genration ID
func (eps *EndpointState) GetGenerationID() string {
	return eps.Endpoint.GenerationID
}

//TrackedDSCs tracked DSCs
func (eps *EndpointState) TrackedDSCs() []string {

	dscs, _ := eps.stateMgr.sm.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if eps.stateMgr.sm.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) &&
			dsc.DistributedServiceCard.Status.PrimaryMAC == eps.Endpoint.Spec.NodeUUID {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}

	return trackedDSCs
}

//GetEndpointWatchOptions gets options
func (sma *SmEndpoint) GetEndpointWatchOptions() *api.ListWatchOptions {
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
func (sma *SmEndpoint) OnEndpointReconnect() {
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
			log.Errorf("Error getting endpoint %v", err)
			continue
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
func (sma *SmEndpoint) OnEndpointCreate(epinfo *ctkit.Endpoint) error {
	sm := sma.sm
	log.Infof("Creating endpoint: %#v", epinfo)

	// find network
	ns, err := sm.FindNetwork(epinfo.Tenant, epinfo.Status.Network)
	if err != nil {
		//Retry again, Create network may be lagging.
		//return kvstore.NewKeyNotFoundError(epinfo.Status.Network, 0)
		log.Errorf("could not find the network %s for endpoint %+v. Err: %v", epinfo.Status.Network, epinfo.ObjectMeta, err)
		return fmt.Errorf("could not find the network %s for endpoint %+v. Err: %v", epinfo.Status.Network, epinfo.ObjectMeta, err)
	}
	// create a new endpoint instance
	eps, err := NewEndpointState(epinfo)
	if err != nil {
		log.Errorf("Error creating endpoint state from spec{%+v}, Err: %v", epinfo, err)
		return err
	}

	existing, ok := sma.FindEndpointByMacAddr(eps)
	if ok && existing.Endpoint.Status.WorkloadName != eps.Endpoint.Status.WorkloadName {
		sma.delDuplicateMacEndpoints(eps)
		log.Errorf("Duplicate Mac Address %v for endpoint %+v, workload Added for reconcile", existing.Endpoint.Status.WorkloadName, epinfo.ObjectMeta)
		return fmt.Errorf("Duplicate Mac Address %v for endpoint %+v", existing.Endpoint.Status.WorkloadName, epinfo.ObjectMeta)
	}

	// save the endpoint in the database
	ns.AddEndpoint(eps)

	if eps.Endpoint.Status.Migration != nil && eps.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String() {
		return sm.handleMigration(epinfo, &eps.Endpoint.Endpoint)
	}
	sma.addEndpoint(eps)

	err = sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	if err != nil {
		log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
		return nil
	}
	return nil
}

// OnEndpointUpdate handles update event
func (sma *SmEndpoint) OnEndpointUpdate(epinfo *ctkit.Endpoint, nep *workload.Endpoint) error {
	sm := sma.sm
	log.Infof("Got EP update. %v", nep)
	epinfo.ObjectMeta = nep.ObjectMeta

	eps, err := sm.FindEndpoint(epinfo.Tenant, epinfo.Name)
	if err != nil {
		log.Errorf("Failed to find EP. Err : %v", err)
		return err
	}

	ws, err := sm.FindWorkload(nep.Tenant, getWorkloadNameFromEPName(nep.Name))
	if err != nil {
		log.Errorf("Failed to get workload for EP [%v]. Err : %v", nep.Name, err)
	}

	if (ws != nil && ws.isMigrating()) || (nep.Status.Migration != nil && nep.Status.Migration.Status == workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String()) {
		return sm.handleMigration(epinfo, nep)
	}

	if epinfo.Endpoint.Status.Network != nep.Status.Network || epinfo.Endpoint.Status.MicroSegmentVlan != nep.Status.MicroSegmentVlan {
		log.Infof("Network updated for EP %v from %v to %v. Adding object to Node %v", nep.Name, epinfo.Endpoint.Status.Network, nep.Status.Network, nep.Spec.NodeUUID)
		sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
		epinfo.Endpoint.Spec = nep.Spec
		epinfo.Endpoint.Status = nep.Status
		sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	} else {
		log.Infof("Updating EP. Sending the object to DSC %v. MoveEP %v", eps.Endpoint.Spec.NodeUUID, eps.moveEP)
		sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
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

	ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
	if err != nil {
		log.Errorf("Failed to get workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
		return err
	}

	if nep.Status.Migration == nil {
		log.Errorf("Migration status is nil")
		return fmt.Errorf("handle migration received with empty status")
	}

	if nep.Status.Migration.Status == workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String() {
		// We should get here primarily when we create the EP for the first time.
		log.Infof("EP %v is being moved from non-pensando host to DSC %v", epinfo.Endpoint.Name, epinfo.Endpoint.Spec.NodeUUID)
		eps.Endpoint.Status.NodeUUID = eps.Endpoint.Spec.NodeUUID
		eps.Endpoint.Status.MicroSegmentVlan = eps.Endpoint.Spec.MicroSegmentVlan
		eps.Endpoint.Status.Migration = &workload.EndpointMigrationStatus{Status: workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String()}
		eps.Endpoint.Status.HomingHostName = ws.Workload.Status.HostName
		eps.Endpoint.Status.HomingHostAddr = eps.Endpoint.Spec.HomingHostAddr

		err := sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
		if err != nil {
			log.Errorf("Error storing the sg policy in memdb. Err: %v", err)
			return nil
		}

		return eps.Write()
	}

	if nep.Status.Migration.Status == workload.EndpointMigrationStatus_ABORTED.String() {
		log.Infof("Handle migration called for EP %v with aborted migration.", epinfo.Endpoint.Name)
		return nil
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
		if eps.moveEP == nil {
			log.Errorf("moveEP is nil when the migration is in progress.")
			return nil
		}

		eps.moveEP.Spec.Migration = netproto.MigrationState_DONE.String()
		eps.stateMgr.sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
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
func (sma *SmEndpoint) OnEndpointDelete(epinfo *ctkit.Endpoint) error {
	sm := sma.sm
	log.Infof("Deleting Endpoint: %#v", epinfo)
	sm.networkKindLock.Lock()
	defer sm.networkKindLock.Unlock()

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
			err = ns.freeIPv4Addr(ipv4add)
			if err != nil {
				log.Errorf("Error freeing the endpoint address. Err: %v", err)
			}

			// write the modified network state to api server
			err = ns.Network.Write()
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
	sma.deleteEndpoint(eps)

	sm.DeleteObjectToMbus(epinfo.Endpoint.MakeKey("cluster"), eps, references(epinfo))
	log.Infof("Deleted endpoint: %+v", eps)

	return nil
}

func (sm *Statemgr) moveEndpoint(epinfo *ctkit.Endpoint, nep *workload.Endpoint, ws *WorkloadState) {
	log.Infof("Moving Endpoint %v from DSC %v to DSC %v", nep.Name, nep.Status.NodeUUID, nep.Spec.NodeUUID)
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

	// The only real purpose of the oldEP will be to send the delete to the old DSC
	// once the migration is successful
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

	log.Infof("[%v] EP object for the new host is : %v", nep.Name, newEP)
	log.Infof("[%v] EP object for the old host is : %v", nep.Name, oldEP)

	handleDone := func() {
		log.Infof("EP [%v] move context cancelled.", newEP.Name)
		// We only get here if it's an error case - The migration my have been aborted/failed
		eps, err := EndpointStateFromObj(epinfo)
		if err != nil {
			log.Errorf("Failed to get endpoint state. Err : %v", err)
			return
		}
		eps.Endpoint.Lock()
		// Get WorkloadState object
		ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
		if err != nil {
			log.Errorf("Failed to get the workload state for object %v. Err : %v", eps.Endpoint.Name, err)
			return
		}
		eps.Endpoint.Unlock()

		if ws.Workload.Status.MigrationStatus == nil {
			log.Errorf("Workload [%v] has no migration status.", ws.Workload.Name)
			return
		}

		// We perform a negative check here, to avoid any race condition in setting of the Status in workload
		// Treat force stop migration caused by DSC profile update while migration is in flight as ABORT
		if !ws.forceStopMigration && ws.Workload.Status.MigrationStatus.Stage != workload.WorkloadMigrationStatus_MIGRATION_ABORT.String() {
			eps.Endpoint.Lock()
			log.Infof("Move timed out for EP %v. Sending delete EP to old DSC. Traffic flows might be affected. [%v]", eps.Endpoint.Name, oldEP.Spec.NodeUUID)

			// Update the boldDB EP object in both DSCs
			newEP.Spec.Migration = netproto.MigrationState_DONE.String()
			eps.moveEP = &newEP
			sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

			// FIXME replace the sleep with polling from the dataplane
			log.Infof("waiting for dataplane to finish last sync")
			time.Sleep(3 * time.Second)

			// Send Delete to the old DSC
			oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
			oldEP.Spec.HomingHostAddr = ""
			eps.moveEP = &oldEP
			sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

			// Update the memdb copy of EP
			newEP.Status.NodeUUID = newEP.Spec.NodeUUID
			newEP.Spec.Migration = netproto.MigrationState_NONE.String()
			newEP.Status.NodeUUID = newEP.Spec.NodeUUID
			newEP.Spec.HomingHostAddr = ""
			eps.moveEP = &newEP
			sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

			// Fix the etcd copy of Endpoint object
			eps.Endpoint.Status.NodeUUID = eps.Endpoint.Spec.NodeUUID
			eps.Endpoint.Status.MicroSegmentVlan = eps.Endpoint.Spec.MicroSegmentVlan
			eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_DONE.String()
			eps.Endpoint.Status.HomingHostName = ws.Workload.Status.HostName
			eps.Endpoint.Status.HomingHostAddr = eps.Endpoint.Spec.HomingHostAddr

			oldDSC, err := sm.FindDistributedServiceCard(eps.Endpoint.Tenant, oldEP.Spec.NodeUUID)
			if err == nil {
				eps.stopDSCTracking(oldDSC.DistributedServiceCard.DistributedServiceCard.Name)
			}

			eps.moveEP = nil
			if err := eps.Write(); err != nil {
				log.Errorf("Failed to write EP %v to API Server. Err : %v", eps.Endpoint.Name, err)
			}

			log.Infof("EP [%v] migrated to [%v] after timeout.", eps.Endpoint.Name, eps.Endpoint.Status.NodeUUID)
			eps.Endpoint.Unlock()
			return
		}

		log.Errorf("Error while moving Endpoint [%v]. Expected [done] state, got [%v]. Aborting migration.", eps.Endpoint.Name, eps.Endpoint.Status.Migration.Status)
		newEP.Spec.Migration = netproto.MigrationState_ABORT.String()
		newEP.Spec.HomingHostAddr = ""

		eps.Endpoint.Lock()
		// There can only be one object reference in the memdb for a particular object name, kind
		// To use nimbus channel, update the existingObj in memdb to point to the desired object
		eps.moveEP = &newEP

		// Update the Endpoint object in the boltDB of both DSCs
		sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

		// Send DELETE operation to both DSCs
		sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

		// Clean up all the migration state in the netproto object
		oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
		oldEP.Spec.HomingHostAddr = ""

		eps.Endpoint.Spec.NodeUUID = eps.Endpoint.Status.NodeUUID
		eps.Endpoint.Spec.MicroSegmentVlan = eps.Endpoint.Status.MicroSegmentVlan
		eps.Endpoint.Spec.HomingHostAddr = eps.Endpoint.Status.HomingHostAddr
		eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_ABORTED.String()

		// Update the memdb reference for EP to point to the OLD DSC
		eps.moveEP = &oldEP
		sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

		eps.moveEP = nil
		eps.Endpoint.Unlock()
		// Use Update API here, because Write only updates the Status part
		if err := eps.stateMgr.sm.ctrler.Endpoint().Update(&eps.Endpoint.Endpoint); err != nil {
			log.Errorf("Failed to write EP %v to API Server. Err : %v", eps.Endpoint.Name, err)
		}
		log.Infof("Migration aborted for %v", eps.Endpoint.Name)
		return
	}

	if ws.moveCtx == nil {
		handleDone()
		return
	}
	eps, _ := EndpointStateFromObj(epinfo)
	if eps == nil {
		log.Errorf("Failed to get EPS object for EP %v. Cannot start migration.", epinfo)
		return
	}

	eps.Endpoint.Lock()
	eps.moveEP = &newEP
	eps.Endpoint.Unlock()

	sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
	checkDataplaneMigration := time.NewTicker(defaultMigrationPollInterval)
	for {
		select {
		case <-ws.moveCtx.Done():
			handleDone()
			return
		case <-checkDataplaneMigration.C:
			eps, _ := EndpointStateFromObj(epinfo)
			if eps != nil {
				eps.Endpoint.Lock()
				eps.moveEP = &newEP
				if eps.Endpoint.Status.Migration == nil {
					log.Errorf("Unexpected EP [%v] migration status is nil. Exiting move.", eps.Endpoint.Name)
					eps.moveEP = nil
					eps.Endpoint.Unlock()
					return
				}

				if eps.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
					log.Infof("Waiting for last sync to finish for EP [%v].", eps.Endpoint.Name)
					lastSyncRetryCount = lastSyncRetryCount + 1
					//sm.mbus.UpdateObjectWithReferences(epinfo.MakeKey("cluster"), &newEP, references(epinfo))
					// TODO : use the DONE instead of just using the lastsync timer
					//if eps.migrationState == DONE || lastSyncRetryCount > defaultLastSyncRetries

					// If the EP has already moved or has finished last sync retries
					if lastSyncRetryCount > defaultLastSyncRetries || eps.Endpoint.Status.NodeUUID == eps.Endpoint.Spec.NodeUUID {
						log.Infof("Move was successful for %v. Sending delete EP to old DSC. [%v]", eps.Endpoint.Name, oldEP.Spec.NodeUUID)
						oldEP.Spec.Migration = netproto.MigrationState_NONE.String()
						oldEP.Spec.HomingHostAddr = ""
						eps.moveEP = &oldEP
						sm.UpdateObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))
						sm.DeleteObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

						newEP.Spec.Migration = netproto.MigrationState_NONE.String()
						newEP.Spec.HomingHostAddr = ""
						// Ensure the Spec and Status NodeUUID are same when finally setting the memdb object
						newEP.Status.NodeUUID = newEP.Spec.NodeUUID
						eps.moveEP = &newEP
						sm.AddObjectToMbus(epinfo.MakeKey("cluster"), eps, references(epinfo))

						ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
						if err != nil {
							log.Errorf("Could not find workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
							eps.moveEP = nil
							eps.Endpoint.Unlock()
							return
						}

						eps.Endpoint.Status.NodeUUID = eps.Endpoint.Spec.NodeUUID
						eps.Endpoint.Status.MicroSegmentVlan = eps.Endpoint.Spec.MicroSegmentVlan
						eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_DONE.String()
						eps.Endpoint.Status.HomingHostName = ws.Workload.Status.HostName
						eps.Endpoint.Status.HomingHostAddr = eps.Endpoint.Spec.HomingHostAddr

						oldDSC, err := sm.FindDistributedServiceCard(eps.Endpoint.Tenant, oldEP.Spec.NodeUUID)
						if err == nil {
							eps.stopDSCTracking(oldDSC.DistributedServiceCard.DistributedServiceCard.Name)
						}

						ws.incrMigrationSuccess()
						eps.migrationState = NONE
						eps.moveEP = nil
						if err := eps.Write(); err != nil {
							log.Errorf("Failed to write EP %v to API Server. Err : %v", eps.Endpoint.Name, err)
						}

						log.Infof("EP [%v] migrated to [%v] successfully.", eps.Endpoint.Name, eps.Endpoint.Status.NodeUUID)
						eps.Endpoint.Unlock()
						return
					}

					// TODO : Move the last sync retry check here instead of for success once the feedback from netagent is built
					// We would want to declare dataplane success only if the dataplane declares success, it should be failed otherwise
					if eps.migrationState == FAILED {
						log.Infof("Migration of EP [%v] failed in Dataplane. Traffic to the EP will be affected.", eps.Endpoint.Name)
						// TODO : Generate an event here

						// Set the migration of workload to failed
						ws, err := sm.FindWorkload(eps.Endpoint.Tenant, getWorkloadNameFromEPName(eps.Endpoint.Name))
						if err != nil {
							log.Errorf("Could not find workload for EP [%v]. Err : %v", eps.Endpoint.Name, err)
							eps.moveEP = nil
							eps.Endpoint.Unlock()
							return
						}

						eps.migrationState = NONE
						eps.moveEP = nil
						// Since the EP is in a bad state at this point, we do not update all spec and status fields
						// This is to ensure we have enough information for debugging and communicate what went wrong
						// during migration
						eps.Endpoint.Status.Migration.Status = workload.EndpointMigrationStatus_FAILED.String()
						if err := eps.Write(); err != nil {
							log.Errorf("Failed to write EP %v to API Server. Err : %v", eps.Endpoint.Name, err)
						}

						ws.incrMigrationFailure()
						log.Infof("EP [%v] migration to [%v] failed.", eps.Endpoint.Name, eps.Endpoint.Status.NodeUUID)
						eps.Endpoint.Unlock()
						return
					}
				}
				// Unlock if the EP sync is still not complete or failed
				eps.Endpoint.Unlock()
			}
		}
	}
}

var smgrEndpoint *SmEndpoint

// SmEndpoint is statemanager struct for Fwprofile
type SmEndpoint struct {
	featureMgrBase
	sm        *Statemgr
	macAddrDB *sync.Map // mac-tenant to workload
}

func initSmEndpoint() {
	mgr := MustGetStatemgr()
	smgrEndpoint = &SmEndpoint{
		sm: mgr,
	}
	smgrEndpoint.macAddrDB = new(sync.Map)
	mgr.Register("statemgrendpoint", smgrEndpoint)
}

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmEndpoint) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }

	//	initSmEndpoint()
	log.Infof("Got CompleteRegistration for SmEndpoint")
	sma.sm.SetEndpointReactor(smgrEndpoint)
}

func (eps *EndpointState) getMacKey() string {
	key := fmt.Sprintf("%v-%v", eps.Endpoint.Tenant, eps.Endpoint.Status.MacAddress)
	return key
}

// AddEndpoint add endpoint to macdb
func (sma *SmEndpoint) addEndpoint(eps *EndpointState) error {
	sma.macAddrDB.Store(eps.getMacKey(), eps)
	return nil
}

// deleteEndpoint add endpoint to macdb
func (sma *SmEndpoint) deleteEndpoint(eps *EndpointState) error {
	if i, ok := sma.macAddrDB.Load(eps.getMacKey()); ok {
		eeps := i.(*EndpointState)
		if eeps.Endpoint.Status.WorkloadName == eps.Endpoint.Status.WorkloadName {
			sma.macAddrDB.Delete(eps.getMacKey())
		}
	}
	return nil
}

// FindEndpointByMacAddr find endpoint to macdb
func (sma *SmEndpoint) FindEndpointByMacAddr(eps *EndpointState) (*EndpointState, bool) {
	if eeps, ok := smgrEndpoint.macAddrDB.Load(eps.getMacKey()); ok {
		return eeps.(*EndpointState), true
	}
	return nil, false
}

// FindEndpointByMacAddr find endpoint to macdb
func (sm *Statemgr) FindEndpointByMacAddr(tenant, mac string) (*EndpointState, bool) {
	key := fmt.Sprintf("%v-%v", tenant, mac)
	if eps, ok := smgrEndpoint.macAddrDB.Load(key); ok {
		return eps.(*EndpointState), true
	}
	return nil, false
}

func init() {
	initSmEndpoint()
}

//ProcessDSCCreate create
func (sma *SmEndpoint) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

	if sma.sm.isDscEnforcednMode(dsc) {
		sma.dscTracking(dsc, true)
	}
}

func (sma *SmEndpoint) dscTracking(dsc *cluster.DistributedServiceCard, start bool) {

	eps, err := sma.sm.ListEndpoints()
	if err != nil {
		log.Errorf("Error listing profiles %v", err)
		return
	}

	for _, aps := range eps {
		if start && dsc.Status.PrimaryMAC == aps.Endpoint.Spec.NodeUUID {
			aps.smObjectTracker.startDSCTracking(dsc.Name)
		} else {
			aps.smObjectTracker.stopDSCTracking(dsc.Name)
		}
	}
}

//ProcessDSCUpdate update
func (sma *SmEndpoint) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if sma.sm.dscDecommissioned(ndsc) {
		sma.dscTracking(ndsc, false)
		return
	}

	//Run only if profile changes.
	if dsc.Spec.DSCProfile != ndsc.Spec.DSCProfile || sma.sm.dscRecommissioned(dsc, ndsc) {
		if sma.sm.isDscEnforcednMode(ndsc) {
			sma.dscTracking(ndsc, true)
		} else {
			sma.dscTracking(ndsc, false)
		}
	}
}

//ProcessDSCDelete delete
func (sma *SmEndpoint) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.dscTracking(dsc, false)
}

//delDuplicateMacEndpoints workload to cleanup errored out endpoint
func (sma *SmEndpoint) delDuplicateMacEndpoints(eps *EndpointState) error {
	workloadName := getWorkloadNameFromEPName(eps.Endpoint.Name)
	ws, err := sma.sm.FindWorkload(eps.Endpoint.Tenant, workloadName)
	if err != nil {
		log.Infof("Error finding workload :%v err : %v", workloadName, err)
		return err
	}
	return ws.doReconcile()
}
