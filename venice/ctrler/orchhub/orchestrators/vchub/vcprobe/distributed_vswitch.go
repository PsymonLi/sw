package vcprobe

import (
	"errors"
	"fmt"

	"github.com/vmware/govmomi"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/vim25/mo"
	"github.com/vmware/govmomi/vim25/types"
)

// PenDVSPortSettings represents a group of DVS port settings
// The key of this map represents the key of port on a DVS
// The value of this map represents the new setting of this port
type PenDVSPortSettings map[string]types.BaseVmwareDistributedVirtualSwitchVlanSpec

// AddPenDVS adds a new PenDVS to the given vcprobe instance
func (v *VCProbe) AddPenDVS(dcName string, dvsCreateSpec *types.DVSCreateSpec, diffFn DVSConfigDiff, retry int) error {
	dvsName := dvsCreateSpec.ConfigSpec.GetDVSConfigSpec().Name

	getAndCheck := func(client *govmomi.Client) (*object.DistributedVirtualSwitch, *mo.DistributedVirtualSwitch, bool, error) {
		dvsObj, err := v.getDVSObj(dcName, dvsName, client)
		if err != nil {
			v.Log.Errorf("Failed to get DVS")
			return nil, nil, false, err
		}

		dvs := &mo.DistributedVirtualSwitch{}
		err = dvsObj.Properties(v.ClientCtx, dvsObj.Reference(), []string{"config"}, dvs)
		if err != nil {
			v.Log.Errorf("Failed at getting dvs properties, err: %s", err)
			return nil, nil, false, err
		}
		// Check if config is equal
		if diffFn != nil && diffFn(dvsCreateSpec, dvs) == nil {
			v.Log.Infof("DC: %s - DVS %s  config in vCenter is equal to given spec", dcName, dvsName)
			return dvsObj, dvs, true, nil
		}
		return dvsObj, dvs, false, nil
	}

	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()

		// Check if it exists first
		var task *object.Task
		dvsObj, dvs, isEqual, err := getAndCheck(client)
		if err == nil {
			if isEqual {
				return nil, nil
			}

			if diffFn != nil {
				dvsCreateSpec = diffFn(dvsCreateSpec, dvs)
			}

			dvsCreateSpec.ConfigSpec.GetDVSConfigSpec().ConfigVersion = dvs.Config.GetDVSConfigInfo().ConfigVersion
			v.Log.Infof("DC: %s - DVS already exists, reconfiguring...", dcName)
			task, err = dvsObj.Reconfigure(v.ClientCtx, dvsCreateSpec.ConfigSpec)
		} else {
			dc, dcErr := v.getDCObj(dcName, client)
			if dcErr != nil {
				v.Log.Errorf("Datacenter: %s doesn't exist, err: %s", dcName, err)
				return nil, dcErr
			}

			folders, folderErr := dc.Folders(v.ClientCtx)
			if folderErr != nil {
				return nil, folderErr
			}

			task, err = folders.NetworkFolder.CreateDVS(v.ClientCtx, *dvsCreateSpec)
		}

		if err != nil {
			v.Log.Errorf("Failed at creating dvs: %s, err: %s", dvsName, err)

			// Verify that the DVS did not get written
			_, _, isEqual, err1 := getAndCheck(client)
			if err1 == nil && isEqual {
				return nil, nil
			}

			return nil, err
		}

		_, err = task.WaitForResult(v.ClientCtx, nil)
		if err != nil {
			v.Log.Errorf("Failed at waiting results of creating dvs: %s, err: %s", dvsName, err)

			_, _, isEqual, err1 := getAndCheck(client)
			if err1 == nil && isEqual {
				return nil, nil
			}

			return nil, err
		}

		return nil, nil
	}
	_, err := v.withRetry(fn, retry)
	return err
}

// GetPenDVS returns the DVS if it exists or an error
func (v *VCProbe) GetPenDVS(dcName, dvsName string, retry int) (*object.DistributedVirtualSwitch, error) {
	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()
		return v.getDVSObj(dcName, dvsName, client)
	}
	ret, err := v.withRetry(fn, retry)
	if ret == nil {
		return nil, err
	}
	return ret.(*object.DistributedVirtualSwitch), err
}

