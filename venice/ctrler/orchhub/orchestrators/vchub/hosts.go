package vchub

import (
	"fmt"
	"reflect"
	"sort"

	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/ref"
	conv "github.com/pensando/sw/venice/utils/strconv"
)

func (v *VCHub) handleHost(m defs.VCEventMsg) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Got handle host event for host %s in DC %s", m.Key, m.DcID)
	penDC := v.GetDC(dcName)
	if penDC == nil {
		v.Log.Errorf("Skip host event,for host %s in DC %s. No DC object found", m.Key, m.DcID)
		return
	}
	if !penDC.isManagedMode() && !penDC.isMonitoringMode() {
		v.Log.Infof("Skip host event for host %s in DC %s", m.Key, m.DcID)
		return
	}

	meta := &api.ObjectMeta{
		Name: v.createHostName(m.DcID, m.Key),
	}
	stateMgrHost, err := v.StateMgr.Controller().Host().Find(meta)
	if err != nil {
		stateMgrHost = nil
	}

	existingHost := v.cache.GetHost(meta)
	var hostObj *defs.VCHubHost

	if existingHost == nil {
		v.Log.Debugf("This is a new host - %s", meta.Name)
		hostObj = &defs.VCHubHost{
			Host: &cluster.Host{
				TypeMeta: api.TypeMeta{
					Kind:       "Host",
					APIVersion: "v1",
				},
				ObjectMeta: *meta,
			},
			HostInfo: defs.HostInfo{},
		}
	} else {
		// Copying to prevent modifying of ctkit state
		temp := ref.DeepCopy(*existingHost).(defs.VCHubHost)
		hostObj = &temp
	}

	if m.UpdateType == types.ObjectUpdateKindLeave {
		// Object was deleted
		if existingHost == nil {
			// Don't have object we received delete for
			return
		}
		v.deleteHostState(hostObj, true)
		return
	}

	if hostObj.Labels == nil {
		hostObj.Labels = map[string]string{}
	}

	utils.AddOrchNameLabel(hostObj.Labels, v.OrchConfig.Name)
	utils.AddOrchNamespaceLabel(hostObj.Labels, dcName)

	var hConfig *types.HostConfigInfo
	var dispName string
	configChanged := false
	for _, prop := range m.Changes {
		switch defs.VCProp(prop.Name) {
		case defs.HostPropConfig:
			propConfig, _ := prop.Val.(types.HostConfigInfo)
			hConfig = &propConfig
			configChanged = v.processHostConfig(prop, m.DcID, dcName, hostObj)
		case defs.HostPropName:
			dispName = prop.Val.(string)
			// store display name in the wrapper rather than labels
			hostObj.DisplayName = dispName
			v.cache.SetHostInfo(hostObj)
			addNameLabel(hostObj.Labels, dispName)
		default:
			v.Log.Errorf("host prop change %s - not handled", prop.Name)
		}
	}

	// Construct DSC info
	v.populateDSCs(hostObj, penDC)

	if existingHost != nil && len(hostObj.Spec.DSCs) == 0 {
		// host does not use pensando DSC uplinks anymore, cleanup any workloads on it and
		// delete the host
		// TODO: skip this step if monitoring on non-pen host is allowed
		v.Log.Errorf("Removing host %s and all workloads on it", hostObj.Name)
		v.hostRemovedFromDVS(existingHost)
	}

	dispName = hostObj.DisplayName
	if dispName != "" {
		penDC.addHostNameKey(dispName, m.Key)
		v.updateVmkWorkloadLabels(hostObj.Name, dcName, dispName)
	}

	if stateMgrHost == nil {
		v.Log.Infof("Creating host %s", hostObj.Name)
		// Check if there are any stale hosts with the same DSC
		v.fixStaleHost(m.Key, hostObj)
		// commit the object to cache - this is needed to rebuild the workloads
	}
	v.cache.SetHost(hostObj, true)
	if configChanged || stateMgrHost == nil {
		if ok, _ := v.cache.ValidateHost(hostObj); ok {
			pcacheWorkloads := v.cache.ListWorkloads(v.Ctx, true)
			// TODO: Keep a host -> workload mapping for better performance.
			// This should be done when host -> MAC mapping is added for non-pensando oui
			for _, wl := range pcacheWorkloads {
				if wl.Spec.HostName == hostObj.Name {
					v.Log.Infof("Rebuilding workload %s", wl.Name)
					v.rebuildWorkload(wl.Name)
				}
			}
		}
	}
	// create/update workloads for vmkNics
	v.syncHostVmkNics(penDC, dispName, m.Key, hConfig)
}

