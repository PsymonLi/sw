package vchub

import (
	"fmt"
	"sync"
	"time"

	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api/generated/orchestration"
	apiserverutils "github.com/pensando/sw/api/hooks/apiserver/utils"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/useg"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/utils/events/recorder"
)

const (
	// DefaultMTU is the default MTU to be set in DVS
	DefaultMTU = 1500
)

type portEntry struct {
	vlan int
	host string
}

// PenDVS represents an instance of a Distributed Virtual Switch
type PenDVS struct {
	sync.Mutex
	*defs.State
	*DCShared
	DvsName string
	DvsRef  types.ManagedObjectReference
	// Map from PG display name to PenPG
	Pgs map[string]*PenPG
	// Map from PG ID to PenPG
	pgIDMap            map[string]*PenPG
	UsegMgr            useg.Inf
	writeTaskScheduled bool
	workloadsToWrite   map[string][]overrideReq
	ports              map[string]portEntry
	uplinkPortNames    []string // user given names to uplink ports - may not be needed
	dvsMode            string
}

// PenDVSPortSettings represents a group of DVS port settings
// The key of this map represents the key of port on a DVS
// The value of this map represents the new setting of this port
type PenDVSPortSettings map[string]*types.VMwareDVSPortSetting

// Only handles config spec being all creates, (not modifies or deletes)
func (d *PenDC) dvsConfigDiff(spec *types.DVSCreateSpec, dvs *mo.DistributedVirtualSwitch) *types.DVSCreateSpec {
	config, ok := dvs.Config.(*types.VMwareDVSConfigInfo)
	if !ok {
		d.Log.Infof("ConfigCheck: dvs.Config was of type %T", dvs.Config)
		return spec
	}

	configSpec, ok := spec.ConfigSpec.(*types.VMwareDVSConfigSpec)
	if !ok {
		d.Log.Infof("ConfigCheck: spec.ConfigSpec was of type %T", spec.ConfigSpec)
		return spec
	}
	pvlanItems := map[string]types.VMwareDVSPvlanMapEntry{}
	for _, item := range configSpec.PvlanConfigSpec {
		key := fmt.Sprintf("%d-%d", item.PvlanEntry.PrimaryVlanId, item.PvlanEntry.SecondaryVlanId)
		pvlanItems[key] = item.PvlanEntry
	}

	pvlanConfigSpecArray := []types.VMwareDVSPvlanConfigSpec{}

	for _, item := range config.PvlanConfig {
		key := fmt.Sprintf("%d-%d", item.PrimaryVlanId, item.SecondaryVlanId)
		entry, ok := pvlanItems[key]
		if !ok {
			// extra config - delete
			pvlanSpec := types.VMwareDVSPvlanConfigSpec{
				PvlanEntry: item,
				Operation:  "remove",
			}
			pvlanConfigSpecArray = append(pvlanConfigSpecArray, pvlanSpec)
			continue
		}
		delete(pvlanItems, key)
		if item.PvlanType != entry.PvlanType {
			// config modified, edit back
			pvlanSpecProm := types.VMwareDVSPvlanConfigSpec{
				PvlanEntry: entry,
				Operation:  "edit",
			}
			pvlanConfigSpecArray = append(pvlanConfigSpecArray, pvlanSpecProm)
		}
	}

	// Add remaining items
	for _, entry := range pvlanItems {
		pvlanSpec := types.VMwareDVSPvlanConfigSpec{
			PvlanEntry: entry,
			Operation:  "add",
		}
		pvlanConfigSpecArray = append(pvlanConfigSpecArray, pvlanSpec)
	}
	if len(pvlanConfigSpecArray) == 0 {
		d.Log.Infof("existing DVS matches config")
		return nil
	}

	newSpec := &types.DVSCreateSpec{
		ConfigSpec: &types.VMwareDVSConfigSpec{
			PvlanConfigSpec:             pvlanConfigSpecArray,
			MaxMtu:                      configSpec.MaxMtu,
			MulticastFilteringMode:      configSpec.MulticastFilteringMode,
			LinkDiscoveryProtocolConfig: configSpec.LinkDiscoveryProtocolConfig,
		},
	}
	return newSpec
}

