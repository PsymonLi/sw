// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	"github.com/pensando/sw/venice/utils/strconv"
)

const (
	defaultMigrationTimeout = "3m" // 3 minutes
	// DuplicateMacErr error string for duplicate mac
	DuplicateMacErr = "Propagation Failed. Duplicate MAC address"
)

// WorkloadState is a wrapper for host object
type WorkloadState struct {
	Workload  *ctkit.Workload `json:"-"` // workload object
	stateMgr  *Statemgr       // pointer to state manager
	endpoints sync.Map        // list of endpoints

	// migration related state
	moveCtx    context.Context
	moveCancel context.CancelFunc
	moveWg     sync.WaitGroup
	epMoveWg   sync.WaitGroup

	migrationState        sync.Mutex
	migrationEPCount      int
	migrationSuccessCount int
	migrationFailureCount int
	forceStopMigration    bool
}

// WorkloadStateFromObj conerts from ctkit object to workload state
func WorkloadStateFromObj(obj runtime.Object) (*WorkloadState, error) {
	switch obj.(type) {
	case *ctkit.Workload:
		wobj := obj.(*ctkit.Workload)
		switch wobj.HandlerCtx.(type) {
		case *WorkloadState:
			nsobj := wobj.HandlerCtx.(*WorkloadState)
			return nsobj, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

//GetWorkloadWatchOptions gets options
func (sm *Statemgr) GetWorkloadWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec", "Status.MigrationStatus.Stage"}
	return &opts
}

// NewWorkloadState creates new workload state object
func NewWorkloadState(wrk *ctkit.Workload, stateMgr *Statemgr) (*WorkloadState, error) {
	w := &WorkloadState{
		Workload: wrk,
		stateMgr: stateMgr,
	}
	wrk.HandlerCtx = w

	return w, nil
}

// networkName returns network name for the external vlan
func (sm *Statemgr) networkName(extVlan uint32) string {
	return "Network-Vlan-" + fmt.Sprintf("%d", extVlan)
}

// OnWorkloadCreate handle workload creation
func (sm *Statemgr) OnWorkloadCreate(w *ctkit.Workload) error {
	log.Infof("Creating workload: %+v", w)

	// create new workload object
	ws, err := NewWorkloadState(w, sm)
	if err != nil {
		log.Errorf("Error creating workload %+v. Err: %v", w, err)
		return err
	}

	// find the host for the workload
	host, err := ws.stateMgr.ctrler.Host().Find(&api.ObjectMeta{Name: w.Spec.HostName})
	if err != nil {
		// retry again if we cant find the host. In cases where host and workloads are created back to back,
		// there might be slight delay before host is available
		time.Sleep(20 * time.Millisecond)
		host, err = ws.stateMgr.ctrler.Host().Find(&api.ObjectMeta{Name: w.Spec.HostName})
		if err != nil {
			log.Errorf("Error finding the host %s for workload %v. Err: %v", w.Spec.HostName, w.Name, err)
			return kvstore.NewKeyNotFoundError(w.Spec.HostName, 0)
		}
	}

	// lock the host to make sure only one workload is operating on the host
	host.Lock()
	defer host.Unlock()

	hsts, err := HostStateFromObj(host)
	if err != nil {
		log.Errorf("Error finding the host state %s for workload %v. Err: %v", w.Spec.HostName, w.Name, err)
		return err
	}

	hsts.addWorkload(w)
	err = ws.createEndpoints()
	if err != nil {
		log.Warnf("Gracefully return success for provisioning")
		return nil
	}

	snic := hsts.getDSCs()

	// Upon restart, if a migration is in progress and not timed out
	if ws.isMigrating() && len(snic) > 0 {
		err = sm.reconcileWorkload(w, hsts, snic[0])
		// For Workloads moving from non_pensando to pensando host, ensure the appropriate state is slected
		if err != nil {
			log.Errorf("Failed to reconcile workload [%v]. Err : %v", ws.Workload.Name, err)
			return err
		}

		log.Infof("Handling migration for reconciled workload [%v]", ws.Workload.Name)
		return ws.handleMigration()
	}

	return err
}

// OnWorkloadUpdate handles workload update event
func (sm *Statemgr) OnWorkloadUpdate(w *ctkit.Workload, nwrk *workload.Workload) error {
	w.ObjectMeta = nwrk.ObjectMeta

	log.Infof("Updating workload: %+v", nwrk)

	recreate := false
	// check if host parameter has changed or migration has been initiated
	if nwrk.Spec.HostName != w.Spec.HostName {
		log.Infof("Workload %v host changed from %v to %v.", nwrk.Name, w.Spec.HostName, nwrk.Spec.HostName)
		recreate = true
	}

	if nwrk.Status.MigrationStatus != nil && (nwrk.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_STARTED.String() || nwrk.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_NONE.String()) {
		log.Infof("Hot migration of workload [%v] stage [%v]", nwrk.Name, nwrk.Status.MigrationStatus.Stage)
		ws, err := sm.FindWorkload(w.Tenant, w.Name)
		if err != nil {
			log.Errorf("Could not find workload. Err : %v", err)
			return nil
		}
		ws.Workload.Spec = nwrk.Spec
		ws.Workload.Status = nwrk.Status

		return ws.handleMigration()
	}

	ws, err := sm.FindWorkload(w.Tenant, w.Name)
	if err != nil {
		return err
	}

	// if we dont need to recreate endpoints, we are done
	if !ws.isInterfaceChanged(nwrk) && !recreate {
		return nil
	}

	// delete old endpoints
	err = ws.deleteEndpoints()
	if err != nil {
		log.Errorf("Error deleting old endpoint. Err: %v", err)
		return err
	}

	// update the spec
	w.Spec = nwrk.Spec
	ws.Workload.Spec = nwrk.Spec

	// trigger creation of new endpoints
	return ws.createEndpoints()
}

// reconcileWorkload checks if the endpoints are create for the workload and tries to create them
func (sm *Statemgr) reconcileWorkload(w *ctkit.Workload, hst *HostState, snic *DistributedServiceCardState) error {
	// find workload
	ws, err := sm.FindWorkload(w.Tenant, w.Name)
	if err != nil {
		return err
	}
	if snic == nil {
		// delete all endpoint for this workload since we dont have an associated smartnic
		err := ws.deleteEndpoints()
		if err != nil {
			log.Errorf("Error deleting all endpoints on workload %v: %v", w.Name, err)
		}
	} else {
		// make sure we have endpoint for all workload interfaces
		for ii := range w.Spec.Interfaces {
			// check if we already have the endpoint for this workload
			name, _ := strconv.ParseMacAddr(w.Spec.Interfaces[ii].MACAddress)
			epName := w.Name + "-" + name
			_, err := sm.FindEndpoint(w.Tenant, epName)
			pending, _ := sm.EndpointIsPending(w.Tenant, epName)
			if err != nil && !pending {
				err = ws.createEndpoints()
				if err != nil {
					log.Errorf("Error creating endpoints for workload. Err: %v", err)
				}
			}
		}
	}

	return nil
}

// OnWorkloadDelete handles workload deletion
func (sm *Statemgr) OnWorkloadDelete(w *ctkit.Workload) error {
	log.Infof("Deleting workload: %+v", w)

	// find the host for the workload
	host, err := sm.FindHost("", w.Spec.HostName)
	if err != nil {
		log.Errorf("Error finding the host %s for workload %v. Err: %v", w.Spec.HostName, w.Name, err)
	} else {
		host.removeWorkload(w)
	}

	ws, err := WorkloadStateFromObj(w)
	if err != nil {
		log.Errorf("Error finding workload state %s for workload %v. Err: %v", w.Spec.HostName, w.Name, err)
		return err
	}

	return ws.deleteEndpoints()
}

// OnWorkloadReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnWorkloadReconnect() {
	return
}

// createNetwork creates a network for workload's external vlan
func (ws *WorkloadState) createNetwork(netName string, extVlan uint32) error {
	ws.stateMgr.networkKindLock.Lock()
	defer ws.stateMgr.networkKindLock.Unlock()

	nwt := network.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
		ObjectMeta: api.ObjectMeta{
			Name:      netName,
			Namespace: ws.Workload.Namespace,
			Tenant:    ws.Workload.Tenant,
			Labels:    map[string]string{},
		},
		Spec: network.NetworkSpec{
			Type:        network.NetworkType_Bridged.String(),
			IPv4Subnet:  "",
			IPv4Gateway: "",
			VlanID:      extVlan,
		},
		Status: network.NetworkStatus{
			// set operState to active as internal(as opposed to user) create runs checks on status as well
			// this will be changed by npm correctly when processing the watch event
			// OperState: network.OperState_Active.String(),
		},
	}
	pnwt := &nwt
	if !isOrchHubKeyPresent(ws.Workload.Labels) {
		AddNpmSystemLabel(pnwt.Labels)
	}

	// create it in apiserver
	return ws.stateMgr.ctrler.Network().SyncCreate(&nwt)
}

