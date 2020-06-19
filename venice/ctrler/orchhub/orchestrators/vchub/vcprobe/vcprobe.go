package vcprobe

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/property"
	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/session"
)

const (
	retryDelay = 500 * time.Millisecond
)

var vmProps = []string{"config", "name", "runtime", "guest"}

type dcCtxEntry struct {
	ctx    context.Context
	cancel context.CancelFunc
}

// VCProbe represents an instance of a vCenter Interface
// This is comprised of a SOAP interface and a REST interface
type VCProbe struct {
	*defs.State
	*session.Session
	TagsProbeInf
	probeCtx     context.Context
	probeCancel  context.CancelFunc
	ProbeWg      sync.WaitGroup
	outbox       chan<- defs.Probe2StoreMsg
	eventCh      chan<- defs.Probe2StoreMsg
	Started      bool
	vcProbeLock  sync.Mutex
	dcCtxMapLock sync.Mutex
	// Map from ID to ctx to use
	dcCtxMap         map[string]dcCtxEntry
	eventInitialized map[string]bool // whether event processing is initialized for a given vc object (datacenter)
	eventTrackerLock sync.Mutex
}

// KindTagMapEntry needed as explicit type for gomock to compile successfully
type KindTagMapEntry map[string][]string

// TagsProbeInf is the interface Probe implements
type TagsProbeInf interface {
	// Tag methods
	// Read methods
	StartWatch()
	SetupBaseTags() bool
	GetPensandoTagsOnObjects(refs []types.ManagedObjectReference) (map[string]KindTagMapEntry, error)
	ResyncVMTags(string) (defs.Probe2StoreMsg, error)

	// Write methods
	TagObjsAsManaged(refs []types.ManagedObjectReference) error
	TagObjAsManaged(ref types.ManagedObjectReference) error
	RemoveTagObjManaged(ref types.ManagedObjectReference) error
	TagObjWithVlan(ref types.ManagedObjectReference, vlan int) error
	RemoveTagObjVlan(ref types.ManagedObjectReference) error
	RemoveTag(ref types.ManagedObjectReference, tagName string) error
	RemovePensandoTags(ref types.ManagedObjectReference) []error
	DestroyTags() error

	// Utility
	IsManagedTag(tag string) bool
	IsVlanTag(tag string) (int, bool)
	GetWriteTags() map[string]string
	SetWriteTags(map[string]string)
}

// IsPGConfigEqual checks whether the create config is equal to the existing config
type IsPGConfigEqual func(spec *types.DVPortgroupConfigSpec, config *mo.DistributedVirtualPortgroup) bool

// DVSConfigDiff returns the diff of the two configs that should be written back to vcenter, or nil if there is none
type DVSConfigDiff func(spec *types.DVSCreateSpec, config *mo.DistributedVirtualSwitch) *types.DVSCreateSpec

// ProbeInf is the interface Probe implements
type ProbeInf interface {
	// Start runs the probe.
	Start() error
	Stop()
	IsSessionReady() bool
	StartWatchers()
	ListVM(dcRef *types.ManagedObjectReference) ([]mo.VirtualMachine, error)
	ListDC() ([]mo.Datacenter, error)
	GetDCMap() (map[string]mo.Datacenter, error)
	ListDVS(dcRef *types.ManagedObjectReference) ([]mo.VmwareDistributedVirtualSwitch, error)
	ListPG(dcRef *types.ManagedObjectReference) ([]mo.DistributedVirtualPortgroup, error)
	ListHosts(dcRef *types.ManagedObjectReference) ([]mo.HostSystem, error)
	StopWatchForDC(dcName, dcID string)
	StartWatchForDC(dcName, dcID string)
	GetVM(vmID string, retry int) (mo.VirtualMachine, error)
	WithSession(func(context.Context) (interface{}, error)) (interface{}, error)

	// datacenter.go functions
	AddPenDC(dcName string, retry int) error
	RenameDC(oldName string, newName string, retry int) error
	RemovePenDC(dcName string, retry int) error

	// port_group.go functions
	AddPenPG(dcName, dvsName string, pgConfigSpec *types.DVPortgroupConfigSpec, equalFn IsPGConfigEqual, retry int) error

	GetPenPG(dcName string, pgName string, retry int) (*object.DistributedVirtualPortgroup, error)

	GetPGConfig(dcName string, pgName string, ps []string, retry int) (*mo.DistributedVirtualPortgroup, error)
	RenamePG(dcName, oldName string, newName string, retry int) error
	RemovePenPG(dcName, pgName string, retry int) error

	// distributed_vswitch.go functions
	AddPenDVS(dcName string, dvsCreateSpec *types.DVSCreateSpec, equalFn DVSConfigDiff, retry int) error
	RenameDVS(dcName, oldName string, newName string, retry int) error
	GetPenDVS(dcName, dvsName string, retry int) (*object.DistributedVirtualSwitch, error)
	UpdateDVSPortsVlan(dcName, dvsName string, portsSetting PenDVSPortSettings, forceWrite bool, retry int) error
	GetPenDVSPorts(dcName, dvsName string, criteria *types.DistributedVirtualSwitchPortCriteria, retry int) ([]types.DistributedVirtualPort, error)
	RemovePenDVS(dcName, dvsName string, retry int) error

	IsREST401(error) bool

	// Tag methods
	TagsProbeInf
}

