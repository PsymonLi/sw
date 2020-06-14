// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"strings"
	"time"

	"net"

	"github.com/gogo/protobuf/types"
	"github.com/willf/bitset"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

// networkObjState current state of object
type networkObjState int32

// NetworkLive to track object state
const (
	networkLive       networkObjState = iota // Valid network
	networkMarkDelete                        // Marked for garbage collection
	networkDeleted                           // Delete request sent to API server object invalid
)
const (
	defaultGarbageCollectionInterval = 600 // cleanup timer set to 10 minutes as much of contention happens for networkkindlock
)

// NetworkState is a wrapper for network object
type NetworkState struct {
	smObjectTracker
	Network    *ctkit.Network            `json:"-"` // network object
	endpointDB map[string]*EndpointState // endpoint database
	macaddrDB  map[string]*EndpointState // mapping of mac address to endpoint
	stateMgr   *Statemgr                 // pointer to network manager
	curState   networkObjState           // maintaines the object lifecycle
}

// NetworkStateFromObj conerts from memdb object to network state
func NetworkStateFromObj(obj runtime.Object) (*NetworkState, error) {
	switch obj.(type) {
	case *ctkit.Network:
		nobj := obj.(*ctkit.Network)
		switch nobj.HandlerCtx.(type) {
		case *NetworkState:
			nts := nobj.HandlerCtx.(*NetworkState)
			return nts, nil
		default:
			return nil, ErrIncorrectObjectType
		}
	default:
		return nil, ErrIncorrectObjectType
	}
}

// utility functions to convert IPv4 address to int and vice versa
func ipv42int(ip net.IP) uint32 {
	if len(ip) == 16 {
		return binary.BigEndian.Uint32(ip[12:16])
	}

	return binary.BigEndian.Uint32(ip)
}

func int2ipv4(nn uint32) net.IP {
	ip := make(net.IP, 4)
	binary.BigEndian.PutUint32(ip, nn)
	return ip
}

func ipv4toMac(macPrefix []byte, ip net.IP) net.HardwareAddr {
	if len(ip) == 16 {
		return net.HardwareAddr(append(macPrefix[0:2], ip[12:16]...))
	}
	return net.HardwareAddr(append(macPrefix[0:2], ip...))
}

func convertNetwork(nw *NetworkState) *netproto.Network {
	creationTime, _ := types.TimestampProto(time.Now())

	ntn := netproto.Network{
		TypeMeta:   nw.Network.TypeMeta,
		ObjectMeta: agentObjectMeta(nw.Network.ObjectMeta),
		Spec: netproto.NetworkSpec{
			VlanID:     nw.Network.Spec.VlanID,
			VrfName:    nw.Network.Spec.VirtualRouter,
			VxLANVNI:   nw.Network.Spec.VxlanVNI,
			IPAMPolicy: nw.Network.Spec.IPAMPolicy,
		},
	}
	ntn.CreationTime = api.Timestamp{Timestamp: *creationTime}

	if nw.Network.Spec.IPv4Subnet != "" {
		ipn, err := ParseToIPPrefix(nw.Network.Spec.IPv4Subnet)
		if err != nil {
			log.Errorf("failed to parse IP Subnet [%v](%s)", nw.Network.Spec.IPv4Subnet, err)
			return nil
		}
		gw, err := ParseToIPAddress(nw.Network.Spec.IPv4Gateway)
		if err != nil {
			log.Errorf("failed to parse IP Gateway")
		} else {
			// XXX-TODO(): validate that gateway is in the subnet.
			ipn.Address = *gw
		}
		ntn.Spec.V4Address = append(ntn.Spec.V4Address, *ipn)
	}

	if nw.Network.Spec.RouteImportExport != nil {
		ntn.Spec.RouteImportExport = &netproto.RDSpec{
			AddressFamily: nw.Network.Spec.RouteImportExport.AddressFamily,
			RDAuto:        nw.Network.Spec.RouteImportExport.RDAuto,
		}
		ntn.Spec.RouteImportExport.RD = cloneRD(nw.Network.Spec.RouteImportExport.RD)
		for _, r := range nw.Network.Spec.RouteImportExport.ImportRTs {
			ntn.Spec.RouteImportExport.ImportRTs = append(ntn.Spec.RouteImportExport.ImportRTs, cloneRD(r))
		}
		for _, r := range nw.Network.Spec.RouteImportExport.ExportRTs {
			ntn.Spec.RouteImportExport.ExportRTs = append(ntn.Spec.RouteImportExport.ExportRTs, cloneRD(r))
		}
	}

	if nw.Network.Spec.IngressSecurityPolicy != nil {
		ntn.Spec.IngV4SecurityPolicies = append(ntn.Spec.IngV4SecurityPolicies, nw.Network.Spec.IngressSecurityPolicy...)
	}

	if nw.Network.Spec.EgressSecurityPolicy != nil {
		ntn.Spec.EgV4SecurityPolicies = append(ntn.Spec.EgV4SecurityPolicies, nw.Network.Spec.EgressSecurityPolicy...)
	}

	return &ntn
}

