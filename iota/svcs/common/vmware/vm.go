package vmware

import (
	"fmt"
	"path"

	"github.com/pensando/sw/venice/utils/log"
	"github.com/pkg/errors"
	"github.com/vmware/govmomi/object"
	"github.com/vmware/govmomi/property"
	"github.com/vmware/govmomi/vim25/progress"
	"github.com/vmware/govmomi/vim25/types"
)

// VM encapsulates a single VM
type VM struct {
	entity *Entity
	name   string
	vm     *object.VirtualMachine
}

// VMInfo contains info about a vm
type VMInfo struct {
	Name string
	IP   string
}

// Name returns name of the vm
func (vm VM) Name() string {
	return vm.name
}

// NewVM creates a new virtual machine
func (h *Host) NewVM(name string) (*VM, error) {
	finder := h.Finder()

	vm, err := finder.VirtualMachine(h.Ctx(), name)
	if err != nil {
		return nil, err
	}

	return h.makeVM(name, vm), nil
}

// Destroy the VM
func (vm *VM) Destroy() error {
	if err := vm.PowerOff(); err != nil {
		return err
	}

	task, err := vm.vm.Destroy(vm.entity.Ctx())
	if err != nil {
		return err
	}

	return task.Wait(vm.entity.Ctx())
}

// PowerOff the VM
func (vm *VM) PowerOff() error {
	state, err := vm.vm.PowerState(vm.entity.Ctx())
	if err != nil {
		return err
	}

	if state != types.VirtualMachinePowerStatePoweredOff {
		task, err := vm.vm.PowerOff(vm.entity.Ctx())
		if err != nil {
			return err
		}

		return task.Wait(vm.entity.Ctx())
	}

	return nil
}

type vmotionSinker struct {
	reportChan chan progress.Report
}

func (vs *vmotionSinker) Sink() chan<- progress.Report {
	return vs.reportChan
}

func (vm *VM) getRelocationDeviceChangeSpec(dstDC *DataCenter) ([]types.BaseVirtualDeviceConfigSpec, error) {

	changeSpecs := []types.BaseVirtualDeviceConfigSpec{}
	getNwName := func(uuid, key string) (string, error) {
		netList, err := vm.entity.Finder().NetworkList(vm.entity.Ctx(), "*")
		if err != nil {
			return "", errors.Wrap(err, "Failed Fetch networks")
		}

		for _, net := range netList {
			nwRef, err := net.EthernetCardBackingInfo(vm.entity.Ctx())
			if err != nil {
				continue
			}
			switch nw := nwRef.(type) {
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				if nw.Port.SwitchUuid == uuid && nw.Port.PortgroupKey == key {
					return net.GetInventoryPath(), nil
				}
			}

		}
		return "", errors.Wrap(err, "Failed Fetch networks")
	}

	getDstDvsPG := func(entity *Entity, dvsPG string) (*types.VirtualEthernetCardDistributedVirtualPortBackingInfo, error) {
		netList, err := entity.Finder().NetworkList(entity.Ctx(), "*")
		if err != nil {
			return nil, errors.Wrap(err, "Failed Fetch networks")
		}

		for _, net := range netList {
			nwRef, err := net.EthernetCardBackingInfo(entity.Ctx())
			if err != nil {
				continue
			}
			switch nw := nwRef.(type) {
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				if path.Base(net.GetInventoryPath()) == dvsPG {
					return nw, nil
				}
			}

		}
		return nil, errors.Wrap(err, "Failed Fetch networks")
	}

	devList, err := vm.vm.Device(vm.entity.Ctx())
	if err != nil {
		return nil, errors.Wrap(err, "Failed to device list of VM")
	}

	devices := devList.SelectByType((*types.VirtualEthernetCard)(nil))
	for _, d := range devices {
		veth := d.GetVirtualDevice()

		switch a := veth.Backing.(type) {
		case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:

			nwName, _ := getNwName(a.Port.SwitchUuid, a.Port.PortgroupKey)
			fmt.Printf("Connected to DVS PG %v %v %v\n", a.Port.SwitchUuid, a.Port.PortgroupKey, nwName)
			nw, _ := getDstDvsPG(&dstDC.vc.Entity, path.Base(nwName))
			fmt.Printf("New Connected to DVS PG %v %v %v\n", a.Port.SwitchUuid, a.Port.PortgroupKey, nw)

			veth.Backing = &types.VirtualEthernetCardDistributedVirtualPortBackingInfo{
				VirtualDeviceBackingInfo: types.VirtualDeviceBackingInfo{},
				Port: types.DistributedVirtualSwitchPortConnection{
					SwitchUuid:   nw.Port.SwitchUuid,
					PortKey:      nw.Port.PortKey,
					PortgroupKey: nw.Port.PortgroupKey,
				},
			}
			changeSpecs = append(changeSpecs, &types.VirtualDeviceConfigSpec{Operation: types.VirtualDeviceConfigSpecOperationEdit, Device: d})
		case *types.VirtualEthernetCardNetworkBackingInfo:
			fmt.Printf("Connected to PG %v\n", veth.Backing.(*types.VirtualEthernetCardNetworkBackingInfo).DeviceName)
		}

	}

	return changeSpecs, nil
}