// NewVCProbe returns a new probe
func NewVCProbe(hOutbox, hEventCh chan<- defs.Probe2StoreMsg, state *defs.State) *VCProbe {
	probeCtx, probeCancel := context.WithCancel(state.Ctx)
	probe := &VCProbe{
		State:            state,
		Started:          false,
		probeCtx:         probeCtx,
		probeCancel:      probeCancel,
		outbox:           hOutbox,
		eventCh:          hEventCh,
		Session:          session.NewSession(probeCtx, hOutbox, state.VcURL, state.Log, state.OrchConfig),
		dcCtxMap:         map[string]dcCtxEntry{},
		eventInitialized: make(map[string]bool),
	}
	probe.newTagsProbe()
	return probe
}

// Start creates a client and view manager
// vcWriteOnly indicates that this probe is only used for writing to vcenter
func (v *VCProbe) Start() error {
	v.Log.Info("Starting probe")
	v.vcProbeLock.Lock()
	if v.Started {
		v.vcProbeLock.Unlock()
		return errors.New("Already Started")
	}
	v.Started = true
	v.ProbeWg.Add(1)
	go v.PeriodicSessionCheck(&v.ProbeWg)

	v.vcProbeLock.Unlock()

	return nil
}

// Stop stops the probe
func (v *VCProbe) Stop() {
	v.Log.Info("Stopping vcprobe...")
	v.vcProbeLock.Lock()
	v.Started = false
	v.probeCancel()
	v.ProbeWg.Wait()
	v.Log.Info("vcprobe wg stopped")
	v.ClearSession()
	v.vcProbeLock.Unlock()
	v.Log.Info("vcprobe stopped")
}

// If fn returns false, the function will not retry
func (v *VCProbe) tryForever(fn func() bool) {
	err := v.ReserveClient()
	if err != nil {
		return
	}
	go func() {
		defer v.ReleaseClient()

		for {
			ctx := v.ClientCtx
			if ctx == nil {
				return
			}
			if ctx.Err() != nil {
				return
			}
			retry := fn()
			if !retry {
				return
			}
			v.Log.Infof("try forever exited, retrying...")
			time.Sleep(retryDelay)
		}
	}()
}

// StartWatchers starts the watches for vCenter objects
func (v *VCProbe) StartWatchers() {
	if !v.Started || !v.IsSessionReady() {
		return
	}
	v.Log.Infof("Start Watchers starting")

	// Adding to watcher group so that clientCtx will be valid when accessed
	v.WatcherWg.Add(1)
	go func() {
		defer v.WatcherWg.Done()
		if !v.Started || !v.IsSessionReady() {
			return
		}
		ctx := v.ClientCtx
		// ClientCtx can potentially be nil since we haven't got the clientLock.
		for ctx != nil && ctx.Err() == nil {

			// DC watch
			v.tryForever(func() bool {
				v.Log.Infof("DC watch Started")
				v.startWatch(ctx, defs.Datacenter, []string{"name"},
					v.sendVCEvent, nil)
				return true
			})

			v.tryForever(func() bool {
				v.TagsProbeInf.StartWatch()
				v.Log.Infof("tag probe finished")
				return true
			})
			select {
			case <-ctx.Done():
			}

		}

	}()

	return
}

