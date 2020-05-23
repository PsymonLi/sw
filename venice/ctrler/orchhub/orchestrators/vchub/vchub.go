package vchub

import (
	"context"
	"fmt"
	"net/url"
	"sync"
	"time"

	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/pcache"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/timerqueue"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
)

const (
	storeQSize          = 64
	defaultRetryCount   = 3
	defaultTagSyncDelay = 2 * time.Minute
	retryDelay          = 30 * time.Second
)

// VCHub instance
type VCHub struct {
	*defs.State
	cancel       context.CancelFunc
	vcOpsChannel chan *kvstore.WatchEvent
	vcReadCh     chan defs.Probe2StoreMsg
	vcEventCh    chan defs.Probe2StoreMsg
	pCache       *pcache.PCache
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
	useMockProbe          bool
	tagSyncDelay          time.Duration
	tagSyncInitializedMap map[string]bool
	orchUpdateLock        sync.Mutex
}

// Option specifies optional values for vchub
type Option func(*VCHub)

// WithMockProbe uses a probe interceptor to handle issues with vcsim
func WithMockProbe(v *VCHub) {
	v.useMockProbe = true
}

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

	if config.Spec.ManageNamespaces == nil ||
		len(config.Spec.ManageNamespaces) == 0 {
		logger.Infof("No DCs specified, no DCs will be managed")
	}

	forceDCMap := map[string]bool{}
	logger.Infof("Forced DC %s: Only events for this(these) DC(s) will be processed", config.Spec.ManageNamespaces)
	for _, dc := range config.Spec.ManageNamespaces {
		forceDCMap[dc] = true
	}

	orchID := fmt.Sprintf("orch%d", config.Status.OrchID)

	// Set connection status as unknown
	o, err := stateMgr.Controller().Orchestrator().Find(&config.ObjectMeta)
	if err != nil {
		logger.Errorf("Orchestrator Object %v does not exist",
			config.GetKey())
	} else {
		v.orchUpdateLock.Lock()
		o.Orchestrator.Status.Status = orchestration.OrchestratorStatus_Unknown.String()
		o.Orchestrator.Status.LastTransitionTime = &api.Timestamp{}
		o.Orchestrator.Status.LastTransitionTime.SetTime(time.Now())
		o.Orchestrator.Status.Message = ""
		o.Write()
		v.orchUpdateLock.Unlock()
	}

	state := defs.State{
		VcURL:        vcURL,
		VcID:         config.GetName(),
		OrchID:       orchID,
		Ctx:          ctx,
		Log:          logger.WithContext("submodule", fmt.Sprintf("VCHub-%s(%s)", config.GetName(), orchID)),
		StateMgr:     stateMgr,
		Wg:           &sync.WaitGroup{},
		DcIDMap:      map[string]types.ManagedObjectReference{}, // Obj name to ID
		DvsIDMap:     map[string]types.ManagedObjectReference{}, // Obj name to ID
		ForceDCNames: forceDCMap,
		TimerQ:       timerqueue.NewQueue(),
		OrchConfig:   ref.DeepCopy(config).(*orchestration.Orchestrator),
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

	v.setupPCache()

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
	probe := vcprobe.NewVCProbe(v.vcReadCh, v.vcEventCh, v.State)
	if v.useMockProbe {
		v.probe = mock.NewProbeMock(probe)
	} else {
		v.probe = probe
	}
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
	v.Log.Infof("VCHub config updated. : Orch : %v", config.ObjectMeta)
	v.reconcileNamespaces(config)

	if v.isCredentialChanged(config) {
		v.Log.Infof("Credentials were updated. Restarting VCHub")
		v.Destroy(false)
		v.setupVCHub(v.StateMgr, config, v.Log, v.opts...)
	} else {
		// If setupVCHub is not called, we have to update the forceDCMaps and config
		forceDCMap := map[string]bool{}
		for _, dc := range config.Spec.ManageNamespaces {
			forceDCMap[dc] = true
		}

		v.ForceDCNamesLock.Lock()
		v.ForceDCNames = forceDCMap
		v.ForceDCNamesLock.Unlock()
		v.OrchConfig = ref.DeepCopy(config).(*orchestration.Orchestrator)
	}
}

