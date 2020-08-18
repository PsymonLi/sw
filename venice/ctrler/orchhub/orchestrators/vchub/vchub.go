package vchub

import (
	"context"
	"fmt"
	"net/url"
	"reflect"
	"sync"
	"time"

	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/cache"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/timerqueue"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
)

const (
	storeQSize          = 64
	defaultRetryCount   = 1
	defaultTagSyncDelay = 2 * time.Minute
	retryDelay          = 30 * time.Second
)

var (
	managedMode   = orchestration.NamespaceSpec_Managed.String()
	monitoredMode = orchestration.NamespaceSpec_Monitored.String()
)

// VCHub instance
type VCHub struct {
	*defs.State
	cancel       context.CancelFunc
	vcOpsChannel chan *kvstore.WatchEvent
	vcReadCh     chan defs.Probe2StoreMsg
	vcEventCh    chan defs.Probe2StoreMsg
	cache        *cache.Cache
	probe        vcprobe.ProbeInf
	DcMapLock    sync.Mutex
	// TODO: don't use DC display name as key, use ID instead
	DcMap map[string]*PenDC
	// Will contain entries for non-pensando managed DCs
	// Needed for discoveredDCs list when a DC is renamed/deleted
	DcID2NameMap map[string]string
	// Will be taken with write lock by sync
	syncLock          sync.RWMutex
	syncDone          bool
	watchStarted      bool
	discoveredDCsLock sync.Mutex
	discoveredDCs     []string
	// Timestamp of when orchhub was created. Useful when looking at age of go-routines
	launchTime string
	// whether to act upon venice network events
	// When we are disconnected, we do not want to act upon network events
	// until we are reconnected and sync has finished.
	// processVeniceEvents will be set to false once we are disconnected.
	// Any network events will be drained and thrown out.
	// Once connection is restored, sync will take syncLock and set processVeniceEvents to be
	// true. Any new network events will be blocked until sync has finished, and then
	// will be reacted to.
	// A lock is needed for this flag since periodic sync can be triggered by another thread
	// while the connection status is being reacted to in store
	processVeniceEvents     bool
	processVeniceEventsLock sync.Mutex
	// Opts is options used during creation of this instance
	opts                  []Option
	tagSyncDelay          time.Duration
	tagSyncInitializedMap map[string]bool
	orchUpdateLock        sync.Mutex
}

// Option specifies optional values for vchub
type Option func(*VCHub)

// WithVcReadCh replaces the readCh
func WithVcReadCh(ch chan defs.Probe2StoreMsg) Option {
	return func(v *VCHub) {
		v.vcReadCh = ch
	}
}

// WithVcEventsCh listens to the supplied channel for vC notifcation events
func WithVcEventsCh(ch chan defs.Probe2StoreMsg) Option {
	return func(v *VCHub) {
		v.vcEventCh = ch
	}
}

// WithTagSyncDelay sets the tag write sync delay
func WithTagSyncDelay(delay time.Duration) Option {
	return func(v *VCHub) {
		v.tagSyncDelay = delay
	}
}

// LaunchVCHub starts VCHub
func LaunchVCHub(stateMgr *statemgr.Statemgr, config *orchestration.Orchestrator, logger log.Logger, opts ...Option) *VCHub {
	logger.Infof("VCHub instance for %s(orch-%d) is starting...", config.GetName(), config.Status.OrchID)
	vchub := &VCHub{}
	vchub.setupVCHub(stateMgr, config, logger, opts...)

	return vchub
}

