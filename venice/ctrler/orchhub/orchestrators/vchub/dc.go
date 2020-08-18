package vchub

import (
	"fmt"
	"sync"

	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
)

// DCShared Holds common state for a PenDC and it's children (PenDVS, PenPG)
type DCShared struct {
	DcName string
	DcRef  types.ManagedObjectReference
	Probe  vcprobe.ProbeInf
}

// PenDC represents an instance of a Datacenter
type PenDC struct {
	sync.Mutex
	*defs.State
	*DCShared
	// Map from dvs display name to PenDVS inside this DC
	PenDvsMap map[string]*PenDVS
	// Map from user created dvsID to dvs object
	UserDvsIDMap map[string]*PenDVS
	HostName2Key map[string]string
	mode         defs.Mode
}

// DcID2Name returns name given a DC ID
func (v *VCHub) DcID2Name(id string) string {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	return v.DcID2NameMap[id]
}

// NewPenDC creates a new penDC object
func (v *VCHub) NewPenDC(dcName, dcID string, mode defs.Mode) (*PenDC, error) {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()

	var dc *PenDC
	// TODO: DC name might change, in that case lookp using ID
	if dcExisting, ok := v.DcMap[dcName]; ok {
		dc = dcExisting
	} else {
		dcRef := types.ManagedObjectReference{
			Type:  string(defs.Datacenter),
			Value: dcID,
		}
		dc = &PenDC{
			State: v.State,
			DCShared: &DCShared{
				DcName: dcName,
				DcRef:  dcRef,
				Probe:  v.probe,
			},
			PenDvsMap:    map[string]*PenDVS{},
			UserDvsIDMap: map[string]*PenDVS{},
			HostName2Key: map[string]string{},
			mode:         mode,
		}
		v.DcMap[dcName] = dc

		v.State.DcMapLock.Lock()
		v.State.DcIDMap[dcName] = dcRef
		v.State.DcMapLock.Unlock()

		if v.DcID2NameMap == nil {
			v.DcID2NameMap = map[string]string{}
		}
		v.DcID2NameMap[dcID] = dcName

		v.Log.Infof("adding dc %s ( %s )", dcName, dcID)

		if dc.isManagedMode() {
			err := v.probe.TagObjAsManaged(dcRef)
			if err != nil {
				v.Log.Errorf("Failed to tag DC as managed, %s", err)
				// Error isn't worth failing the operation for
			}
		}
	}

	if dc.isManagedMode() {
		err := dc.AddPenDVS()
		if err != nil {
			evtMsg := fmt.Sprintf("%v : Failed to create DVS in Datacenter %s. %v", v.State.OrchConfig.Name, dcName, err)
			v.Log.Errorf(evtMsg)

			if v.Ctx.Err() == nil && v.probe.IsSessionReady() {
				recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, v.State.OrchConfig)
			}

			return nil, err
		}
	}

	return dc, nil
}

