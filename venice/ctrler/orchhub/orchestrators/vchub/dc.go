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

// PenDC represents an instance of a Datacenter
type PenDC struct {
	sync.Mutex
	*defs.State
	Name  string
	dcRef types.ManagedObjectReference
	// Map from dvs display name to PenDVS inside this DC
	DvsMap       map[string]*PenDVS
	probe        vcprobe.ProbeInf
	HostName2Key map[string]string
}

// NewPenDC creates a new penDC object
func (v *VCHub) NewPenDC(dcName, dcID string) (*PenDC, error) {
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
			State:        v.State,
			probe:        v.probe,
			Name:         dcName,
			dcRef:        dcRef,
			DvsMap:       map[string]*PenDVS{},
			HostName2Key: map[string]string{},
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

		err := v.probe.TagObjAsManaged(dcRef)
		if err != nil {
			v.Log.Errorf("Failed to tag DC as managed, %s", err)
			// Error isn't worth failing the operation for
		}
	}

	err := dc.AddPenDVS()
	if err != nil {
		evtMsg := fmt.Sprintf("%v : Failed to create DVS in Datacenter %s. %v", v.State.OrchConfig.Name, dcName, err)
		v.Log.Errorf(evtMsg)

		if v.Ctx.Err() == nil {
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, v.State.OrchConfig)
		}

		return nil, err
	}

	return dc, nil
}

// RemovePenDC removes all DC state on remove DC event from vCenter
func (v *VCHub) RemovePenDC(dcName string) {
	v.Log.Infof("Stopping management of DC %v", dcName)
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()

	existingDC, ok := v.DcMap[dcName]
	if !ok {
		// nothing to do
		v.Log.Errorf("Remove DC called on %s but there is no entry for it", dcName)
		return
	}

	// Stop watcher for this DC
	v.probe.StopWatchForDC(dcName, existingDC.dcRef.Value)
	// Remove tag on DC

	existingDC.Lock()
	for dvsName, dvs := range existingDC.DvsMap {
		err := v.probe.RemovePenDVS(dcName, dvsName, 5)
		if err != nil {
			v.Log.Errorf("failed to delete DVS %v from DC %v. Err : %v", dvsName, dcName, err)
			err := v.probe.RemoveTagObjManaged(dvs.DvsRef)
			if err != nil {
				v.Log.Errorf("failed to remove managed tag from %v in DC %v. Err : %v", dvsName, dcName, err)
			}
		}
	}
	existingDC.Unlock()

	// Delete any Workloads or hosts associated with this DC
	// There may be stale hosts or workloads if we were disconnected
	opts := api.ListWatchOptions{}
	opts.LabelSelector = fmt.Sprintf("%v=%v,%v=%v", utils.OrchNameKey, v.OrchConfig.Name, utils.NamespaceKey, dcName)

	workloads, err := v.StateMgr.Controller().Workload().List(v.Ctx, &opts)
	if err != nil {
		v.Log.Errorf("Failed to get workload list for DC %v. Err : %v", dcName, err)
	}
	for _, workload := range workloads {
		v.deleteWorkload(&workload.Workload)
	}

	hosts := v.pCache.ListHosts(v.Ctx)
	for _, host := range hosts {
		if utils.IsObjForOrch(host.Labels, v.OrchConfig.Name, dcName) {
			v.deleteHostFromDc(host, existingDC)
		}
	}

	// Delete entries in map
	delete(v.DcMap, existingDC.Name)
	delete(v.DcID2NameMap, existingDC.dcRef.Value)

	v.State.DcMapLock.Lock()
	delete(v.State.DcIDMap, existingDC.Name)
	v.State.DcMapLock.Unlock()

	existingDC.Lock()
	for dvsName := range existingDC.DvsMap {
		v.State.DvsMapLock.Lock()
		if _, ok := v.State.DvsIDMap[dvsName]; ok {
			delete(v.State.DvsIDMap, dvsName)
		}
		v.State.DvsMapLock.Unlock()
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

// GetDCFromID returns the DC by vcenter DcID
func (v *VCHub) GetDCFromID(dcID string) *PenDC {
	v.DcMapLock.Lock()
	defer v.DcMapLock.Unlock()
	dcName, ok := v.DcID2NameMap[dcID]
	if !ok {
		return nil
	}
	if dc, ok := v.DcMap[dcName]; ok {
		return dc
	}
	return nil
}

// AddPG adds a PG to all DVS in this DC, unless dvsName is not blank
func (d *PenDC) AddPG(pgName string, networkMeta api.ObjectMeta, dvsName string) []error {
	d.Lock()
	defer d.Unlock()
	var errs []error
	for _, penDVS := range d.DvsMap {
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

// GetDVS returns the DVS with matching name
func (d *PenDC) GetDVS(dvsName string) *PenDVS {
	d.Lock()
	defer d.Unlock()
	dvs, ok := d.DvsMap[dvsName]
	if !ok {
		return nil
	}
	return dvs
}

// GetPG returns the pg with the matching name. Looks thorugh all
// DVS unless dvsName is supplied
func (d *PenDC) GetPG(pgName string, dvsName string) *PenPG {
	d.Lock()
	defer d.Unlock()
	for _, penDVS := range d.DvsMap {
		if dvsName == "" || dvsName == penDVS.DvsName {
			pg := penDVS.GetPenPG(pgName)
			if pg != nil {
				return pg
			}
		}
	}
	return nil
}

// GetPGByID returns the Pen PG object with the given vcenter ID
func (d *PenDC) GetPGByID(pgID string) *PenPG {
	d.Lock()
	defer d.Unlock()
	for _, penDVS := range d.DvsMap {
		pg := penDVS.GetPenPGByID(pgID)
		if pg != nil {
			return pg
		}
	}
	return nil
}

// RemovePG removes a PG from all DVS in this DC, unless dvsName is not blank
// Returns a map from dvs name to number of PG allocations still available
func (d *PenDC) RemovePG(pgName string, dvsName string) (map[string]int, []error) {
	d.Log.Infof("Removing PG %s...", pgName)
	d.Lock()
	defer d.Unlock()
	var errs []error
	for _, penDVS := range d.DvsMap {
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
	for _, penDVS := range d.DvsMap {
		if dvsName == "" || dvsName == penDVS.DvsName {
			ret[penDVS.DvsName] = penDVS.UsegMgr.GetRemainingPGCount()
		}
	}
	return ret
}

func (d *PenDC) addHostNameKey(name, key string) {
	d.Lock()
	defer d.Unlock()
	d.Log.Infof("DC %v: Adding host %v:%v to DC", d.Name, name, key)
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