// WithSession ensures the given function runs till completion before
// tearing down the session.
func (v *VCProbe) WithSession(fn func(ctx context.Context) (interface{}, error)) (interface{}, error) {
	if !v.Started || !v.IsSessionReady() {
		return nil, fmt.Errorf("Session is not ready")
	}
	v.WatcherWg.Add(1)
	defer v.WatcherWg.Done()
	return fn(v.ClientCtx)
}

// StopWatchForDC terminates DC level watchers
func (v *VCProbe) StopWatchForDC(dcName, dcID string) {
	v.dcCtxMapLock.Lock()
	defer v.dcCtxMapLock.Unlock()
	// Cancel watch for this DC
	ctxEntry, ok := v.dcCtxMap[dcID]
	if !ok {
		v.Log.Infof("DC %s removed, but found no active watchers", dcID)
	} else {
		v.Log.Infof("Canceling watchers for DC %s", dcID)
		ctxEntry.cancel()
		delete(v.dcCtxMap, dcID)
	}

	return
}

// StartWatchForDC starts DC level watchers
func (v *VCProbe) StartWatchForDC(dcName, dcID string) {
	v.dcCtxMapLock.Lock()
	defer v.dcCtxMapLock.Unlock()
	if ctxEntry, ok := v.dcCtxMap[dcID]; ok && ctxEntry.ctx.Err() == nil {
		v.Log.Infof("DC watchers already running for DC %s", dcName)
		// watchers already exist
		return
	}

	if !v.IsSessionReady() {
		return
	}

	// Starting watches on objects inside the given DC
	clientCtx := v.ClientCtx
	if clientCtx == nil {
		return
	}
	ctx, cancel := context.WithCancel(clientCtx)
	v.dcCtxMap[dcID] = dcCtxEntry{ctx, cancel}

	ref := types.ManagedObjectReference{
		Type:  string(defs.Datacenter),
		Value: dcID,
	}

	v.tryForever(func() bool {
		v.Log.Infof("Host watch Started on DC %s", dcName)
		v.startWatch(ctx, defs.HostSystem, []string{"config", "name"}, v.vcEventHandlerForDC(dcID, dcName), &ref)
		if ctx.Err() != nil {
			v.Log.Infof("Host watch exiting on DC %s", dcName)
			return false
		}
		return true
	})

	v.tryForever(func() bool {
		v.Log.Infof("VM watch Started on DC %s", dcName)
		v.startWatch(ctx, defs.VirtualMachine, vmProps, v.vcEventHandlerForDC(dcID, dcName), &ref)
		if ctx.Err() != nil {
			v.Log.Infof("VM watch exiting on DC %s", dcName)
			return false
		}
		return true
	})

	v.tryForever(func() bool {
		v.Log.Infof("PG watch Started")
		v.startWatch(ctx, defs.DistributedVirtualPortgroup, []string{"config"}, v.vcEventHandlerForDC(dcID, dcName), &ref)
		if ctx.Err() != nil {
			v.Log.Infof("PG watch exiting on DC %s", dcName)
			return false
		}
		return true
	})

	v.tryForever(func() bool {
		v.Log.Debugf("DVS watch started")
		v.startWatch(ctx, defs.VmwareDistributedVirtualSwitch, []string{"name", "config"}, v.vcEventHandlerForDC(dcID, dcName), &ref)
		if ctx.Err() != nil {
			v.Log.Infof("DVS watch exiting on DC %s", dcName)
			return false
		}
		return true
	})

	err := v.ReserveClient()
	if err != nil {
		return
	}
	go func() {
		defer v.ReleaseClient()
		v.runEventReceiver(ref)
	}()
}