func (v *VCHub) setupVCHub(stateMgr *statemgr.Statemgr, config *orchestration.Orchestrator, logger log.Logger, opts ...Option) {
	ctx, cancel := context.WithCancel(context.Background())

	vcURL := &url.URL{
		Scheme: "https",
		Host:   config.Spec.URI,
		Path:   "/sdk",
	}
	vcURL.User = url.UserPassword(config.Spec.Credentials.UserName, config.Spec.Credentials.Password)

	orchID := fmt.Sprintf("orch%d", config.Status.OrchID)

	state := defs.State{
		VcURL:      vcURL,
		VcID:       config.GetName(),
		OrchID:     orchID,
		Ctx:        ctx,
		Log:        logger.WithContext("oName", fmt.Sprintf("%s", config.GetName()), "oID", fmt.Sprintf("%s", orchID)),
		StateMgr:   stateMgr,
		Wg:         &sync.WaitGroup{},
		DcIDMap:    map[string]types.ManagedObjectReference{}, // Obj name to ID
		DvsIDMap:   map[string]types.ManagedObjectReference{}, // Obj name to ID
		TimerQ:     timerqueue.NewQueue(),
		OrchConfig: ref.DeepCopy(config).(*orchestration.Orchestrator),
	}

	v.State = &state
	v.cancel = cancel
	v.DcMap = map[string]*PenDC{}
	v.DcID2NameMap = map[string]string{}
	v.vcReadCh = make(chan defs.Probe2StoreMsg, storeQSize)
	v.vcEventCh = make(chan defs.Probe2StoreMsg, storeQSize)
	v.tagSyncDelay = defaultTagSyncDelay
	v.opts = opts
	v.discoveredDCs = []string{}
	v.tagSyncInitializedMap = map[string]bool{}
	v.launchTime = time.Now().String()

	v.buildDCSpec(config)

	v.orchUpdateLock.Lock()
	v.OrchConfig.Status.Status = orchestration.OrchestratorStatus_Unknown.String()
	v.OrchConfig.Status.LastTransitionTime = &api.Timestamp{}
	v.OrchConfig.Status.LastTransitionTime.SetTime(time.Now())
	v.OrchConfig.Status.Message = ""

	err := v.writeOrchStatus()
	if err != nil {
		logger.Errorf("Failed to write orch status %s", err)
	}

	v.orchUpdateLock.Unlock()

	v.cache = cache.NewCache(v.StateMgr, v.Log)

	clusterItems, err := v.StateMgr.Controller().Cluster().List(context.Background(), &api.ListWatchOptions{})
	if err != nil {
		logger.Errorf("Failed to get cluster object, %s", err)
	} else if len(clusterItems) == 0 {
		logger.Errorf("Cluster list returned 0 objects, %s", err)
	} else {
		cluster := clusterItems[0]
		state.ClusterID = defs.CreateClusterID(cluster.Cluster)
	}

	for _, opt := range opts {
		if opt != nil {
			opt(v)
		}
	}

	v.Wg.Add(1)
	go v.startEventsListener()
	v.createProbe(config)
	v.Wg.Add(1)
	go func() {
		defer v.Wg.Done()
		v.TimerQ.Run(ctx)
	}()
	v.Wg.Add(1)
	go v.periodicTagSync()

	// periodic override check
	v.TimerQ.Add(v.overrideCheckFn, retryDelay)
}

func (v *VCHub) overrideCheckFn() {
	v.verifyOverrides(false)
	v.TimerQ.Add(v.overrideCheckFn, retryDelay)
}

func (v *VCHub) createProbe(config *orchestration.Orchestrator) {
	v.probe = vcprobe.NewVCProbe(v.vcReadCh, v.vcEventCh, v.State)
	v.probe.Start()
}

