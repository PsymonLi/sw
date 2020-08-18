package vchub

import (
	"fmt"
	"sort"
	"sync"

	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/defs"
	"github.com/pensando/sw/venice/ctrler/orchhub/orchestrators/vchub/vcprobe"
	"github.com/pensando/sw/venice/utils/events/recorder"
)

// PenPG represents an instance of a port group on a DVS
type PenPG struct {
	sync.Mutex
	*defs.State
	*DCShared
	penDVS      *PenDVS
	PgName      string
	PgRef       types.ManagedObjectReference
	NetworkMeta api.ObjectMeta
	PgTeaming   PGTeamingInfo
	VlanID      int32 // used in monitoring mode - until PSM can support multiple networks with same vlan ID
}

// PGTeamingInfo stores all active and standby uplinks used by a PG
type PGTeamingInfo struct {
	uplinks []string // store all active and standby links ["uplink1", "uplink2"...]
}

// AddPenPG creates a PG
func (d *PenDVS) AddPenPG(pgName string, networkMeta api.ObjectMeta) error {
	primaryVlan, secondaryVlan, err := d.UsegMgr.GetVlansForPG(pgName)
	if err != nil {
		{
			primaryVlan, secondaryVlan, err = d.UsegMgr.AssignVlansForPG(pgName)
			if err != nil {
				d.Log.Errorf("Failed to assign vlans for PG")
				if err.Error() == "PG limit reached" {
					// Generate error
					evtMsg := fmt.Sprintf("%v : Failed to create port group %s in Datacenter %s. Max port group limit reached.", d.State.OrchConfig.Name, networkMeta.Name, d.DcName)
					d.Log.Errorf(evtMsg)

					recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, d.State.OrchConfig)
				}
				return err
			}
		}
	}
	d.Log.Debugf("Adding PG %s with pvlan of %d and %d", pgName, primaryVlan, secondaryVlan)
	err = d.AddPenPGWithVlan(pgName, networkMeta, primaryVlan, secondaryVlan)
	if err != nil {
		evtMsg := fmt.Sprintf("%v : Failed to set configuration for port group %s in Datacenter %s. %v", d.State.OrchConfig.Name, networkMeta.Name, d.DcName, err)
		d.Log.Errorf("%s, err %s", evtMsg, err)
		if d.Ctx.Err() == nil && d.Probe.IsSessionReady() {
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, d.State.OrchConfig)
		}
	}
	return err
}

func (d *PenDVS) createPGConfigCheck(pvlan int) vcprobe.IsPGConfigEqual {
	return func(spec *types.DVPortgroupConfigSpec, config *mo.DistributedVirtualPortgroup) bool {

		policy, ok := config.Config.Policy.(*types.VMwareDVSPortgroupPolicy)
		if !ok {
			d.Log.Infof("ConfigCheck: dvs.Config was of type %T", config.Config.Policy)
			return false
		}
		if !policy.VlanOverrideAllowed ||
			!policy.PortConfigResetAtDisconnect {
			d.Log.Infof("ConfigCheck: dvs policy settings were incorrect")
			return false
		}
		portConfig, ok := config.Config.DefaultPortConfig.(*types.VMwareDVSPortSetting)
		if !ok {
			d.Log.Infof("ConfigCheck: portConfig was of type %T", config.Config.DefaultPortConfig)
			return false
		}
		pvlanConfig, ok := portConfig.Vlan.(*types.VmwareDistributedVirtualSwitchPvlanSpec)
		if !ok {
			d.Log.Infof("ConfigCheck: pvlanConfig was of type %T", portConfig.Vlan)
			return false
		}
		if pvlanConfig.PvlanId != int32(pvlan) {
			d.Log.Infof("ConfigCheck: pvlan did not match")
			return false
		}
		return true
	}
}