func (vm *VM) getRelocateSpec(dstDC *DataCenter, cluster, dstHostName string) (types.VirtualMachineRelocateSpec, error) {

	var err error
	config := types.VirtualMachineRelocateSpec{}

	config.Datastore, err = dstDC.getDatastoreRefForHost(dstHostName)
	if err != nil {
		return config, err
	}

	dstHostRef, err := dstDC.findHost(cluster, dstHostName)
	if err != nil {
		return config, err
	}
	href := dstHostRef.Host.hs.Reference()
	config.Host = &href

	rp, err := dstHostRef.hs.ResourcePool(dstDC.vc.Ctx())
	if err != nil {
		return config, errors.Wrap(err, "Get resource pool failed")
	}

	ref := rp.Reference()
	config.Pool = &ref

	config.DeviceChange, err = vm.getRelocationDeviceChangeSpec(dstDC)

	if err != nil {
		return config, err
	}

	return config, nil
}

// Migrate VM to destination host
func (vm *VM) Migrate(dstDC *DataCenter, cluster, dstHostName string, abortTime int) error {

	var err error
	var task *object.Task

	config, err := vm.getRelocateSpec(dstDC, cluster, dstHostName)
	if err != nil {
		return errors.Wrapf(err, "Error building relocation spec")
	}

	if abortTime == 0 {
		task, err = vm.vm.Relocate(vm.entity.Ctx(), config,
			types.VirtualMachineMovePriorityDefaultPriority)

	} else {
		task, err = vm.vm.Relocate(vm.entity.Ctx(), config,
			types.VirtualMachineMovePriorityLowPriority)
	}

	if err != nil {
		return err
	}

	vsinker := &vmotionSinker{}
	vsinker.reportChan = make(chan progress.Report, 20)
	go func() {
		for {
			select {
			case rep, ok := <-vsinker.reportChan:
				if !ok {
					return
				}
				//After 35%, vmotion abort
				if abortTime > 0 && rep.Percentage() >= 30 {
					log.Infof("VM %s : Vmotion Percentage complete %v, cancelling", vm.name, rep.Percentage())
					task.Cancel(vm.entity.Ctx())
					return
				}
			}
		}
	}()

	_, err = task.WaitForResult(vm.entity.Ctx(), vsinker)
	return err
}