// RemovePenDC removes all DC state on remove DC event from vCenter
func (v *VCHub) RemovePenDC(dcName string) {
	v.Log.Infof("Stopping management of DC %v", dcName)
	v.DcMapLock.Lock()
	existingDC, ok := v.DcMap[dcName]
	v.DcMapLock.Unlock()
	if !ok {
		// nothing to do
		v.Log.Errorf("Remove DC called on %s but there is no entry for it", dcName)
		return
	}

	// Stop watcher for this DC
	v.probe.StopWatchForDC(dcName, existingDC.DcRef.Value)
	// Remove tag on DC

	existingDC.Lock()
	for dvsName, dvs := range existingDC.PenDvsMap {
		err := v.probe.RemovePenDVS(dcName, dvsName, 5)
		if err != nil {
			v.Log.Errorf("failed to delete DVS %v from DC %v. Err : %v", dvsName, dcName, err)
			err := v.probe.RemoveTagObjManaged(dvs.DvsRef)
			if err != nil {
				v.Log.Errorf("failed to remove managed tag from %v in DC %v. Err : %v", dvsName, dcName, err)
			}
		} else {
			v.Log.Infof("Deleted PenDVS %v from DC %v", dvsName, dcName)
		}
	}
	existingDC.Unlock()

	// Delete any Workloads or hosts associated with this DC
	// There may be stale hosts or workloads if we were disconnected
	workloads := v.cache.ListWorkloads(v.Ctx, false)
	for _, workload := range workloads {
		if utils.IsObjForOrch(workload.Labels, v.OrchConfig.Name, dcName) {
			v.deleteWorkload(workload)
		}
	}

	hosts := v.cache.ListHosts(v.Ctx, false)
	for _, host := range hosts {
		if utils.IsObjForOrch(host.Labels, v.OrchConfig.Name, dcName) {
			v.deleteHostStateFromDc(host, existingDC, true)
		}
	}

	// Delete entries in map
	v.DcMapLock.Lock()
	delete(v.DcMap, existingDC.DcName)
	v.DcMapLock.Unlock()

	v.State.DcMapLock.Lock()
	delete(v.State.DcIDMap, existingDC.DcName)
	v.State.DcMapLock.Unlock()

	existingDC.Lock()
	for dvsName := range existingDC.PenDvsMap {
		v.State.DvsIDMapLock.Lock()
		if _, ok := v.State.DvsIDMap[dvsName]; ok {
			delete(v.State.DvsIDMap, dvsName)
		}
		v.State.DvsIDMapLock.Unlock()
	}
	existingDC.Unlock()

	return
}

// GetDC returns the DC by display name
func (v *VCHub) GetDC(name string) *PenDC {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	dc, ok := v.DcMap[name]
	if !ok {
		return nil
	}
	return dc
}

// RenameDC renames a DC
func (v *VCHub) RenameDC(oldName, newName string) {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	dc, ok := v.DcMap[oldName]
	if !ok {
		v.Log.Infof("RenameDC called on a DC that does not exist. Old name %s new name %s", oldName, newName)
		return
	}
	delete(v.DcMap, oldName)
	v.DcMap[newName] = dc
	dc.DcName = newName

	v.State.DcMapLock.Lock()
	delete(v.State.DcIDMap, oldName)
	v.State.DcIDMap[newName] = dc.DcRef
	v.State.DcMapLock.Unlock()
	return
}

// GetDCFromID returns the DC by vcenter DcID
func (v *VCHub) GetDCFromID(dcID string) *PenDC {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	dcName, ok := v.DcID2NameMap[dcID]
	if !ok {
		v.Log.Errorf("No entry in DcID2NameMap for dcID %s", dcID)
		return nil
	}
	if dc, ok := v.DcMap[dcName]; ok {
		return dc
	}
	v.Log.Errorf("No DC object for DC %s", dcName)
	return nil
}

// AddPG adds a PG to all DVS in this DC, unless dvsName is not blank
func (d *PenDC) AddPG(pgName string, networkMeta api.ObjectMeta, dvsName string) []error {
	d.Lock()
	defer d.Unlock()
	var errs []error

	if len(d.PenDvsMap) == 0 {
		// If the number of element in PenDvsMap equals to 0, then something bad happens.
		// Could be the given account doesn't have write permission to vCenter.
		// In this case, DVS was not able to be created previously.
		evtMsg := fmt.Sprintf("%v : Failed to add network %s in Datacenter %s due to missing DVS. Please check vCenter account permissions.", d.State.OrchConfig.Name, networkMeta.Name, d.DcName)
		err := fmt.Errorf(evtMsg)
		errs = append(errs, err)
		if d.Ctx.Err() == nil && d.Probe.IsSessionReady() {
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, d.State.OrchConfig)
		}

		return errs
	}

	for _, penDVS := range d.PenDvsMap {
		if dvsName == "" || dvsName == penDVS.DvsName {
			err := penDVS.AddPenPG(pgName, networkMeta)
			if err != nil {
				d.Log.Errorf("Create PG for dvs %s returned err %s", penDVS.DvsName, err)
				errs = append(errs, err)
			}
		}
	}

	return errs
}