// Destroy tears down VCHub instance
func (v *VCHub) Destroy(delete bool) {
	// Teardown probe and store
	v.Log.Infof("Destroying VCHub....")
	v.cancel()
	v.Wg.Wait()
	v.probe.Stop()
	v.Log.Infof("vchub waitgroup stopped")

	if delete {
		ctx, cancel := context.WithCancel(context.Background())
		v.Ctx = ctx
		v.cancel = cancel

		// Create new probe in write only mode (we don't start watchers or update vcenter status)
		probe := vcprobe.NewVCProbe(nil, nil, v.State)

		// Copy over tag info so it doesn't need to be fetched again
		probe.SetWriteTags(v.probe.GetWriteTags())
		v.probe = probe
		v.probe.Start()

		v.Log.Infof("Cleaning up state on VCenter.")
		waitCh := make(chan bool, 1)

		// Cleanup might end up getting blocked forever if vCenter cannot be reached
		// Give up after 500 ms
		v.Wg.Add(1)
		go func() {
			defer v.Wg.Done()
			for !v.probe.IsSessionReady() {
				select {
				case <-v.Ctx.Done():
					return
				case <-time.After(50 * time.Millisecond):
				}
			}
			v.deleteAllDVS()
			v.probe.DestroyTags()
			waitCh <- true
		}()
		// Timeout limit for vcenter cleanup
		select {
		case <-waitCh:
			v.Log.Infof("VCenter cleanup finished")
		case <-time.After(5 * time.Second):
			v.Log.Infof("Failed to cleanup vcenter within 5s")
		}
		v.cancel()
		v.Wg.Wait()
		v.probe.Stop()

		v.DeleteHosts()
	}
	v.Log.Infof("VCHub Destroyed")
}

// isCredentialChanged returns true if the new config credentials are different from the existing one
func (v *VCHub) isCredentialChanged(config *orchestration.Orchestrator) bool {
	if v.OrchConfig.Spec.URI != config.Spec.URI {
		return true
	}

	if v.OrchConfig.Spec.Credentials.AuthType != config.Spec.Credentials.AuthType {
		return true
	}

	switch v.OrchConfig.Spec.Credentials.AuthType {
	case monitoring.ExportAuthType_AUTHTYPE_USERNAMEPASSWORD.String():
		return (v.OrchConfig.Spec.Credentials.UserName != config.Spec.Credentials.UserName) ||
			(v.OrchConfig.Spec.Credentials.Password != config.Spec.Credentials.Password) ||
			(v.OrchConfig.Spec.Credentials.DisableServerAuthentication != config.Spec.Credentials.DisableServerAuthentication) ||
			(v.OrchConfig.Spec.Credentials.CaData != config.Spec.Credentials.CaData)
	}

	return false
}

// UpdateConfig handles if the Orchestrator config has changed
func (v *VCHub) UpdateConfig(config *orchestration.Orchestrator) {
	v.Log.Infof("VCHub config updated. : Orch : %v", config.GetKey())
	if v.isCredentialChanged(config) {
		v.Log.Infof("Credentials were updated. Restarting VCHub")
		if v.OrchConfig.Spec.URI != config.Spec.URI {
			// Delete DCs
			wg := sync.WaitGroup{}
			v.DcMapLock.Lock()
			for name := range v.DcMap {
				v.Log.Infof("Cleaning up DC : %v", name)
				wg.Add(1)
				go func(name string) {
					defer wg.Done()
					v.RemovePenDC(name)
				}(name)
			}
			v.DcMapLock.Unlock()
			wg.Wait()
		}
		v.Log.Infof("Credentials were updated. Restarting VCHub")
		v.Destroy(false)
		v.setupVCHub(v.StateMgr, config, v.Log, v.opts...)
	}
	v.reconcileOrchNamespaces(config)
}