// CreateDefaultDVSSpec returns the default create spec for a Pen DVS
func (d *PenDC) CreateDefaultDVSSpec() *types.DVSCreateSpec {
	dvsName := CreateDVSName(d.DcName)
	// Create a pen dvs
	// Pvlan allocations on the dvs
	pvlanConfigSpecArray := []types.VMwareDVSPvlanConfigSpec{}

	// Setup all the pvlan allocations now
	// vlans 0 and 1 are reserved
	for i := useg.FirstPGVlan; i < useg.FirstUsegVlan; i += 2 {
		PvlanEntryProm := types.VMwareDVSPvlanMapEntry{
			PrimaryVlanId:   int32(i),
			PvlanType:       "promiscuous",
			SecondaryVlanId: int32(i),
		}
		pvlanMapEntry := types.VMwareDVSPvlanMapEntry{
			PrimaryVlanId:   int32(i),
			PvlanType:       "isolated",
			SecondaryVlanId: int32(i + 1),
		}
		pvlanSpecProm := types.VMwareDVSPvlanConfigSpec{
			PvlanEntry: PvlanEntryProm,
			Operation:  "add",
		}
		pvlanSpec := types.VMwareDVSPvlanConfigSpec{
			PvlanEntry: pvlanMapEntry,
			Operation:  "add",
		}
		pvlanConfigSpecArray = append(pvlanConfigSpecArray, pvlanSpecProm)
		pvlanConfigSpecArray = append(pvlanConfigSpecArray, pvlanSpec)
	}

	// TODO: Set number of uplinks
	var spec types.DVSCreateSpec
	spec.ConfigSpec = &types.VMwareDVSConfigSpec{
		PvlanConfigSpec: pvlanConfigSpecArray,
	}
	spec.ConfigSpec.GetDVSConfigSpec().Name = dvsName
	spec.ProductInfo = new(types.DistributedVirtualSwitchProductSpec)
	spec.ProductInfo.Version = "6.5.0"
	return &spec
}

// AddUserDVS adds state for a user created dvs
func (d *PenDC) AddUserDVS(ref types.ManagedObjectReference, name string) {
	d.Lock()
	defer d.Unlock()

	dvs := d.getUserDVSByID(ref.Value)
	if dvs == nil {
		dvs = &PenDVS{
			State:    d.State,
			DCShared: d.DCShared,
			Pgs:      map[string]*PenPG{},
			pgIDMap:  map[string]*PenPG{},
			dvsMode:  orchestration.NamespaceSpec_Monitored.String(),
		}
		d.UserDvsIDMap[ref.Value] = dvs
	}
	dvs.Lock()
	dvs.DvsName = name
	dvs.DvsRef = ref
	dvs.Unlock()
}

// RemoveUserDVS removes state for a user created DVS
func (d *PenDC) RemoveUserDVS(id string) {
	d.Lock()
	defer d.Unlock()

	dvs := d.getUserDVSByID(id)
	if dvs == nil {
		return
	}
	delete(d.UserDvsIDMap, id)
}

// GetDVSConfig creates the DVS spec using the parameters passed in the orchestration config
func (d *PenDC) GetDVSConfig(nsSpec *orchestration.ManagedNamespaceSpec) *types.DVSCreateSpec {
	dcName := d.DcName
	dvsSpec := d.CreateDefaultDVSSpec()
	var spec orchestration.ManagedNamespaceSpec

	d.DcSpecLock.Lock()
	defer d.DcSpecLock.Unlock()

	if nsSpec == nil {
		ok := false
		spec, ok = d.ManagedDCs[dcName]
		if !ok {
			return dvsSpec
		}
	} else {
		spec = *nsSpec
	}

	dvsConfig := dvsSpec.ConfigSpec.(*types.VMwareDVSConfigSpec)
	switch spec.MulticastFilter {
	case orchestration.ManagedNamespaceSpec_Snooping.String():
		dvsConfig.MulticastFilteringMode = string(types.VMwareDvsMulticastFilteringModeSnooping)
	case orchestration.ManagedNamespaceSpec_Basic.String():
		dvsConfig.MulticastFilteringMode = string(types.VMwareDvsMulticastFilteringModeLegacyFiltering)
	}

	dvsConfig.LinkDiscoveryProtocolConfig = &types.LinkDiscoveryProtocolConfig{}
	switch spec.DiscoveryProtocol {
	// Select default Discovery operation as "Listen"
	case orchestration.ManagedNamespaceSpec_Disabled.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Protocol = string(types.LinkDiscoveryProtocolConfigProtocolTypeLldp)
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeNone)
	case orchestration.ManagedNamespaceSpec_LLDP.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Protocol = string(types.LinkDiscoveryProtocolConfigProtocolTypeLldp)
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeListen)
	case orchestration.ManagedNamespaceSpec_CDP.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Protocol = string(types.LinkDiscoveryProtocolConfigProtocolTypeCdp)
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeListen)
	}

	switch spec.DiscoveryOperation {
	case orchestration.ManagedNamespaceSpec_None.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeNone)
	case orchestration.ManagedNamespaceSpec_Advertise.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeAdvertise)
	case orchestration.ManagedNamespaceSpec_Both.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeBoth)
	case orchestration.ManagedNamespaceSpec_Listen.String():
		dvsConfig.LinkDiscoveryProtocolConfig.Operation = string(types.LinkDiscoveryProtocolConfigOperationTypeListen)
	}

	if spec.MTU != 0 {
		dvsConfig.MaxMtu = int32(spec.MTU)
	} else {
		dvsConfig.MaxMtu = DefaultMTU
	}

	// Todo : Add Number of uplinks
	return dvsSpec
}