// allocIPv4Addr allocates new IP address
func (ns *NetworkState) allocIPv4Addr(reqAddr string) (string, error) {
	var allocatedAddr string

	// parse the subnet
	baseAddr, ipnet, err := net.ParseCIDR(ns.Network.Spec.IPv4Subnet)
	if err != nil {
		log.Errorf("Error parsing subnet %v. Err: %v", ns.Network.Spec.IPv4Subnet, err)
		return "", err
	}

	// read ipv4 allocation bitmap
	subnetMaskLen, maskLen := ipnet.Mask.Size()
	subnetSize := 1 << uint32(maskLen-subnetMaskLen)
	buf := bytes.NewBuffer(ns.Network.Status.AllocatedIPv4Addrs)
	bs := bitset.New(uint(subnetSize))
	bs.ReadFrom(buf)

	// see if caller requested a specific addr
	if reqAddr != "" {
		reqIPAddr, _, err := net.ParseCIDR(reqAddr)
		if err != nil {
			reqIPAddr = net.ParseIP(reqAddr)
		}

		// verify requested address is in this subnet
		if !ipnet.Contains(reqIPAddr) {
			log.Errorf("Requested address %s is not in subnet %s", reqAddr, ns.Network.Spec.IPv4Subnet)
			return "", fmt.Errorf("requested address not in subnet")
		}

		// determine the bit in bitmask
		addrBit := ipv42int(reqIPAddr) - ipv42int(ipnet.IP)

		// check if address is already allocated
		if bs.Test(uint(addrBit)) {
			log.Errorf("Address %s is already allocated", reqAddr)
			return "", fmt.Errorf("Requested address not available")
		}

		// alloc the bit
		bs.Set(uint(addrBit))
		allocatedAddr = reqIPAddr.String()

		log.Infof("Allocating requested addr: %v, bit: %d", allocatedAddr, addrBit)
	} else {
		// allocate next available address
		addrBit, ok := bs.NextClear(0)
		if !ok || addrBit >= uint(subnetSize) {
			log.Errorf("Could not find a free bit in bitset")
			return "", fmt.Errorf("Could not find a free addr")
		}

		bs.Set(uint(addrBit))
		allocatedAddr = int2ipv4(uint32(uint(ipv42int(baseAddr)) + addrBit)).String()

		log.Infof("Allocating addr: %v, bit: %d", allocatedAddr, addrBit)
	}

	// save the bitset back
	bs.WriteTo(buf)
	ns.Network.Status.AllocatedIPv4Addrs = buf.Bytes()

	return allocatedAddr, nil
}