func (ws *WorkloadState) isMigrating() bool {
	w := ws.Workload.Workload
	if w.Status.MigrationStatus == nil {
		return false
	}

	// If workload has not reached terminal state
	return w.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_STARTED.String() || w.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_NONE.String()
}

// createEndpoints tries to create all endpoints for a workload
func (ws *WorkloadState) createEndpoints() error {
	var ns *NetworkState
	// find the host for the workload
	host, err := ws.stateMgr.FindHost("", ws.Workload.Spec.HostName)
	if err != nil {
		log.Errorf("Error finding the host %s for workload %v. Err: %v", ws.Workload.Spec.HostName, ws.Workload.Name, err)
		return kvstore.NewKeyNotFoundError(ws.Workload.Spec.HostName, 0)
	}

	// loop over each interface of the workload
	ws.stateMgr.Lock()
	var eps []*workload.Endpoint
	createErr := false
	for ii := range ws.Workload.Spec.Interfaces {
		var netName string
		if len(ws.Workload.Spec.Interfaces[ii].Network) == 0 {
			ns, err = ws.stateMgr.FindNetworkByVlanID(ws.Workload.Tenant, ws.Workload.Spec.Interfaces[ii].ExternalVlan)
			if err != nil {

				// check if we have a network for this workload
				if len(ws.Workload.Spec.Interfaces[ii].Network) > 0 {
					netName = ws.Workload.Spec.Interfaces[ii].Network
				} else {
					netName = ws.stateMgr.networkName(ws.Workload.Spec.Interfaces[ii].ExternalVlan)
				}
				ns, err = ws.stateMgr.FindNetwork(ws.Workload.Tenant, netName)
				if err != nil {
					// Create networks since all creates are idempotent
					err = ws.createNetwork(netName, ws.Workload.Spec.Interfaces[ii].ExternalVlan)
					if err != nil {
						log.Errorf("Error creating network, ignoring  Err: %v", err)
					}
				}
			} else {
				netName = ns.Network.Network.Name
			}
		} else {
			netName = ws.Workload.Spec.Interfaces[ii].Network
		}

		ns, _ = ws.stateMgr.FindNetwork(ws.Workload.Tenant, netName)

		// check if we already have the endpoint for this workload
		name, _ := strconv.ParseMacAddr(ws.Workload.Spec.Interfaces[ii].MACAddress)
		epName := ws.Workload.Name + "-" + name
		nodeUUID := ""
		dscs := host.getDSCs()
		if len(dscs) == 0 {
			log.Errorf("Failed to find DSC on host %v", host.Host.Name)
			ws.stateMgr.Unlock()
			return fmt.Errorf("No DSC found for Host %v", host.Host.Name)
		}

		// TODO : Here, an implicit assumption of each Host having one DSC is here
		// This will have to be updated once we start supporting Workloads on
		// Multi DSC per host deployment
		nodeUUID = dscs[0].DistributedServiceCard.Name

		/* FIXME: comment out this code till CMD publishes associated NICs in host
		// check if the host has associated smart nic
		if len(host.Host.Status.AdmittedDSCs) == 0 {
			log.Errorf("Host %v does not have a smart nic", w.Spec.HostName)
			return fmt.Errorf("Host does not have associated smartnic")
		}

		for _, snicName := range host.Host.Status.AdmittedDSCs {
			snic, err := sm.FindDistributedServiceCard(host.Host.Tenant, snicName)
			if err != nil {
				log.Errorf("Error finding smart nic object for %v", snicName)
				return err
			}
			nodeUUID = snic.DistributedServiceCard.Name
		}
		*/

		// check if an endpoint with this mac address already exists in this network
		epMac := ws.Workload.Spec.Interfaces[ii].MACAddress
		if ns != nil {
			mep, err := ns.FindEndpointByMacAddr(epMac)
			if err == nil && mep.Endpoint.Name != epName {
				// we found a duplicate mac address
				log.Errorf("Error creating endpoint %s. Macaddress %s already exists in ep %s", epName, epMac, mep.Endpoint.Name)
				ws.Workload.Status.PropagationStatus.Status = DuplicateMacErr

				// write the status back
				ws.Workload.Write()
				createErr = true
				continue
			}
		}

		// create the endpoint for the interface
		epInfo := workload.Endpoint{
			TypeMeta: api.TypeMeta{Kind: "Endpoint"},
			ObjectMeta: api.ObjectMeta{
				Name:         epName,
				Tenant:       ws.Workload.Tenant,
				Namespace:    ws.Workload.Namespace,
				GenerationID: "1",
			},
			Spec: workload.EndpointSpec{
				NodeUUID:         nodeUUID,
				HomingHostAddr:   "",
				MicroSegmentVlan: ws.Workload.Spec.Interfaces[ii].MicroSegVlan,
			},
			Status: workload.EndpointStatus{
				Network:            netName,
				NodeUUID:           nodeUUID,
				WorkloadName:       ws.Workload.Name,
				WorkloadAttributes: ws.Workload.Labels,
				MacAddress:         epMac,
				HomingHostAddr:     "", // TODO: get host address
				HomingHostName:     ws.Workload.Spec.HostName,
				MicroSegmentVlan:   ws.Workload.Spec.Interfaces[ii].MicroSegVlan,
			},
		}
		if len(ws.Workload.Spec.Interfaces[ii].IpAddresses) > 0 {
			epInfo.Status.IPv4Addresses = ws.Workload.Spec.Interfaces[ii].IpAddresses
		}

		if ws.Workload.Status.MigrationStatus != nil && ws.Workload.Status.MigrationStatus.Stage == workload.WorkloadMigrationStatus_MIGRATION_FROM_NON_PEN_HOST.String() {
			epInfo.Status.Migration.Status = workload.EndpointMigrationStatus_FROM_NON_PEN_HOST.String()
		}

		eps = append(eps, &epInfo)

	}

	ws.stateMgr.Unlock()
	for _, epInfo := range eps {

		//NodeUUID may not be set because DSC event may not be received yet.
		if epInfo.Spec.NodeUUID == "" {
			log.Errorf("Error creating endpoint %v, destination node not found", epInfo.Name)
			return kvstore.NewTxnFailedError()
		}
		// create new endpoint
		err = ws.stateMgr.ctrler.Endpoint().Create(epInfo)
		if err != nil {
			log.Errorf("Error creating endpoint. Err: %v", err)
			return kvstore.NewTxnFailedError()
		}
	}
	if !createErr {
		// Clear propagation error message
		ws.Workload.Status.PropagationStatus.Status = ""
		err = ws.Workload.Write()
		if err != nil {
			log.Errorf("failed to clear propagation status. Err : %v", err)
		}

	}

	return nil
}