// AddPenDVS adds a new PenDVS to the given vcprobe instance
func (d *PenDC) AddPenDVS() error {
	d.Lock()
	defer d.Unlock()

	dvsName := CreateDVSName(d.DcName)
	dcName := d.DcName
	dvsConfig := d.GetDVSConfig(nil)

	err := d.Probe.AddPenDVS(dcName, dvsConfig, d.dvsConfigDiff, defaultRetryCount)
	if err != nil {
		d.Log.Errorf("Failed to create %s in DC %s: %s", dvsName, dcName, err)
		return err
	}

	dvs, err := d.Probe.GetPenDVS(dcName, dvsName, defaultRetryCount)
	if err != nil {
		return err
	}

	penDVS := d.getPenDVS(dvsName)
	if penDVS == nil {
		useg, err := useg.NewUsegAllocator()
		if err != nil {
			d.Log.Errorf("Creating useg mgr for DC %s - penDVS %s failed, %v", dcName, dvsName, err)
			return err
		}
		penDVS = &PenDVS{
			State:            d.State,
			DCShared:         d.DCShared,
			DvsName:          dvsName,
			UsegMgr:          useg,
			Pgs:              map[string]*PenPG{},
			pgIDMap:          map[string]*PenPG{},
			ports:            map[string]portEntry{},
			workloadsToWrite: map[string][]overrideReq{},
			dvsMode:          orchestration.NamespaceSpec_Managed.String(),
		}

		d.PenDvsMap[dvsName] = penDVS
	}
	d.Log.Infof("adding dvs %s ( %s )", dvsName, dvs.Reference().Value)

	penDVS.DvsRef = dvs.Reference()

	d.State.DvsIDMapLock.Lock()
	d.State.DvsIDMap[dvsName] = dvs.Reference()
	d.State.DvsIDMapLock.Unlock()

	err = d.Probe.TagObjAsManaged(dvs.Reference())
	if err != nil {
		d.Log.Errorf("Failed to tag DVS as managed, %s", err)
		// Error isn't worth failing the operation for
	}

	return nil
}

// GetPenDVS returns the PenDVS with the given name
func (d *PenDC) GetPenDVS(dvsName string) *PenDVS {
	d.Lock()
	defer d.Unlock()
	return d.getPenDVS(dvsName)
}

// GetPenDVSForDC returns the PenDVS for a given DC
func (d *PenDC) GetPenDVSForDC() *PenDVS {
	dvsName := CreateDVSName(d.DcName)
	return d.GetPenDVS(dvsName)
}

func (d *PenDC) getPenDVS(dvsName string) *PenDVS {
	dvs, ok := d.PenDvsMap[dvsName]
	if !ok {
		return nil
	}
	return dvs
}

// GetDVS searches both penDVS map and userDVS map
func (d *PenDC) GetDVS(dvsName string) *PenDVS {
	// Check managed DVS first
	if dvs := d.getPenDVS(dvsName); dvs != nil {
		return dvs
	}
	return d.GetUserDVS(dvsName)
}

