package statemgr

import (
	"fmt"
	"math"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/venice/utils/log"
)

//StatusWriter write interface
type stateObj interface {
	GetKey() string
	GetKind() string
	Write() error
	TrackedDSCs() []string
	getPropStatus() objPropagationStatus
}

type objectTrackerIntf interface {
	reinitObjTracking(objGenID, genID string) error
	resetObjTracking(genID string)
	startDSCTracking(string) error
	stopDSCTracking(string) error
	incrementGenID() string
	updateNotificationEnabled() bool
}

type objPropagationStatus struct {
	generationID         string
	internalgenerationID string
	updated              int32
	pending              int32
	minVersion           string
	pendingDSCs          []string
	status               string
	maxTime              int64
	minTime              int64
	meanTime             int64
}

type nodeTrackData struct {
	version    string
	updateTime int64
	sentTime   int64
}

type smObjectTracker struct {
	nodeVersions  map[string]*nodeTrackData // Map for node -> version
	obj           stateObj
	generationID  string
	objGenID      string
	noUpdateNotif bool
	trackerLock   sync.Mutex
	pushTime      int64
}

func (objTracker *smObjectTracker) init(obj stateObj) {
	objTracker.obj = obj
	objTracker.nodeVersions = make(map[string]*nodeTrackData)
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
func (objTracker *smObjectTracker) reinitObjTracking(objGenID, genID string) error {

	objTracker.trackerLock.Lock()
	defer objTracker.trackerLock.Unlock()

	dscs := objTracker.obj.TrackedDSCs()

	//reset as some DSCs may not be valid
	objTracker.nodeVersions = make(map[string]*nodeTrackData)
	// walk all smart nics
	sentTime := time.Now().Unix()
	for _, dsc := range dscs {
		if _, ok := objTracker.nodeVersions[dsc]; !ok {
			objTracker.nodeVersions[dsc] = &nodeTrackData{sentTime: sentTime}
		}
	}

	objTracker.generationID = genID
	//For status always set the generation ID of the object meta
	objTracker.objGenID = objGenID
	objTracker.pushTime = time.Now().Unix()

	return nil
}

func (objTracker *smObjectTracker) resetObjTracking(genID string) {

	objTracker.trackerLock.Lock()
	defer objTracker.trackerLock.Unlock()
	objTracker.nodeVersions = make(map[string]*nodeTrackData)
	objTracker.generationID = genID
}

func (objTracker *smObjectTracker) startDSCTracking(dsc string) error {

	objTracker.trackerLock.Lock()

	update := false
	if _, ok := objTracker.nodeVersions[dsc]; !ok {
		log.Infof("DSC %v is being tracked for propogation status for object %s", dsc, objTracker.obj.GetKey())
		objTracker.nodeVersions[dsc] = &nodeTrackData{sentTime: time.Now().Unix()}
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

	data, ok := objTracker.nodeVersions[nodeuuid]
	//Update only if entry found or gen ID matches
	if ok && objTracker.generationID == generationID && data.version != generationID {
		objTracker.nodeVersions[nodeuuid].version = generationID
		data.updateTime = time.Now().Unix()
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

func max(x, y int64) int64 {
	if x < y {
		return y
	}
	return x
}

func min(x, y int64) int64 {
	if x > y {
		return y
	}
	return x
}

func (objTracker *smObjectTracker) getPropStatus() objPropagationStatus {

	objTracker.trackerLock.Lock()
	defer objTracker.trackerLock.Unlock()

	propStatus := objPropagationStatus{generationID: objTracker.objGenID, internalgenerationID: objTracker.generationID,
		minTime: math.MaxInt64}
	for node, data := range objTracker.nodeVersions {
		if objTracker.generationID != data.version {
			propStatus.pendingDSCs = append(propStatus.pendingDSCs, node)
			propStatus.pending++
			if propStatus.minVersion == "" || versionToInt(data.version) < versionToInt(propStatus.minVersion) {
				propStatus.minVersion = data.version
			}
		} else {
			propStatus.updated++
			propTime := data.updateTime - data.sentTime
			propStatus.maxTime = max(propStatus.maxTime, propTime)
			propStatus.minTime = min(propStatus.minTime, propTime)
			propStatus.meanTime += propTime
		}

	}

	if propStatus.pending == 0 && propStatus.updated == 0 {
		propStatus.status = fmt.Sprintf("N/A")
	} else if propStatus.pending == 0 {
		propStatus.status = fmt.Sprintf("Propagation Complete")
	} else {
		propStatus.status = fmt.Sprintf("Propagation pending on: %s", strings.Join(propStatus.pendingDSCs, ", "))
	}
	if propStatus.updated != 0 {
		propStatus.meanTime = propStatus.meanTime / int64(propStatus.updated)
	}
	return propStatus
}
