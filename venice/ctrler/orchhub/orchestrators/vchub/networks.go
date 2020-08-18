package vchub

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/useg"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/channelqueue"
	"github.com/pensando/sw/venice/utils/kvstore"
)

const networkKind = "Network"

func (v *VCHub) handleNetworkEvent(evtType kvstore.WatchEventType, nwMeta *api.ObjectMeta) {
	// Update calls might actually be create calls for us, since the active flag would not be set on object creation
	v.Log.Infof("Handling network event for %v", nwMeta.Name)
	v.syncLock.RLock()
	defer v.syncLock.RUnlock()
	// TODO: check res version to prevent double ops
	nwCtkit, err := v.StateMgr.Controller().Network().Find(nwMeta)
	var nw *network.Network
	if err != nil && evtType != kvstore.Deleted {
		v.Log.Infof("network no longer exists, nothing to do. err %s", err)
		return
	}
	if evtType == kvstore.Deleted {
		// Create an empty config
		nw = &network.Network{
			ObjectMeta: *nwMeta,
			Spec:       network.NetworkSpec{},
		}
	} else {
		nw = &nwCtkit.Network
	}
	v.Log.Infof("Handling network event %v nw %v", evtType, nw)

	pgName := CreatePGName(nwMeta.Name)

	if evtType == kvstore.Created && len(nw.Spec.Orchestrators) == 0 {
		return
	}
	dcs := map[string]bool{}
	for _, orch := range nw.Spec.Orchestrators {
		if orch.Name == v.OrchConfig.GetName() {
			if orch.Namespace == utils.ManageAllDcs {
				// Add all known DCs to the map
				for _, dc := range v.DcMap {
					dcs[dc.DcName] = true
				}
			} else {
				dcs[orch.Namespace] = true
			}
		}
	}
	v.Log.Infof("evt %s network %s event for dcs %v", evtType, nwMeta.Name, dcs)
	retryFn := func() {
		v.Log.Infof("Retry Event: Create PG running")
		// Verify this is still needed
		nw, err := v.StateMgr.Controller().Network().Find(nwMeta)
		if err != nil {
			v.Log.Infof("Retry event: network no longer exists, nothing to do. err %s", err)
			return
		}
		apiServerChQ, err := v.StateMgr.GetProbeChannel(v.OrchConfig.GetName())
		if err != nil {
			v.Log.Errorf("Retry event: Could not get probe channel for %s. Err : %v", v.OrchConfig.GetKey(), err)
			return
		}
		evt := channelqueue.Item{
			EvtType: kvstore.Updated,
			ObjMeta: nw.Network.GetObjectMeta(),
			Kind:    string(network.KindNetwork),
		}
		apiServerChQ.Send(evt)
	}
	for _, penDC := range v.DcMap {
		if _, ok := dcs[penDC.DcName]; ok {
			if evtType == kvstore.Created || evtType == kvstore.Updated {
				v.Log.Infof("Adding PG %s in DC %s", pgName, penDC.DcName)
				// IF PG NOT in our local state, but does already exist in vCenter,
				// then we need to resync workloads after creating internal state
				resync := false
				if penDC.GetPG(pgName, "") == nil {
					_, err := v.probe.GetPenPG(penDC.DcName, pgName, defaultRetryCount)
					if err == nil {
						resync = true
					}
				}
				errs := penDC.AddPG(pgName, nw.ObjectMeta, "")
				if len(errs) == 0 && resync {
					v.syncLock.RUnlock()
					err := v.fetchVMs(penDC)
					v.syncLock.RLock()
					if err != nil {
						errs = append(errs, err)
						goto checkErrors
					}

					dcRef := penDC.DcRef
					vcHosts, err := v.probe.ListHosts(&dcRef)
					if errs != nil {
						errs = append(errs, err)
					}
					for _, host := range vcHosts {
						evt := v.convertHostToEvent(host, dcRef.Value, penDC.DcName)
						v.handleHost(evt)
					}
				}
			checkErrors:
				if len(errs) != 0 {
					// add to queue to retry later
					v.Log.Infof("Failed to add PG, adding to retry queue")
					v.TimerQ.Add(retryFn, retryDelay)
				}
			} else {
				// err is already logged inside function
				remainingAllocs, _ := penDC.RemovePenPG(pgName, "")
				// if we just deleted a workload check if we just went below capacity
				for _, count := range remainingAllocs {
					if count == useg.MaxPGCount-1 {
						// Need to recheck networks now that we have space for new networks
						v.checkNetworks(penDC.DcName)
					}
				}
			}
		} else if evtType == kvstore.Updated || evtType == kvstore.Deleted {
			// Check if we need to delete
			if penDC.GetPG(pgName, "") != nil {
				_, errs := penDC.RemovePenPG(pgName, "")
				if len(errs) != 0 {
					// Retry delete
					v.Log.Infof("Failed to remove PG, adding to retry queue")
					v.TimerQ.Add(retryFn, retryDelay)
				}
			}
		}
	}
}

// checks if we need to create PGs for the given network
func (v *VCHub) checkNetworks(dcName string) {
	// Check if we have any networks for this new DC
	opts := api.ListWatchOptions{}
	networks, err := v.StateMgr.Controller().Network().List(v.Ctx, &opts)
	if err != nil {
		v.Log.Errorf("Failed to get network list. Err : %v", err)
	}
	for _, nw := range networks {
		v.Log.Debugf("Checking nw %s", nw.Network.Name)
		for _, orch := range nw.Network.Spec.Orchestrators {
			if orch.Name == v.VcID &&
				(orch.Namespace == dcName || orch.Namespace == utils.ManageAllDcs) {
				penDC := v.GetDC(dcName)
				pgName := CreatePGName(nw.Network.Name)
				errs := penDC.AddPG(pgName, nw.Network.ObjectMeta, "")
				if len(errs) != 0 {
					// add to queue to retry later
					v.Log.Infof("Failed to add PG, adding to retry queue")
					v.TimerQ.Add(func() {
						v.checkNetworks(dcName)
					}, retryDelay)
				}
				v.Log.Infof("Create Pen PG %s returned %s", pgName, errs)
			} else {
				v.Log.Debugf("vcID %s dcName %s does not match orch-spec %s - %s",
					v.VcID, dcName, orch.Name, orch.Namespace)
			}
		}
	}
}