// GetUserDVS returns the userDVS with the given name
func (d *PenDC) GetUserDVS(dvsName string) *PenDVS {
	d.Lock()
	defer d.Unlock()
	for _, dvs := range d.UserDvsIDMap {
		dvs.Lock()
		if dvsName == dvs.DvsName {
			dvs.Unlock()
			return dvs
		}
		dvs.Unlock()
	}
	return nil
}

// GetDVSByID returns DVS object for a given ID
func (d *PenDC) GetDVSByID(dvsID string) *PenDVS {
	penDVS := d.GetPenDVSForDC()
	if penDVS != nil && penDVS.DvsRef.Value == dvsID {
		return penDVS
	}
	return d.GetUserDVSByID(dvsID)
}

// GetUserDVSByID returns the user dvs object with the given name
func (d *PenDC) GetUserDVSByID(dvsID string) *PenDVS {
	d.Lock()
	defer d.Unlock()
	return d.getUserDVSByID(dvsID)
}

func (d *PenDC) getUserDVSByID(dvsID string) *PenDVS {
	dvs, ok := d.UserDvsIDMap[dvsID]
	if !ok {
		return nil
	}
	return dvs
}

// GetDVSForPGID returns dvs object associated with a PG
func (d *PenDC) GetDVSForPGID(pgID string) *PenDVS {
	d.Lock()
	defer d.Unlock()
	for _, penDVS := range d.PenDvsMap {
		if penDVS.GetPGByID(pgID) != nil {
			return penDVS
		}
	}
	for _, penDVS := range d.UserDvsIDMap {
		if penDVS.GetPGByID(pgID) != nil {
			return penDVS
		}
	}
	return nil
}

// GetPortSettings returns the port settings of the dvs
func (d *PenDVS) GetPortSettings() ([]types.DistributedVirtualPort, error) {
	return d.Probe.GetPenDVSPorts(d.DcName, d.DvsName, &types.DistributedVirtualSwitchPortCriteria{}, 1)
}

type overrideReq struct {
	port       string
	vlan       int
	mac        string
	hostSystem string
}

// ReleasePort releases the port override state
func (d *PenDVS) ReleasePort(port string) {
	d.Log.Infof("Release port %s", port)
	d.Lock()
	delete(d.ports, port)
	d.Unlock()
}

// SetVMVlanOverrides sets all overrides for the given workload
func (d *PenDVS) SetVMVlanOverrides(interfaces []overrideReq, workloadName string, force bool) error {
	if len(interfaces) == 0 {
		return nil
	}
	// Get lock to prevent two different threads
	// configuring the dvs at the same time.
	d.Lock()
	defer d.Unlock()
	ports := vcprobe.PenDVSPortSettings{}
	req := map[string]int{}
	d.Log.Infof("SetVMVlanOverrides called for workload %s", workloadName)
	d.Log.Debugf("SetVMVlanOverrides called for workload %s with %+v", workloadName, interfaces)
	var portSettings map[string]bool
	var connectedHosts map[string]bool
	for _, inf := range interfaces {
		port := inf.port
		vlan := inf.vlan
		if entry, ok := d.ports[port]; ok && entry.vlan != vlan {
			// Already have an entry for a different override
			// Check if the override became unset on the DVS
			// workload.go should call ReleasePort, but this will
			// derive it as a failsafe.
			if portSettings == nil {
				// Fetch port and build portSettings
				portSettings = map[string]bool{}
				portInfo, err := d.GetPortSettings()
				if err != nil {
					d.Log.Errorf("Failed to get port settings: %s", err)
					return err // have this vlan request go to retry queue
				}
				for _, p := range portInfo {
					setting, ok := p.Config.Setting.(*types.VMwareDVSPortSetting)
					if ok {
						if vlanSpec, ok := setting.Vlan.(*types.VmwareDistributedVirtualSwitchVlanIdSpec); ok {
							if dvsEntry, ok := d.ports[p.Key]; ok && vlanSpec.VlanId == int32(dvsEntry.vlan) {
								// vlan override exists on this port and is equal to what our config says
								portSettings[p.Key] = true
							}
						}
					}
				}
				d.Log.Infof("Port settings: %v", portSettings)
			}
			if portSettings[port] {
				// DVS matches our state. Check if incoming request is for a host that is not connected
				if connectedHosts == nil {
					var err error
					connectedHosts, err = d.buildConnectedHostMap()
					if err != nil {
						return err // Request will be put on retryQueue
					}
				}
				if _, ok := connectedHosts[inf.hostSystem]; !ok {
					// This request is bad. Keep what is currently programmed
					d.Log.Errorf("Skipping vlan override for port %v, mac %v, vlan %v since the host %s is not connected to dvs", inf.port, inf.mac, inf.vlan, inf.hostSystem)
					continue
				}
			}
		}
		ports[port] = &types.VmwareDistributedVirtualSwitchVlanIdSpec{
			VlanId: int32(vlan),
		}
		req[port] = vlan
		d.ports[port] = portEntry{
			vlan: vlan,
			host: inf.hostSystem,
		}
	}
	if len(ports) == 0 {
		d.Log.Infof("No port overrides to apply")
		return nil
	}
	err := d.Probe.UpdateDVSPortsVlan(d.DcName, d.DvsName, ports, force, defaultRetryCount)
	if err != nil {
		d.Log.Errorf("Failed to set vlan override for DC %s - dvs %s, err %s", d.DcName, d.DvsName, err)

		d.Log.Errorf("Failed request was %+v", req)

		evtMsg := fmt.Sprintf("%v : Failed to set vlan override in Datacenter %s for workload %s. Traffic may be impacted. %v", d.State.OrchConfig.Name, d.DcName, workloadName, err)

		if workloadName != "" && d.Ctx.Err() == nil && d.Probe.IsSessionReady() {
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, d.State.OrchConfig)
		}
		return err
	}
	return nil
}