func (v *VCProbe) getDVSObj(dcName, dvsName string, client *govmomi.Client) (*object.DistributedVirtualSwitch, error) {

	v.State.DvsIDMapLock.RLock()
	dvsRef, ok := v.State.DvsIDMap[dvsName]
	v.State.DvsIDMapLock.RUnlock()
	if !ok {
		dc, err := v.getDCObj(dcName, client)
		if err != nil {
			v.Log.Errorf("Datacenter: %s doesn't exist, err: %s", dcName, err)
			return nil, err
		}
		finder := v.CreateFinder(client)
		finder.SetDatacenter(dc)

		net, err := finder.Network(v.ClientCtx, dvsName)
		if err != nil {
			v.Log.Errorf("Failed at finding network: %s, err: %s", dvsName, err)
			return nil, err
		}

		objDvs, ok := net.(*object.DistributedVirtualSwitch)
		if !ok {
			v.Log.Errorf("Failed at getting DVS object")
			return nil, errors.New("Failed at getting DVS object")
		}
		return objDvs, nil
	}
	dvsObj := object.NewDistributedVirtualSwitch(client.Client, dvsRef)
	// check ref is still valid
	var dvs mo.DistributedVirtualSwitch
	err := dvsObj.Properties(v.ClientCtx, dvsObj.Reference(), []string{"name"}, &dvs)
	if err != nil {
		v.Log.Errorf("Failed at getting dvs properties, err: %s", err)
		return nil, err
	}
	return dvsObj, nil
}

// RenameDVS renames the given dvs
func (v *VCProbe) RenameDVS(dcName string, oldName string, newName string, retry int) error {
	fn := func() (interface{}, error) {
		dvsObj, err := v.GetPenDVS(dcName, oldName, 1)
		if err != nil {
			v.Log.Errorf("Failed to rename DVS %s to %s, failed to find DVS: err %s", oldName, newName, err)
			return nil, err
		}
		ctx := v.ClientCtx
		if ctx == nil {
			return nil, fmt.Errorf("Client Context was nil")
		}
		task, err := dvsObj.Rename(ctx, newName)
		if err != nil {
			// Failed to rename DVS
			v.Log.Errorf("Failed to rename DVS %s to %s, err %s", oldName, newName, err)
			return nil, err
		}

		_, err = task.WaitForResult(ctx, nil)
		if err != nil {
			// Failed to rename DVS
			v.Log.Errorf("Failed to wait for DVS rename %s to %s, err %s", oldName, newName, err)
			return nil, err
		}

		return nil, nil
	}
	_, err := v.withRetry(fn, retry)
	return err
}

// RemovePenDVS removes the DVS
func (v *VCProbe) RemovePenDVS(dcName, dvsName string, retry int) error {
	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()

		objDvs, err := v.getDVSObj(dcName, dvsName, client)
		if err != nil {
			return nil, err
		}

		task, err := objDvs.Destroy(v.ClientCtx)
		if err != nil {
			v.Log.Errorf("Failed at destroying DVS: %s from datacenter: %s, err: %s", dvsName, dcName, err)
			return nil, err
		}

		err = task.Wait(v.ClientCtx)
		if err != nil {
			v.Log.Errorf("Failed at destroying DVS: %s from datacenter: %s, err: %s", dvsName, dcName, err)
			return nil, err
		}

		return nil, nil
	}
	_, err := v.withRetry(fn, retry)
	return err
}

// GetPenDVSPorts returns the port configuration of the given dvs
func (v *VCProbe) GetPenDVSPorts(dcName, dvsName string, criteria *types.DistributedVirtualSwitchPortCriteria, retry int) ([]types.DistributedVirtualPort, error) {
	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()
		return v.getPenDVSPorts(dcName, dvsName, criteria, client)
	}
	ret, err := v.withRetry(fn, retry)
	if ret == nil {
		return nil, err
	}
	return ret.([]types.DistributedVirtualPort), err
}

func (v *VCProbe) getPenDVSPorts(dcName, dvsName string, criteria *types.DistributedVirtualSwitchPortCriteria, client *govmomi.Client) ([]types.DistributedVirtualPort, error) {

	dvsObj, err := v.getDVSObj(dcName, dvsName, client)
	if err != nil {
		return nil, err
	}
	ret, err := dvsObj.FetchDVPorts(v.ClientCtx, criteria)
	if err != nil {
		v.Log.Errorf("Can't find ports, err: %s", err)
		return nil, err
	}
	return ret, nil
}

// GetDVSConfig returns the mo object for the given DVS
func (v *VCProbe) GetDVSConfig(dcName string, dvsName string, retry int) (*mo.DistributedVirtualSwitch, error) {
	fn := func() (interface{}, error) {
		objDvs, err := v.GetPenDVS(dcName, dvsName, 1)
		if err != nil {
			return nil, err
		}
		var dvsObj mo.DistributedVirtualSwitch
		err = objDvs.Properties(v.ClientCtx, objDvs.Reference(), []string{"name", "runtime"}, &dvsObj)
		if err != nil {
			v.Log.Errorf("Failed at getting dv port group properties, err: %s", err)
			return nil, err
		}
		return &dvsObj, nil
	}
	ret, err := v.withRetry(fn, retry)
	if ret == nil {
		return nil, err
	}
	return ret.(*mo.DistributedVirtualSwitch), err
}

