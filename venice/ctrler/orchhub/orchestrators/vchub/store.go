package vchub

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"time"

	"github.com/vmware/govmomi/vim25/soap"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/mock"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe/session"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
)

func (v *VCHub) startEventsListener() {
	v.Log.Infof("Running store")
	v.Log.Infof("Starting probe channel watch for %s", v.OrchConfig.Name)
	defer v.Wg.Done()
	apiServerChQ, err := v.StateMgr.GetProbeChannel(v.OrchConfig.GetName())
	if err != nil {
		v.Log.Errorf("Could not get probe channel for %s. Err : %v", v.OrchConfig.GetKey(), err)
		return
	}
	apiServerCh := apiServerChQ.ReadCh()

	for {
		select {
		case <-v.Ctx.Done():
			return

		case m, active := <-v.vcEventCh:
			if !active {
				return
			}
			switch m.MsgType {
			case defs.VCNotification:
				// These are notifications from vcenter's EventManager
				v.handleVCNotification(m.Val.(defs.VCNotificationMsg))
			default:
				v.Log.Errorf("Unknown event %s", m.MsgType)
			}
		case m, active := <-v.vcReadCh:
			if !active {
				return
			}
			switch m.MsgType {
			case defs.VCEvent:
				// These are watch events
				v.handleVCEvent(m.Val.(defs.VCEventMsg))
			case defs.RetryEvent:
				v.handleRetryEvent(m.Val.(defs.RetryMsg))
			case defs.VCConnectionStatus:
				// we've reconnected, trigger sync
				connStatus := m.Val.(session.ConnectionState)
				o, err := v.StateMgr.Controller().Orchestrator().Find(&v.OrchConfig.ObjectMeta)
				// Connection transisitions
				// only healthy can go to degraded.
				if err != nil {
					// orchestrator object does not exists anymore
					// this should never happen
					v.Log.Errorf("Orchestrator Object %v does not exist",
						v.OrchConfig.GetKey())
					return
				}

				previousState := v.OrchConfig.Status.Status
				previousMsg := v.OrchConfig.Status.Message

				v.Log.Infof("Updating orchestrator connection status to %v", connStatus.State)

				var evt eventtypes.EventType
				var msg string
				var evtMsg string
				if connStatus.State == orchestration.OrchestratorStatus_Success.String() {
					// Degraded -> Success should not resync
					if previousState == orchestration.OrchestratorStatus_Degraded.String() ||
						previousState == orchestration.OrchestratorStatus_Success.String() {
						if v.IsSyncDone() {
							break
						}
						// Sync was not completed on last connection event.
					}

					// Check if this is vcsim
					if connStatus.IsVCSim {
						if probe, ok := v.probe.(*vcprobe.VCProbe); ok {
							// Replace with mock
							v.Log.Infof("Wrapping probe with mock probe")
							v.probe = mock.NewProbeMock(probe)
						}
					}

					// sync and start watchers, network event watcher
					// will not start until after sync finishes (blocked on processVeniceEvents flag)
					v.discoveredDCsLock.Lock()
					v.discoveredDCs = []string{}
					v.discoveredDCsLock.Unlock()

					// Sync is guaranteed to be the first function to run after connection success
					// Connection will not be torn down before this point.
					// We keep attempting to sync until successful or until the session
					// is being torn down, in which case another connection event will be
					// sent, triggering sync
					_, err := v.probe.WithSession(func(ctx context.Context) (interface{}, error) {
						synced := v.sync()
						for !synced && ctx.Err() == nil {
							select {
							case <-ctx.Done():
							case <-time.After(5 * time.Second):
								synced = v.sync()
							}
						}
						return synced, ctx.Err()
					})
					if err != nil {
						// Session is being rebuilt now.
						return
					}
					v.probe.StartWatchers()

					v.watchStarted = true

				} else if connStatus.State == orchestration.OrchestratorStatus_Failure.String() {
					evt = eventtypes.ORCH_CONNECTION_ERROR
					msg = connStatus.Err.Error()
					evtMsg = fmt.Sprintf("%v : Connection issues with orchestrator. %s", v.OrchConfig.Name, msg)
					// Generate event depending on error type
					if strings.HasPrefix(msg, utils.UnsupportedVersionMsg) {
						evt = eventtypes.ORCH_UNSUPPORTED_VERSION
					} else if soap.IsSoapFault(connStatus.Err) {
						// Check if it is a credential issue
						soapErr := soap.ToSoapFault(connStatus.Err)
						msg = soapErr.String
						evtMsg = fmt.Sprintf("%v : Connection issues with orchestrator. %s", v.OrchConfig.Name, msg)
						if _, ok := soapErr.Detail.Fault.(types.InvalidLogin); ok {
							// Login error
							evt = eventtypes.ORCH_LOGIN_FAILURE
						}
					}
					// Stop acting on network events from venice until reconnect sync
					// Lock must be taken in case periodic sync runs
					v.processVeniceEventsLock.Lock()
					v.processVeniceEvents = false
					v.processVeniceEventsLock.Unlock()
					v.syncLock.Lock()
					v.syncDone = false
					v.syncLock.Unlock()
				} else if connStatus.State == orchestration.OrchestratorStatus_Degraded.String() {
					evt = eventtypes.ORCH_CONNECTION_ERROR
					msg = connStatus.Err.Error()
					evtMsg = fmt.Sprintf("%v : Connection issues with the orchestrator. %s", v.OrchConfig.Name, msg)
					if v.probe.IsREST401(connStatus.Err) {
						evt = eventtypes.ORCH_LOGIN_FAILURE
					}
				}

				if connStatus.State == previousState && msg == previousMsg {
					// Duplicate event, nothing to do
					v.Log.Debugf("Duplicate connection event, nothing to do.")
					break
				}

				if len(evtMsg) != 0 && v.Ctx.Err() == nil {
					recorder.Event(evt, evtMsg, v.State.OrchConfig)
				}

				v.Log.Infof("Writing status update")
				v.orchUpdateLock.Lock()
				v.OrchConfig.Status.Status = connStatus.State
				v.OrchConfig.Status.LastTransitionTime = &api.Timestamp{}
				v.OrchConfig.Status.LastTransitionTime.SetTime(time.Now())
				v.OrchConfig.Status.Message = msg
				if connStatus.State == orchestration.OrchestratorStatus_Failure.String() {
					v.discoveredDCsLock.Lock()
					v.discoveredDCs = []string{}
					v.discoveredDCsLock.Unlock()
					v.OrchConfig.Status.DiscoveredNamespaces = []string{}
				}
				o.Orchestrator.Status = v.OrchConfig.Status
				o.Write()
				v.orchUpdateLock.Unlock()

			default:
				v.Log.Errorf("Unknown event %s", m.MsgType)
			}
		case evt, ok := <-apiServerCh:
			// processVeniceEvents will be false when we are disconnected,
			// and will be set to true once connection is restored AND sync has run
			v.processVeniceEventsLock.Lock()
			processEvent := v.processVeniceEvents
			v.processVeniceEventsLock.Unlock()
			v.Log.Infof("Received API event")

			if ok && processEvent {
				switch evt.Kind {
				case string(network.KindNetwork):
					v.Log.Infof("Processing network event %s", evt.ObjMeta.Name)
					v.handleNetworkEvent(evt.EvtType, evt.ObjMeta)
				case string(workload.KindWorkload):
					v.Log.Infof("Processing workload event %s", evt.ObjMeta.Name)
					v.handleWorkloadEvent(evt.EvtType, evt.ObjMeta)
				}
			} else {
				v.Log.Infof("Skipped API Event %v", ok && processEvent)
			}
		}
	}
}