// freeIPv4Addr free the address
func (ns *NetworkState) freeIPv4Addr(reqAddr string) error {
	/*
		log.Infof("Freeing IPv4 address: %v", reqAddr)

		reqIP, _, err := net.ParseCIDR(reqAddr)
		if err != nil {
			log.Errorf("Error parsing ip address: %v. Err: %v", reqAddr, err)
			return err
		}

		// parse the subnet
		baseAddr, ipnet, err := net.ParseCIDR(ns.Network.Spec.IPv4Subnet)
		if err != nil {
			log.Errorf("Error parsing subnet %v. Err: %v", ns.Network.Spec.IPv4Subnet, err)
			return err
		}

		// verify the address is in subnet
		if !ipnet.Contains(reqIP) {
			log.Errorf("Requested address %s is not in subnet %s", reqAddr, ns.Network.Spec.IPv4Subnet)
			return fmt.Errorf("requested address not in subnet")
		}

		// read ipv4 allocation bitmap
		subnetMaskLen, maskLen := ipnet.Mask.Size()
		subnetSize := 1 << uint32(maskLen-subnetMaskLen)
		buf := bytes.NewBuffer(ns.Network.Status.AllocatedIPv4Addrs)
		bs := bitset.New(uint(subnetSize))
		bs.ReadFrom(buf)

		// determine the bit in bitmask
		addrBit := ipv42int(reqIP) - ipv42int(baseAddr)

		// verify the address is still allocated
		if !bs.Test(uint(addrBit)) {
			log.Errorf("Address %s was not allocated", reqAddr)
			return fmt.Errorf("Requested address was not allocated")
		}

		// clear the bit
		bs.Clear(uint(addrBit))

		// save the bitset back
		bs.WriteTo(buf)
		ns.Network.Status.AllocatedIPv4Addrs = buf.Bytes()
	*/

	return nil
}

// AddEndpoint adds endpoint to network
func (ns *NetworkState) AddEndpoint(ep *EndpointState) error {
	ns.Network.Lock()
	defer ns.Network.Unlock()
	if ns.isNetworkDeleted() {
		return fmt.Errorf("garbage collector deleted object")
	}
	ns.endpointDB[ep.endpointKey()] = ep
	ns.macaddrDB[ep.Endpoint.Status.MacAddress] = ep
	ns.curState = networkLive
	return nil
}

// RemoveEndpoint removes an endpoint from network
func (ns *NetworkState) RemoveEndpoint(ep *EndpointState) error {
	ns.Network.Lock()
	defer ns.Network.Unlock()
	if ns.isNetworkDeleted() {
		panic("Bug - Removing deleted endpoint")
	}
	delete(ns.endpointDB, ep.endpointKey())
	delete(ns.macaddrDB, ep.Endpoint.Status.MacAddress)
	if len(ns.endpointDB) == 0 && IsObjInternal(ns.Network.Labels) {
		ns.curState = networkMarkDelete
		log.Errorf("mark for garbage collection networkstate: {%+v}", ns)
	}
	return nil
}

// FindEndpoint finds an endpoint in a network
func (ns *NetworkState) FindEndpoint(epName string) (*EndpointState, bool) {
	// lock the endpoint db
	ns.Network.Lock()
	defer ns.Network.Unlock()

	// find the endpoint in the DB
	eps, ok := ns.endpointDB[epName]
	if !ok {
		return nil, ok
	}

	return eps, true
}

// FindEndpointByMacAddr finds an endpoint in a network by its mac address
func (ns *NetworkState) FindEndpointByMacAddr(macaddr string) (*EndpointState, error) {
	// lock the endpoint db
	ns.Network.Lock()
	defer ns.Network.Unlock()

	// find the endpoint in the DB
	eps, ok := ns.macaddrDB[macaddr]
	if !ok {
		return nil, fmt.Errorf("Endpoint not found")
	}

	return eps, nil
}

// ListEndpoints lists all endpoints on this network
func (ns *NetworkState) ListEndpoints() []*EndpointState {
	var eplist []*EndpointState

	// lock the endpoint db
	ns.Network.Lock()
	defer ns.Network.Unlock()

	// walk all endpoints
	for _, ep := range ns.endpointDB {
		eplist = append(eplist, ep)
	}

	return eplist
}

// Delete cleans up all state associated with the network
func (ns *NetworkState) Delete() error {
	// check if network still has endpoints
	if len(ns.endpointDB) != 0 {
		log.Errorf("Can not delete the network, still has endpoints {%+v}", ns)
		return fmt.Errorf("Network still has endpoints")
	}

	return nil
}