// AddPenPGWithVlan creates a PG with the given pvlan values
func (d *PenDVS) AddPenPGWithVlan(pgName string, networkMeta api.ObjectMeta, primaryVlan, secondaryVlan int) error {
	d.Lock()
	defer d.Unlock()

	spec := types.DVPortgroupConfigSpec{
		Name: pgName,
		Type: string(types.DistributedVirtualPortgroupPortgroupTypeEarlyBinding),
		Policy: &types.VMwareDVSPortgroupPolicy{
			DVPortgroupPolicy: types.DVPortgroupPolicy{
				BlockOverrideAllowed:               true,
				ShapingOverrideAllowed:             false,
				VendorConfigOverrideAllowed:        false,
				LivePortMovingAllowed:              false,
				PortConfigResetAtDisconnect:        true,
				NetworkResourcePoolOverrideAllowed: types.NewBool(false),
				TrafficFilterOverrideAllowed:       types.NewBool(false),
			},
			VlanOverrideAllowed:           true,
			UplinkTeamingOverrideAllowed:  false,
			SecurityPolicyOverrideAllowed: false,
			IpfixOverrideAllowed:          types.NewBool(false),
		},
		DefaultPortConfig: &types.VMwareDVSPortSetting{
			Vlan: &types.VmwareDistributedVirtualSwitchPvlanSpec{
				PvlanId: int32(secondaryVlan),
			},
			// Teaming policy is not setup here. Let vcenter set the defaults
			// which we will read-back later
		},
	}
	moPG, err := d.Probe.GetPGConfig(d.DcName, pgName, []string{"config"}, 1)

	if err == nil {
		// If PG exists on the vcenter, update required info from vcenter
		policySpec, ok := moPG.Config.Policy.(*types.VMwareDVSPortgroupPolicy)
		if ok {
			policySpec.VlanOverrideAllowed = true
			policySpec.PortConfigResetAtDisconnect = true
			spec.Policy = policySpec
		}
		portSetting := moPG.Config.DefaultPortConfig.(*types.VMwareDVSPortSetting)
		teamingPolicy := portSetting.UplinkTeamingPolicy
		specPortSetting := spec.DefaultPortConfig.(*types.VMwareDVSPortSetting)
		specPortSetting.UplinkTeamingPolicy = teamingPolicy
	}

	err = d.Probe.AddPenPG(d.DcName, d.DvsName, &spec, d.createPGConfigCheck(secondaryVlan), defaultRetryCount)
	if err != nil {
		return err
	}

	pg, err := d.Probe.GetPenPG(d.DcName, pgName, defaultRetryCount)
	if err != nil {
		return err
	}

	penPG := d.getPG(pgName)
	if penPG == nil {
		penPG = &PenPG{
			State:    d.State,
			DCShared: d.DCShared,
			PgName:   pgName,
			penDVS:   d,
		}
		d.Pgs[pgName] = penPG
	}
	// Get the teaming config from vcenter and populate the local information
	moPG, err = d.Probe.GetPGConfig(d.DcName, pgName, []string{"config"}, defaultRetryCount)
	if err != nil {
		return err
	}
	penPG.PopulateTeamingInfo(&moPG.Config)

	penPG.PgRef = pg.Reference()
	d.pgIDMap[pg.Reference().Value] = penPG

	penPG.NetworkMeta = networkMeta

	d.Log.Infof("adding PG %s ( %s ) for network %s", penPG.PgName, penPG.PgRef.Value, networkMeta.Name)

	err = d.Probe.TagObjAsManaged(penPG.PgRef)
	if err != nil {
		d.Log.Errorf("Failed to tag PG %s as managed, %s", pgName, err)
		// Error isn't worth failing the operation for
	}

	nw, err := d.StateMgr.Controller().Network().Find(&networkMeta)
	if err == nil {
		externalVlan := int(nw.Spec.VlanID)
		err = d.Probe.TagObjWithVlan(penPG.PgRef, externalVlan)
		if err != nil {
			d.Log.Errorf("Failed to tag PG %s with vlan tag, %s", pgName, err)
			// Error isn't worth failing the operation for
		}
	} else {
		d.Log.Errorf("Couldn't tag PG %s with vlan tag since we couldn't find the network info: networkMeta %v, err %s", pgName, networkMeta, err)
		// Error isn't worth failing the operation for
	}

	return nil
}