func (v *VCProbe) startWatch(ctx context.Context, vcKind defs.VCObject, props []string, updateFn func(objUpdate types.ObjectUpdate, kind defs.VCObject), container *types.ManagedObjectReference) {
	v.Log.Debugf("Start watch called for %s", vcKind)
	kind := string(vcKind)

	var err error
	client, viewMgr, _ := v.GetClients()

	root := client.ServiceContent.RootFolder
	if container == nil {
		container = &root
	}
	kinds := []string{}

	cView, err := viewMgr.CreateContainerView(ctx, *container, kinds, true)
	if err != nil {
		v.Log.Errorf("CreateContainerView returned %v", err)
		v.CheckSession = true
		time.Sleep(1 * time.Second)
		return
	}

	// Fields to watch for change
	objRef := types.ManagedObjectReference{Type: kind}
	filter := new(property.WaitFilter).Add(cView.Reference(), objRef.Type, props, cView.TraversalSpec())
	updFunc := func(updates []types.ObjectUpdate) bool {
		for _, update := range updates {
			if update.Obj.Type != kind {
				v.Log.Errorf("Expected %s, got %+v", kind, update.Obj)
				continue
			}
			updateFn(update, vcKind)
		}
		// Must return false, returning true will cause waitForUpdates to exit.
		return false
	}

	for {
		err = property.WaitForUpdates(ctx, property.DefaultCollector(client.Client), filter, updFunc)

		if err != nil {
			v.Log.Errorf("property.WaitForView returned %v", err)
		}

		if ctx.Err() != nil {
			return
		}
		v.Log.Infof("%s property.WaitForView exited", kind)
		v.CheckSession = true
		time.Sleep(1 * time.Second)
	}
}

func (v *VCProbe) sendVCEvent(update types.ObjectUpdate, kind defs.VCObject) {

	key := update.Obj.Value
	m := defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val: defs.VCEventMsg{
			VcObject:   kind,
			Key:        key,
			Changes:    update.ChangeSet,
			UpdateType: update.Kind,
			Originator: v.VcID,
		},
	}
	v.Log.Debugf("Sending message to store, key: %s, obj: %s, props: %+v", key, kind, update.ChangeSet)
	if v.outbox != nil {
		select {
		case <-v.probeCtx.Done():
			return
		case v.outbox <- m:
		}
	}
}

func (v *VCProbe) vcEventHandlerForDC(dcID string, dcName string) func(update types.ObjectUpdate, kind defs.VCObject) {
	return func(update types.ObjectUpdate, kind defs.VCObject) {
		key := update.Obj.Value
		m := defs.Probe2StoreMsg{
			MsgType: defs.VCEvent,
			Val: defs.VCEventMsg{
				VcObject:   kind,
				DcID:       dcID,
				DcName:     dcName,
				Key:        key,
				Changes:    update.ChangeSet,
				UpdateType: update.Kind,
				Originator: v.VcID,
			},
		}
		v.Log.Debugf("Sending message to store, key: %s, obj: %s", key, kind)
		if v.outbox != nil {
			select {
			case <-v.probeCtx.Done():
				return
			case v.outbox <- m:
			}
		}
	}
}

// ListObj performs a list operation in vCenter
func (v *VCProbe) ListObj(vcKind defs.VCObject, props []string, dst interface{}, container *types.ManagedObjectReference) error {
	err := v.ReserveClient()
	if err != nil {
		return err
	}
	defer v.ReleaseClient()
	client, viewMgr, _ := v.GetClients()

	root := client.ServiceContent.RootFolder

	if container == nil {
		container = &root
	}
	kinds := []string{}
	cView, err := viewMgr.CreateContainerView(v.probeCtx, *container, kinds, true)
	if err != nil {
		v.CheckSession = true
		return err
	}
	defer cView.Destroy(v.probeCtx)
	err = cView.Retrieve(v.probeCtx, []string{string(vcKind)}, props, dst)
	return err
}

// ListVM returns a list of vms
func (v *VCProbe) ListVM(dcRef *types.ManagedObjectReference) ([]mo.VirtualMachine, error) {
	var vms []mo.VirtualMachine
	err := v.ListObj(defs.VirtualMachine, vmProps, &vms, dcRef)
	return vms, err
}

// ListDC returns a list of DCs
func (v *VCProbe) ListDC() ([]mo.Datacenter, error) {
	var dcs []mo.Datacenter
	err := v.ListObj(defs.Datacenter, []string{"name"}, &dcs, nil)
	return dcs, err
}

// GetDCMap returns a DC Name to DC map
func (v *VCProbe) GetDCMap() (map[string]mo.Datacenter, error) {
	dcMap := make(map[string]mo.Datacenter)

	dcList, err := v.ListDC()
	if err != nil {
		return dcMap, err
	}
	for _, dc := range dcList {
		dcMap[dc.Name] = dc
	}

	return dcMap, nil
}