func (v *VCHub) buildDCSpec(config *orchestration.Orchestrator) {
	v.DcSpecLock.Lock()
	v.ManagedDCs = map[string]orchestration.ManagedNamespaceSpec{}
	v.MonitoredDCs = map[string]orchestration.MonitoredNamespaceSpec{}
	// Supporting old method of managing namespaces
	for _, dc := range config.Spec.ManageNamespaces {
		v.ManagedDCs[dc] = defs.DefaultDCManagedConfig()
	}

	for _, nsSpec := range config.Spec.Namespaces {
		switch nsSpec.Mode {
		case managedMode:
			spec := defs.DefaultDCManagedConfig()
			if nsSpec.ManagedSpec != nil {
				spec = *nsSpec.ManagedSpec
			}
			v.ManagedDCs[nsSpec.Name] = spec
		case monitoredMode:
			spec := orchestration.MonitoredNamespaceSpec{}
			if nsSpec.MonitoredSpec != nil {
				spec = *nsSpec.MonitoredSpec
			}
			v.MonitoredDCs[nsSpec.Name] = spec
		}
	}
	v.Log.Infof("Managed DCs: %v", v.ManagedDCs)
	v.Log.Infof("Monitored DCs: %v", v.MonitoredDCs)
	v.DcSpecLock.Unlock()
}

// deleteAllDVS cleans up all the PensandoDVS present in the DCs within the VC deployment
func (v *VCHub) deleteAllDVS() {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()

	for _, dc := range v.DcMap {
		if !v.isManagedNamespace(dc.DcName) {
			v.Log.Infof("Skipping deletion of DVS from %v.", dc.DcName)
			continue
		}

		dc.Lock()
		for _, penDVS := range dc.PenDvsMap {
			err := v.probe.RemovePenDVS(dc.DcName, penDVS.DvsName, defaultRetryCount)
			if err != nil {
				v.Log.Errorf("Failed deleting DVS %v in DC %v. Err : %v", penDVS.DvsName, dc.DcName, err)
			}
		}
		dc.Unlock()
	}
}

// Sync handles an instance manager request to reqsync the inventory
func (v *VCHub) Sync() {
	v.Log.Debugf("VCHub Sync starting")
	// Bring useg to VCHub struct
	v.Wg.Add(1)
	go func() {
		defer v.Wg.Done()
		v.sync()
	}()
}

// Debug returns debug info
func (v *VCHub) Debug(action string, params map[string]string) (interface{}, error) {
	return v.debugHandler(action, params)
}

// Reconnect is called when a ctkit watcher client reconnects to apiserver
// Attempt to write any pending objects in the pcache
func (v *VCHub) Reconnect(kind string) {
	v.Log.Infof("Reconnect called for %s", kind)
	switch kind {
	case string(workload.KindWorkload):
		v.cache.RevalidateWorkloads()
	case string(cluster.KindHost):
		v.cache.RevalidateHosts()
		// Also trigger workloads in case they were relying on a host that just got written
		v.cache.RevalidateWorkloads()
	}
}