func (v *VCHub) verifyOverridesOnDVS(dvs *PenDVS, forceWrite bool) {
	v.Log.Infof("Verify overrides running for dvs %s, forceWrite %v", dvs.DvsName, forceWrite)
	count := 3
	for !v.probe.IsSessionReady() && count > 0 {
		select {
		case <-v.Ctx.Done():
			return
		case <-time.After(1 * time.Second):
			count--
		}
	}
	if count == 0 {
		v.Log.Errorf("Probe session isn't connected")
		return
	}

	dvs.Lock()
	defer dvs.Unlock()
	dvsName := dvs.DvsName
	dcName := dvs.DcName
	ports := []types.DistributedVirtualPort{}
	if !forceWrite {
		var err error
		ports, err = dvs.GetPortSettings()
		if err != nil {
			v.Log.Errorf("Failed to get ports for dc %s dvs %s, %s", dcName, dvsName, err)
			return
		}
	}
	// extract overrides
	currOverrides := map[string]int{}
	for _, port := range ports {
		portKey := port.Key
		portSetting, ok := port.Config.Setting.(*types.VMwareDVSPortSetting)
		if !ok {
			continue
		}
		vlanSpec, ok := portSetting.Vlan.(*types.VmwareDistributedVirtualSwitchVlanIdSpec)
		if !ok {
			continue
		}
		if vlanSpec.VlanId == 0 {
			v.Log.Infof("Vlan override for port %s is 0", portKey)
			continue
		}
		currOverrides[portKey] = int(vlanSpec.VlanId)
	}

	workloads := v.cache.ListWorkloads(v.Ctx, false)

	workloadOverride := map[string]struct {
		vlan int
		host string
	}{}
	for _, workload := range workloads {
		if !utils.IsObjForOrch(workload.Labels, v.VcID, dcName) {
			// Filter out workloads not for this Orch/DC
			v.Log.Debugf("Skipping workload %s", workload.Name)
			continue
		}

		host := workload.Spec.HostName

		// if workload is migrating, skip processing
		if apiserverutils.IsWorkloadMigrating(workload.Workload) {
			v.Log.Debugf("Skipping migrating workload %s", workload.Name)
			continue
		}

		vnics := workload.Vnics
		if len(vnics) == 0 {
			v.Log.Debugf("No VNIC info for workload %s", workload.Name)
			continue
		}

		var connectedHosts map[string]bool
		for _, entry := range vnics {
			if entry.PortOverrideSet {
				vlan, err := dvs.UsegMgr.GetVlanForVnic(entry.MacAddress, host)
				if err != nil {
					v.Log.Errorf("Failed to get vlan for vnic workload %s mac %s", workload.Name, entry.MacAddress)
					continue
				}
				if oldEntry, ok := workloadOverride[entry.Port]; ok {
					v.Log.Errorf("Duplicate info for port %s, Mac %s, new vlan %v new host %s, old vlan %v old host %s", entry.Port, entry.MacAddress, vlan, host, oldEntry.vlan, oldEntry.host)
					dvsEntry, ok := dvs.ports[entry.Port]
					// Take whichever is set in dvs ports map
					if ok && dvsEntry.vlan == oldEntry.vlan {
						// old value is correct
						v.Log.Infof("Dvs ports has old vlan set for the port. Skipping this entry")
						continue
					} else if ok && dvsEntry.vlan == vlan {
						// new value is correct. set in map
						v.Log.Infof("Dvs ports has new vlan set for the port. Updating override map")
					} else {
						// This shouldn't happen. Fetch dvs host members and figure out which is correct
						v.Log.Infof("Dvs ports doesn't match either vlan. Fetching host members")
						if connectedHosts == nil {
							var err error
							connectedHosts, err = dvs.buildConnectedHostMap()
							if err != nil {
								v.Log.Infof("Failed to get connected host map, skipping entry...")
								continue
							}
						}
						if _, ok := connectedHosts[host]; !ok {
							// This request is bad. Keep what is currently programmed
							v.Log.Errorf("Skipping vlan override for port %v, mac %v, vlan %v since the host %s is not connected to dvs", entry.Port, entry.MacAddress, vlan, host)
							continue
						}
						v.Log.Infof("Taking newer port entry")
					}
				}
				workloadOverride[entry.Port] = struct {
					vlan int
					host string
				}{
					vlan: vlan,
					host: host,
				}
			}
		}
	}
	portSetting := vcprobe.PenDVSPortSettings{}

	for port, entry := range workloadOverride {
		if currVlan, ok := currOverrides[port]; !ok || currVlan != entry.vlan {
			portSetting[port] = &types.VmwareDistributedVirtualSwitchVlanIdSpec{
				VlanId: int32(entry.vlan),
			}
		}
	}
	if len(portSetting) == 0 {
		v.Log.Debugf("No overrides to apply")
		return
	}

	v.Log.Infof("Writing back overrides %+v", workloadOverride)
	err := v.probe.UpdateDVSPortsVlan(dcName, dvsName, portSetting, forceWrite, defaultRetryCount)
	if err != nil {
		v.Log.Errorf("Failed to set vlan overrides for DC %s - dvs %s, err %s", dcName, dvsName, err)
		v.Log.Errorf("Failed request was %+v", workloadOverride)
	} else if !forceWrite {
		// If there are any changes someone else must have modified the DVS. Raise event...
		evtMsg := fmt.Sprintf("%v : Port overrides were modified for DVS %v in Datacenter %v. Overrides have been changed back.", v.State.OrchConfig.Name, dvsName, dcName)
		recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)
	}
}