// ListDVS returns a list of DVS objects
func (v *VCProbe) ListDVS(dcRef *types.ManagedObjectReference) ([]mo.VmwareDistributedVirtualSwitch, error) {
	var dcs []mo.VmwareDistributedVirtualSwitch
	// any properties changed here need to be changed in probe's mock.go
	err := v.ListObj(defs.VmwareDistributedVirtualSwitch, []string{"name", "config"}, &dcs, dcRef)
	return dcs, err
}

// ListPG returns a list of PGs
func (v *VCProbe) ListPG(dcRef *types.ManagedObjectReference) ([]mo.DistributedVirtualPortgroup, error) {
	var dcs []mo.DistributedVirtualPortgroup
	err := v.ListObj(defs.DistributedVirtualPortgroup, []string{"config", "name", "tag"}, &dcs, dcRef)
	return dcs, err
}

// ListHosts returns a list of Hosts
func (v *VCProbe) ListHosts(dcRef *types.ManagedObjectReference) ([]mo.HostSystem, error) {
	var hosts []mo.HostSystem
	err := v.ListObj(defs.HostSystem, []string{"config", "name"}, &hosts, dcRef)
	return hosts, err
}

// GetVM fetches the given VM by ID
func (v *VCProbe) GetVM(vmID string, retry int) (mo.VirtualMachine, error) {
	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()
		vmRef := types.ManagedObjectReference{
			Type:  string(defs.VirtualMachine),
			Value: vmID,
		}
		var vm mo.VirtualMachine
		objVM := object.NewVirtualMachine(client.Client, vmRef)
		err = objVM.Properties(v.ClientCtx, vmRef, vmProps, &vm)
		return vm, err
	}
	ret, err := v.withRetry(fn, retry)
	if ret == nil {
		return mo.VirtualMachine{}, err
	}
	return ret.(mo.VirtualMachine), err
}

// // TagObjAsManaged tags the given ref with a Pensando managed tag
// func (v *VCProbe) TagObjAsManaged(ref types.ManagedObjectReference) error {
// 	return v.tp.TagObjAsManaged(ref)
// }

// func (v *VCProbe) SetupBaseTags() {
// 	v.tp.SetupBaseTags()
// }

// // RemoveTagObjManaged removes the pensando managed tag
// func (v *VCProbe) RemoveTagObjManaged(ref types.ManagedObjectReference) error {
// 	return v.tp.RemoveTagObjManaged(ref)
// }

// // TagObjWithVlan tags the object with the given vlan value
// func (v *VCProbe) TagObjWithVlan(ref types.ManagedObjectReference, vlanValue int) error {
// 	return v.tp.TagObjWithVlan(ref, vlanValue)
// }

// // RemoveTagObjVlan removes the vlan tag on the given object
// func (v *VCProbe) RemoveTagObjVlan(ref types.ManagedObjectReference) error {
// 	return v.tp.RemoveTagObjVlan(ref)
// }

// // RemovePensandoTags removes all pensando tags
// func (v *VCProbe) RemovePensandoTags(ref types.ManagedObjectReference) []error {
// 	return v.tp.RemovePensandoTags(ref)
// }

func (v *VCProbe) withRetry(fn func() (interface{}, error), count int) (interface{}, error) {
	if count < 1 {
		return nil, fmt.Errorf("Supplied invalid count")
	}
	i := 0
	var ret interface{}
	err := fmt.Errorf("Client Ctx cancelled")
	ctx := v.ClientCtx
	for i < count && ctx != nil && ctx.Err() == nil {
		ret, err = fn()
		if err == nil {
			return ret, err
		}
		i++
		if i < count {
			v.Log.Errorf("oper failed: %s, retrying...", err)
			v.CheckSession = true
		} else if i == count {
			return ret, err
		}
		select {
		case <-ctx.Done():
			return nil, err
		case <-time.After(retryDelay):
		}
	}
	return nil, err
}

func (v *VCProbe) initEventTracker(ref types.ManagedObjectReference) {
	v.eventTrackerLock.Lock()
	defer v.eventTrackerLock.Unlock()
	v.Log.Infof("Start Event Receiver for %s", ref.Value)
	v.eventInitialized[ref.Value] = false
}

func (v *VCProbe) deleteEventTracker(ref types.ManagedObjectReference) {
	v.eventTrackerLock.Lock()
	defer v.eventTrackerLock.Unlock()
	if _, ok := v.eventInitialized[ref.Value]; ok {
		v.Log.Infof("Stop event receiver for %s", ref.Value)
		delete(v.eventInitialized, ref.Value)
	}
}