func (v *VCHub) populateDSCs(hostObj *defs.VCHubHost, dc *PenDC) {
	v.StateMgr.DscMapLock.RLock()
	validDSCs := map[string]bool{}
	hostObj.Spec.DSCs = []cluster.DistributedServiceCardID{}
	penDVS := dc.GetPenDVSForDC()
	if dc.isManagedMode() && penDVS == nil {
		// In managed mode process only PenDVS
		v.Log.Errorf("DVS for managed DC %s is not found", dc.DcName)
		v.StateMgr.DscMapLock.RUnlock()
		return
	}
	for dvsID, dvsProxy := range hostObj.DvsProxyInfo {
		// TODO: Check if a DC is in both managed and monitoring mode
		if dc.isManagedMode() && penDVS.DvsRef.Value != dvsID {
			// process only PenDVS in managed mode
			continue
		}
		if dc.isMonitoringMode() && penDVS != nil && penDVS.DvsRef.Value == dvsID {
			// skip PenDVS in monitoring mode
			continue
		}
		v.Log.Infof("host %s has %d connected pnics", hostObj.Name, len(dvsProxy.ConnectedPnics))
		for _, pnic := range dvsProxy.ConnectedPnics {
			// Finding DSC in the DSCMap indirectly confirms that this is Pensando Host
			if dscMac, ok := v.StateMgr.DscMap[pnic]; ok {
				v.Log.Infof("host %s has connected pnics from DSC", hostObj.Name)
				validDSCs[dscMac] = true
			}
		}
	}
	v.StateMgr.DscMapLock.RUnlock()

	DSCs := []cluster.DistributedServiceCardID{}
	for dsc := range validDSCs {
		DSCs = append(DSCs, cluster.DistributedServiceCardID{
			MACAddress: dsc,
		})
	}

	// Sort before storing, so that if we receive the Pnics
	// in a different order later we don't generate
	// an unnecessary update
	sort.Slice(DSCs, func(i, j int) bool {
		return DSCs[i].MACAddress < DSCs[j].MACAddress
	})
	hostObj.Spec.DSCs = DSCs
	v.Log.Infof("host %s has DSCs %v", hostObj.Name, hostObj.Spec.DSCs)
}

func (v *VCHub) processHostConfig(prop types.PropertyChange, dcID string, dcName string, hostObj *defs.VCHubHost) bool {
	propConfig, _ := prop.Val.(types.HostConfigInfo)
	hConfig := &propConfig

	nwInfo := hConfig.Network
	if nwInfo == nil {
		v.Log.Errorf("Missing hConfig.Network")
		return false
	}
	return v.populateHostInfo(dcName, hostObj, hConfig)
}