//ReconfigureNetwork will connect interface connected from one network to other network
func (vm *VM) ReconfigureNetwork(currNW string, newNW string, maxReconfigs int) error {

	var task *object.Task
	var devList object.VirtualDeviceList
	var err error

	net, err := vm.entity.Finder().Network(vm.entity.Ctx(), newNW)
	if err != nil {
		return errors.Wrap(err, "Failed Find Network")
	}

	curNet, err := vm.entity.Finder().Network(vm.entity.Ctx(), currNW)
	if err != nil {
		return errors.Wrap(err, "Failed Find current Network")
	}

	curNetRef, err := curNet.EthernetCardBackingInfo(vm.entity.Ctx())
	if err != nil {
		return errors.Wrap(err, "Failed Find current Network")
	}

	nwRef, err := net.EthernetCardBackingInfo(vm.entity.Ctx())
	if err != nil {
		return errors.Wrap(err, "Failed Find Network devices")
	}

	devList, err = vm.vm.Device(vm.entity.Ctx())
	if err != nil {
		return errors.Wrap(err, "Failed to device list of VM")
	}

	reconfigs := 0
	devices := devList.SelectByType((*types.VirtualEthernetCard)(nil))
	for _, d := range devices {
		veth := d.GetVirtualDevice()

		switch nw := nwRef.(type) {
		case *types.VirtualEthernetCardNetworkBackingInfo:
			switch a := veth.Backing.(type) {
			case *types.VirtualEthernetCardNetworkBackingInfo:
				if veth.Backing.(*types.VirtualEthernetCardNetworkBackingInfo).DeviceName != currNW {
					fmt.Println("Skipping as current network does not match ", currNW)
					continue
				}
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				switch curn := curNetRef.(type) {
				case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
					if a.Port.SwitchUuid != curn.Port.SwitchUuid ||
						a.Port.PortgroupKey != curn.Port.SwitchUuid {
						fmt.Println("Skipping as current network does not match ", currNW)
						continue
					}
				}
			}

			veth.Backing = &types.VirtualEthernetCardNetworkBackingInfo{
				VirtualDeviceDeviceBackingInfo: types.VirtualDeviceDeviceBackingInfo{
					VirtualDeviceBackingInfo: types.VirtualDeviceBackingInfo{},
					DeviceName:               newNW,
				},
			}
			veth.Connectable = &types.VirtualDeviceConnectInfo{
				StartConnected: true,
				Connected:      true,
			}
		case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
			switch a := veth.Backing.(type) {
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				switch curn := curNetRef.(type) {
				case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
					if a.Port.SwitchUuid != curn.Port.SwitchUuid ||
						a.Port.PortgroupKey != curn.Port.SwitchUuid {
						fmt.Println("Skipping as network already connected ", currNW)
						continue
					}
				default:
					continue

				}

			case *types.VirtualEthernetCardNetworkBackingInfo:
				if veth.Backing.(*types.VirtualEthernetCardNetworkBackingInfo).DeviceName != currNW {
					fmt.Println("Skipping as network already connected ", currNW)
					continue
				}
			}
			veth.Backing = &types.VirtualEthernetCardDistributedVirtualPortBackingInfo{
				VirtualDeviceBackingInfo: types.VirtualDeviceBackingInfo{},
				Port: types.DistributedVirtualSwitchPortConnection{
					SwitchUuid:   nw.Port.SwitchUuid,
					PortKey:      nw.Port.PortKey,
					PortgroupKey: nw.Port.PortgroupKey,
				},
			}
		}

		fmt.Println("Configuring network...", newNW)
		task, err = vm.vm.Reconfigure(vm.entity.Ctx(),
			types.VirtualMachineConfigSpec{
				DeviceChange: []types.BaseVirtualDeviceConfigSpec{&types.VirtualDeviceConfigSpec{Operation: types.VirtualDeviceConfigSpecOperationEdit, Device: d}}})
		if err != nil {
			return err
		}
		if err := task.Wait(vm.entity.Ctx()); err != nil {
			return errors.Wrap(err, "Reconfiguring to network failed")
		}

		reconfigs++
		if maxReconfigs != 0 && reconfigs == maxReconfigs {
			break
		}
	}

	return nil
}

