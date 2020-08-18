package statemgr

import (
	"fmt"
	"strconv"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/runtime"
)

const defaultNumMacs = uint64(24)

// DistributedServiceCardState is a wrapper for cluster object
type DistributedServiceCardState struct {
	sync.Mutex
	DistributedServiceCard *ctkit.DistributedServiceCard
	stateMgr               *Statemgr
	profile                string
	admission              string
	orchVal                string
}

//GetDistributedServiceCardWatchOptions gets options
func (sm *Statemgr) GetDistributedServiceCardWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec.DSCProfile", "Status.AdmissionPhase", "Meta.Labels"}
	return &opts
}

// OnDistributedServiceCardCreate creates a DistributedServiceCard based on watch event
func (sm *Statemgr) OnDistributedServiceCardCreate(dsc *ctkit.DistributedServiceCard) error {
	dscState, err := NewDSCState(dsc, sm)
	if err != nil {
		return err
	}

	dscState.profile = dsc.DistributedServiceCard.Spec.DSCProfile
	dscState.admission = dsc.DistributedServiceCard.Status.AdmissionPhase

	sm.populateDSCMap(dsc, true)

	// Trigger re-validation of all hosts
	sm.SendProbeEvent(dsc.GetObjectMeta(), dsc.GetObjectKind(), kvstore.Created, "")

	dps, err := sm.FindDSCProfile("", dsc.DistributedServiceCard.Spec.DSCProfile)
	if err == nil {
		dps.Lock()
		dps.dscs[dsc.DistributedServiceCard.Name] = true
		dps.Unlock()
	}

	return nil
}

func (sm *Statemgr) populateDSCMap(dsc *ctkit.DistributedServiceCard, adding bool) {
	// Populate Mac map
	sm.DscMapLock.Lock()
	defer sm.DscMapLock.Unlock()
	if adding {
		sm.logger.Infof("Adding entries to dsc map for %s", dsc.Name)
	} else {
		sm.logger.Infof("Removing entries from dsc map for %s", dsc.Name)
	}
	// Take last 6 hex digits
	lastBits := dsc.Name[7:9] + dsc.Name[10:14]
	base, err := strconv.ParseUint(lastBits, 16, 0)
	if err != nil {
		sm.logger.Errorf("Failed to parse last three hex values of %s: %s", dsc.Name, err)
		return
	}
	var i uint64
	numMacs := uint64(dsc.Status.NumMacAddress)
	if numMacs == 0 {
		numMacs = defaultNumMacs
	}
	for i = 0; i < numMacs; i++ {
		macEnding := fmt.Sprintf("%06x", base+i)
		mac := fmt.Sprintf("%s%s.%s", dsc.Name[0:7], macEnding[0:2], macEnding[2:6])
		if adding {
			if entry, ok := sm.DscMap[mac]; ok && entry != dsc.Name {
				sm.logger.Errorf("Already have DSC Mac entry %s for %s. Conflicting with %s", entry, mac, dsc.Name)
			}
			sm.DscMap[mac] = dsc.Name
		} else {
			delete(sm.DscMap, mac)
		}
	}
}

