package objects

import (
	"fmt"
	"math/rand"

	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/willf/bitset"
)

// Network represents a Vlan with a subnet (called network in venice)
type Network struct {
	Name          string // subnet name
	vlan          uint32
	ipPrefix      string
	bitmask       *bitset.BitSet
	subnetSize    int
	VeniceNetwork *network.Network
}

// NetworkCollection is a list of subnets
type NetworkCollection struct {
	CollectionCommon
	err     error
	subnets []*Network
}

func (n *Network) IPPrefix() string {
	return n.ipPrefix
}

//NewNetworkCollectionsFromNetworks For every network create a new collector
func NewNetworkCollectionsFromNetworks(client objClient.ObjClient, nws []*network.Network) []*NetworkCollection {

	var newNc []*NetworkCollection
	for index := 0; index < len(nws); index++ {
		sNet := nws[index : index+1]
		newNc = append(newNc, NewNetworkCollectionFromNetworks(client, sNet))
	}
	return newNc
}

//NewNetworkCollection create a new collector
func NewNetworkCollectionFromNetworks(client objClient.ObjClient, nws []*network.Network) *NetworkCollection {

	nwc := NetworkCollection{CollectionCommon: CollectionCommon{Client: client}}
	for _, nw := range nws {
		nwc.subnets = append(nwc.subnets,
			&Network{VeniceNetwork: nw, Name: nw.Name, ipPrefix: nw.Spec.IPv4Subnet})
	}

	return &nwc
}

func (n *NetworkCollection) Subnets() []*Network {
	return n.subnets
}

func (n *NetworkCollection) Count() int {
	return len(n.subnets)
}

func (n *NetworkCollection) AddSubnet(nw *Network) {
	n.subnets = append(n.subnets, nw)
}

func (n *NetworkCollection) GetTenant() string {
	return n.subnets[0].VeniceNetwork.Tenant
}

// Any returns any one of the subnets in random
func (snc *NetworkCollection) Any(num int) *NetworkCollection {
	if snc.HasError() || len(snc.subnets) <= num {
		return snc
	}

	newSnc := NetworkCollection{subnets: []*Network{}, CollectionCommon: snc.CollectionCommon}
	tmpArry := make([]*Network, len(snc.subnets))
	copy(tmpArry, snc.subnets)
	for i := 0; i < num; i++ {
		idx := rand.Intn(len(tmpArry))
		sn := tmpArry[idx]
		tmpArry = append(tmpArry[:idx], tmpArry[idx+1:]...)
		newSnc.subnets = append(newSnc.subnets, sn)
	}

	return &newSnc
}

// Pop returns popped element and removes from currnet collection
func (snc *NetworkCollection) Pop(num int) *NetworkCollection {
	if snc.HasError() || len(snc.subnets) <= num {
		return snc
	}

	newSnc := NetworkCollection{subnets: []*Network{}, CollectionCommon: snc.CollectionCommon}
	tmpArry := make([]*Network, len(snc.subnets))
	copy(tmpArry, snc.subnets)
	for i := 0; i < num; i++ {
		idx := rand.Intn(len(tmpArry))
		sn := tmpArry[idx]
		tmpArry = append(tmpArry[:idx], tmpArry[idx+1:]...)
		newSnc.subnets = append(newSnc.subnets, sn)
	}

	snc.subnets = snc.subnets[num:]

	return &newSnc
}

// Commit writes the VPC config to venice
func (nwc *NetworkCollection) Commit() error {
	errs := make(chan error, len(nwc.subnets))
	for index, nw := range nwc.subnets {
		nw := nw
		go func(index int) {
			err := nwc.Client.GetRestClientByID(index).CreateNetwork(nw.VeniceNetwork)
			if err != nil {
				err = nwc.Client.GetRestClientByID(index).UpdateNetwork(nw.VeniceNetwork)
				if err != nil {
					log.Infof("Creating/updating network failed %v", err)
				}
			}
			errs <- err
			log.Debugf("Created/updated network : %#v", nwc.Subnets())
		}(index)

	}

	for i := 0; i < len(nwc.subnets); i++ {
		err := <-errs
		if nwc.err == nil {
			nwc.err = err
		}
	}

	return nwc.err
}

// Delete deletes all networks/subnets in collection
func (nwc *NetworkCollection) Delete() error {

	// walk all sessions and delete them
	errs := make(chan error, len(nwc.subnets))
	for index, nw := range nwc.subnets {
		nw := nw
		go func(index int) {
			err := nwc.Client.GetRestClientByID(index).DeleteNetwork(nw.VeniceNetwork)
			errs <- err
		}(index)
	}

	for i := 0; i < len(nwc.subnets); i++ {
		err := <-errs
		if nwc.err == nil {
			nwc.err = err
		}
	}

	return nwc.err
}

func NewNetworkCollection(client objClient.ObjClient, testbed *testbed.TestBed) *NetworkCollection {
	return &NetworkCollection{
		CollectionCommon: CollectionCommon{Client: client, Testbed: testbed},
	}
}

//SetIngressSecurityPolicy sets ingress security policy to this
func (nwc *NetworkCollection) SetIngressSecurityPolicy(policies *NetworkSecurityPolicyCollection) error {

	for _, nw := range nwc.subnets {
		nw.VeniceNetwork.Spec.IngressSecurityPolicy = []string{}
		if policies != nil {
			for _, pol := range policies.Policies {
				nw.VeniceNetwork.Spec.IngressSecurityPolicy = append(nw.VeniceNetwork.Spec.IngressSecurityPolicy, pol.VenicePolicy.Name)
			}
		}
		err := nwc.Client.UpdateNetwork(nw.VeniceNetwork)
		if err != nil {
			return err
		}
	}
	return nil
}

