package statemgr

import (
	"fmt"
	"strconv"
	"strings"
	"sync"

	"github.com/pensando/sw/venice/utils/log"
)

//StatusWriter write interface
type stateObj interface {
	GetKey() string
	Write() error
	TrackedDSCs() []string
	getPropStatus() objPropagationStatus
}

type objectTrackerIntf interface {
	reinitObjTracking(genID string) error
	startDSCTracking(string) error
	stopDSCTracking(string) error
	incrementGenID() string
	updateNotificationEnabled() bool
}

type objPropagationStatus struct {
	generationID string
	updated      int32
	pending      int32
	minVersion   string
	pendingDSCs  []string
	status       string
}

type smObjectTracker struct {
	nodeVersions  map[string]string // Map for node -> version
	obj           stateObj
	generationID  string
	noUpdateNotif bool
	trackerLock   sync.Mutex
}

func (objTracker *smObjectTracker) init(obj stateObj) {
	objTracker.obj = obj
	objTracker.nodeVersions = make(map[string]string)
}

func (objTracker *smObjectTracker) updateNotificationEnabled() bool {
	return !objTracker.noUpdateNotif
}

func (objTracker *smObjectTracker) incrementGenID() string {
	genID, _ := strconv.Atoi(objTracker.generationID)
	genID++
	objTracker.generationID = strconv.Itoa(genID)
	return objTracker.generationID
}

// initNodeVersions initializes node versions for the policy
func (objTracker *smObjectTracker) reinitObjTracking(genID string) error {

	objTracker.trackerLock.Lock()
	defer objTracker.trackerLock.Unlock()

	dscs := objTracker.obj.TrackedDSCs()

	//objTracker.nodeVersions = make(map[string]string)
	// walk all smart nics
	for _, dsc := range dscs {
		if _, ok := objTracker.nodeVersions[dsc]; !ok {
			objTracker.nodeVersions[dsc] = ""
		}
	}

	objTracker.generationID = genID

	return nil
}

func (objTracker *smObjectTracker) startDSCTracking(dsc string) error {

	objTracker.trackerLock.Lock()

	update := false
	if _, ok := objTracker.nodeVersions[dsc]; !ok {
		log.Infof("DSC %v is being tracked for propogation status for object %s", dsc, objTracker.obj.GetKey())
		objTracker.nodeVersions[dsc] = ""
		update = true
	}

	objTracker.trackerLock.Unlock()

	if update && !objTracker.noUpdateNotif {
		mgr := MustGetStatemgr()
		mgr.PeriodicUpdaterPush(objTracker.obj)
	}

	return nil
}

func (objTracker *smObjectTracker) skipUpdateNotification() {
	objTracker.noUpdateNotif = true
}

func (objTracker *smObjectTracker) stopDSCTracking(dsc string) error {

	objTracker.trackerLock.Lock()

	update := false
	_, ok := objTracker.nodeVersions[dsc]
	if ok {
		log.Infof("DSC %v is being untracked for propogation status for object %s", dsc, objTracker.obj.GetKey())
		delete(objTracker.nodeVersions, dsc)
		update = true
	}

	objTracker.trackerLock.Unlock()

	if update && !objTracker.noUpdateNotif {
		mgr := MustGetStatemgr()
		mgr.PeriodicUpdaterPush(objTracker.obj)
	}

	return nil
}

func (objTracker *smObjectTracker) updateNodeVersion(nodeuuid, generationID string) {

	objTracker.trackerLock.Lock()

	update := false

	currentVersion, ok := objTracker.nodeVersions[nodeuuid]
	//Update only if entry found or gen ID matches
	if ok && objTracker.generationID == generationID && currentVersion != generationID {
		objTracker.nodeVersions[nodeuuid] = generationID
		update = true
	}

	objTracker.trackerLock.Unlock()

	if update && !objTracker.noUpdateNotif {
		mgr := MustGetStatemgr()
		mgr.PeriodicUpdaterPush(objTracker.obj)
	}
}

func versionToInt(v string) int {
	i, err := strconv.Atoi(v)
	if err != nil {
		return 0
	}
	return i
}

func (objTracker *smObjectTracker) getPropStatus() objPropagationStatus {

	objTracker.trackerLock.Lock()
	defer objTracker.trackerLock.Unlock()

	propStatus := objPropagationStatus{generationID: objTracker.generationID}
	for node, version := range objTracker.nodeVersions {
		if objTracker.generationID != version {
			propStatus.pendingDSCs = append(propStatus.pendingDSCs, node)
			propStatus.pending++
			if propStatus.minVersion == "" || versionToInt(version) < versionToInt(propStatus.minVersion) {
				propStatus.minVersion = version
			}
		} else {
			propStatus.updated++
		}

	}
	if propStatus.pending == 0 {
		propStatus.status = fmt.Sprintf("Propagation Complete")
	} else {
		propStatus.status = fmt.Sprintf("Propagation pending on: %s", strings.Join(propStatus.pendingDSCs, ", "))
	}
	return propStatus
}
