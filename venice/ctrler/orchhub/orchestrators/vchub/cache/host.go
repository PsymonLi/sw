package cache

import (
	"context"
	"fmt"
	"reflect"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/ref"
)

const (
	hostKind = string(cluster.KindHost)
)

// ValidateHost returns whether a host is valid and whether to write to statemgr
func (c *Cache) ValidateHost(in interface{}) (bool, bool) {
	obj, ok := in.(*defs.VCHubHost)
	if !ok {
		return false, false
	}
	if len(obj.Spec.DSCs) == 0 {
		c.Log.Errorf("host %s has no DSC", obj.GetObjectMeta().Name)
		return false, true
	}
	return true, true
}

// RevalidateHosts re-validates all hosts in the cache
func (c *Cache) RevalidateHosts() {
	c.pCache.RevalidateKind(hostKind)
}

// SetHost adds the host to the pcache
func (c *Cache) SetHost(host *defs.VCHubHost, push bool) {
	c.pCache.Set(hostKind, host, push)
}

// DeleteHost removes the host
func (c *Cache) DeleteHost(host *defs.VCHubHost) error {
	return c.pCache.Delete(hostKind, host)
}

// SetHostInfo only sets the HostInfo attribute and doesn't push to statemgr
func (c *Cache) SetHostInfo(host *defs.VCHubHost) {
	pcacheCopy := c.GetHost(host.GetObjectMeta())
	if pcacheCopy == nil {
		temp := ref.DeepCopy(*host).(defs.VCHubHost)
		pcacheCopy = &temp
	}
	pcacheCopy.HostInfo = host.HostInfo
	// Write pcacheCopy not wl so that later modifications to the obj don't affect pcache
	c.SetHost(pcacheCopy, false)
}

// GetHost Retrieves a host
func (c *Cache) GetHost(meta *api.ObjectMeta) *defs.VCHubHost {
	// checking for ok will prevent nil type conversion errors
	pObj, _ := c.pCache.Get(hostKind, meta).(*defs.VCHubHost)

	var smObj *ctkit.Host
	if obj, err := c.stateMgr.Controller().FindObject(hostKind, meta); err == nil {
		smObj = obj.(*ctkit.Host)
	}

	ret, _ := mergeHost(pObj, smObj).(*defs.VCHubHost)
	return ret
}

func mergeHost(pcacheObj, stateMgrObj interface{}) interface{} {
	pObj, _ := pcacheObj.(*defs.VCHubHost)
	smObj, _ := stateMgrObj.(*ctkit.Host)
	if pObj == nil && smObj == nil {
		return nil
	} else if pObj == nil {
		return &defs.VCHubHost{
			Host:     &smObj.Host,
			HostInfo: defs.HostInfo{},
		}
	} else if smObj == nil {
		return pObj
	}

	// Merge objects
	if pObj.Host == nil {
		pObj.Host = &smObj.Host
	}

	// take status from apiserver
	pObj.Status = smObj.Host.Status
	return pObj
}

// GetHostByName Retrieves a host
func (c *Cache) GetHostByName(hostName string) *defs.VCHubHost {
	meta := &api.ObjectMeta{
		Name: hostName,
	}
	return c.GetHost(meta)
}

// ListHosts gets all hosts in pcache + statemgr
func (c *Cache) ListHosts(ctx context.Context, pcacheOnly bool) map[string]*defs.VCHubHost {
	items := map[string]*defs.VCHubHost{}
	for key, entry := range c.pCache.List(hostKind) {
		items[key] = entry.(*defs.VCHubHost)
	}

	ctkitHosts, err := c.stateMgr.Controller().Host().List(ctx, &api.ListWatchOptions{})
	if err != nil {
		c.Log.Errorf("Failed to get hosts in statemgr")
		return items
	}

	for _, obj := range ctkitHosts {
		key := obj.GetKey()
		if _, ok := items[key]; !ok && !pcacheOnly {
			items[key] = &defs.VCHubHost{
				Host:     &obj.Host,
				HostInfo: defs.HostInfo{},
			}
		} else if ok {
			if entry, ok := mergeHost(items[key], obj).(*defs.VCHubHost); ok {
				items[key] = entry
			}
		}
	}

	return items
}

// Logic for writing the host object to stateMgr
func (c *Cache) writeHost(pcacheObj interface{}) error {
	ctrler := c.stateMgr.Controller()
	switch obj := pcacheObj.(type) {
	case *defs.VCHubHost:
		var writeErr error
		meta := obj.GetObjectMeta()
		if currObj, err := ctrler.Host().Find(meta); err == nil {
			// Merge user labels
			obj.Labels = utils.MergeLabels(currObj.Labels, obj.Labels)
			// Object exists and is changed
			if !reflect.DeepEqual(currObj.Host.Spec, obj.Spec) ||
				!reflect.DeepEqual(currObj.Host.Labels, obj.Labels) {
				// Fetch stateMgr host and get it's status and resVer
				// CAS check is needed in case user adds labels to an object
				obj.ResourceVersion = currObj.ResourceVersion
				obj.Status = currObj.Status
				c.Log.Debugf("%s %s statemgr update called", hostKind, meta.GetKey())
				writeErr = ctrler.Host().SyncUpdate(obj.Host)
			}
		} else {
			c.Log.Debugf("%s %s statemgr create called", hostKind, meta.GetKey())
			writeErr = ctrler.Host().SyncCreate(obj.Host)
		}
		c.Log.Debugf("%s %s write to statemgr returned %v", hostKind, meta.GetKey(), writeErr)
		return writeErr
	}
	return fmt.Errorf("writeHosts called on unsupported object")
}

// Logic for deleting the host object from stateMgr
func (c *Cache) deleteHost(pcacheObj interface{}) error {
	ctrler := c.stateMgr.Controller()
	switch obj := pcacheObj.(type) {
	case *defs.VCHubHost:
		var writeErr error
		meta := obj.GetObjectMeta()
		if _, err := ctrler.Host().Find(meta); err == nil {
			// Object exists
			c.Log.Debugf("%s %s deleting from statemgr", hostKind, meta.GetKey())
			// Delete workloads on the host first
			// Don't want other operations on workloads happening at the same time
			c.pCache.LockKind(workloadKind)
			opts := api.ListWatchOptions{}
			c.Log.Infof("removing workloads on host %s from statemgr", meta.GetKey())
			wlList, err := ctrler.Workload().List(context.Background(), &opts)
			if err == nil {
				for _, wl := range wlList {
					if wl.Spec.HostName != obj.Name {
						continue
					}
					c.Log.Infof("removing workload %s on host %s from statemgr", wl.Name, meta.GetKey())
					err := ctrler.Workload().SyncDelete(&wl.Workload)
					if err != nil {
						c.Log.Infof("Failed to delete workload: %s", err)
					}
				}
			} else {
				c.Log.Errorf("Workload list failed %s", err)
			}

			writeErr = ctrler.Host().SyncDelete(obj.Host)
			// Unlock after host delete so that any calls to validateWorkload happen after this host is removed
			// from statemgr
			c.pCache.UnlockKind(workloadKind)
			c.Log.Debugf("%s %s deleting from statemgr returned %v", hostKind, meta.GetKey(), writeErr)
		}
		return writeErr
	}
	// Unsupported object, we only write it to cache
	c.Log.Errorf("deleteStatemgr called on unsupported object %+v", pcacheObj)
	return nil
}