func (v *VCHub) handleVCEvent(m defs.VCEventMsg) {
	v.Log.Infof("Msg from %v, key: %s prop: %s", m.Originator, m.Key, m.VcObject)
	v.syncLock.RLock()
	defer v.syncLock.RUnlock()
	switch m.VcObject {
	case defs.VirtualMachine:
		v.handleVM(m)
	case defs.HostSystem:
		v.handleHost(m)
	case defs.DistributedVirtualPortgroup:
		v.handlePG(m)
	case defs.VmwareDistributedVirtualSwitch:
		v.handleDVS(m)
	case defs.Datacenter:
		v.handleDC(m)
	default:
		v.Log.Errorf("Unknown object %s", m.VcObject)
	}
}

func (v *VCHub) handleRetryEvent(m defs.RetryMsg) {
	v.Log.Infof("Retry msg oper: %s object: %s", m.Oper, m.ObjectKey)
	v.syncLock.RLock()
	defer v.syncLock.RUnlock()
	switch m.Oper {
	case defs.WorkloadOverride:
		// Rewrites the individual workload
		// Fetch workload
		meta := &api.ObjectMeta{
			Name: m.ObjectKey,
			// TODO: Don't use default tenant
			Tenant:    globals.DefaultTenant,
			Namespace: globals.DefaultNamespace,
		}
		wlObj := v.pCache.GetWorkload(meta)
		if wlObj == nil {
			v.Log.Infof("retry request for %s skipped since workload no longer exists", m.ObjectKey)
			return
		}
		v.setVlanOverride(wlObj, true, false)
	default:
		v.Log.Errorf("Unknown rewrite oper %s", m.Oper)
	}
}