func (d *PenDVS) buildConnectedHostMap() (map[string]bool, error) {
	connectedHosts := map[string]bool{}
	dvsObj, err := d.Probe.GetDVSConfig(d.DcName, d.DvsName, defaultRetryCount)
	if err != nil {
		d.Log.Errorf("Failed to get penDVS: %s", err)
		return nil, err
	}
	if dvsObj.Runtime != nil {
		for _, hostEntry := range dvsObj.Runtime.HostMemberRuntime {
			connectedHosts[hostEntry.Host.Value] = true
		}
	}
	d.Log.Infof("connected hosts: %v", connectedHosts)
	return connectedHosts, nil
}

func (v *VCHub) verifyOverrides(forceWrite bool) {
	v.Log.Infof("Verify overrides running")
	count := 3
	for !v.probe.IsSessionReady() && count > 0 {
		select {
		case <-v.Ctx.Done():
			return
		case <-time.After(1 * time.Second):
			count--
		}
	}
	if count == 0 {
		v.Log.Errorf("Probe session isn't connected")
		return
	}
	if !v.IsSyncDone() {
		// Wait for sync to complete
		v.Log.Errorf("Sync has not completed")
		return
	}

	wg := sync.WaitGroup{}
	processDC := func(dc *PenDC) {
		defer wg.Done()

		dvsWg := sync.WaitGroup{}
		dc.Lock()
		for _, dvs := range dc.PenDvsMap {
			dvsWg.Add(1)
			go func(dvsObj *PenDVS) {
				defer dvsWg.Done()
				v.verifyOverridesOnDVS(dvsObj, false)
			}(dvs)
		}
		dc.Unlock()

		dvsWg.Wait()
	}

	v.DcMapLock.Lock()
	for _, dc := range v.DcMap {
		wg.Add(1)
		go processDC(dc)
	}
	v.DcMapLock.Unlock()

	wg.Wait()
	v.Log.Infof("Verify overrides done")
}