// deleteEndpoints deletes all endpoints for a workload
func (ws *WorkloadState) deleteEndpoints() error {
	// loop over each interface of the workload
	for ii := range ws.Workload.Spec.Interfaces {
		var nw *NetworkState
		var err error
		// find the network for the interface
		if len(ws.Workload.Spec.Interfaces[ii].Network) != 0 {
			netName := ws.Workload.Spec.Interfaces[ii].Network
			nw, err = ws.stateMgr.FindNetwork(ws.Workload.Tenant, netName)
		} else {
			nw, err = ws.stateMgr.FindNetworkByVlanID(ws.Workload.Tenant, ws.Workload.Spec.Interfaces[ii].ExternalVlan)
			if err != nil {
				netName := ws.stateMgr.networkName(ws.Workload.Spec.Interfaces[ii].ExternalVlan)
				nw, err = ws.stateMgr.FindNetwork(ws.Workload.Tenant, netName)
			}
		}

		if err != nil {
			log.Errorf("Error finding the network. Err: %v", err)
		} else {
			// check if we created an endpoint for this workload interface
			name, _ := strconv.ParseMacAddr(ws.Workload.Spec.Interfaces[ii].MACAddress)
			epName := ws.Workload.Name + "-" + name
			_, ok := nw.FindEndpoint(epName)
			if !ok {
				log.Errorf("Could not find endpoint %v", epName)
			} else {

				epInfo := workload.Endpoint{
					TypeMeta: api.TypeMeta{Kind: "Endpoint"},
					ObjectMeta: api.ObjectMeta{
						Name:      epName,
						Tenant:    ws.Workload.Tenant,
						Namespace: ws.Workload.Namespace,
					},
				}

				// delete the endpoint in api server
				err = ws.stateMgr.ctrler.Endpoint().Delete(&epInfo)
				if err != nil {
					log.Errorf("Error deleting the endpoint. Err: %v", err)
					return kvstore.NewTxnFailedError()
				}
			}
		}
	}

	return nil
}

