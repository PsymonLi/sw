package objects

import (
	"fmt"

	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
)

// InterfaceCollection wrapper for interface object
type NetworkInterfaceCollection struct {
	CollectionCommon
	Interfaces []*network.NetworkInterface
	err        error
}

//InterfaceCollection create a new collection
func NewInterfaceCollection(client objClient.ObjClient, testbed *testbed.TestBed) *NetworkInterfaceCollection {
	return &NetworkInterfaceCollection{
		CollectionCommon: CollectionCommon{Client: client,
			Testbed: testbed},
	}
}

// Error returns the error in collection
func (intf *NetworkInterfaceCollection) Error() error {
	return intf.err
}

// Len returns length
func (intf *NetworkInterfaceCollection) Len() int {
	return len(intf.Interfaces)
}

// Uplinks get uplink
func (intf *NetworkInterfaceCollection) Uplinks() *NetworkInterfaceCollection {

	nintfc := NewInterfaceCollection(intf.Client, intf.Testbed)

	for _, interf := range intf.Interfaces {
		if interf.Spec.Type == network.NetworkInterfaceStatus_UPLINK_ETH.String() {
			nintfc.Interfaces = append(nintfc.Interfaces, interf)
		}

	}

	return nintfc
}

// Loopbacks get loopback interfaces
func GetLoopbacks(client objClient.ObjClient, testbed *testbed.TestBed) *NetworkInterfaceCollection {

	intfc := NewInterfaceCollection(client, testbed)

	intfs, err := client.ListNetworkLoopbackInterfaces()

	if err != nil {
		intfc.SetError(err)
		return intfc
	}
	for _, intf := range intfs {
		intfc.Interfaces = append(intfc.Interfaces, intf)
	}
	return intfc
}

// AddLabel add label
func (intf *NetworkInterfaceCollection) AddLabel(label map[string]string) *NetworkInterfaceCollection {

	for _, interf := range intf.Interfaces {
		interf.ObjectMeta.Labels = label
	}

	return intf
}

// Commit updates network interface
func (intf *NetworkInterfaceCollection) Commit() error {

	var rerr error
	for _, interf := range intf.Interfaces {
		err := intf.Client.UpdateNetworkInterface(interf)
		if err != nil {
			rerr = err
		}
	}

	return rerr
}

func (intf *NetworkInterfaceCollection) fetch() error {

	nIntfs, err := intf.Client.ListNetworkInterfaces()
	if err != nil {
		return err
	}

	for index, interf := range intf.Interfaces {
		for _, nintf := range nIntfs {
			if interf.Name == nintf.Name {
				intf.Interfaces[index] = nintf
			}
		}
	}

	return nil
}

// VerifyMirrors verify mirrors are present as expected
func (intf *NetworkInterfaceCollection) VerifyMirrors(mirrors []string) error {

	err := intf.fetch()
	if err != nil {
		return err
	}

	for _, interf := range intf.Interfaces {
		for _, mirror := range mirrors {
			mirrorPresent := false
			for _, intfMirror := range interf.Status.MirroSessions {
				if intfMirror == mirror {
					mirrorPresent = true
					break
				}
			}
			if !mirrorPresent {
				return fmt.Errorf("Interface %v does not have mirror %v", interf.Name, mirror)
			}
		}
	}

	return nil
}

// VerifyNoMirrors verify no mirrors are present as expected
func (intf *NetworkInterfaceCollection) VerifyNoMirrors(mirrors []string) error {

	err := intf.fetch()
	if err != nil {
		return err
	}

	for _, interf := range intf.Interfaces {
		for _, mirror := range mirrors {
			mirrorPresent := false
			for _, intfMirror := range interf.Status.MirroSessions {
				if intfMirror == mirror {
					mirrorPresent = true
					break
				}
			}
			if mirrorPresent {
				return fmt.Errorf("Interface %v has mirror %v", interf.Name, mirror)
			}
		}
	}

	return nil
}

func (intf *NetworkInterfaceCollection) AttachNetwork(tenant, nw string) error {

	for _, interf := range intf.Interfaces {
		interf.Spec.AttachNetwork = nw
		interf.Spec.AttachTenant = tenant
		err := intf.Client.UpdateNetworkInterface(interf)
		if err != nil {
			return err
		}
	}
	return nil
}

func (intf *NetworkInterfaceCollection) GetTenant() string {
	return intf.Interfaces[0].Spec.AttachTenant
}

func GetNetworkInterfaceBySubnet(naples, subnet string, client objClient.ObjClient, tb *testbed.TestBed) (*NetworkInterfaceCollection, error) {

	filter := fmt.Sprintf("spec.type=host-pf,spec.attach-network=%v,status.dsc=%v", subnet, naples)
	intfs, err := client.ListNetowrkInterfacesByFilter(filter)
	if err != nil {
		return nil, err
	}

	if len(intfs) == 0 {
		err = fmt.Errorf("no host-pf on %v attached to subnet %v", naples, subnet)
		return nil, err
	}

	intfc := NewInterfaceCollection(client, tb)
	for _, wf := range intfs {
		intfc.Interfaces = append(intfc.Interfaces, wf)
	}

	return intfc, nil
}

func GetPFNetworkInterfacesForTenant(tenant, naples string,
	client objClient.ObjClient, tb *testbed.TestBed) (*NetworkInterfaceCollection, error) {

	filter := fmt.Sprintf("spec.type=host-pf,spec.attach-tenant=%v,status.dsc=%v", tenant, naples)
	intfs, err := client.ListNetowrkInterfacesByFilter(filter)
	if err != nil {
		return nil, err
	}

	if len(intfs) == 0 {
		err = fmt.Errorf("no host-pf on %v for tenant %v", naples, tenant)
		return nil, err
	}

	intfc := NewInterfaceCollection(client, tb)
	for _, intf := range intfs {
		intfc.Interfaces = append(intfc.Interfaces, intf)
	}

	return intfc, nil
}

func GetAllPFNetworkInterfacesForTenant(tenant string,
	client objClient.ObjClient, tb *testbed.TestBed) (*NetworkInterfaceCollection, error) {

	filter := fmt.Sprintf("spec.type=host-pf,spec.attach-tenant=%v", tenant)
	intfs, err := client.ListNetowrkInterfacesByFilter(filter)
	if err != nil {
		return nil, err
	}

	if len(intfs) == 0 {
		err = fmt.Errorf("no host-pf interfaces for tenant %v", tenant)
		return nil, err
	}

	intfc := NewInterfaceCollection(client, tb)
	for _, intf := range intfs {
		intfc.Interfaces = append(intfc.Interfaces, intf)
	}

	return intfc, nil
}
