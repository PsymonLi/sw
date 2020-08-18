package cache

import (
	"context"
	"fmt"
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/ref"
)

const (
	workloadKind = string(workload.KindWorkload)
)

// ValidateWorkload returns whether a workload is valid and whether to write to statemgr
func (c *Cache) ValidateWorkload(in interface{}) (bool, bool) {
	obj, ok := in.(*defs.VCHubWorkload)
	if !ok {
		return false, false
	}
	if len(obj.Spec.HostName) == 0 {
		return false, true
	}
	hostMeta := &api.ObjectMeta{
		Name: obj.Spec.HostName,
	}
	// check that host has been created already
	if _, err := c.stateMgr.Controller().Host().Find(hostMeta); err != nil {
		c.Log.Errorf("Couldn't find host %s for workload %s", hostMeta.Name, obj.GetObjectMeta().Name)
		return false, true
	}
	if len(obj.Spec.Interfaces) == 0 {
		c.Log.Errorf("workload %s has no interfaces", obj.GetObjectMeta().Name)
		return false, true
	}
	// XXX check that the host has a DSC that is used by each interface. A host may have one DSC while
	// workload interface may be using another DSC depending on the monaged/monitoring modes
	// check that workload is no longer in pvlan mode
	for _, inf := range obj.Spec.Interfaces {
		if inf.MicroSegVlan == 0 {
			c.Log.Errorf("inf %s has no useg for workload %s", inf.MACAddress, obj.GetObjectMeta().Name)
			return false, false
		}
	}
	return true, true
}

// RevalidateWorkloads re-validates all workloads in the cache
func (c *Cache) RevalidateWorkloads() {
	c.pCache.RevalidateKind(workloadKind)
}

// SetWorkload adds the workload to the pcache
func (c *Cache) SetWorkload(wl *defs.VCHubWorkload, push bool) {
	c.pCache.Set(workloadKind, wl, push)
}

// DeleteWorkload removes the workload
func (c *Cache) DeleteWorkload(wl *defs.VCHubWorkload) error {
	return c.pCache.Delete(workloadKind, wl)
}

// SetVMInfo only sets the VMInfo attribute and doesn't push to statemgr
func (c *Cache) SetVMInfo(wl *defs.VCHubWorkload) {
	pcacheCopy := c.GetWorkload(wl.GetObjectMeta())
	if pcacheCopy == nil {
		temp := ref.DeepCopy(*wl).(defs.VCHubWorkload)
		pcacheCopy = &temp
	}
	pcacheCopy.VMInfo = wl.VMInfo
	// Write pcacheCopy not wl so that later modifications to wl don't affect pcache
	c.SetWorkload(pcacheCopy, false)
}

// GetWorkload Retrieves a workload from pcache + statemgr
func (c *Cache) GetWorkload(meta *api.ObjectMeta) *defs.VCHubWorkload {
	// checking for ok will prevent nil type conversion errors
	pObj, _ := c.pCache.Get(workloadKind, meta).(*defs.VCHubWorkload)

	var smObj *ctkit.Workload
	if obj, err := c.stateMgr.Controller().FindObject(workloadKind, meta); err == nil {
		smObj = obj.(*ctkit.Workload)
	}
	ret, _ := mergeWorkload(pObj, smObj).(*defs.VCHubWorkload)
	return ret
}

func mergeWorkload(pcacheObj, stateMgrObj interface{}) interface{} {
	pObj, _ := pcacheObj.(*defs.VCHubWorkload)
	smObj, _ := stateMgrObj.(*ctkit.Workload)
	if pObj == nil && smObj == nil {
		return nil
	} else if pObj == nil {
		return &defs.VCHubWorkload{
			Workload: &smObj.Workload,
			VMInfo: defs.VMInfo{
				Vnics: map[string]*defs.VnicEntry{},
			},
		}
	} else if smObj == nil {
		return pObj
	}

	// Merge objects
	if pObj.Workload == nil {
		pObj.Workload = &smObj.Workload
	}

	// take status from apiserver
	pObj.Status = smObj.Workload.Status
	return pObj
}