// NewNetworkState creates new network state object
func NewNetworkState(nw *ctkit.Network, stateMgr *Statemgr) (*NetworkState, error) {
	// parse the subnet
	if nw.Spec.IPv4Subnet != "" {
		_, ipnet, err := net.ParseCIDR(nw.Spec.IPv4Subnet)
		if err != nil {
			log.Errorf("Error parsing subnet %v. Err: %v", nw.Spec.IPv4Subnet, err)
			return nil, err
		}

		subnetMaskLen, maskLen := ipnet.Mask.Size()
		subnetSize := 1 << uint32(maskLen-subnetMaskLen)

		// build the ip allocation bitset
		bs := bitset.New(uint(subnetSize))
		bs.ClearAll()

		// set first and last bits as used
		bs.Set(0).Set(uint(subnetSize - 1))

		// write the bitset into network object
		buf := bytes.NewBuffer([]byte{})
		bs.WriteTo(buf)
		nw.Status.AllocatedIPv4Addrs = buf.Bytes()
	}

	// create the network state
	ns := &NetworkState{
		Network:    nw,
		endpointDB: make(map[string]*EndpointState),
		macaddrDB:  make(map[string]*EndpointState),
		stateMgr:   stateMgr,
	}
	nw.HandlerCtx = ns

	// mark gateway addr as used
	if nw.Spec.IPv4Gateway != "" {
		log.Infof("Requested gw addr %s", nw.Spec.IPv4Gateway)
		allocAddr, err := ns.allocIPv4Addr(nw.Spec.IPv4Gateway)
		if err != nil {
			log.Errorf("Error allocating gw address. Err: %v", err)
			return nil, err
		} else if allocAddr != nw.Spec.IPv4Gateway {
			log.Errorf("Error allocating gw address(req: %v, alloc: %v)", nw.Spec.IPv4Gateway, allocAddr)
			return nil, fmt.Errorf("Error allocating gw addr")
		}
	}

	// save it to api server
	err := ns.Network.Write()
	if err != nil {
		log.Errorf("Error writing the network state to api server. Err: %v", err)
		return nil, err
	}

	ns.smObjectTracker.init(ns)
	return ns, nil
}

// FindNetwork finds a network
func (sm *Statemgr) FindNetwork(tenant, name string) (*NetworkState, error) {
	// find the object
	obj, err := sm.FindObject("Network", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return NetworkStateFromObj(obj)
}

// FindNetworkByVlanID finds a network by vlan-id
func (sm *Statemgr) FindNetworkByVlanID(tenant string, vlanID uint32) (*NetworkState, error) {
	// find the object
	nObjs, err := sm.ListNetworks()
	if err != nil {
		return nil, err
	}

	for _, nw := range nObjs {
		if nw.Network.Network.Spec.VlanID == vlanID {
			return nw, nil
		}
	}

	return nil, fmt.Errorf("failed to find network with VLAN ID : %v", vlanID)
}

// ListNetworks lists all networks
func (sm *Statemgr) ListNetworks() ([]*NetworkState, error) {
	objs := sm.ListObjects("Network")

	var networks []*NetworkState
	for _, obj := range objs {
		nso, err := NetworkStateFromObj(obj)
		if err != nil {
			return networks, err
		}

		networks = append(networks, nso)
	}

	return networks, nil
}

func (sm *Statemgr) listNetworksByTenant(tenant string) ([]*NetworkState, error) {
	objs := sm.ListObjects("Network")

	var networks []*NetworkState
	for _, obj := range objs {
		nso, err := NetworkStateFromObj(obj)
		if err != nil {
			return networks, err
		}

		if nso.Network.Tenant == tenant {
			networks = append(networks, nso)
		}
	}

	return networks, nil
}

//GetNetworkWatchOptions gets options
func (sm *Statemgr) GetNetworkWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec"}
	return &opts
}