func (v *VCHub) fixStaleHost(hostID string, host *defs.VCHubHost) error {
	// check if there is another host with the same MACAddr (DSC)
	// If that host belongs to this VC it is likely that vcenter host-id changed for some
	// reason, delete that host and create new one.
	// TODO: If host was moved from one VCenter to another, we can just check if it has
	// some VC association and do the same?? (linked VC case)
	// List hosts
	hosts := v.cache.ListHosts(v.Ctx, false)
	var hostFound *defs.VCHubHost
	var matchingDSC string
searchHosts:
	for _, hostNIC := range host.Spec.DSCs {
		// Check for dsc object to get ID
		dsc, err := v.StateMgr.Controller().DistributedServiceCard().Find(&api.ObjectMeta{
			Name: hostNIC.MACAddress,
		})
		dscID := ""
		if err == nil {
			dscID = dsc.DistributedServiceCard.Spec.ID
		}
		for _, otherHost := range hosts {
			if host.Name == otherHost.Name {
				continue
			}
			for _, otherHostDSC := range otherHost.Spec.DSCs {
				if otherHostDSC.MACAddress == hostNIC.MACAddress || (dscID != "" && otherHostDSC.ID == dscID) {
					// Match found
					hostFound = otherHost
					matchingDSC = hostNIC.MACAddress
					if dsc != nil {
						matchingDSC = dsc.DistributedServiceCard.Spec.ID
					}
					break searchHosts
				}
			}
		}
	}

	if hostFound == nil {
		v.Log.Infof("No duplicate host found")
		return nil
	}
	// Verify host isn't in a disconnected state
	connState, err := v.probe.GetHostConnectionState(hostID, defaultRetryCount)
	if err == nil {
		if connState != types.HostSystemConnectionStateConnected {
			// Host is not in a connected state
			err = fmt.Errorf("Host %s %s has connection state %v", hostID, host.Name, connState)
			v.Log.Errorf("%s", err)
			return err
		}
	} else {
		errMsg := fmt.Errorf("Failed to fetch host %s %s, assuming to be disconnected. Err %s", hostID, host.Name, err)
		v.Log.Errorf("%s", errMsg)
		return errMsg
	}

	var vcID string
	ok := false
	if hostFound.Labels != nil {
		vcID, ok = hostFound.Labels[utils.OrchNameKey]
	}
	if !ok {
		err := fmt.Errorf("Duplicate Host %s is being used by non-VC application", hostFound.Name)
		v.Log.Infof("%s", err)
		evtMsg := fmt.Sprintf("%v : Failed to create host due to DSC %s being used by host %s. PSM host object must be deleted and host must be re-added to DVS.", v.State.OrchConfig.Name, matchingDSC, hostFound.Name)

		v.Log.Infof("%s", evtMsg)
		recorder.Event(eventtypes.ORCH_HOST_CONFLICT, evtMsg, hostFound)
		return err
	}

	if !utils.IsObjForOrch(hostFound.Labels, v.VcID, "") {
		// host found, but not used by this VC
		v.Log.Infof("Deleting host that belonged to another VC %s", vcID)
	} else if hostFound.Name == host.Name {
		// Host we found matches our state. Nothing to do.
		return nil
	}
	v.deleteHostState(hostFound, true)
	return nil
}

func (v *VCHub) hostRemovedFromDVS(host *defs.VCHubHost) {
	v.Log.Infof("Host %s Removed from DVS", host.Name)
	v.deleteHostState(host, false)
}

func (v *VCHub) deleteHostState(obj *defs.VCHubHost, deleteObj bool) {
	if obj.Labels == nil {
		// all hosts created from orchhub will have labels
		v.Log.Debugf("deleteHost - no labels")
		return
	}
	dcName, ok := obj.Labels[utils.NamespaceKey]
	if !ok {
		v.Log.Debugf("deleteHost - no namespace")
		return
	}
	penDC := v.GetDC(dcName)
	v.deleteHostStateFromDc(obj, penDC, deleteObj)

	return
}

func (v *VCHub) deleteHostStateFromDc(obj *defs.VCHubHost, penDC *PenDC, deleteObj bool) {
	if obj.Labels == nil {
		// all hosts created from orchhub will have labels
		v.Log.Debugf("deleteHostFromDc - no lables")
		return
	}
	hostName, ok := obj.Labels[NameKey]
	if penDC != nil && ok {
		if hKey, ok := penDC.findHostKeyByName(hostName); ok {
			// Delete vmkworkload
			vmkWlName := v.createVmkWorkloadName(penDC.DcRef.Value, hKey)
			v.deleteWorkloadByName(vmkWlName)
			penDC.delHostNameKey(hostName)
		}
	}
	if deleteObj {
		v.Log.Infof("Deleting host %s", obj.Name)
		// Delete from apiserver, but not from pcache so that we still have
		if err := v.cache.DeleteHost(obj); err != nil {
			v.Log.Errorf("Could not delete host from Venice %s", obj.Name)
		}
	}
}