// AddUserPG adds or updates a user PG entry
func (d *PenDVS) AddUserPG(pgName, pgID string, vlan int32) {
	d.Lock()
	defer d.Unlock()
	d.Log.Infof("Creating user PG %s with vlan %v", pgID, vlan)
	penPG, ok := d.pgIDMap[pgID]
	if !ok {
		penPG = &PenPG{
			State:    d.State,
			DCShared: d.DCShared,
			PgName:   pgName,
			penDVS:   d,
		}
		ref := types.ManagedObjectReference{
			Type:  string(defs.DistributedVirtualPortgroup),
			Value: pgID,
		}
		penPG.PgRef = ref
		d.pgIDMap[pgID] = penPG
	}
	if penPG.PgName != pgName {
		delete(d.Pgs, pgName)
	}
	d.Pgs[pgName] = penPG
	penPG.VlanID = vlan
}

// RemoveUserPG removes a user PG
func (d *PenDVS) RemoveUserPG(pgID string) {
	d.Lock()
	defer d.Unlock()
	penPG, ok := d.pgIDMap[pgID]
	if !ok {
		return
	}
	delete(d.Pgs, penPG.PgName)
	delete(d.pgIDMap, pgID)
}

// PopulateTeamingInfo returns whether the teaming info was updated
func (d *PenPG) PopulateTeamingInfo(config *types.DVPortgroupConfigInfo) bool {
	policy, ok := config.DefaultPortConfig.(*types.VMwareDVSPortSetting)
	if !ok {
		d.Log.Infof("ConfigCheck: pg Config was of type %T", config.Policy)
		return false
	}
	uplinks := []string{}
	if policy.UplinkTeamingPolicy == nil || policy.UplinkTeamingPolicy.UplinkPortOrder == nil {
		d.Log.Infof("Teaming info was empty")
	} else {
		for _, uplink := range policy.UplinkTeamingPolicy.UplinkPortOrder.ActiveUplinkPort {
			uplinks = append(uplinks, uplink)
		}
		for _, uplink := range policy.UplinkTeamingPolicy.UplinkPortOrder.StandbyUplinkPort {
			uplinks = append(uplinks, uplink)
		}
	}
	sort.Strings(uplinks)

	isEq := func(a, b []string) bool {
		if len(a) != len(b) {
			return false
		}

		for i := range a {
			if a[i] != b[i] {
				return false
			}
		}
		return true
	}

	if !isEq(d.PgTeaming.uplinks, uplinks) {
		d.PgTeaming.uplinks = uplinks
		return true
	}
	return false

}

// GetPG returns the PenPG with the given name
func (d *PenDVS) GetPG(pgName string) *PenPG {
	d.Lock()
	defer d.Unlock()
	return d.getPG(pgName)
}

func (d *PenDVS) getPG(pgName string) *PenPG {
	pg, ok := d.Pgs[pgName]
	if !ok {
		return nil
	}
	return pg
}

// GetPGByID fetches the pen PG object by ID
func (d *PenDVS) GetPGByID(pgID string) *PenPG {
	d.Lock()
	defer d.Unlock()
	pg, ok := d.pgIDMap[pgID]
	if !ok {
		return nil
	}
	return pg
}

// RemovePenPG removes the pg from the dvs
func (d *PenDVS) RemovePenPG(pgName string) error {
	d.Lock()
	defer d.Unlock()

	var ref types.ManagedObjectReference
	if penPG, ok := d.Pgs[pgName]; ok {
		ref = penPG.PgRef
		id := penPG.PgRef.Value
		delete(d.Pgs, pgName)

		if _, ok := d.pgIDMap[id]; ok {
			delete(d.pgIDMap, id)
		} else {
			d.Log.Errorf("Removed entry in PG map that wasn't in pgIDMap, pgName %s", pgName)
		}
	}

	err := d.Probe.RemovePenPG(d.DcName, pgName, defaultRetryCount)
	if err != nil {
		d.Log.Errorf("Failed to delete PG %s, removing management tag. Err %s", pgName, err)
		tagErrs := d.Probe.RemovePensandoTags(ref)
		if len(tagErrs) != 0 {
			d.Log.Errorf("Failed to remove tags, errs %v", tagErrs)
		}
		return err
	}

	return nil
}