// Resetting vlan overrides is not needed. As soon as port is disconnected,
// the pvlan settings are restored.
//
// SetPvlanForPorts undoes the vlan override and restores the given ports with the pvlan of the given pg
// Input is a map from pg name to ports to set
// func (d *PenDVS) SetPvlanForPorts(pgMap map[string][]string) error {
// 	d.Log.Debugf("SetPvlanForPorts called with %v", pgMap)
// 	portSetting := vcprobe.PenDVSPortSettings{}
// 	for pg, ports := range pgMap {
// 		_, vlan2, err := d.UsegMgr.GetVlansForPG(pg)
// 		if err != nil {
// 			// Don't have assignments for this PG.
// 			d.Log.Infof("PG %s had no vlans, err %s", pg, err)
// 			continue
// 		}
// 		for _, p := range ports {
// 			portSetting[p] = &types.VmwareDistributedVirtualSwitchPvlanSpec{
// 				PvlanId: int32(vlan2),
// 			}
// 		}
// 	}
// 	err := d.probe.UpdateDVSPortsVlan(d.DcName, d.DvsName, portSetting, 1)
// 	if err != nil {
// 		d.Log.Errorf("Failed to set pvlans back for DC %s - dvs %s, err %s", d.DcName, d.DvsName, err)
// 		return err
// 	}
// 	return nil
// }

func (v *VCHub) handleDVS(m defs.VCEventMsg) {
	// Determine type of DVS
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Got handle DVS event for DVS %s in DC %s %s", m.Key, dcName, m.DcID)

	penDC := v.GetDCFromID(m.DcID)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", dcName)
		return
	}

	processed := false
	if penDC.isManagedMode() {
		dvsName := CreateDVSName(dcName)
		dvs := penDC.GetPenDVS(dvsName)
		if dvs != nil && m.Key == dvs.DvsRef.Value {
			processed = true
			v.handlePenDVS(m)
		}
	}
	if !processed && penDC.isMonitoringMode() {
		v.handleUserDVS(m)
	}
}

func (v *VCHub) handleUserDVS(m defs.VCEventMsg) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Got handle User DVS event for DVS %s in DC %s %s", m.Key, dcName, m.DcID)
	penDC := v.GetDCFromID(m.DcID)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", m.DcID)
		return
	}
	dvs := penDC.GetUserDVSByID(m.Key)
	// Create DVS object if needed.
	// Rename if needed.
	if m.UpdateType == types.ObjectUpdateKindLeave {
		if dvs == nil {
			// nothing to do
			return
		}
		// Object was deleted
		v.Log.Infof("User DVS %s was removed", dvs.DvsName)
		penDC.RemoveUserDVS(m.Key)
		return
	}

	// extract config
	if len(m.Changes) == 0 {
		v.Log.Errorf("Received dvs event with no changes")
		return
	}

	var name string

	for _, prop := range m.Changes {
		switch val := prop.Val.(type) {
		case types.VMwareDVSConfigInfo:
		case string:
			name = val
		default:
			v.Log.Errorf("Got unexpected type in dvs changes, got %T", val)
		}
	}
	if dvs == nil {
		ref := types.ManagedObjectReference{
			Type:  string(defs.VmwareDistributedVirtualSwitch),
			Value: m.Key,
		}
		penDC.AddUserDVS(ref, name)
		return
	}
	if len(name) != 0 && dvs.DvsName != name {
		v.Log.Infof("User DVS renamed from %s to %s", dvs.DvsName, name)
		dvs.Lock()
		dvs.DvsName = name
		dvs.Unlock()
	}
}