//SetEgressSecurityPolicy sets egress security policy to this
func (nwc *NetworkCollection) SetEgressSecurityPolicy(policies *NetworkSecurityPolicyCollection) error {

	for _, nw := range nwc.subnets {
		nw.VeniceNetwork.Spec.EgressSecurityPolicy = []string{}
		if policies != nil {
			for _, pol := range policies.Policies {
				nw.VeniceNetwork.Spec.EgressSecurityPolicy = append(nw.VeniceNetwork.Spec.EgressSecurityPolicy, pol.VenicePolicy.Name)
			}
		}
		err := nwc.Client.UpdateNetwork(nw.VeniceNetwork)
		if err != nil {
			return err
		}
	}
	return nil
}

// SetIPAM sets IPAM on the networks
func (nwc *NetworkCollection) SetIPAMOnNetwork(network *Network, ipam string) error {

	for _, nw := range nwc.subnets {
		if nw.Name == network.Name {
			nw.VeniceNetwork.Spec.IPAMPolicy = ipam
			err := nwc.Client.UpdateNetwork(nw.VeniceNetwork)
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func (nw *Network) UpdateIPv4Subnet(prefix string) error {
	if nw.VeniceNetwork == nil {
		return fmt.Errorf("Invalid network\n")
	}

	spec := &nw.VeniceNetwork.Spec
	spec.IPv4Subnet = prefix
	return nil
}

func (nw *Network) UpdateIPv4Gateway(gw string) error {
	if nw.VeniceNetwork == nil {
		return fmt.Errorf("Invalid network\n")
	}

	spec := &nw.VeniceNetwork.Spec
	spec.IPv4Gateway = gw
	return nil
}

func VpcNetworkCollection(tenant, vpc string, client objClient.ObjClient) (*NetworkCollection, error) {
	nws, err := client.ListNetwork(tenant)
	if err != nil {
		return nil, err
	}

	var nws_vpc []*network.Network
	count := 0
	for _, nw := range nws {
		if nw.Spec.VirtualRouter == vpc {
			nws_vpc = append(nws_vpc, nw)
			count++
		}
	}

	if count == 0 {
		return nil, fmt.Errorf("No Networks on VPC %s", vpc)
	}

	nwc := NewNetworkCollectionFromNetworks(client, nws_vpc)
	return nwc, nil
}

func VpcAttachedNetworkCollection(tenant, vpc string, client objClient.ObjClient, testbed *testbed.TestBed) (*NetworkCollection, error) {
	nwc, err := VpcNetworkCollection(tenant, vpc, client)
	if err != nil {
		return nil, err
	}

	/* return only the subnets that are attached to host-pfs */
	nwIfs, err := GetAllPFNetworkInterfacesForTenant(tenant, client, testbed)
	if err != nil {
		return nil, err
	}

	newNwc := NetworkCollection{subnets: []*Network{}, CollectionCommon: nwc.CollectionCommon}
	for _, nw := range nwc.subnets {
		for _, intf := range nwIfs.Interfaces {
			if intf.Spec.AttachNetwork == nw.Name && intf.Spec.AttachTenant == tenant {
				newNwc.subnets = append(newNwc.subnets, nw)
				break
			}
		}
	}

	log.Infof("GetNetworkCollectionFromVPC: returning subnets: %v", len(newNwc.subnets))
	return &newNwc, nil
}

// returns the subnet collection of default config
func DefaultNetworkCollection(client objClient.ObjClient) (*NetworkCollection, error) {
	ten, err := client.ListTenant()
	if err != nil {
		return nil, err
	}
	if len(ten) == 0 {
		return nil, fmt.Errorf("No tenants to get network collection")
	}

	vpcs, err := client.ListVPC(ten[0].Name)
	if err != nil {
		return nil, err
	}
	if len(vpcs) == 0 {
		return nil, fmt.Errorf("No VPCs on tenant %V to get network collection", ten[0].Name)
	}

	vpc := ""
	for _, v := range vpcs {
		// stop at the first tenant VPC
		if v.Spec.Type == "tenant" {
			vpc = v.Name
			break
		}
	}

	nws, err := client.ListNetwork(ten[0].Name)
	if err != nil {
		return nil, err
	}

	var nws_vpc []*network.Network
	for _, nw := range nws {
		if nw.Spec.VirtualRouter == vpc {
			nws_vpc = append(nws_vpc, nw)
		}
	}

	nwc := NewNetworkCollectionFromNetworks(client, nws_vpc)
	return nwc, nil
}

// verifies status for propagation of network objects to DSCs
func (nwc *NetworkCollection) VerifyPropagationStatus(dscCount int32) error {
	if nwc.HasError() {
		return nwc.Error()
	}
	if len(nwc.subnets) == 0 {
		return nil
	}

	for _, nw := range nwc.subnets {
		propStatus := nw.VeniceNetwork.Status.GetPropagationStatus()
		objMeta := nw.VeniceNetwork.GetObjectMeta()
		if propStatus.GenerationID != objMeta.GenerationID {
			log.Warnf("Propagation generation id did not match: Meta: %+v, PropagationStatus: %+v",
				objMeta, propStatus)
			return fmt.Errorf("Propagation generation id did not match")
		}
		if (nw.VeniceNetwork.Status.PropagationStatus.Updated != dscCount) ||
			(nw.VeniceNetwork.Status.PropagationStatus.Pending != 0) {
			log.Warnf("Propagation status incorrect: Expected updates: %+v, PropagationStatus: %+v",
				dscCount, nw.VeniceNetwork.Status.PropagationStatus)
			return fmt.Errorf("Propagation status was incorrect")
		}
	}

	return nil
}