func (v *VCHub) handlePenPG(m defs.VCEventMsg, dvs *PenDVS, pgConfig *types.DVPortgroupConfigInfo) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Processing handle PenPG event in PenDVS for PG %s in DC %s", m.Key, dcName)
	penDC := v.GetDCFromID(m.DcID)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", dcName)
		return
	}

	if pgConfig.DistributedVirtualSwitch.Reference().Value != dvs.DvsRef.Value {
		// Not for pensando DVS
		v.Log.Debugf("Skipping PG event as its not attached to a PenDVS")
		return
	}

	// Check name change
	penPG := dvs.GetPGByID(m.Key)
	if penPG == nil {
		// Not pensando PG
		v.Log.Debugf("Not a pensando PG - %s", m.Key)
		return
	}

	teamingChanged := penPG.PopulateTeamingInfo(pgConfig)
	if teamingChanged {
		v.Log.Infof("PG teaming info changed")
		// Rebuild workloads
		pcacheWorkloads := v.cache.ListWorkloads(v.Ctx, true)
		for _, wl := range pcacheWorkloads {
			for _, vnic := range wl.VMInfo.Vnics {
				if vnic.PG == m.Key {
					v.Log.Infof("Rebuilding workload %s", wl.Name)
					v.rebuildWorkload(wl.Name)
					break
				}
			}
		}
	}

	if penPG.PgName != pgConfig.Name {
		evtMsg := fmt.Sprintf("%v : User renamed Pensando created port group from %v to %v. Port group name has been changed back.", v.State.OrchConfig.Name, penPG.PgName, pgConfig.Name)
		recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)
		// Put object name back
		err := v.probe.RenamePG(dcName, pgConfig.Name, penPG.PgName, defaultRetryCount)
		if err != nil {
			v.Log.Errorf("Failed to rename PG, %s", err)
		}
		// Don't check vlan config now, name change will trigger another event
		return
	}

	// Check vlan config
	// Pensando PG, reset config if changed
	_, secondary, err := dvs.UsegMgr.GetVlansForPG(pgConfig.Name)
	if err != nil {
		v.Log.Errorf("Failed to get assigned vlans for PG %s", err)
		return
	}

	pgMo := &mo.DistributedVirtualPortgroup{
		Config: *pgConfig,
	}
	equal := dvs.createPGConfigCheck(secondary)(nil, pgMo)
	if equal {
		v.Log.Debugf("Config is equal, nothing to write back")
		return
	}
	v.Log.Infof("PG %s change detected, writing back", pgConfig.Name)
	// Vlan spec is not what we expect, set it back
	err = dvs.AddPenPG(pgConfig.Name, penPG.NetworkMeta)
	if err != nil {
		v.Log.Errorf("Failed to set vlan config for PG %s, %s", pgConfig.Name, err)

		evtMsg := fmt.Sprintf("%v : Failed to set vlan config for PG %s. %s", v.State.OrchConfig.Name, pgConfig.Name, err)
		if v.Ctx.Err() == nil && v.probe.IsSessionReady() {
			recorder.Event(eventtypes.ORCH_CONFIG_PUSH_FAILURE, evtMsg, v.State.OrchConfig)
		}
		return
	}

	evtMsg := fmt.Sprintf("%v : User modified vlan settings for Pensando created port group %v. Port group settings have been changed back.", v.State.OrchConfig.Name, pgConfig.Name)
	recorder.Event(eventtypes.ORCH_INVALID_ACTION, evtMsg, v.State.OrchConfig)
}