// OnNetworkCreate creates local network state based on watch event
func (sm *Statemgr) OnNetworkCreate(nw *ctkit.Network) error {
	// create new network state
	ns, err := NewNetworkState(nw, sm)
	if err != nil {
		log.Errorf("Error creating new network state. Err: %v", err)
		return err
	}

	if IsObjInternal(ns.Network.Labels) {
		ns.Network.SetInternal()
	}
	// On restart npm might receive create events for networks that were rejected earlier.
	// skip those here.. they are processed after watcher has started
	if nw.Status.OperState == network.OperState_Rejected.String() {
		log.Errorf("Network %v is in %s state, skip", nw.Name, nw.Status.OperState)
		return nil
	}

	// Check if there is another network with the same vlan id, it is not allowed but can slip-thru
	// the hooks - raise an error
	isOk := true
	networks, _ := sm.ListNetworks()
	for _, nso := range networks {
		if strings.ToLower(nso.Network.Spec.Type) == strings.ToLower(network.NetworkType_Routed.String()) {
			//Ignore if network is routed types
			continue
		}
		if nso.Network.Network.Name == nw.Name {
			continue
		}
		if nso.Network.Network.Spec.VlanID == nw.Spec.VlanID {
			// TODO: convert this to some kind of event
			log.Errorf("Network %s is created with same vlan id as another network %s", nw.Name, nso.Network.Name)
			isOk = false
			// keep finding more clashes and log errors
		}
	}
	if isOk {
		nw.Status.OperState = network.OperState_Active.String()
		// store it in local DB
		sm.AddObjectToMbus(nw.MakeKey("network"), ns, references(nw))
	} else {
		nw.Status.OperState = network.OperState_Rejected.String()
	}
	nw.Write()
	log.Infof("Created Network state {Meta: %+v, Spec: %+v, OperState: %+v}",
		ns.Network.ObjectMeta, ns.Network.Spec, ns.Network.Status.OperState)

	return nil
}

// OnNetworkUpdate handles network update
func (sm *Statemgr) OnNetworkUpdate(nw *ctkit.Network, nnw *network.Network) error {
	// see if anything changed
	_, ok := ref.ObjDiff(nw.Spec, nnw.Spec)
	ok = ok || (nw.Network.Status.OperState != nnw.Status.OperState)
	rejected := nnw.Status.OperState == network.OperState_Rejected.String()
	if ((nnw.GenerationID == nw.GenerationID) && !ok) || rejected {
		nw.ObjectMeta = nnw.ObjectMeta
		return nil
	}

	if IsObjInternal(nw.Network.Labels) {
		nw.SetInternal()
	}
	// find the network state
	nwState, err := NetworkStateFromObj(nw)
	if err != nil {
		log.Errorf("Can't find network for updating {%+v}. Err: {%v}", nw.ObjectMeta, err)
		return fmt.Errorf("Can not find network")
	}

	nw.ObjectMeta = nnw.ObjectMeta
	nw.Spec = nnw.Spec

	err = sm.UpdateObjectToMbus(nnw.MakeKey(string(apiclient.GroupNetwork)), nwState, references(nnw))
	if err != nil {
		log.Errorf("could not add Network to DB (%s)", err)
	}

	return nil
}

// OnNetworkDelete deletes a network
func (sm *Statemgr) OnNetworkDelete(nto *ctkit.Network) error {
	log.Infof("Delete Network {Meta: %+v, Spec: %+v}", nto.Network.ObjectMeta, nto.Network.Spec)
	// see if we already have it
	nso, err := sm.FindObject("Network", nto.Tenant, "default", nto.Name)
	if err != nil {
		log.Errorf("Can not find the network %s|%s", nto.Tenant, nto.Name)
		return fmt.Errorf("Network not found")
	}

	// convert it to network state
	ns, err := NetworkStateFromObj(nso)
	if err != nil {
		return err
	}

	// cleanup network state
	err = ns.Delete()
	if err != nil {
		log.Errorf("Error deleting the network {%+v}. Err: %v", ns, err)
		return err
	}
	// delete it from the DB
	err = sm.DeleteObjectToMbus(nto.MakeKey("network"), ns, references(nto))
	if err == nil {
		// If there was another network with same vlanid that was in rejected state, it can be
		// accepted now
		networks, err := sm.ListNetworks()
		if err != nil {
			return err
		}
		for _, nso := range networks {
			if nso.Network.Network.Spec.VlanID == nto.Network.Spec.VlanID &&
				nso.Network.Network.Status.OperState == network.OperState_Rejected.String() {
				log.Infof("Activate network %s", nso.Network.Network.Name)
				nso.Network.Network.Status.OperState = network.OperState_Active.String()
				// update the status to good status, it will be processed as watch event
				nso.Network.Write()
				break
			}
		}
	}
	return err
}

// OnNetworkReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnNetworkReconnect() {
	return
}

func (sm *Statemgr) checkRejectedNetworks() {
	sm.networkKindLock.Lock()
	defer sm.networkKindLock.Unlock()
	// check all the rejected networks and see if they can be accepted
	// this is used on npm restart to handle any deleted networks when npm was down
	rejectedNetworks := []*NetworkState{}
	networks, err := sm.ListNetworks()
	if err != nil {
		return
	}
	for _, nso := range networks {
		if nso.Network.Network.Status.OperState == network.OperState_Rejected.String() {
			rejectedNetworks = append(rejectedNetworks, nso)
		}
	}
	for _, rso := range rejectedNetworks {
		isOk := true
		for _, nso := range networks {
			if nso.Network.Network.Spec.VlanID == rso.Network.Network.Spec.VlanID &&
				nso.Network.Network.Status.OperState != network.OperState_Rejected.String() {
				// still clashes with an accepted network
				isOk = false
				break
			}
		}
		if isOk {
			rso.Network.Network.Status.OperState = network.OperState_Active.String()
		}
		rso.Network.Write()
	}
}

// npm cleans up any network which is created by npm and no references to endpoint
func (sm *Statemgr) cleanUnusedNetworks() {
	sm.networkKindLock.Lock()
	defer sm.networkKindLock.Unlock()
	nws, _ := sm.ListNetworks()
	for _, nw := range nws {
		if nw.isNetworkMarkedDelete() && IsObjInternal(nw.Network.Labels) {
			log.Infof("network garbage collector delete: %+v", nw)
			// delete the endpoint in api server
			nwInfo := network.Network{
				TypeMeta: api.TypeMeta{Kind: "Network"},
				ObjectMeta: api.ObjectMeta{
					Name:      nw.Network.Name,
					Tenant:    nw.Network.Tenant,
					Namespace: nw.Network.Namespace,
				},
			}
			err := nw.stateMgr.ctrler.Network().Delete(&nwInfo)
			nw.curState = networkDeleted
			if err != nil {
				log.Errorf("Ignore error deleting the network. Err: %v", err)
			}
		} else if len(nw.endpointDB) == 0 && IsObjInternal(nw.Network.Labels) {
			// rel A to rel B stale networks gets cleaned up
			log.Errorf("marking for garbage collection networkstate: {%+v}", nw)
			nw.curState = networkMarkDelete
		}
	}

}

func (sm *Statemgr) runNwGarbageCollector() {
	if sm.garbageCollector == nil {
		sm.garbageCollector = &GarbageCollector{Seconds: defaultGarbageCollectionInterval}
	}
	gc := sm.garbageCollector
	gc.CollectorChan = make(chan bool)
	ticker := time.NewTicker(time.Duration(gc.Seconds) * time.Second)
	log.Infof("Network Garbage Collector Started with %v seconds timer", gc.Seconds)
	for {
		select {
		case _, ok := <-gc.CollectorChan:
			if !ok {
				log.Errorf("network garbage collector stopped")
				return
			}
		case _ = <-ticker.C:
			sm.cleanUnusedNetworks()
		}
	}
}

// StartGarbageCollection start garbage collection for system created network
func (sm *Statemgr) StartGarbageCollection() {
	go sm.runNwGarbageCollector()
}

func (ns *NetworkState) isNetworkLive() bool {
	return ns.curState == networkLive
}
func (ns *NetworkState) isNetworkMarkedDelete() bool {
	return ns.curState == networkMarkDelete
}
func (ns *NetworkState) isNetworkDeleted() bool {
	return ns.curState == networkDeleted
}