func (v *VCHub) reconcileOrchNamespaces(config *orchestration.Orchestrator) {
	v.Log.Infof("Reconciling namespaces for orchestrator [%v]", config.Name)
	updateVCHub := func() {
		v.buildDCSpec(config)
		// Don't copy over status
		v.OrchConfig.Spec = ref.DeepCopy(config.Spec).(orchestration.OrchestratorSpec)
	}
	// Cleanup and additions are only needed if we are connected to vCenter
	if !v.probe.IsSessionReady() {
		v.Log.Infof("Reconciling namespaces skipped because probe isn't ready")
		updateVCHub()
		return
	}

	// If sync is running, we don't want to modify state until it is done.
	// We also want to make sure that any vcenter events that are in the middle of processing are finished.
	v.syncLock.Lock()
	defer v.syncLock.Unlock()

	dcMap, err := v.probe.GetDCMap()
	if err != nil {
		v.Log.Errorf("Failed to get dc mapping during reconcileManagedNamepsace, %s", err)
		v.Destroy(false)
		v.setupVCHub(v.StateMgr, config, v.Log, v.opts...)
		return
	}

	// Get currently managed namespaces
	currDCList := map[string]*orchestration.NamespaceSpec{}

	v.DcMapLock.Lock()
	for _, dc := range v.DcMap {
		mode := string(dc.mode)
		currDCList[dc.DcName] = &orchestration.NamespaceSpec{
			Mode: mode,
			Name: dc.DcName,
		}
		if mode == managedMode {
			v.DcSpecLock.Lock()
			spec := v.ManagedDCs[dc.DcName]
			currDCList[dc.DcName].ManagedSpec = &spec
			v.DcSpecLock.Unlock()
		}
	}
	v.DcMapLock.Unlock()

	// If url is different, then we have all deletes.
	newNamespaces := map[string]*orchestration.NamespaceSpec{}
	if config.Spec.URI == v.OrchConfig.Spec.URI {
		for _, dc := range config.Spec.ManageNamespaces {
			if dc == utils.ManageAllDcs {
				for k := range dcMap {
					managedSpec := defs.DefaultDCManagedConfig()
					newNamespaces[k] = &orchestration.NamespaceSpec{
						Mode:        orchestration.NamespaceSpec_Managed.String(),
						Name:        k,
						ManagedSpec: &managedSpec,
					}
				}
			} else {
				managedSpec := defs.DefaultDCManagedConfig()
				newNamespaces[dc] = &orchestration.NamespaceSpec{
					Mode:        orchestration.NamespaceSpec_Managed.String(),
					Name:        dc,
					ManagedSpec: &managedSpec,
				}
			}
		}

		for _, nsSpec := range config.Spec.Namespaces {
			if nsSpec.Name == utils.ManageAllDcs {
				for k := range dcMap {
					spec := ref.DeepCopy(*nsSpec).(orchestration.NamespaceSpec)
					spec.Name = k
					newNamespaces[k] = &spec
				}
			} else {
				newNamespaces[nsSpec.Name] = nsSpec
			}
		}
	}
	v.Log.Infof("Old datacenter list %+v", currDCList)
	v.Log.Infof("New datacenter list %+v", newNamespaces)

	updateVCHub()
	for name, spec := range currDCList {
		if nSpec, ok := newNamespaces[name]; ok {
			if nSpec.Mode != spec.Mode {
				// Mode changed. Tear down and create
				v.RemovePenDC(name)
				// Add DC back
				dc, ok := dcMap[name]
				if !ok {
					// This should never happen since currDCList is built from dcMap
					v.Log.Errorf("DC %v not found in VCenter DC List.", name)
					continue
				}
				v.addDC(dc.Name, dc.Self.Value)
			} else {
				switch nSpec.Mode {
				case managedMode:
					if reflect.DeepEqual(nSpec.ManagedSpec, spec.ManagedSpec) {
						v.Log.Infof("Managed namespace spec changed for DC %v", name)
						v.DcMapLock.Lock()
						d, ok := v.DcMap[name]
						v.DcMapLock.Unlock()
						if ok {
							err := d.AddPenDVS()
							if err != nil {
								v.Log.Errorf("Failed to add DVS to DC %v. %v", name, err)
							}
						}
					}
				}
			}
		} else {
			// No longer interested
			v.RemovePenDC(name)
		}
	}

	for name := range newNamespaces {
		if _, ok := currDCList[name]; !ok {
			// Addition
			dc, ok := dcMap[name]
			if !ok {
				v.Log.Errorf("DC %v not found in VCenter DC List.", name)
				continue
			}
			v.addDC(dc.Name, dc.Self.Value)
		}
	}

	return
}