// DeleteHosts deletes all host objects from API server for this VCHub instance
func (v *VCHub) DeleteHosts() {
	// List hosts
	hosts := v.cache.ListHosts(v.Ctx, false)
	for _, host := range hosts {
		if !utils.IsObjForOrch(host.Labels, v.VcID, "") {
			// Filter out hosts not for this Orch
			v.Log.Debugf("Skipping host %s", host.Name)
			continue
		}
		// Delete host
		v.deleteHostState(host, true)
	}
}

func (v *VCHub) getDCNameForHost(hostName string) string {
	hostObj := v.cache.GetHostByName(hostName)
	if hostObj == nil {
		v.Log.Errorf("Host %s not found", hostName)
		return ""
	}
	if hostObj.Labels == nil {
		v.Log.Errorf("Host %s has no labels", hostName)
		return ""
	}
	if dcName, ok := hostObj.Labels[utils.NamespaceKey]; ok {
		return dcName
	}
	v.Log.Errorf("Host %s has no namespace label", hostName)
	return ""
}

func (v *VCHub) updateVmkWorkloadLabels(hostName, dcName, dispName string) {
	wlName := createVmkWorkloadNameFromHostName(hostName)
	workloadObj := v.getWorkloadCopy(wlName)
	if workloadObj == nil {
		return
	}
	v.addWorkloadLabels(workloadObj.Workload, dispName, dcName)
}

func (v *VCHub) getHostDscInterface(hostName string, dvsID string, uplink string) (string, error) {
	var dscInterface string
	// Map uplink name to DSC Interface MAC
	// TODO: Test with LACP uplinks
	hostObj := v.cache.GetHostByName(hostName)
	if hostObj == nil {
		errMsg := fmt.Errorf("Host %s not found", hostName)
		v.Log.Errorf("%s", errMsg)
		return dscInterface, errMsg
	}
	dvsProxy, ok := hostObj.HostInfo.DvsProxyInfo[dvsID]
	if !ok {
		errMsg := fmt.Errorf("DVSproxy %s not found on host %s", dvsID, hostName)
		v.Log.Errorf("%s", errMsg)
		return dscInterface, errMsg
	}

	pnicName, ok := dvsProxy.UplinkPnics[uplink]
	if !ok {
		errMsg := fmt.Errorf("PG uplink %s is not connected", uplink)
		v.Log.Errorf("%s", errMsg)
		return dscInterface, errMsg
	}
	// TODO: Add a check for pensando DSC pnic if needed
	dscInterface, ok = hostObj.HostInfo.Pnics[pnicName]
	if !ok {
		errMsg := fmt.Errorf("pnic %s not found", pnicName)
		v.Log.Errorf("%s", errMsg)
		return "", errMsg
	}
	return dscInterface, nil
}