func (v *VCProbe) runEventReceiver(ref types.ManagedObjectReference) {

	v.dcCtxMapLock.Lock()
	dcID := ref.Value
	ctxEntry, ok := v.dcCtxMap[dcID]
	v.dcCtxMapLock.Unlock()
	if !ok {
		v.Log.Infof("No context for DC %s", dcID)
		return
	}
	ctx := ctxEntry.ctx
releaseEventTracker:
	for ctx.Err() == nil {
		for !v.SessionReady {
			select {
			case <-ctx.Done():
				break releaseEventTracker
			case <-time.After(50 * time.Millisecond):
			}
		}
		eventMgr := v.GetEventManager()
		if eventMgr == nil {
			break releaseEventTracker
		}
		// Events() will deliver the last N events, and then will block and call receiveEvents on new events.
		// TODO there should be a way to request events newer than a eventId, not clear if that is
		// supported
		v.initEventTracker(ref)
		eventMgr.Events(ctx, []types.ManagedObjectReference{ref}, 10, true, false, v.receiveEvents)
		v.deleteEventTracker(ref)
		if ctx.Err() != nil {
			break releaseEventTracker
		}
		time.Sleep(500 * time.Millisecond)
		v.Log.Infof("Event Receiver for DC %s restarting", dcID)
	}
	v.Log.Infof("Event Receiver for DC %s Exited", dcID)
}