// only for rel A to rel B
// TODO : remove on rel C and up
// This approach will mark all the network which are not associated
// with orchhub to be labled for garbage collection when they become
// unused by npm
func (sm *Statemgr) labelInternalNetworkObjects() {
	// 1. get all network and create a networkMap without any orch reference
	// 2. find any workload owned by orchhub
	// 3. remove associated networks from the networkMap
	// 4. Mark all remaining networks which are in networkMap
	sm.networkKindLock.Lock()
	defer sm.networkKindLock.Unlock()
	networkMap := make(map[uint32]*ctkit.Network)
	networkVlanMap := make(map[string]uint32)
	netObjs, err := sm.ctrler.Network().List(context.Background(), &api.ListWatchOptions{})
	if err == nil {
		for _, nw := range netObjs {
			if len(nw.Network.Spec.Orchestrators) > 0 {
				// orchhub reference networks are not to be removed by garbage
				// collector
				log.Infof("network [%+v] with orch ref will not be labeled", nw.Network)
				continue
			}
			networkMap[nw.Network.Spec.VlanID] = nw
			networkVlanMap[nw.Network.Name] = nw.Network.Spec.VlanID
		}
	} else {
		log.Infof("Error getting network list %v", err)
		return
	}
	var workloads []*ctkit.Workload
	workloads, err = sm.ctrler.Workload().List(context.Background(), &api.ListWatchOptions{})
	if err == nil {
		for _, ws := range workloads {
			if !isOrchHubKeyPresent(ws.Workload.Labels) {
				log.Infof("venice owned workload [%v] network will be labeled", ws)
				continue
			}

			for ii := range ws.Workload.Spec.Interfaces {
				intf := ws.Workload.Spec.Interfaces[ii]
				if len(intf.Network) > 0 {
					if vlan, ok := networkVlanMap[intf.Network]; ok {
						//log.Infof("Orchhub owned Named Network [%v] will not be labeled", intf.Network)
						delete(networkMap, vlan)
					}
				} else {
					//log.Infof("Orchhub owned External Network [%v] will not be labeled", intf.ExternalVlan)
					delete(networkMap, intf.ExternalVlan)
				}
			}
		}
	} else {
		log.Infof("Error getting workload list %v", err)
	}
	for _, ns := range networkMap {
		//Ignore internal network labeling if tenant is default
		if ns.Tenant != "default" {
			continue
		}
		if !IsObjInternal(ns.Labels) {
			log.Infof("Updating Network %v with internal label", ns)
			nwt := network.Network{
				TypeMeta: api.TypeMeta{Kind: "Network"},
				ObjectMeta: api.ObjectMeta{
					Name:      ns.Name,
					Tenant:    ns.Tenant,
					Namespace: ns.Namespace,
					Labels:    ns.Labels,
				},
				Spec:   ns.Spec,
				Status: ns.Status,
			}
			if nwt.Labels == nil {
				nwt.Labels = map[string]string{}
			}
			AddNpmSystemLabel(nwt.Labels)
			err := sm.ctrler.Network().Update(&nwt)
			if err != nil {
				log.Errorf("Ignore error updating the network[%v]. Err: %v", nwt, err)
			}

		}
	}
}

// Write writes the object to api server
func (ns *NetworkState) Write() error {
	var err error

	ns.Network.Lock()
	defer ns.Network.Unlock()

	prop := &ns.Network.Status.PropagationStatus
	propStatus := ns.getPropStatus()
	newProp := &security.PropagationStatus{
		GenerationID:  propStatus.generationID,
		MinVersion:    propStatus.minVersion,
		Pending:       propStatus.pending,
		PendingNaples: propStatus.pendingDSCs,
		Updated:       propStatus.updated,
		Status:        propStatus.status,
	}

	//Do write only if changed
	if ns.stateMgr.propgatationStatusDifferent(prop, newProp) {
		ns.Network.Status.PropagationStatus = *newProp
		err = ns.Network.Write()
		if err != nil {
			ns.Network.Status.PropagationStatus = *prop
		}
	}

	return err
}

// OnNetworkCreateReq gets called when agent sends create request
func (sm *Statemgr) OnNetworkCreateReq(nodeID string, objinfo *netproto.Network) error {
	return nil
}

// OnNetworkUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnNetworkUpdateReq(nodeID string, objinfo *netproto.Network) error {
	return nil
}

// OnNetworkDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnNetworkDeleteReq(nodeID string, objinfo *netproto.Network) error {
	return nil
}