// deleteAllDVS cleans up all the PensandoDVS present in the DCs within the VC deployment
func (v *VCHub) deleteAllDVS() {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()

	for _, dc := range v.DcMap {
		if !v.isManagedNamespace(dc.Name) {
			v.Log.Infof("Skipping deletion of DVS from %v.", dc.Name)
			continue
		}

		dc.Lock()
		for _, penDVS := range dc.DvsMap {
			err := v.probe.RemovePenDVS(dc.Name, penDVS.DvsName, defaultRetryCount)
			if err != nil {
				v.Log.Errorf("Failed deleting DVS %v in DC %v. Err : %v", penDVS.DvsName, dc.Name, err)
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
		v.pCache.RevalidateKind(string(workload.KindWorkload))
	case string(cluster.KindHost):
		v.pCache.RevalidateKind(string(cluster.KindHost))
		// Also trigger workloads in case they were relying on a host that just got written
		v.pCache.RevalidateKind(string(workload.KindWorkload))
	}
}

// ListPensandoHosts List only Pensando Hosts from vCenter
func (v *VCHub) ListPensandoHosts(dcRef *types.ManagedObjectReference) []mo.HostSystem {
	hosts := v.probe.ListHosts(dcRef)
	var penHosts []mo.HostSystem
	for _, host := range hosts {
		if !isPensandoHost(host.Config) {
			v.Log.Debugf("Skipping non-Pensando Host %s", host.Name)
			continue
		}
		penHosts = append(penHosts, host)
	}
	return penHosts
}

func (v *VCHub) reconcileNamespaces(config *orchestration.Orchestrator) error {
	v.Log.Infof("Reconciling namespaces for orchestrator [%v]", config.Name)
	// Cleanup and additions are only needed if we are connected to vCenter
	if !v.probe.IsSessionReady() {
		v.Log.Infof("Reconciling namespaces skipped because probe isn't ready")
		return nil
	}

	// If sync is running, we don't want to modify state until it is done.
	v.syncLock.RLock()
	defer v.syncLock.RUnlock()

	managedNamespaces := v.getManagedNamespaceList()
	newManagedNamespaces := []string{}

	// If url is different, then we have all deletes.
	if config.Spec.URI == v.OrchConfig.Spec.URI {
		if len(config.Spec.ManageNamespaces) == 1 && config.Spec.ManageNamespaces[0] == utils.ManageAllDcs {
			for k := range v.probe.GetDCMap() {
				newManagedNamespaces = append(newManagedNamespaces, k)
			}
		} else {
			newManagedNamespaces = config.Spec.ManageNamespaces
		}
	}

	addedNs, deletedNs, nochangeNs := utils.DiffNamespace(managedNamespaces, newManagedNamespaces)
	if len(addedNs) == 0 && len(deletedNs) == 0 {
		v.Log.Info("No namespaces to reconcile")
		return nil
	}

	if len(nochangeNs) == 0 && len(addedNs) == 0 {
		v.Log.Infof("All namespaces associated with the orchestrator object have been deleted")
	}

	// Cleanup DCs no longer managed by OrchHub
	for _, ns := range deletedNs {
		v.Log.Infof("Cleaning up DC : %v", ns)
		v.RemovePenDC(ns)
	}

	if len(addedNs) > 0 {
		dcMap := v.probe.GetDCMap()
		for _, ns := range addedNs {
			dc, ok := dcMap[ns]
			if !ok {
				v.Log.Errorf("DC %v not found in VCenter DC List.", ns)
				continue
			}

			v.Log.Infof("Initializing DC : %v", dc.Name)
			_, err := v.NewPenDC(dc.Name, dc.Self.Value)
			if err == nil {
				v.probe.StartWatchForDC(dc.Name, dc.Self.Value)
				v.checkNetworks(dc.Name)
			} else {
				retryFn := func() {
					v.Log.Infof("Retry Event: Create DC running")
					name := dc.Name
					dcs := v.probe.GetDCMap()
					dcObj, ok := dcs[name]
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
								types.PropertyChange{
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
				v.Log.Info("Creating state for DC %s failed, pushing to retry queue", dc.Name)
				v.TimerQ.Add(retryFn, retryDelay)
			}
		}
	}

	return nil
}

func (v *VCHub) getManagedNamespaceList() []string {
	nsList := []string{}
	v.ForceDCNamesLock.RLock()
	defer v.ForceDCNamesLock.RUnlock()

	if ok := v.ForceDCNames[utils.ManageAllDcs]; ok {
		for k := range v.probe.GetDCMap() {
			nsList = append(nsList, k)
		}
	} else {
		for k := range v.ForceDCNames {
			nsList = append(nsList, k)
		}
	}

	return nsList
}

// Check if the given DC(namespace) is managed by this vchub
func (v *VCHub) isManagedNamespace(name string) bool {
	v.ForceDCNamesLock.RLock()
	defer v.ForceDCNamesLock.RUnlock()
	if _, ok := v.ForceDCNames[name]; ok {
		return true
	}
	if _, ok := v.ForceDCNames[utils.ManageAllDcs]; ok {
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

// IsSyncDone return status of the sync - used by test programs only
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