func (v *VCHub) populateHostInfo(dcName string, hostObj *defs.VCHubHost, hConfig *types.HostConfigInfo) bool {
	// populate addtional VC info for the host
	penDC := v.GetDC(dcName)
	if penDC == nil {
		return false
	}
	// Populate pnic name -> mac information, it can only change when host is created and/or host
	// was rebooted (with changes to nics)
	pnicsChanged := false
	hostPnics := map[string]string{}
	for _, pnic := range hConfig.Network.Pnic {
		macStr, err := conv.ParseMacAddr(pnic.Mac)
		if err != nil {
			v.Log.Errorf("Failed to parse Mac, %s", err)
			continue
		}
		oldMac, ok := hostObj.Pnics[pnic.Device]
		if !ok || oldMac != macStr {
			pnicsChanged = true
		}
		hostPnics[pnic.Device] = macStr
	}
	// check if any pnics got removed (new pnics and changes to mac are already detected)
	if len(hostObj.Pnics) != len(hostPnics) {
		pnicsChanged = true
	}
	if pnicsChanged {
		hostObj.Pnics = hostPnics
	}
	v.Log.Debugf("Host pnics for host %s: %v", hostObj.Name, hostObj.Pnics)
	// clear DVSproxyInfo map and re-populate it
	dvsProxyInfo := map[string]*defs.DVSProxyInfo{}
	for _, proxyInfo := range hConfig.Network.ProxySwitch {
		v.Log.Infof("Populate Proxy switch Info on %s for %s", hostObj.Name, proxyInfo.DvsName)
		// Get DVS key
		dvs := penDC.GetDVS(proxyInfo.DvsName)
		if dvs == nil {
			// DVS is not discovered yet - should not happen
			v.Log.Errorf("DVS %s is not discovered yet", proxyInfo.DvsName)
			continue
		}
		if _, ok := dvsProxyInfo[dvs.DvsRef.Value]; !ok {
			dvsProxyInfo[dvs.DvsRef.Value] = &defs.DVSProxyInfo{}
		}
		dvsProxy := dvsProxyInfo[dvs.DvsRef.Value]
		dvsProxy.ConnectedPnics = []string{}
		dvsProxy.UplinkPnics = map[string]string{}
		pnicBacking, ok := proxyInfo.Spec.Backing.(*types.DistributedVirtualSwitchHostMemberPnicBacking)
		if !ok {
			v.Log.Errorf("Pnic Backing Spec is missing")
			continue
		}
		uplinkKv := map[string]string{}
		for _, uplinkPort := range proxyInfo.UplinkPort {
			// "key": "uplink_name"
			uplinkKv[uplinkPort.Key] = uplinkPort.Value
		}

		if len(pnicBacking.PnicSpec) == 0 {
			v.Log.Infof("Pnic backing is empty")
		}
		for _, pnicSpec := range pnicBacking.PnicSpec {
			pnicDevice := pnicSpec.PnicDevice
			pnicKey := pnicSpec.UplinkPortKey
			pnicMac, ok := hostObj.Pnics[pnicDevice]
			if !ok {
				v.Log.Errorf("Cannot map %s to mac address", pnicDevice)
				continue
			}
			v.Log.Debugf("Find uplink for pnic %s", pnicDevice)
			if uplinkName, ok := uplinkKv[pnicKey]; ok {
				// "uplinkName": "vmnic_Name"
				dvsProxy.UplinkPnics[uplinkName] = pnicDevice
				dvsProxy.ConnectedPnics = append(dvsProxy.ConnectedPnics, pnicMac)
			}
		}
		// sort connectedPnics
		sort.Slice(dvsProxy.ConnectedPnics, func(i, j int) bool {
			return dvsProxy.ConnectedPnics[i] < dvsProxy.ConnectedPnics[j]
		})
		v.Log.Infof("Connected pnics for host %s on DVS %s: %v", hostObj.Name, dvs.DvsRef.Value,
			dvsProxyInfo[dvs.DvsRef.Value].ConnectedPnics)
		v.Log.Infof("Uplink pnics for host %s: %v", hostObj.Name,
			dvsProxyInfo[dvs.DvsRef.Value].UplinkPnics)
		dvsProxyInfo[dvs.DvsRef.Value] = dvsProxy
	}
	dvsInfoChanged := !reflect.DeepEqual(dvsProxyInfo, hostObj.DvsProxyInfo)
	if dvsInfoChanged {
		hostObj.DvsProxyInfo = dvsProxyInfo
	}
	if dvsInfoChanged || pnicsChanged {
		// commit the info to cache
		v.Log.Debugf("Commit HostInfo - %v - %v", hostObj.ObjectMeta, hostObj.HostInfo)
		v.cache.SetHostInfo(hostObj)
		return true
	}
	return false
}