func (v *VCHub) handleVCNotification(m defs.VCNotificationMsg) {
	v.Log.Infof("Received VC Notification %s %v", m.Type, m.Msg)
	switch m.Msg.(type) {
	case defs.VMotionStartMsg:
		v.handleVMotionStart(m.Msg.(defs.VMotionStartMsg))
	case defs.VMotionFailedMsg:
		v.handleVMotionFailed(m.Msg.(defs.VMotionFailedMsg))
	case defs.VMotionDoneMsg:
		v.handleVMotionDone(m.Msg.(defs.VMotionDoneMsg))
	}
}

func (v *VCHub) handleDC(m defs.VCEventMsg) {
	// Check if we have a DC object
	v.Log.Infof("Handle DC called")
	existingDC := v.GetDCFromID(m.Key)
	if m.UpdateType == types.ObjectUpdateKindLeave {
		v.Log.Infof("DC delete for %s", m.Key)
		if existingDC == nil {
			v.Log.Infof("No state for DC %s", m.Key)
			v.DcMapLock.Lock()
			if dcName, ok := v.DcID2NameMap[m.Key]; ok {
				v.removeDiscoveredDC(dcName)
				delete(v.DcID2NameMap, m.Key)
			}
			v.DcMapLock.Unlock()
			return
		}
		// Cleanup internal state
		v.RemovePenDC(existingDC.Name)
		v.removeDiscoveredDC(existingDC.Name)
		return
	}

	for _, prop := range m.Changes {
		name := prop.Val.(string)
		v.Log.Infof("Handle DC %s %s", name, m.Key)
		// Check ID first to detect rename
		v.DcMapLock.Lock()

		oldName, ok := v.DcID2NameMap[m.Key]
		if ok && oldName != name {
			// Check if we are managing it
			if !v.isManagedNamespace(oldName) {
				// DC we aren't managing is renamed, update map entry
				v.DcID2NameMap[m.Key] = name
				v.DcMapLock.Unlock()
				v.renameDiscoveredDC(oldName, name)
				return
			}

			v.Log.Infof("DC %s renamed to %s, changing back...", name, oldName)

			evtMsg := fmt.Sprintf("%v : User renamed a Pensando managed Datacenter from %v to %v. Datacenter name has been changed back to %v.", v.State.OrchConfig.Name, name, oldName, name)
			recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)

			err := v.probe.RenameDC(name, oldName, 3)
			if err != nil {
				v.Log.Errorf("Failed to rename DC %s back to %s, err %s", name, oldName, err)
			}
			v.DcMapLock.Unlock()
			return
		}

		if penDc, ok := v.DcMap[name]; ok {
			penDc.Lock()
			if _, ok := penDc.DvsMap[CreateDVSName(name)]; ok {
				penDc.Unlock()
				v.DcMapLock.Unlock()
				v.probe.StartWatchForDC(name, m.Key)
				v.addDiscoveredDC(name)
				return
			}
			penDc.Unlock()
		}
		v.DcMapLock.Unlock()

		// We create DVS and check networks
		if !v.isManagedNamespace(name) {
			v.Log.Infof("Skipping DC event for DC %s", name)
			v.DcMapLock.Lock()
			v.DcID2NameMap[m.Key] = name
			v.DcMapLock.Unlock()
		} else {
			v.Log.Infof("new DC %s", name)
			_, err := v.NewPenDC(name, m.Key)
			if err == nil {
				v.probe.StartWatchForDC(name, m.Key)
				v.checkNetworks(name)
			} else {
				// Verify DC still exists
				retryFn := func() {
					v.retryDCEvent(name, m)
				}
				v.TimerQ.Add(retryFn, retryDelay)
			}
		}

		// update discovered list
		v.addDiscoveredDC(name)
	}
}

func (v *VCHub) retryDCEvent(name string, m defs.VCEventMsg) {
	v.Log.Infof("Retry Event: Create DC running")
	dcs, err := v.probe.ListDC()
	if err != nil {
		v.Log.Errorf("Failed to get a list of DCs, %s", err)
		retryFn := func() {
			v.retryDCEvent(name, m)
		}
		v.TimerQ.Add(retryFn, retryDelay)
		return
	}
	found := false
	for _, dc := range dcs {
		if name == dc.Name {
			found = true
			break
		}
	}

	if !found {
		v.Log.Infof("Retry event: DC %s no longer exists, nothing to do", name)
		return
	}
	msg := defs.Probe2StoreMsg{
		MsgType: defs.VCEvent,
		Val:     m,
	}
	select {
	case <-v.Ctx.Done():
		return
	case v.vcReadCh <- msg:
	}
}

