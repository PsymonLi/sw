// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"context"
	"fmt"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

// DSCProfileState is a wrapper for dscProfile object
type DSCProfileState struct {
	sync.Mutex
	DSCProfile *ctkit.DSCProfile // dscProfile object
	stateMgr   *Statemgr         // pointer to state manager
	dscs       map[string]bool
}

// DSCProfileStateFromObj converts from memdb object to dscProfile state
func DSCProfileStateFromObj(obj runtime.Object) (*DSCProfileState, error) {
	switch obj.(type) {
	case *ctkit.DSCProfile:
		nsobj := obj.(*ctkit.DSCProfile)
		switch nsobj.HandlerCtx.(type) {
		case *DSCProfileState:
			dps := nsobj.HandlerCtx.(*DSCProfileState)
			return dps, nil
		default:
			return nil, fmt.Errorf("unknown object")
		}
	default:
		return nil, fmt.Errorf("unknown object")
	}
}

//GetDSCProfileWatchOptions gets options
func (sm *Statemgr) GetDSCProfileWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{}
	return &opts
}

// NewDSCProfileState creates new dscProfile state object
func NewDSCProfileState(dscProfile *ctkit.DSCProfile, stateMgr *Statemgr) (*DSCProfileState, error) {
	dps := DSCProfileState{
		DSCProfile: dscProfile,
		stateMgr:   stateMgr,
		dscs:       make(map[string]bool),
	}
	dscProfile.HandlerCtx = &dps

	return &dps, nil
}

// OnDSCProfileCreate handles dscProfile creation
func (sm *Statemgr) OnDSCProfileCreate(dscProfile *ctkit.DSCProfile) error {
	sm.logger.Infof("Creating dscProfile: %+v", dscProfile)

	// create new dscProfile object
	dps, err := NewDSCProfileState(dscProfile, sm)
	if err != nil {
		sm.logger.Errorf("Error creating dscProfile %+v. Err: %v", dscProfile, err)
		return err
	}

	// List DSCs and see if any are on this profile
	dps.Lock()
	defer dps.Unlock()
	objs, err := sm.Controller().DistributedServiceCard().List(context.Background(), &api.ListWatchOptions{})
	if err != nil {
		return nil
	}
	for _, dsc := range objs {
		if dsc.DistributedServiceCard.Spec.DSCProfile == dscProfile.Name {
			dps.dscs[dsc.DistributedServiceCard.Name] = true
		}
	}

	return nil
}

func (dps *DSCProfileState) isOrchestrationCompatible() bool {
	return dps.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_VIRTUALIZED.String() && dps.DSCProfile.Spec.FeatureSet == cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String()
}

// OnDSCProfileUpdate handles update event
func (sm *Statemgr) OnDSCProfileUpdate(dscProfile *ctkit.DSCProfile, nfwp *cluster.DSCProfile) error {
	// see if anything changed
	sm.logger.Infof("Received update %v\n", nfwp)
	_, ok := ref.ObjDiff(dscProfile.Spec, nfwp.Spec)
	if (nfwp.GenerationID == dscProfile.GenerationID) && !ok {
		sm.logger.Infof("No update received")
		dscProfile.ObjectMeta = nfwp.ObjectMeta
		return nil
	}

	dps, err := sm.FindDSCProfile(dscProfile.Tenant, dscProfile.Name)
	if err != nil {
		sm.logger.Errorf("Could not find the dsc profile %+v. Err: %v", dscProfile.ObjectMeta, err)
		return err
	}

	dps.DSCProfile.Spec = nfwp.Spec
	dps.DSCProfile.ObjectMeta = nfwp.ObjectMeta
	dps.DSCProfile.Status = cluster.DSCProfileStatus{}

	dps.Lock()
	defer dps.Unlock()
	for dsc := range dps.dscs {
		dscState := sm.FindDSC(dsc, "")
		if dscState == nil {
			continue
		}

		orchNameValue, ok := dscState.DistributedServiceCard.Labels[utils.OrchNameKey]
		if ok {
			if err := dscState.isOrchestratorCompatible(); err != nil {
				sm.AddIncompatibleDSCToOrch(dscState.DistributedServiceCard.Name, orchNameValue)
				recorder.Event(eventtypes.ORCH_DSC_MODE_INCOMPATIBLE,
					fmt.Sprintf("DSC %v mode must be updated to work with orchestrator, %v", dscState.DistributedServiceCard.Spec.ID, err), &dscState.DistributedServiceCard.DistributedServiceCard)
			} else {
				sm.RemoveIncompatibleDSCFromOrch(dscState.DistributedServiceCard.Name, orchNameValue)
			}
		}
	}

	return nil
}

// OnDSCProfileDelete handles dscProfile deletion
func (sm *Statemgr) OnDSCProfileDelete(dscProfile *ctkit.DSCProfile) error {
	return nil
}

// OnDSCProfileReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnDSCProfileReconnect() {
	return
}

// FindDSCProfile finds a dscProfile
func (sm *Statemgr) FindDSCProfile(tenant, name string) (*DSCProfileState, error) {
	// find the object
	obj, err := sm.FindObject("DSCProfile", "", "", name)
	if err != nil {
		sm.logger.Infof("Unable to find the profile")
		return nil, err
	}
	sm.logger.Infof("Found the profile")
	return DSCProfileStateFromObj(obj)
}

// ListDSCProfiles lists all apps
func (sm *Statemgr) ListDSCProfiles() ([]*DSCProfileState, error) {
	objs := sm.ctrler.ListObjects("DSCProfile")

	var fwps []*DSCProfileState
	for _, obj := range objs {
		fwp, err := DSCProfileStateFromObj(obj)
		if err != nil {
			return fwps, err
		}

		fwps = append(fwps, fwp)
	}

	return fwps, nil
}