func (v *VCHub) handleUserPG(m defs.VCEventMsg, userDVS *PenDVS, pgConfig *types.DVPortgroupConfigInfo) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Processing handle User PG event in DVS for PG %s in DC %s", m.Key, dcName)
	penDC := v.GetDCFromID(m.DcID)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", dcName)
		return
	}

	pg := penDC.GetPGByID(m.Key)
	// Extract vlan
	portConfig, ok := pgConfig.DefaultPortConfig.(*types.VMwareDVSPortSetting)
	if !ok {
		v.Log.Infof("Extracting vlan failed for %s: portConfig was of type %T", m.Key, pgConfig.DefaultPortConfig)
		return
	}
	vlanSpec, ok := portConfig.Vlan.(*types.VmwareDistributedVirtualSwitchVlanIdSpec)
	if !ok {
		v.Log.Infof("Skipping PG since vlan config is %T", portConfig.Vlan)
		return
	}
	vlan := vlanSpec.VlanId

	if pg == nil {
		userDVS.AddUserPG(pgConfig.Name, m.Key, vlan)
		pg = penDC.GetPGByID(m.Key)
		pg.PopulateTeamingInfo(pgConfig)
		return
	}
	pg.Lock()
	vlanChanged := vlan != pg.VlanID
	renamed := pg.PgName != pgConfig.Name
	pg.Unlock()

	if vlanChanged || renamed {
		// Update entry
		userDVS.AddUserPG(pgConfig.Name, m.Key, vlan)
	}
	teamingChanged := pg.PopulateTeamingInfo(pgConfig)

	if vlanChanged || teamingChanged {
		v.Log.Infof("PG teaming info changed: %v. Vlan changed: %v.", teamingChanged, vlanChanged)
		// Rebuild workloads
		pcacheWorkloads := v.cache.ListWorkloads(v.Ctx, true)
		for _, wl := range pcacheWorkloads {
			for _, vnic := range wl.VMInfo.Vnics {
				if vnic.PG == m.Key {
					v.Log.Infof("Rebuilding workload %s", wl.Name)
					v.rebuildWorkload(wl.Name)
					break
				}
			}
		}
	}
}

func (v *VCHub) handlePG(m defs.VCEventMsg) {
	dcName := v.DcID2Name(m.DcID)
	v.Log.Infof("Got handle PG event for PG %s in DC %s", m.Key, dcName)
	// If non-pensando PG, check whether we need to reserve useg space for it
	// If it is pensando PG, verify pvlan config has not been modified

	penDC := v.GetDCFromID(m.DcID)
	if penDC == nil {
		v.Log.Errorf("DC not found for %s", dcName)
		return
	}

	pg := penDC.GetPGByID(m.Key)

	if m.UpdateType == types.ObjectUpdateKindLeave {
		// Object was deleted
		if pg == nil {
			// Object was deleted but we have no state for it
			v.Log.Errorf("Got PG deletion for PG %s but don't have state for it.", m.Key)
			return
		}

		// Add back in managed mode
		if pg.penDVS.isManagedMode() {
			v.Log.Infof("Recreating PG %s", pg.PgName)
			err := pg.penDVS.AddPenPG(pg.PgName, pg.NetworkMeta)
			if err != nil {
				v.Log.Errorf("Failed to set vlan config for PG %s, %s", pg.PgName, err)
			}
		} else if pg.penDVS.isMonitoringMode() {
			v.Log.Infof("Got PG deletion for PG %s", m.Key)
			pg.penDVS.RemoveUserPG(m.Key)
		}
		return
	}

	// extract config
	if len(m.Changes) == 0 {
		v.Log.Errorf("Received pg event with no changes")
		return
	}

	var pgConfig *types.DVPortgroupConfigInfo

	for _, prop := range m.Changes {
		config, ok := prop.Val.(types.DVPortgroupConfigInfo)
		if !ok {
			v.Log.Errorf("Expected prop to be of type DVPortgroupConfigInfo, got %T", config)
			continue
		}
		pgConfig = &config
	}

	if pgConfig == nil {
		v.Log.Errorf("Insufficient PG config %p", pgConfig)
		return
	}

	var penDVS *PenDVS
	if pgConfig.DistributedVirtualSwitch == nil {
		// find DVS from pg-key
		penDVS = penDC.GetDVSForPGID(m.Key)
	} else {
		penDVS = penDC.GetDVSByID(pgConfig.DistributedVirtualSwitch.Reference().Value)
	}
	if penDVS == nil {
		v.Log.Errorf("DVS for PG %s not found", m.Key)
		return
	}
	// Check managed mode
	if penDVS.isManagedMode() {
		v.handlePenPG(m, penDVS, pgConfig)
	} else {
		v.handleUserPG(m, penDVS, pgConfig)
	}
}

func extractVlanConfig(pgConfig types.DVPortgroupConfigInfo) (types.BaseVmwareDistributedVirtualSwitchVlanSpec, error) {
	portConfig, ok := pgConfig.DefaultPortConfig.(*types.VMwareDVSPortSetting)
	if !ok {
		return nil, fmt.Errorf("ignoring PG %s as casting to VMwareDVSPortSetting failed %+v", pgConfig.Name, pgConfig.DefaultPortConfig)
	}
	return portConfig.Vlan, nil
}