func (v *VCHub) removeDiscoveredDC(dcName string) {
	v.Log.Infof("removing DC %s to discovered DCs", dcName)
	v.discoveredDCsLock.Lock()
	// Not using defer v.discoveredDCsLock.Unlock() as orchhub shouldn't block if unable
	// to write status update to apiserver
	dcList := v.discoveredDCs
	found := -1
	for i, entry := range dcList {
		if dcName == entry {
			found = i
		}
	}
	if found >= 0 {
		dcList = append(dcList[:found], dcList[found+1:]...)
		v.discoveredDCs = dcList
		v.discoveredDCsLock.Unlock()

		v.orchUpdateLock.Lock()
		defer v.orchUpdateLock.Unlock()
		o, err := v.StateMgr.Controller().Orchestrator().Find(&v.OrchConfig.ObjectMeta)
		if err != nil {
			// orchestrator object does not exists anymore
			// this should never happen
			v.Log.Errorf("removeDiscoveredDC: Orchestrator Object %v does not exist",
				v.OrchConfig.GetKey())
			return
		}

		v.OrchConfig.Status.DiscoveredNamespaces = dcList
		o.Orchestrator.Status = v.OrchConfig.Status
		err = o.Write()

		if err != nil {
			v.Log.Errorf("removeDiscoveredDC: Failed to update orch status %s", err)
		}
	} else {
		v.discoveredDCsLock.Unlock()
	}
}

func (v *VCHub) addDiscoveredDC(dcName string) {
	v.Log.Infof("adding DC %s to discovered DCs", dcName)
	v.discoveredDCsLock.Lock()
	// Not using defer v.discoveredDCsLock.Unlock() as orchhub shouldn't block if unable
	// to write status update to apiserver
	dcList := v.discoveredDCs
	found := -1
	for i, entry := range dcList {
		if dcName == entry {
			found = i
		}
	}
	if found == -1 {
		dcList = append(dcList, dcName)
		sort.Strings(dcList)
		v.discoveredDCs = dcList
		v.discoveredDCsLock.Unlock()

		v.orchUpdateLock.Lock()
		defer v.orchUpdateLock.Unlock()
		o, err := v.StateMgr.Controller().Orchestrator().Find(&v.OrchConfig.ObjectMeta)
		if err != nil {
			// orchestrator object does not exists anymore
			// this should never happen
			v.Log.Errorf("AddDiscoveredDC: Orchestrator Object %v does not exist",
				v.OrchConfig.GetKey())
			return
		}

		v.OrchConfig.Status.DiscoveredNamespaces = dcList
		o.Orchestrator.Status = v.OrchConfig.Status
		err = o.Write()

		if err != nil {
			v.Log.Errorf("AddDiscoveredDC: Failed to update orch status %s", err)
		}
	} else {
		v.discoveredDCsLock.Unlock()
	}
}

func (v *VCHub) renameDiscoveredDC(oldName, newName string) {
	v.Log.Infof("renaming DC %s to %s in discovered DCs", oldName, newName)
	v.discoveredDCsLock.Lock()
	// Not using defer v.discoveredDCsLock.Unlock() as orchhub shouldn't block if unable
	// to write status update to apiserver
	dcList := v.discoveredDCs
	found := -1
	for i, entry := range dcList {
		if oldName == entry {
			found = i
		}
	}

	if found >= 0 {
		dcList[found] = newName
		sort.Strings(dcList)
		v.discoveredDCs = dcList
		v.discoveredDCsLock.Unlock()

		v.orchUpdateLock.Lock()
		defer v.orchUpdateLock.Unlock()
		o, err := v.StateMgr.Controller().Orchestrator().Find(&v.OrchConfig.ObjectMeta)
		if err != nil {
			// orchestrator object does not exists anymore
			// this should never happen
			v.Log.Errorf("RenameDiscoveredDC: Orchestrator Object %v does not exist",
				v.OrchConfig.GetKey())
			return
		}

		v.OrchConfig.Status.DiscoveredNamespaces = dcList
		o.Orchestrator.Status = v.OrchConfig.Status
		err = o.Write()

		if err != nil {
			v.Log.Errorf("RenameDiscoveredDC: Failed to update orch status %s", err)
		}
	} else {
		v.discoveredDCsLock.Unlock()
	}
}