// GetPG returns the pg with the matching name. Looks thorugh all
// DVS unless dvsName is supplied
func (d *PenDC) GetPG(pgName string, dvsName string) *PenPG {
	d.Lock()
	defer d.Unlock()
	for _, penDVS := range d.PenDvsMap {
		if dvsName == "" || dvsName == penDVS.DvsName {
			pg := penDVS.GetPG(pgName)
			if pg != nil {
				return pg
			}
		}
	}
	for _, userDVS := range d.UserDvsIDMap {
		if dvsName == "" || dvsName == userDVS.DvsName {
			pg := userDVS.GetPG(pgName)
			if pg != nil {
				return pg
			}
		}
	}
	return nil
}

// GetPGByID returns the PG object with the given vcenter ID
func (d *PenDC) GetPGByID(pgID string) *PenPG {
	d.Lock()
	defer d.Unlock()
	for _, penDVS := range d.PenDvsMap {
		pg := penDVS.GetPGByID(pgID)
		if pg != nil {
			return pg
		}
	}
	for _, userDVS := range d.UserDvsIDMap {
		pg := userDVS.GetPGByID(pgID)
		if pg != nil {
			return pg
		}
	}
	return nil
}

// RemovePenPG removes a PG from all DVS in this DC, unless dvsName is not blank
// Returns a map from dvs name to number of PG allocations still available
func (d *PenDC) RemovePenPG(pgName string, dvsName string) (map[string]int, []error) {
	d.Log.Infof("Removing PG %s...", pgName)
	d.Lock()
	defer d.Unlock()
	var errs []error
	for _, penDVS := range d.PenDvsMap {
		d.Log.Debugf("Removing PG %s in dvs %s", pgName, penDVS.DvsName)
		if dvsName == "" || dvsName == penDVS.DvsName {
			err := penDVS.RemovePenPG(pgName)
			if err != nil {
				d.Log.Errorf("Delete PG %s for dvs %s returned err %s", pgName, penDVS.DvsName, err)
				errs = append(errs, err)
				continue
			}
			err = penDVS.UsegMgr.ReleaseVlansForPG(pgName)
			d.Log.Errorf("Delete PG %s for dvs %s returned err %s", pgName, penDVS.DvsName, err)
			errs = append(errs, err)
		}
	}
	d.Log.Debugf("Removing PG %s completed with errs: %v", pgName, errs)
	remainingAlloc := d.getRemainingPGAllocations(dvsName)
	return remainingAlloc, errs
}

func (d *PenDC) getRemainingPGAllocations(dvsName string) map[string]int {
	ret := map[string]int{}
	for _, penDVS := range d.PenDvsMap {
		if dvsName == "" || dvsName == penDVS.DvsName {
			ret[penDVS.DvsName] = penDVS.UsegMgr.GetRemainingPGCount()
		}
	}
	return ret
}

func (d *PenDC) addHostNameKey(name, key string) {
	d.Lock()
	defer d.Unlock()
	d.Log.Infof("DC %v: Adding host %v:%v to DC", d.DcName, name, key)
	d.HostName2Key[name] = key
}

func (d *PenDC) delHostNameKey(name string) {
	d.Lock()
	defer d.Unlock()
	if _, ok := d.HostName2Key[name]; ok {
		delete(d.HostName2Key, name)
	}
}

func (d *PenDC) findHostKeyByName(name string) (string, bool) {
	d.Lock()
	defer d.Unlock()
	key, ok := d.HostName2Key[name]
	return key, ok
}

func (d *PenDC) findHostNameByKey(key string) (string, bool) {
	d.Lock()
	defer d.Unlock()
	for name, k := range d.HostName2Key {
		if k == key {
			return name, true
		}
	}
	return "", false
}

func (d *PenDC) isMonitoringMode() bool {
	d.Lock()
	defer d.Unlock()
	return d.mode == defs.MonitoringMode || d.mode == defs.MonitoringManagedMode
}

func (d *PenDC) isManagedMode() bool {
	d.Lock()
	defer d.Unlock()
	return d.mode == defs.ManagedMode || d.mode == defs.MonitoringManagedMode
}