func (v *VCHub) handlePenDVS(m defs.VCEventMsg) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Got handle DVS event for DVS %s in DC %s", m.Key, dcName)
	// If it is pensando PG, pvlan config and dvs name are untouched
	// check name change in same loop

	penDC := v.GetDC(dcName)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", dcName)
		return
	}
	dvsName := CreateDVSName(dcName)
	dvs := penDC.GetPenDVS(dvsName)
	if dvs == nil {
		v.Log.Errorf("DVS state for DC %s was nil", dcName)
		return
	}

	if m.Key != dvs.DvsRef.Value {
		// not pensando DVS
		v.Log.Debugf("Change to a non pensando DVS, ignore")
		return
	}

	if m.UpdateType == types.ObjectUpdateKindLeave {
		// Object was deleted, recreate
		// Remove ID first
		v.State.DvsIDMapLock.Lock()
		if _, ok := v.State.DvsIDMap[dvsName]; ok {
			delete(v.State.DvsIDMap, dvsName)
		}
		v.State.DvsIDMapLock.Unlock()

		err := penDC.AddPenDVS()
		if err != nil {
			v.Log.Errorf("Failed to recreate DVS for DC %s, %s", dcName, err)
			// Generate event
			if v.Ctx.Err() == nil && v.probe.IsSessionReady() {
				evtMsg := fmt.Sprintf("%v : Failed to recreate DVS in Datacenter %s. %v", v.State.OrchConfig.Name, dcName, err)
				recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, v.State.OrchConfig)
			}
		} else {
			// Recreate PGs
			v.checkNetworks(dcName)
		}
		return
	}

	// extract config
	if len(m.Changes) == 0 {
		v.Log.Errorf("Received dvs event with no changes")
		return
	}

	var dvsConfig *types.VMwareDVSConfigInfo
	var name string

	for _, prop := range m.Changes {
		switch val := prop.Val.(type) {
		case types.VMwareDVSConfigInfo:
			dvsConfig = &val
		case string:
			name = val
		default:
			v.Log.Errorf("Got unexpected type in dvs changes, got %T", val)
		}
	}

	// Check if we need to rename
	if len(name) != 0 && dvs.DvsName != name {
		// DVS renamed
		evtMsg := fmt.Sprintf("%v : User renamed a Pensando created DVS in Datacenter %v from %v to %v. Name has been changed back.", v.State.OrchConfig.Name, dcName, dvs.DvsName, name)
		recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)

		// Put object name back
		err := v.probe.RenameDVS(dcName, name, dvs.DvsName, defaultRetryCount)
		if err != nil {
			v.Log.Errorf("Failed to rename DVS, %s", err)
		}
	}

	if dvsConfig == nil {
		v.Log.Debugf("No DVS config change")
		return
	}

	spec := penDC.CreateDefaultDVSSpec()

	dvsMo := &mo.DistributedVirtualSwitch{
		Config: dvsConfig,
	}

	// Check config
	diff := penDC.dvsConfigDiff(spec, dvsMo)
	if diff == nil {
		v.Log.Debugf("Config is equal, nothing to write back")
		return
	}
	v.Log.Infof("DVS %s change detected, writing back", dvs.DvsName)
	err := penDC.AddPenDVS()
	if err != nil {
		v.Log.Errorf("Failed to write DVS %s config back, err %s ", dvs.DvsName, err)

		// Generate event
		if v.Ctx.Err() == nil && v.probe.IsSessionReady() {
			evtMsg := fmt.Sprintf("%v : Failed to write DVS %s config back to Datacenter %v. %v", v.State.OrchConfig.Name, dvs.DvsName, dcName, err)
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, v.State.OrchConfig)
		}
		return
	}

	evtMsg := fmt.Sprintf("%v : User modified vlan settings for Pensando created DVS %v in Datacenter %v. DVS settings have been changed back.", v.State.OrchConfig.Name, dvs.DvsName, dcName)
	recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)
}

func (d *PenDVS) isMonitoringMode() bool {
	d.Lock()
	defer d.Unlock()
	return d.dvsMode == orchestration.NamespaceSpec_Monitored.String()
}

func (d *PenDVS) isManagedMode() bool {
	d.Lock()
	defer d.Unlock()
	return d.dvsMode == orchestration.NamespaceSpec_Managed.String()
}