func (v *VCProbe) receiveEvents(ref types.ManagedObjectReference, events []types.BaseEvent) error {
	v.eventTrackerLock.Lock()
	if init, ok := v.eventInitialized[ref.Value]; !ok || !init {
		v.eventInitialized[ref.Value] = true
		v.eventTrackerLock.Unlock()
		// Skip these events
		v.Log.Infof("Initialized event receiver for %s", ref.Value)
		return nil
	}
	v.eventTrackerLock.Unlock()

	// This executes in the context of go routine
	// I think events are ordered as latest event first.. we should process them from back to front
	// and process only new ones
	if len(events) == 0 {
		return nil
	}
	for i := len(events) - 1; i >= 0; i-- {
		ev := events[i]
		// Capture Vmotion events -
		// VmBeingMitrages/Relocated are received at the start of vMotion (about 2sec delay obsered
		// 	on lightly loaded VC/Venice).
		// VmMigrated/Relocated event is received on successful vmotion. Also VM watcher is triggered
		// as VM's runtime info now points to new host. So there is no additinal processing to be
		// done on Migrated event.
		// RelocateFailedEvents are received when vmotion fails or cancelled. In this case there is
		// no workload watch event, as there is no change done to workload.

		// Ignore all events that do not point to Pen-DVS, e.Dvs.Dvs is not populated
		switch e := ev.(type) {
		case *types.VmEmigratingEvent:
			// This event does not contain useful information, but it is generated
			// after migration start event.. it may be a good time assume that traffic on
			// host1 has stopped at this time and change the vlan-overrides ?? Not used rightnow
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
		case *types.VmBeingHotMigratedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, e, e.Vm.Vm.Value)
			msg := defs.VMotionStartMsg{
				VMKey:        e.Vm.Vm.Value,
				DstHostKey:   e.DestHost.Host.Value,
				DcID:         ref.Value,
				HotMigration: true,
			}
			v.generateMigrationEvent(defs.VMotionStart, msg)
		case *types.VmBeingMigratedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionStartMsg{
				VMKey:      e.Vm.Vm.Value,
				DstHostKey: e.DestHost.Host.Value,
				DcID:       ref.Value,
			}
			v.generateMigrationEvent(defs.VMotionStart, msg)
		case *types.VmBeingRelocatedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionStartMsg{
				VMKey:      e.Vm.Vm.Value,
				DstHostKey: e.DestHost.Host.Value,
				DcID:       ref.Value,
			}
			v.generateMigrationEvent(defs.VMotionStart, msg)
		case *types.VmMigratedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionDoneMsg{
				VMKey:      e.Vm.Vm.Value,
				SrcHostKey: e.SourceHost.Host.Value,
				DcID:       ref.Value,
			}
			v.generateMigrationEvent(defs.VMotionDone, msg)
		case *types.VmRelocatedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionDoneMsg{
				VMKey:      e.Vm.Vm.Value,
				SrcHostKey: e.SourceHost.Host.Value,
				DcID:       ref.Value,
			}
			v.generateMigrationEvent(defs.VMotionDone, msg)
		case *types.VmRelocateFailedEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionFailedMsg{
				VMKey:      e.Vm.Vm.Value,
				DstHostKey: e.DestHost.Host.Value,
				DcID:       ref.Value,
				Reason:     e.Reason.LocalizedMessage,
			}
			v.generateMigrationEvent(defs.VMotionFailed, msg)
		case *types.VmFailedMigrateEvent:
			v.Log.Infof("Event %d - %s - %T for VM %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value)
			msg := defs.VMotionFailedMsg{
				VMKey:      e.Vm.Vm.Value,
				DstHostKey: e.DestHost.Host.Value,
				DcID:       ref.Value,
				Reason:     e.Reason.LocalizedMessage,
			}
			v.generateMigrationEvent(defs.VMotionFailed, msg)
		case *types.MigrationEvent:
			// This may be a generic vMotion failure.. but it does not have DestHost Info
			v.Log.Infof("Event %d - %s - %T for VM %s reason %s", e.GetEvent().Key, ref.Value, e, e.Vm.Vm.Value, e.Fault.LocalizedMessage)
			msg := defs.VMotionFailedMsg{
				VMKey:  e.Vm.Vm.Value,
				DcID:   ref.Value,
				Reason: e.Fault.LocalizedMessage,
			}
			v.generateMigrationEvent(defs.VMotionFailed, msg)
		case *types.EventEx:
			s := strings.Split(e.EventTypeId, ".")
			evType := s[len(s)-1]
			if evType == "VmHotMigratingWithEncryptionEvent" {
				v.Log.Infof("EventEx %d - %s - TypeId %s", e.GetEvent().Key, ref.Value, e.EventTypeId)
				msg := defs.VMotionStartMsg{
					VMKey:        e.Vm.Vm.Value,
					DcID:         ref.Value,
					HotMigration: true,
				}
				argsMap := map[string]int{}
				for i, kvarg := range e.Arguments {
					argsMap[kvarg.Key] = i
				}
				if i, ok := argsMap["destHost"]; ok {
					msg.DstHostName, _ = e.Arguments[i].Value.(string)
				}
				if i, ok := argsMap["destDatacenter"]; ok {
					msg.DstDcName, _ = e.Arguments[i].Value.(string)
				}
				v.generateMigrationEvent(defs.VMotionStart, msg)
			} else {
				// v.Log.Debugf("Ignored EventEx %d - %s - TypeId %s...", e.GetEvent().Key, ref.Value, e.EventTypeId)
			}
		case *types.ExtendedEvent:
			// v.Log.Debugf("Ignored ExtendedEvent %d - TypeId %s ...", e.GetEvent().Key, ref.Value, e.EventTypeId)
		default:
			// v.Log.Debugf("Ignored Event %d - %s - %T received ... (+%v)", ev.GetEvent().Key, ref.Value, ev, ev)
			// v.Log.Debugf("Ignored Event %d - %s - %T received ...", ev.GetEvent().Key, ref.Value, ev)
		}
	}
	return nil
}

func (v *VCProbe) generateMigrationEvent(msgType defs.VCNotificationType, msg interface{}) {
	if v.eventCh == nil {
		return
	}

	var m defs.Probe2StoreMsg
	switch msgType {
	case defs.VMotionStart:
		m = defs.Probe2StoreMsg{
			MsgType: defs.VCNotification,
			Val: defs.VCNotificationMsg{
				Type: msgType,
				Msg:  msg.(defs.VMotionStartMsg),
			},
		}
	case defs.VMotionFailed:
		m = defs.Probe2StoreMsg{
			MsgType: defs.VCNotification,
			Val: defs.VCNotificationMsg{
				Type: msgType,
				Msg:  msg.(defs.VMotionFailedMsg),
			},
		}
	case defs.VMotionDone:
		m = defs.Probe2StoreMsg{
			MsgType: defs.VCNotification,
			Val: defs.VCNotificationMsg{
				Type: msgType,
				Msg:  msg.(defs.VMotionDoneMsg),
			},
		}
	default:
		v.Log.Infof("Migration event received unknown msg type %s", msgType)
		return
	}

	select {
	case <-v.probeCtx.Done():
		return
	case v.eventCh <- m:
	}
}