// GetWorkloadByName retrieves a workload by name
func (c *Cache) GetWorkloadByName(wlName string) *defs.VCHubWorkload {
	meta := &api.ObjectMeta{
		Name:      wlName,
		Tenant:    globals.DefaultTenant,
		Namespace: globals.DefaultNamespace,
	}
	return c.GetWorkload(meta)
}

// ListWorkloads gets all workloads in pcache + statemgr
func (c *Cache) ListWorkloads(ctx context.Context, pcacheOnly bool) map[string]*defs.VCHubWorkload {
	items := map[string]*defs.VCHubWorkload{}
	for key, entry := range c.pCache.List(workloadKind) {
		items[key] = entry.(*defs.VCHubWorkload)
	}

	ctkitWorkloads, err := c.stateMgr.Controller().Workload().List(ctx, &api.ListWatchOptions{})
	if err != nil {
		c.Log.Errorf("Failed to get workloads in statemgr")
		return items
	}

	for _, obj := range ctkitWorkloads {
		key := obj.GetKey()
		if _, ok := items[key]; !ok && !pcacheOnly {
			items[key] = &defs.VCHubWorkload{
				Workload: &obj.Workload,
				VMInfo: defs.VMInfo{
					Vnics: map[string]*defs.VnicEntry{},
				},
			}
		} else if ok {
			if entry, ok := mergeWorkload(items[key], obj).(*defs.VCHubWorkload); ok {
				items[key] = entry
			}
		}
	}

	return items
}

// Logic for writing the workload object to stateMgr
func (c *Cache) writeWorkload(pcacheObj interface{}) error {
	ctrler := c.stateMgr.Controller()
	switch obj := pcacheObj.(type) {
	case *defs.VCHubWorkload:
		var writeErr error
		meta := obj.GetObjectMeta()
		if currObj, err := ctrler.Workload().Find(meta); err == nil {
			// Merge user labels
			obj.Labels = utils.MergeLabels(currObj.Labels, obj.Labels)
			// Object exists and is changed
			// Set nil arrays to be arrays of length 0
			currWorkload := currObj.Workload
			for i := range currWorkload.Spec.Interfaces {
				if len(currWorkload.Spec.Interfaces[i].IpAddresses) == 0 {
					currWorkload.Spec.Interfaces[i].IpAddresses = []string{}
				}
			}
			for i := range obj.Spec.Interfaces {
				if len(obj.Spec.Interfaces[i].IpAddresses) == 0 {
					obj.Spec.Interfaces[i].IpAddresses = []string{}
				}
			}

			if !reflect.DeepEqual(currWorkload.Spec, obj.Spec) ||
				!reflect.DeepEqual(currWorkload.Status, obj.Status) ||
				!reflect.DeepEqual(currWorkload.Labels, obj.Labels) {
				// CAS check is needed in case user adds labels to an object
				obj.ResourceVersion = currObj.ResourceVersion
				c.Log.Debugf("%s %s statemgr update called", workloadKind, meta.GetKey())
				writeErr = ctrler.Workload().SyncUpdate(obj.Workload)
			}
		} else {
			c.Log.Debugf("%s %s statemgr create called", workloadKind, meta.GetKey())
			writeErr = ctrler.Workload().SyncCreate(obj.Workload)
		}
		c.Log.Debugf("%s %s write to statemgr returned %v", workloadKind, meta.GetKey(), writeErr)
		return writeErr
	}
	return fmt.Errorf("writeWorkload called for unsupported object %T", pcacheObj)
}

// Logic for deleting the workload object from stateMgr
func (c *Cache) deleteWorkload(pcacheObj interface{}) error {
	ctrler := c.stateMgr.Controller()
	switch obj := pcacheObj.(type) {
	case *defs.VCHubWorkload:
		var writeErr error
		meta := obj.GetObjectMeta()
		if _, err := ctrler.Workload().Find(meta); err == nil {
			// Object exists
			writeErr = ctrler.Workload().SyncDelete(obj.Workload)
		}
		return writeErr
	}
	return fmt.Errorf("DeleteWorkload called for unsupported object %T", pcacheObj)
}