// OnNetworkOperUpdate gets called when policy updates arrive from agents
func (sm *Statemgr) OnNetworkOperUpdate(nodeID string, objinfo *netproto.Network) error {
	sm.UpdateNetworkStatus(nodeID, objinfo.ObjectMeta.Tenant, objinfo.ObjectMeta.Name, objinfo.ObjectMeta.GenerationID)
	return nil
}

// OnNetworkOperDelete gets called when policy delete arrives from agent
func (sm *Statemgr) OnNetworkOperDelete(nodeID string, objinfo *netproto.Network) error {
	return nil
}

//GetDBObject get db object
func (ns *NetworkState) GetDBObject() memdb.Object {
	return convertNetwork(ns)
}

// GetKey returns the key of Network
func (ns *NetworkState) GetKey() string {
	return ns.Network.GetKey()
}

func (ns *NetworkState) isMarkedForDelete() bool {
	return (ns.curState == networkMarkDelete)
}

//TrackedDSCs keeps a list of DSCs being tracked for propagation status
func (ns *NetworkState) TrackedDSCs() []string {
	dscs, _ := ns.stateMgr.ListDistributedServiceCards()

	trackedDSCs := []string{}
	for _, dsc := range dscs {
		if ns.stateMgr.isDscEnforcednMode(&dsc.DistributedServiceCard.DistributedServiceCard) && ns.stateMgr.IsObjectValidForDSC(dsc.DistributedServiceCard.DistributedServiceCard.Status.PrimaryMAC, "Network", ns.Network.ObjectMeta) {
			trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
		}
	}
	return trackedDSCs
}

func (ns *NetworkState) processDSCUpdate(dsc *cluster.DistributedServiceCard) error {
	ns.Network.Lock()
	defer ns.Network.Unlock()

	if ns.stateMgr.isDscEnforcednMode(dsc) && ns.stateMgr.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "Network", ns.Network.ObjectMeta) {
		ns.smObjectTracker.startDSCTracking(dsc.Name)
	} else {
		ns.smObjectTracker.stopDSCTracking(dsc.Name)
	}
	return nil
}

func (ns *NetworkState) processDSCDelete(dsc *cluster.DistributedServiceCard) error {
	ns.Network.Lock()
	defer ns.Network.Unlock()
	if ns.stateMgr.IsObjectValidForDSC(dsc.Status.PrimaryMAC, "Network", ns.Network.ObjectMeta) == true {
		ns.smObjectTracker.stopDSCTracking(dsc.Name)
	}
	return nil
}

// UpdateNetworkStatus updates the status of an sg policy
func (sm *Statemgr) UpdateNetworkStatus(nodeuuid, tenant, name, generationID string) {
	nw, err := sm.FindNetwork(tenant, name)
	if err != nil {
		return
	}

	nw.updateNodeVersion(nodeuuid, generationID)

}

func (sm *Statemgr) handleNetworkPropTopoUpdate(update *memdb.PropagationStTopoUpdate) {

	if update == nil {
		log.Errorf("handleNetworkPropTopoUpdate invalid update received")
		return
	}

	// check if network status needs updates
	for _, d1 := range update.AddDSCs {
		if t, ok := update.AddObjects["Network"]; ok {
			// find all networks with the tenant
			networks, err := sm.listNetworksByTenant(t[0])
			if err != nil {
				sm.logger.Errorf("networks look up failed for tenant: %s", t[0])
				return
			}

			for _, nw := range networks {
				nw.Network.Lock()
				nw.smObjectTracker.startDSCTracking(d1)
				nw.Network.Unlock()
			}
		}
	}

	for _, d2 := range update.DelDSCs {
		if t, ok := update.DelObjects["Network"]; ok {
			// find all networks with the tenant
			networks, err := sm.listNetworksByTenant(t[0])
			if err != nil {
				sm.logger.Errorf("networks look up failed for tenant: %s", t[0])
				return
			}

			for _, nw := range networks {
				nw.Network.Lock()
				nw.smObjectTracker.stopDSCTracking(d2)
				nw.Network.Unlock()
			}
		}
	}
}