//ReconnectLink will reconnect
func (vm *VM) ReconnectLink(currNW string) error {

	var task *object.Task
	var devList object.VirtualDeviceList
	var err error

	curNet, err := vm.entity.Finder().Network(vm.entity.Ctx(), currNW)
	if err != nil {
		return errors.Wrap(err, "Failed Find current Network")
	}

	curNetRef, err := curNet.EthernetCardBackingInfo(vm.entity.Ctx())
	if err != nil {
		return errors.Wrap(err, "Failed Find current Network")
	}

	devList, err = vm.vm.Device(vm.entity.Ctx())
	if err != nil {
		return errors.Wrap(err, "Failed to device list of VM")
	}

	for _, d := range devList.SelectByType((*types.VirtualEthernetCard)(nil)) {
		veth := d.GetVirtualDevice()

		switch curNetRef.(type) {
		case *types.VirtualEthernetCardNetworkBackingInfo:
			switch a := veth.Backing.(type) {
			case *types.VirtualEthernetCardNetworkBackingInfo:
				if veth.Backing.(*types.VirtualEthernetCardNetworkBackingInfo).DeviceName != currNW {
					fmt.Println("Skipping as current network does not match ", currNW)
					continue
				}
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				switch curn := curNetRef.(type) {
				case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
					if a.Port.SwitchUuid != curn.Port.SwitchUuid ||
						a.Port.PortgroupKey != curn.Port.SwitchUuid {
						fmt.Println("Skipping as current network does not match ", currNW)
						continue
					}
				}
			}

			veth.Connectable = &types.VirtualDeviceConnectInfo{
				StartConnected: true,
				Connected:      true,
			}
		case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
			switch a := veth.Backing.(type) {
			case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
				switch curn := curNetRef.(type) {
				case *types.VirtualEthernetCardDistributedVirtualPortBackingInfo:
					if a.Port.SwitchUuid != curn.Port.SwitchUuid ||
						a.Port.PortgroupKey != curn.Port.SwitchUuid {
						fmt.Println("Skipping as network already connected ", currNW)
						continue
					}
				default:
					continue

				}

			case *types.VirtualEthernetCardNetworkBackingInfo:
				if veth.Backing.(*types.VirtualEthernetCardNetworkBackingInfo).DeviceName != currNW {
					fmt.Println("Skipping as network already connected ", currNW)
					continue
				}
			}
			veth.Connectable = &types.VirtualDeviceConnectInfo{
				StartConnected: false,
				Connected:      false,
			}
		}

		task, err = vm.vm.Reconfigure(vm.entity.Ctx(),
			types.VirtualMachineConfigSpec{DeviceChange: []types.BaseVirtualDeviceConfigSpec{&types.VirtualDeviceConfigSpec{Operation: types.VirtualDeviceConfigSpecOperationEdit, Device: d}}})
		if err != nil {
			return err
		}
		if err := task.Wait(vm.entity.Ctx()); err != nil {
			return errors.Wrap(err, "Reconfiguring to network failed")
		}

		veth.Connectable = &types.VirtualDeviceConnectInfo{
			StartConnected: true,
			Connected:      true,
		}

		task, err = vm.vm.Reconfigure(vm.entity.Ctx(),
			types.VirtualMachineConfigSpec{DeviceChange: []types.BaseVirtualDeviceConfigSpec{&types.VirtualDeviceConfigSpec{Operation: types.VirtualDeviceConfigSpecOperationEdit, Device: d}}})
		if err != nil {
			return err
		}
		if err := task.Wait(vm.entity.Ctx()); err != nil {
			return errors.Wrap(err, "Reconfiguring to network failed")
		}

	}

	return nil
}

//ReadMacs gets the mac address
func (vm *VM) ReadMacs() ([]string, error) {

	macs := []string{}
	// device name:network name
	property.Wait(vm.entity.Ctx(), vm.entity.ConnCtx.pc, vm.vm.Reference(), []string{"config.hardware.device"}, func(pc []types.PropertyChange) bool {
		for _, c := range pc {
			//if c.Op != types.PropertyChangeOpAssign {
			//continue
			//}

			changedDevices := c.Val.(types.ArrayOfVirtualDevice).VirtualDevice
			for _, device := range changedDevices {
				if nic, ok := device.(types.BaseVirtualEthernetCard); ok {
					mac := nic.GetVirtualEthernetCard().MacAddress
					if mac == "" {
						continue
					}
					macs = append(macs, mac)
				}
			}
		}
		return true
	})

	return macs, nil
}