// OnDistributedServiceCardUpdate handles update event
func (sm *Statemgr) OnDistributedServiceCardUpdate(dsc *ctkit.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) error {
	dscState, err := DistributedServiceCardStateFromObj(dsc)
	if err != nil {
		sm.logger.Errorf("Error finding smartnic. Err: %v", err)
		return err
	}
	oldProfile := dscState.profile
	newProfile := ndsc.Spec.DSCProfile

	justAdmitted := false
	oldAdmit := dscState.admission
	newAdmit := ndsc.Status.AdmissionPhase
	if oldAdmit != cluster.DistributedServiceCardStatus_ADMITTED.String() && newAdmit == cluster.DistributedServiceCardStatus_ADMITTED.String() {
		// If the DSC is just admitted, we must check the mode incompatibility
		// This is for a case when the DSC was not admitted when the HOST was created
		justAdmitted = true
	}

	sm.logger.Infof("Old Profile : %v, New Profile : %v, Newly admitted : %v", oldProfile, newProfile, justAdmitted)

	oldOrchVal := dscState.orchVal

	// Update the DSC
	dscState.DistributedServiceCard.DistributedServiceCard = *ndsc
	dscState.profile = newProfile
	dscState.admission = newAdmit
	orchVal, ok := dscState.DistributedServiceCard.Labels[utils.OrchNameKey]
	if (oldProfile != newProfile) || justAdmitted {
		dscState.orchVal = orchVal

		// If label is found, and the DSC is not compatible, raise an event
		if ok {
			if err := dscState.isOrchestratorCompatible(); err != nil {
				sm.AddIncompatibleDSCToOrch(dscState.DistributedServiceCard.Name, orchVal)
				recorder.Event(eventtypes.ORCH_DSC_MODE_INCOMPATIBLE,
					fmt.Sprintf("Profile %v added to DSC %v is incompatible with orchestration feature, %v", newProfile, dscState.DistributedServiceCard.DistributedServiceCard.Spec.ID, err), &dscState.DistributedServiceCard.DistributedServiceCard)
			} else {
				sm.RemoveIncompatibleDSCFromOrch(dscState.DistributedServiceCard.Name, orchVal)
			}
		}

		if len(oldOrchVal) > 0 && oldOrchVal != orchVal {
			sm.RemoveIncompatibleDSCFromOrch(dscState.DistributedServiceCard.Name, oldOrchVal)
		}

		oldDps, err := sm.FindDSCProfile("", oldProfile)
		if err == nil {
			oldDps.Lock()
			delete(oldDps.dscs, dscState.DistributedServiceCard.DistributedServiceCard.Name)
			oldDps.Unlock()
		}

		newDps, err := sm.FindDSCProfile("", newProfile)
		if err == nil {
			newDps.Lock()
			newDps.dscs[dscState.DistributedServiceCard.DistributedServiceCard.Name] = true
			newDps.Unlock()
		}

	}

	return nil
}

// OnDistributedServiceCardDelete deletes a DistributedServiceCard
func (sm *Statemgr) OnDistributedServiceCardDelete(dsc *ctkit.DistributedServiceCard) error {
	dps, err := sm.FindDSCProfile("", dsc.DistributedServiceCard.Spec.DSCProfile)
	if err == nil {
		dps.Lock()
		delete(dps.dscs, dsc.DistributedServiceCard.Name)
		dps.Unlock()
	}

	sm.populateDSCMap(dsc, false)

	orchVal, ok := dsc.DistributedServiceCard.Labels[utils.OrchNameKey]
	if ok {
		sm.RemoveIncompatibleDSCFromOrch(dsc.DistributedServiceCard.Name, orchVal)
	}

	// Trigger re-validation of all hosts
	sm.SendProbeEvent(dsc.GetObjectMeta(), dsc.GetObjectKind(), kvstore.Deleted, "")

	return err
}

// OnDistributedServiceCardReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnDistributedServiceCardReconnect() {
	return
}

// NewDSCState create new DSC state
func NewDSCState(dsc *ctkit.DistributedServiceCard, stateMgr *Statemgr) (*DistributedServiceCardState, error) {
	w := &DistributedServiceCardState{
		DistributedServiceCard: dsc,
		stateMgr:               stateMgr,
	}
	dsc.HandlerCtx = w

	return w, nil
}

// DistributedServiceCardStateFromObj conerts from memdb object to cluster state
func DistributedServiceCardStateFromObj(obj runtime.Object) (*DistributedServiceCardState, error) {
	switch obj.(type) {
	case *ctkit.DistributedServiceCard:
		nobj := obj.(*ctkit.DistributedServiceCard)
		switch nobj.HandlerCtx.(type) {
		case *DistributedServiceCardState:
			nts := nobj.HandlerCtx.(*DistributedServiceCardState)
			return nts, nil
		default:
			return nil, fmt.Errorf("Wrong type")
		}
	default:
		return nil, fmt.Errorf("Wrong type")
	}
}