func (v *VCHub) addDC(dcName, dcID string) {
	v.Log.Infof("Initializing DC : %v", dcName)

	synced := v.syncDC(dcName, dcID)
	if synced {
		v.probe.StartWatchForDC(dcName, dcID)
		if v.isManagedNamespace(dcName) {
			v.checkNetworks(dcName)
		}
	} else {
		// Enqueue a dummy create event for this DC so that we will retry to sync and start watchers
		retryFn := func() {
			v.Log.Infof("Retry Event: Create DC running")
			dcMap, err := v.probe.GetDCMap()
			if err != nil {
				v.Log.Errorf("Failed to get dc mapping during retry, %s", err)
				return
			}
			name := dcName
			dcObj, ok := dcMap[name]
			if !ok {
				v.Log.Infof("Retry event: DC %s no longer exists, nothing to do", name)
				return
			}

			msg := defs.Probe2StoreMsg{
				MsgType: defs.VCEvent,
				Val: defs.VCEventMsg{
					VcObject:   defs.Datacenter,
					Key:        dcObj.Self.Value,
					Originator: v.VcID,
					Changes: []types.PropertyChange{
						{
							Name: "name",
							Op:   "add",
							Val:  name,
						},
					},
				},
			}
			select {
			case <-v.Ctx.Done():
				return
			case v.vcReadCh <- msg:
			}
		}
		v.Log.Info("Creating state for DC %s failed, pushing to retry queue", dcName)
		v.TimerQ.Add(retryFn, retryDelay)
	}
}

// Check if the given DC(namespace) is managed by this vchub
func (v *VCHub) isManagedNamespace(name string) bool {
	v.DcSpecLock.RLock()
	defer v.DcSpecLock.RUnlock()
	if _, ok := v.ManagedDCs[name]; ok {
		return true
	}
	if _, ok := v.ManagedDCs[utils.ManageAllDcs]; ok {
		return true
	}
	return false
}

func (v *VCHub) isMonitoredNamespace(name string) bool {
	v.DcSpecLock.RLock()
	defer v.DcSpecLock.RUnlock()
	if _, ok := v.MonitoredDCs[name]; ok {
		return true
	}
	if _, ok := v.MonitoredDCs[utils.ManageAllDcs]; ok {
		return true
	}
	return false
}

// CreateVmkWorkloadName returns vmk workload name string
func (v *VCHub) createVmkWorkloadName(namespace, objName string) string {
	// don't include namespace (DC name) in the vmk workload name
	return fmt.Sprintf("%s%s%s", defs.VmkWorkloadPrefix, utils.Delim, v.createHostName("", objName))
}

// createHostName returns host name string
func (v *VCHub) createHostName(namespace, objName string) string {
	// Remove DC name from the host name. On events like workload update, the DC of the event is
	// different from the DC where the host is
	return fmt.Sprintf("%s", utils.CreateGlobalKey(v.OrchID, "" /* namespace */, objName))
}

func (v *VCHub) createVMWorkloadName(namespace, objName string) string {
	// Note: Changing this function may break upgrade compatability.
	// don't include namespace (DC name) in the workload name
	return fmt.Sprintf("%s", utils.CreateGlobalKey(v.OrchID, "", objName))
}

func (v *VCHub) parseVMKeyFromWorkloadName(workloadName string) (vmKey string) {
	return fmt.Sprintf("%s", utils.ParseGlobalKey(v.OrchID, "", workloadName))
}

func (v *VCHub) parseHostKeyFromWorkloadName(hostName string) (hostKey string) {
	return fmt.Sprintf("%s", utils.ParseGlobalKey(v.OrchID, "", hostName))
}

// IsSyncDone return status of the sync
func (v *VCHub) IsSyncDone() bool {
	v.syncLock.RLock()
	syncDone := v.syncDone
	defer v.syncLock.RUnlock()
	return syncDone
}

// AreWatchersStarted return whether watchers have been started
// Should only be used by test programs
func (v *VCHub) AreWatchersStarted() bool {
	return v.watchStarted
}

// GetMode returns the mode for a given DC
func (v *VCHub) GetMode(dc string) defs.Mode {
	managed := v.isManagedNamespace(dc)
	monitored := v.isMonitoredNamespace(dc)
	var mode defs.Mode
	if managed && monitored {
		mode = defs.MonitoringManagedMode
	} else if managed {
		mode = defs.ManagedMode
	} else if monitored {
		mode = defs.MonitoringMode
	}
	return mode
}