// FindWorkload finds a workload
func (sm *Statemgr) FindWorkload(tenant, name string) (*WorkloadState, error) {
	// find the object
	obj, err := sm.FindObject("Workload", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return WorkloadStateFromObj(obj)
}

//RemoveStaleEndpoints remove stale endpoints
func (sm *Statemgr) RemoveStaleEndpoints() error {

	endpoints, err := sm.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
	if err != nil {
		log.Errorf("Failed to get endpoints. Err : %v", err)
		return err
	}

	workloads, err := sm.ctrler.Workload().List(context.Background(), &api.ListWatchOptions{})
	workloadCacheEmpty := false
	if err != nil {
		log.Errorf("Failed to get workloads. Err : %v", err)
		workloadCacheEmpty = true
	}

	workloadMacPresent := func(wName, mac string) bool {
		for _, workload := range workloads {
			if workload.Name == wName {
				for ii := range workload.Workload.Spec.Interfaces {
					wmac, _ := strconv.ParseMacAddr(workload.Workload.Spec.Interfaces[ii].MACAddress)
					if wmac == mac {
						return true
					}
				}
			}
		}
		return false
	}

	for _, ep := range endpoints {
		splitString := strings.Split(ep.Name, "-")
		if len(splitString) < 2 {
			continue
		}
		workloadName := strings.Join(splitString[0:len(splitString)-1], "-")
		macAddress := splitString[len(splitString)-1]
		if workloadCacheEmpty || !workloadMacPresent(workloadName, macAddress) {
			// delete the endpoint in api server
			epInfo := workload.Endpoint{
				TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				ObjectMeta: api.ObjectMeta{
					Name:      ep.Name,
					Tenant:    ep.Tenant,
					Namespace: ep.Namespace,
				},
			}
			err := sm.ctrler.Endpoint().Delete(&epInfo)
			if err != nil {
				log.Errorf("Error deleting the endpoint. Err: %v", err)
			}
		}
	}

	return nil
}

func (ws *WorkloadState) handleMigration() error {
	log.Infof("Handling migration stage [%v] status [%v] for workload [%v] ", ws.Workload.Status.MigrationStatus.Stage, ws.Workload.Status.MigrationStatus.Status, ws.Workload.Name)
	var sourceDSCs []*DistributedServiceCardState
	var destDSCs []*DistributedServiceCardState

	switch ws.Workload.Status.MigrationStatus.Stage {
	case workload.WorkloadMigrationStatus_MIGRATION_START.String():
		if ws.moveCancel != nil {
			log.Errorf("Migration is already in progress. Unexpected start migration received.")
			return nil
		}
		ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_STARTED.String()
		fallthrough
	case workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String():
		fallthrough
	case workload.WorkloadMigrationStatus_MIGRATION_DONE.String():
		log.Infof("Orchestrator set workload [%v] to migration stage [%v].", ws.Workload.Name, ws.Workload.Status.MigrationStatus.Stage)
		destHost, err := ws.stateMgr.FindHost("", ws.Workload.Spec.HostName)
		if err != nil {
			log.Errorf("Error finding the host %s for workload %v. Err: %v", ws.Workload.Spec.HostName, ws.Workload.Name, err)
			return kvstore.NewKeyNotFoundError(ws.Workload.Spec.HostName, 0)
		}

		destDSCs = destHost.getDSCs()
		if len(destDSCs) == 0 {
			log.Errorf("Failed to get DSCs on Host %v", destHost.Host.Name)
			return fmt.Errorf("Failed to get DSCs on Host %v", destHost.Host.Name)
		}

		sourceHost, err := ws.stateMgr.FindHost("", ws.Workload.Status.HostName)
		if err != nil {
			log.Errorf("Error finding the host %s for workload %v. Err: %v", ws.Workload.Status.HostName, ws.Workload.Name, err)
			return kvstore.NewKeyNotFoundError(ws.Workload.Status.HostName, 0)
		}

		// find the smart nic by name or mac addr
		sourceDSCs = sourceHost.getDSCs()
		if len(sourceDSCs) == 0 {
			log.Errorf("Failed to get DSCs on Host %v", sourceHost.Host.Name)
			return fmt.Errorf("Failed to get DSCs on Host %v", sourceHost.Host.Name)
		}

		// if moveCancel is nil for FINAL_SYNC and DONE state, it would mean that we got here after
		// restart, all the variables have to be appropriately initialized in this case
		if ws.moveCancel == nil {
			var t time.Duration
			var startTime time.Time
			var err error

			ws.migrationState.Lock()
			ws.migrationEPCount = len(ws.Workload.Spec.Interfaces)
			ws.migrationSuccessCount = 0
			ws.migrationFailureCount = 0
			ws.migrationState.Unlock()

			t, err = time.ParseDuration(ws.Workload.Spec.MigrationTimeout)
			if err != nil {
				log.Errorf("Failed to parse migration timeout from the workload spec. Setting default timeout of %v. Err : %v", defaultMigrationTimeout, err)
				t, _ = time.ParseDuration(defaultMigrationTimeout)
			}

			if ws.Workload.Status.MigrationStatus == nil {
				log.Errorf("Migration status for the workload %v is nil. Cannot proceed with the endpoint move.", ws.Workload.Name)
				return fmt.Errorf("nil migration status for the moving workload %v", ws.Workload.Name)
			}

			if ws.Workload.Status.MigrationStatus.GetStartedAt() != nil {
				startTime, err = ws.Workload.Status.MigrationStatus.GetStartedAt().Time()
			} else {
				startTime = time.Now()
				log.Errorf("Started time was nil. Setting the start time to %v", startTime)
				ws.Workload.Status.MigrationStatus.StartedAt = &api.Timestamp{}
				ws.Workload.Status.MigrationStatus.StartedAt.SetTime(startTime)
			}
			if err != nil {
				startTime = time.Now()
				log.Errorf("failed to parse start time %v. Setting migration start time to the current time %v. Err : %v", ws.Workload.Status.MigrationStatus.StartedAt, startTime, err)
			}

			deadline := startTime.Add(t)
			ws.moveCtx, ws.moveCancel = context.WithDeadline(context.Background(), deadline)
			log.Infof("deadline set to %v", deadline)

			ws.forceStopMigration = false
			ws.moveWg.Add(1)
			go ws.trackMigration(sourceDSCs, destDSCs)

		}
		ws.Workload.Write()

	case workload.WorkloadMigrationStatus_MIGRATION_ABORT.String():
		log.Infof("Stopping migration for all EPs of %v", ws.Workload.Name)
		// Set the migration status to ABORTED and write it to API server so that EPs can refer to it
		ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_FAILED.String()

		if ws.moveCancel != nil {
			ws.moveCancel()
			ws.moveWg.Wait()
		}

		log.Infof("Migration of all EPs of %v aborted", ws.Workload.Name)
		ws.Workload.Status.MigrationStatus.CompletedAt = &api.Timestamp{}
		ws.Workload.Status.MigrationStatus.CompletedAt.SetTime(time.Now())
		ws.Workload.Write()
		return nil
	default:
		log.Errorf("unknown migration stage %v", ws.Workload.Status.MigrationStatus.Stage)
		return fmt.Errorf("unknown migration stage %v", ws.Workload.Status.MigrationStatus.Stage)
	}

	return ws.updateEndpoints(sourceDSCs, destDSCs)
}

// updateEndpoints tries to update all endpoints for a workload
func (ws *WorkloadState) updateEndpoints(sourceDSCs, destDSCs []*DistributedServiceCardState) error {
	if len(sourceDSCs) == 0 || len(destDSCs) == 0 {
		return fmt.Errorf("DSC list is empty, cannot update endpoints")
	}

	// loop over each interface of the workload
	ws.stateMgr.Lock()
	defer ws.stateMgr.Unlock()

	var netName string
	for ii := range ws.Workload.Spec.Interfaces {
		// check if we have a network for this workload
		if len(ws.Workload.Spec.Interfaces[ii].Network) != 0 {
			netName = ws.Workload.Spec.Interfaces[ii].Network
		} else {
			netName = ws.stateMgr.networkName(ws.Workload.Spec.Interfaces[ii].ExternalVlan)
		}

		ns, err := ws.stateMgr.FindNetwork(ws.Workload.Tenant, netName)
		if err != nil {
			log.Errorf("Error finding network. Err: %v", err)
			return err
		}

		// check if we already have the endpoint for this workload
		name, _ := strconv.ParseMacAddr(ws.Workload.Spec.Interfaces[ii].MACAddress)
		epName := ws.Workload.Name + "-" + name

		sourceNodeUUID := sourceDSCs[0].DistributedServiceCard.Name
		sourceHostAddress := sourceDSCs[0].DistributedServiceCard.Status.IPConfig.IPAddress

		destNodeUUID := destDSCs[0].DistributedServiceCard.Name
		destHostAddress := destDSCs[0].DistributedServiceCard.Status.IPConfig.IPAddress

		if ws.Workload.Status.MigrationStatus == nil {
			log.Errorf("migration status is nil. cannot proceed")
			return fmt.Errorf("migration status is nil")
		}

		// check if an endpoint with this mac address already exists in this network
		epMac := ws.Workload.Spec.Interfaces[ii].MACAddress
		if ns != nil {
			_, err := ns.FindEndpointByMacAddr(epMac)
			if err != nil {
				log.Errorf("Failed to find endpoint with ep mac - %v", epMac)
				return fmt.Errorf("failed to find ep %v", epMac)
			}

			epInfo := workload.Endpoint{
				TypeMeta: api.TypeMeta{Kind: "Endpoint"},
				ObjectMeta: api.ObjectMeta{
					Name:      epName,
					Tenant:    ws.Workload.Tenant,
					Namespace: ws.Workload.Namespace,
				},
				Spec: workload.EndpointSpec{
					NodeUUID:         destNodeUUID,
					HomingHostAddr:   destHostAddress,
					MicroSegmentVlan: ws.Workload.Spec.Interfaces[ii].MicroSegVlan,
				},
				Status: workload.EndpointStatus{
					Network:            netName,
					NodeUUID:           sourceNodeUUID,
					WorkloadName:       ws.Workload.Name,
					WorkloadAttributes: ws.Workload.Labels,
					MacAddress:         epMac,
					HomingHostAddr:     sourceHostAddress,
					HomingHostName:     ws.Workload.Status.HostName,
					MicroSegmentVlan:   ws.Workload.Status.Interfaces[ii].MicroSegVlan,
					Migration:          &workload.EndpointMigrationStatus{},
				},
			}

			switch ws.Workload.Status.MigrationStatus.Stage {
			case workload.WorkloadMigrationStatus_MIGRATION_START.String():
				epInfo.Status.Migration.Status = workload.EndpointMigrationStatus_START.String()
			case workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String():
				epInfo.Status.Migration.Status = workload.EndpointMigrationStatus_FINAL_SYNC.String()
			case workload.WorkloadMigrationStatus_MIGRATION_DONE.String():
				//epInfo.Status.NodeUUID = epInfo.Spec.NodeUUID
			case workload.WorkloadMigrationStatus_MIGRATION_ABORT.String():
				epInfo.Spec.NodeUUID = epInfo.Status.NodeUUID
				epInfo.Status.Migration.Status = workload.EndpointMigrationStatus_ABORTED.String()
			}

			if len(ws.Workload.Spec.Interfaces[ii].IpAddresses) > 0 {
				epInfo.Status.IPv4Addresses = ws.Workload.Spec.Interfaces[ii].IpAddresses
			}

			// update endpoint
			err = ws.stateMgr.ctrler.Endpoint().Update(&epInfo)
			if err != nil {
				log.Errorf("Error updating endpoint. Err: %v", err)
			}
		}
	}

	return nil
}

func getWorkloadNameFromEPName(epName string) string {
	idx := strings.LastIndex(epName, "-")

	if idx < 0 {
		return epName
	}
	wrkName := epName[:idx]
	log.Infof("Got workload name [%v] for ep [%v].", wrkName, epName)
	return wrkName
}

func (ws *WorkloadState) incrMigrationSuccess() {
	ws.migrationState.Lock()
	defer ws.migrationState.Unlock()

	ws.migrationSuccessCount = ws.migrationSuccessCount + 1
}

func (ws *WorkloadState) incrMigrationFailure() {
	ws.migrationState.Lock()
	defer ws.migrationState.Unlock()

	ws.migrationFailureCount = ws.migrationFailureCount + 1
}

func (ws *WorkloadState) isMigrationFailure() bool {
	ws.migrationState.Lock()
	defer ws.migrationState.Unlock()

	ret := ws.migrationFailureCount > 0
	return ret
}

func (ws *WorkloadState) isMigrationSuccess() bool {
	ws.migrationState.Lock()
	defer ws.migrationState.Unlock()

	ret := ws.migrationEPCount == ws.migrationSuccessCount
	return ret
}

func (ws *WorkloadState) trackMigration(sourceDSCs, destDSCs []*DistributedServiceCardState) {
	log.Infof("Tracking migration for Workload. %v", ws.Workload.Name)
	checkDataplaneMigration := time.NewTicker(time.Second)
	defer ws.moveWg.Done()
	if len(sourceDSCs) == 0 || len(destDSCs) == 0 {
		log.Errorf("DSC list is empty. Exiting migration tracking for workload  %v", ws.Workload.Name)
		return
	}

	// Add the workloads to DSCs for tracking
	sourceDSCs[0].workloadsMigratingOut[ws.Workload.Name] = ws
	destDSCs[0].workloadsMigratingIn[ws.Workload.Name] = ws

	defer func() {
		ws.moveCtx = nil
		ws.moveCancel = nil
		delete(sourceDSCs[0].workloadsMigratingOut, ws.Workload.Name)
		delete(destDSCs[0].workloadsMigratingIn, ws.Workload.Name)
	}()

	for {
		select {
		case <-ws.moveCtx.Done():
			log.Infof("Migration context for workload %v was cancelled.", ws.Workload.Name)
			ws.epMoveWg.Wait()

			// Treat force stopping of migration caused by dsc profile downgrade as a Abort
			if ws.Workload.Status.MigrationStatus.Stage == workload.WorkloadMigrationStatus_MIGRATION_ABORT.String() || ws.forceStopMigration {
				ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_FAILED.String()
				log.Errorf("Workload [%v] migration aborted.", ws.Workload.Name)
			} else if ws.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_STARTED.String() {
				// In case of timeout move the EPs to the new HOST
				log.Errorf("Workload [%v] migration timed out.", ws.Workload.Name)

				// Wait for EP move goroutines to exit before updating the status to API Server
				ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_TIMED_OUT.String()

				vmName, dcName, orchName := ws.getOrchestrationLabels()
				recorder.Event(eventtypes.MIGRATION_TIMED_OUT,
					fmt.Sprintf("%v : VM %v (%v) in Datacenter %v migration timed out. Traffic may be affected.", orchName, vmName, ws.Workload.Name, dcName),
					nil)
			}

			ws.Workload.Status.MigrationStatus.CompletedAt = &api.Timestamp{}
			ws.Workload.Status.MigrationStatus.CompletedAt.SetTime(time.Now())
			ws.Workload.Write()

			return
		case <-checkDataplaneMigration.C:
			if ws.isMigrationSuccess() {
				log.Infof("All EPs of %v successfully moved.", ws.Workload.Name)
				ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_DONE.String()
				ws.Workload.Status.MigrationStatus.CompletedAt = &api.Timestamp{}
				ws.Workload.Status.MigrationStatus.CompletedAt.SetTime(time.Now())
				ws.Workload.Write()
				return
			}

			if ws.isMigrationFailure() {
				log.Errorf("Failed moving EPs for %v.", ws.Workload.Name)
				ws.epMoveWg.Wait()
				ws.Workload.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_FAILED.String()
				ws.Workload.Status.MigrationStatus.CompletedAt = &api.Timestamp{}
				ws.Workload.Status.MigrationStatus.CompletedAt.SetTime(time.Now())
				ws.Workload.Write()

				vmName, dcName, orchName := ws.getOrchestrationLabels()
				recorder.Event(eventtypes.MIGRATION_FAILED,
					fmt.Sprintf("%v : VM %v (%v) in Datacenter %v migration failed. Traffic may be affected.", orchName, vmName, ws.Workload.Name, dcName),
					nil)
				return
			}
		}
	}
}

func (ws *WorkloadState) getOrchestrationLabels() (vmName, dcName, orchName string) {
	if n, ok := ws.Workload.Labels[vchub.NameKey]; ok {
		vmName = n
	}

	if n, ok := ws.Workload.Labels[utils.NamespaceKey]; ok {
		dcName = n
	}

	if n, ok := ws.Workload.Labels[utils.OrchNameKey]; ok {
		orchName = n
	}

	return
}

func (ws *WorkloadState) isInterfaceChanged(nwrk *workload.Workload) bool {

	sliceEqual := func(X, Y []string) bool {
		m := make(map[string]int)

		for _, y := range Y {
			m[y]++
		}

		for _, x := range X {
			if m[x] > 0 {
				m[x]--
				if m[x] == 0 {
					delete(m, x)
				}
				continue
			}
			//not present or execess
			return false
		}

		return len(m) == 0
	}

	if len(nwrk.Spec.Interfaces) != len(ws.Workload.Spec.Interfaces) {
		// number of interfaces changed, delete old ones
		return true
	}

	for _, curIntf := range ws.Workload.Spec.Interfaces {
		found := false
		for _, newIntf := range nwrk.Spec.Interfaces {
			if curIntf.MACAddress != newIntf.MACAddress {
				continue
			}

			found = true
			// check what changed
			if curIntf.Network != newIntf.Network {
				// network changed delete old endpoints
				return true
			}
			if curIntf.ExternalVlan != newIntf.ExternalVlan {
				// external VLAN changed delete old endpoints
				return true
			}
			if curIntf.MicroSegVlan != newIntf.MicroSegVlan {
				// useg vlan changed, delete old endpoint
				return true
			}

			if !sliceEqual(curIntf.IpAddresses, newIntf.IpAddresses) {
				return true
			}
		}

		if !found {
			return true
		}
	}

	return false
}

func (ws *WorkloadState) stopMigration() error {
	log.Infof("Stopping migration for Workload %v", ws.Workload.Name)
	ws.forceStopMigration = true
	if ws.moveCancel != nil {
		ws.moveCancel()
	}

	return nil
}