func (dscState *DistributedServiceCardState) isOrchestratorCompatible() error {
	isVirtualizedMode := false
	dscProfile := dscState.DistributedServiceCard.Spec.DSCProfile
	dscProfileState, _ := dscState.stateMgr.FindDSCProfile("default", dscProfile)
	if dscProfileState == nil {
		return fmt.Errorf("profile %v does not exist", dscProfile)
	}
	virErrMsg := fmt.Errorf("dsc featureset %v is not supported in deployment %v. Only featureset %v and orchestration mode %v are supported",
		dscProfileState.DSCProfile.Spec.FeatureSet,
		cluster.DSCProfileSpec_VIRTUALIZED.String(),
		cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
		orchestration.NamespaceSpec_Managed.String())

	// Only FIREWALL featureset is supported for VIRTUALIZED deployment
	if dscProfileState.DSCProfile.Spec.DeploymentTarget == cluster.DSCProfileSpec_VIRTUALIZED.String() {
		isVirtualizedMode = true
	}

	// We expect the orch label to already be applied to DSC object by the
	// time we check for the compatibility
	orchVal, ok := dscState.DistributedServiceCard.Labels[utils.OrchNameKey]
	if !ok {
		return fmt.Errorf("orchestrator label missing in DSC %v", dscState.DistributedServiceCard.Name)
	}

	hostName := dscState.DistributedServiceCard.Status.Host
	if len(hostName) == 0 {
		return fmt.Errorf("dsc %v does not have a Host associated with it", dscState.DistributedServiceCard.Name)
	}

	host := dscState.stateMgr.FindHost(hostName)
	if host == nil {
		return fmt.Errorf("failed to find host %v", hostName)
	}

	if host.Host.Labels == nil {
		return fmt.Errorf("no labels found on host %v", hostName)
	}

	namespaceVal, ok := host.Host.Labels[utils.NamespaceKey]
	if !ok {
		return fmt.Errorf("no namespace label found on host %v", hostName)
	}

	orch, err := dscState.stateMgr.FindOrchestrator(orchVal)
	if err != nil {
		return err
	}

	if isVirtualizedMode && len(orch.Orchestrator.Spec.ManageNamespaces) > 0 {
		for _, ns := range orch.Orchestrator.Spec.ManageNamespaces {
			if ns == utils.ManageAllDcs || ns == namespaceVal {
				if dscProfileState.DSCProfile.Spec.FeatureSet != cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String() {
					return virErrMsg
				}

				return nil
			}
		}
	}

	found := false
	// TODO : Improve this search - replace it with a map of namespaces in orchestrator object
	for _, ns := range orch.Orchestrator.Spec.Namespaces {
		if ns.Name == namespaceVal || ns.Name == utils.ManageAllDcs {
			found = true
			if isVirtualizedMode {
				if ns.Mode == orchestration.NamespaceSpec_Monitored.String() {
					return fmt.Errorf("only managed namespaces are supported for profiles with deployment %v and featureset %v",
						cluster.DSCProfileSpec_VIRTUALIZED.String(),
						cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String())
				}

				if dscProfileState.DSCProfile.Spec.FeatureSet != cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String() {
					return virErrMsg
				}
			} else if ns.Mode != orchestration.NamespaceSpec_Monitored.String() {
				return fmt.Errorf("only monitored namespaces are supported for profiles with deployment %v",
					cluster.DSCProfileSpec_HOST.String())
			}

			break
		}
	}

	if !found {
		return fmt.Errorf("namespace %v was not found in the orchestrator %v", namespaceVal, orchVal)
	}

	return nil
}

// FindDSC Get DSC State by ID
func (sm *Statemgr) FindDSC(mac, id string) *DistributedServiceCardState {
	objs := sm.ctrler.ListObjects("DistributedServiceCard")

	for _, obj := range objs {
		snic, err := DistributedServiceCardStateFromObj(obj)
		if err != nil {
			continue
		}

		if snic.DistributedServiceCard.Status.PrimaryMAC == mac {
			return snic
		}
	}

	for _, obj := range objs {
		snic, err := DistributedServiceCardStateFromObj(obj)
		if err != nil {
			continue
		}

		if snic.DistributedServiceCard.Spec.ID == id {
			return snic
		}
	}
	return nil
}