func (v *VCProbe) isOverrideEqual(portsSetting PenDVSPortSettings, ports []types.DistributedVirtualPort) bool {
	expMap := map[string]int32{}
	for key, portInfo := range portsSetting {
		expMap[key] = portInfo.(*types.VmwareDistributedVirtualSwitchVlanIdSpec).VlanId
	}

	// If ports is missing entries from expMap, we don't return false.
	// This is to handle cases where VM config gives an invalid port key.
	// For ports that don't match override we set the value in expMap to -1

	for _, port := range ports {
		key := port.Key
		expVlan := expMap[key]
		setting, ok := port.Config.Setting.(*types.VMwareDVSPortSetting)
		if ok {
			vlanSpec, ok := setting.Vlan.(*types.VmwareDistributedVirtualSwitchVlanIdSpec)
			if ok {
				if expVlan == vlanSpec.VlanId {
					delete(expMap, key)
					continue
				}
			}
		}
		expMap[key] = -1
	}
	for _, v := range expMap {
		if v == -1 {
			return false
		}
	}
	return true
}

// UpdateDVSPortsVlan updates the port settings
func (v *VCProbe) UpdateDVSPortsVlan(dcName, dvsName string, portsSetting PenDVSPortSettings, forceWrite bool, retry int) error {
	v.Log.Debugf("UpdateDVSPortsVlan called with %s %s %v", dcName, dvsName, portsSetting)
	getAndCheck := func(client *govmomi.Client) ([]types.DVPortConfigSpec, bool, error) {
		numPorts := len(portsSetting)
		portKeys := make([]string, numPorts)
		portSpecs := []types.DVPortConfigSpec{}
		portUsedMap := map[string]bool{}

		for k := range portsSetting {
			portKeys = append(portKeys, k)
		}
		criteria := &types.DistributedVirtualSwitchPortCriteria{
			PortKey: portKeys,
		}
		ports, err := v.getPenDVSPorts(dcName, dvsName, criteria, client)
		if err != nil {
			return nil, false, err
		}

		for i := range ports {
			portConfig := types.DVPortConfigSpec{
				ConfigVersion: ports[i].Config.ConfigVersion,
				Key:           ports[i].Key,
				Operation:     string("edit"),
				Scope:         ports[i].Config.Scope,
				Setting: &types.VMwareDVSPortSetting{
					Vlan: portsSetting[ports[i].Key],
				},
			}
			portSpecs = append(portSpecs, portConfig)
			portUsedMap[ports[i].Key] = true
		}

		if len(ports) != len(portsSetting) {
			for key := range portsSetting {
				if _, ok := portUsedMap[key]; !ok {
					v.Log.Errorf("GetDVSorts did not return an entry for port %s.", key)
				}
			}
		}

		return portSpecs, !forceWrite && v.isOverrideEqual(portsSetting, ports), nil
	}

	fn := func() (interface{}, error) {
		err := v.ReserveClient()
		if err != nil {
			return nil, err
		}
		defer v.ReleaseClient()
		client := v.GetClient()

		portSpecs, isEqual, err := getAndCheck(client)
		if err != nil {
			return nil, err
		}
		if isEqual {
			return nil, nil
		}

		dvsObj, err := v.getDVSObj(dcName, dvsName, client)
		if err != nil {
			return nil, err
		}

		task, err := dvsObj.ReconfigureDVPort(v.ClientCtx, portSpecs)
		if err != nil {
			v.Log.Errorf("Failed at reconfig DVS ports, err: %s", err)
			_, isEqual, err1 := getAndCheck(client)
			if err1 != nil {
				v.Log.Errorf("Error while checking dvs port overrides, %s", err1)
			} else if isEqual {
				return nil, nil
			}
			return nil, err
		}

		_, err = task.WaitForResult(v.ClientCtx, nil)
		if err != nil {
			v.Log.Errorf("Failed at modifying DVS ports, err: %s", err)
			_, isEqual, err1 := getAndCheck(client)
			if err1 != nil {
				v.Log.Errorf("Error while checking dvs port overrides, %s", err1)
			} else if isEqual {
				return nil, nil
			}
			return nil, err
		}

		return nil, nil
	}
	_, err := v.withRetry(fn, retry)
	return err
}
