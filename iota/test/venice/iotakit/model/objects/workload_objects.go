package objects

import (
	"bytes"
	"context"
	"fmt"
	"math/rand"
	"os"
	"strconv"
	"strings"

	"github.com/pensando/sw/api/generated/workload"
	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

// Workload represents a VM/container/Baremetal workload (endpoints are associated with the workload)
type Workload struct {
	iotaWorkload       *iota.Workload
	VeniceWorkload     *workload.Workload
	host               *Host
	subnet             *Network
	dscUUID            string
	IsFTPServerRunning bool // is FTP server running already on this workload
}

// WorkloadCollection is the collection of Workloads
type WorkloadCollection struct {
	CollectionCommon
	Workloads []*Workload
}

// WorkloadsMove is a pair of Workloads
type WorkloadsMove struct {
	Workload []*Workload
	Dst      *Host
}

// WorkloadsMoveCollection spec
type WorkloadsMoveCollection struct {
	CollectionCommon
	Moves []*WorkloadsMove
}

func (w *Workload) Name() string {
	return w.iotaWorkload.WorkloadName
}

func (w *Workload) NaplesUUID() string {
	//TODO fix it later
	return w.host.Naples.Instances[0].Dsc.Name
}

func (w *Workload) SetNaplesUUID(uuid string) {
	w.dscUUID = uuid
}

func (w *Workload) GetNaplesUUID() string {
	return w.dscUUID
}

func (w *Workload) Host() *Host {
	return w.host
}

func (w *Workload) NaplesMAC() string {
	return w.host.Naples.Instances[0].Dsc.Status.PrimaryMAC
}

func (w *Workload) NodeName() string {
	return w.iotaWorkload.NodeName
}

func (w *Workload) SetNodeName(nodeName string) {
	w.iotaWorkload.NodeName = nodeName
}

func (w *Workload) SetHost(host *Host) {
	w.host = host
}

func (w *Workload) SetMgmtIP(ip string) {
	w.iotaWorkload.MgmtIp = ip
}

func (w *Workload) GetMgmtIP() string {
	return w.iotaWorkload.MgmtIp
}

func (w *Workload) SetIpPrefix(ipPrefix string) {
	w.iotaWorkload.Interfaces[0].IpPrefix = ipPrefix
}

func (w *Workload) GetIP() string {
	return strings.Split(w.iotaWorkload.Interfaces[0].IpPrefix, "/")[0]
}

func (w *Workload) GetIotaWorkload() *iota.Workload {
	return w.iotaWorkload
}

func (w *Workload) SetInterface(intf string) {
	w.iotaWorkload.Interfaces[0].Interface = intf
}

func (w *Workload) SetParentInterface(intf string) {
	w.iotaWorkload.Interfaces[0].ParentInterface = intf
}

func (w *Workload) SetIotaWorkload(wl *iota.Workload) {
	w.iotaWorkload = wl
}

//Used after workload move
func (w *Workload) SetIotaNodeName(name string) {
	w.iotaWorkload.NodeName = name
}

func (w *Workload) GetInterface() string {
	return w.iotaWorkload.Interfaces[0].Interface
}

func (w *Workload) GetGatewayIP() string {
	ip := strings.Split(w.iotaWorkload.Interfaces[0].IpPrefix, "/")[0]
	ipAddrBytes := strings.Split(ip, ".")
	return fmt.Sprintf("%s.%s.%s.1", ipAddrBytes[0], ipAddrBytes[1], ipAddrBytes[2])
}

func (w *Workload) GetNetworkName() string {
	return w.iotaWorkload.Interfaces[0].NetworkName
}

func NewWorkload(host *Host, w *workload.Workload, wtype iota.WorkloadType, wimage string, switchName string, subnets []*Network) *Workload {

	convertMac := func(s string) string {
		mac := strings.Replace(s, ".", "", -1)
		var buffer bytes.Buffer
		var l1 = len(mac) - 1
		for i, rune := range mac {
			buffer.WriteRune(rune)
			if i%2 == 1 && i != l1 {
				buffer.WriteRune(':')
			}
		}
		return buffer.String()
	}

	// create iota workload object
	iotaWorkload := iota.Workload{
		WorkloadType:  wtype,
		WorkloadName:  w.GetName(),
		NodeName:      host.iotaNode.Name,
		WorkloadImage: wimage,
	}

	for _, intf := range w.Spec.Interfaces {

		nwName := ""
		if subnets != nil {
			for _, subnet := range subnets {
				if subnet.VeniceNetwork.Spec.VlanID == intf.ExternalVlan {
					nwName = intf.Network
					break
				}
			}
			if nwName == "" {
				log.Errorf("Error find network for intf %v", intf)
				return nil
			}
		}
		iotaWorkload.Interfaces = append(iotaWorkload.Interfaces, &iota.Interface{
			EncapVlan:       intf.MicroSegVlan,
			MacAddress:      convertMac(intf.MACAddress),
			Interface:       "lif100", // ugly hack here: iota agent creates interfaces like lif100. matching that behavior
			ParentInterface: "lif100", // ugly hack here: iota agent creates interfaces like lif100. matching that behavior
			InterfaceType:   iota.InterfaceType_INTERFACE_TYPE_VSS,
			UplinkVlan:      intf.ExternalVlan,
			SwitchName:      switchName,
			NetworkName:     intf.Network,
		})
	}

	for index := range w.Spec.Interfaces {
		if len(w.Spec.Interfaces[index].IpAddresses) > 0 && w.Spec.Interfaces[index].IpAddresses[0] != "" {
			iotaWorkload.Interfaces[index].IpPrefix = w.Spec.Interfaces[index].IpAddresses[0] + "/24" //Assuming it is /24 for now
		}
	}
	return &Workload{
		iotaWorkload:   &iotaWorkload,
		VeniceWorkload: w,
		host:           host,
		//	subnet:         subnet,
	}
}

func NewWorkloadCollection(client objClient.ObjClient, testbed *testbed.TestBed) *WorkloadCollection {
	return &WorkloadCollection{
		CollectionCommon: CollectionCommon{Client: client,
			Testbed: testbed},
	}
}

func NewWorkloadPairCollection(client objClient.ObjClient, testbed *testbed.TestBed) *WorkloadPairCollection {
	return &WorkloadPairCollection{
		CollectionCommon: CollectionCommon{Client: client,
			Testbed: testbed},
	}
}

// WorkloadPair is a pair of Workloads
type WorkloadPair struct {
	First   *Workload
	Second  *Workload
	Proto   string
	Ports   []int
	Network *Network
}

// WorkloadPairCollection is collection of workload pairs
type WorkloadPairCollection struct {
	CollectionCommon
	Pairs   []*WorkloadPair
	Network *Network
}

// GetSingleWorkloadPair gets a single pair collection based on index
func (wlpc *WorkloadPairCollection) GetSingleWorkloadPair(i int) *WorkloadPairCollection {
	collection := WorkloadPairCollection{}
	collection.Pairs = append(collection.Pairs, wlpc.Pairs[i])
	return &collection
}

// ListIPAddr lists work load ip address
func (wpc *WorkloadPairCollection) ListIPAddr() [][]string {
	workloadNames := [][]string{}

	for _, pair := range wpc.Pairs {
		workloadNames = append(workloadNames, []string{pair.First.GetIP(), pair.Second.GetIP()})
	}

	return workloadNames
}

// WithinNetwork filters workload pairs to only withon subnet
func (wpc *WorkloadPairCollection) WithinNetwork() *WorkloadPairCollection {
	if wpc.HasError() {
		return wpc
	}
	newCollection := WorkloadPairCollection{}

	for _, pair := range wpc.Pairs {
		if pair.First.iotaWorkload.Interfaces[0].UplinkVlan == pair.Second.iotaWorkload.Interfaces[0].UplinkVlan ||
			(pair.First.iotaWorkload.Interfaces[0].NetworkName != "" && pair.First.iotaWorkload.Interfaces[0].NetworkName == pair.Second.iotaWorkload.Interfaces[0].NetworkName) {
			newCollection.Pairs = append(newCollection.Pairs, pair)
		}
	}

	return &newCollection
}

//AcrossNetwork filters workload pairs to different subnet
func (wpc *WorkloadPairCollection) AcrossNetwork(workload *Workload) *WorkloadPairCollection {
	if wpc.HasError() {
		return wpc
	}
	newCollection := WorkloadPairCollection{}

	for _, pair := range wpc.Pairs {
		if pair.First.iotaWorkload.Interfaces[0].NetworkName == workload.iotaWorkload.Interfaces[0].NetworkName &&
			pair.First.iotaWorkload.Interfaces[0].UplinkVlan == workload.iotaWorkload.Interfaces[0].UplinkVlan &&
			pair.Second.iotaWorkload.Interfaces[0].NetworkName != workload.iotaWorkload.Interfaces[0].NetworkName {
			newCollection.Pairs = append(newCollection.Pairs, pair)
		}
	}
	return &newCollection
}

// OnNetwork workloads on particular network
func (wpc *WorkloadPairCollection) OnNetwork(network *Network) *WorkloadPairCollection {
	if wpc.HasError() {
		return wpc
	}
	newCollection := WorkloadPairCollection{}

	for _, pair := range wpc.Pairs {
		if pair.First.iotaWorkload.Interfaces[0].NetworkName == network.VeniceNetwork.Name &&
			pair.Second.iotaWorkload.Interfaces[0].NetworkName == network.VeniceNetwork.Name {
			newCollection.Pairs = append(newCollection.Pairs, pair)
		}
	}

	return &newCollection
}

func (wpc *WorkloadPairCollection) SpecificPair(node1, node2 string, n1, n2 *Network) *WorkloadPairCollection {
	if wpc.HasError() {
		return wpc
	}
	newCollection := WorkloadPairCollection{}

	for _, pair := range wpc.Pairs {
		if (pair.First.NaplesUUID() == node1 && pair.Second.NaplesUUID() == node2 &&
			pair.First.GetNetworkName() == n1.VeniceNetwork.Name && pair.Second.GetNetworkName() == n2.VeniceNetwork.Name) ||
			(pair.First.NaplesUUID() == node2 && pair.Second.NaplesUUID() == node1 &&
				pair.First.GetNetworkName() == n2.VeniceNetwork.Name && pair.Second.GetNetworkName() == n1.VeniceNetwork.Name) {
			newCollection.Pairs = append(newCollection.Pairs, pair)
			break
		}
	}
	return &newCollection
}

type FilterOpType int

const (
	//DefaultModel for GS
	FilterOpAny FilterOpType = 0
	FilterOpAnd FilterOpType = 0
)

type WorkloadFilter struct {
	op      FilterOpType
	naples  *NaplesCollection
	network *NetworkCollection
}

func (filter *WorkloadFilter) SetNaplesCollection(naples *NaplesCollection) {
	filter.naples = naples
}

func (filter *WorkloadFilter) SetNetworkCollection(network *NetworkCollection) {
	filter.network = network
}

func (filter *WorkloadFilter) SetOp(op FilterOpType) {
	filter.op = op
}

func (filter *WorkloadFilter) naplesMatched(w *Workload) bool {
	if filter.naples == nil {
		return true
	}
	matched := false
L:
	for _, n := range filter.naples.Nodes {
		for _, naples := range n.Instances {
			if naples.Dsc.Status.PrimaryMAC == w.GetNaplesUUID() {
				matched = true
				break L
			}
		}
	}

	return matched
}

func (filter *WorkloadFilter) networkMatched(w *Workload) bool {
	if filter.network == nil {
		return true
	}
	matched := false
L:
	for _, n := range filter.network.Subnets() {
		if n.VeniceNetwork.Name == w.GetNetworkName() {
			matched = true
			break L
		}
	}

	return matched
}

func (filter *WorkloadFilter) matched(w *Workload) bool {
	return filter.networkMatched(w) && filter.naplesMatched(w)
}

//Filter based
func (wpc *WorkloadCollection) Filter(filter *WorkloadFilter) *WorkloadCollection {
	if wpc.HasError() {
		return wpc
	}
	newCollection := WorkloadCollection{}

	for _, w := range wpc.Workloads {
		if filter.matched(w) {
			newCollection.Workloads = append(newCollection.Workloads, w)
		}
	}
	return &newCollection
}

func (wpc *WorkloadPairCollection) policyHelper(policyCollection *NetworkSecurityPolicyCollection, action, proto string) *WorkloadPairCollection {
	if policyCollection == nil || len(policyCollection.Policies) == 0 {
		return wpc
	}
	if wpc.err != nil {
		return wpc
	}
	newCollection := WorkloadPairCollection{}
	type ipPair struct {
		sip   string
		dip   string
		proto string
		ports string
	}
	actionCache := make(map[string][]ipPair)

	ipPairPresent := func(pair ipPair) bool {
		for _, pairs := range actionCache {
			for _, ippair := range pairs {
				if ippair.dip == pair.dip && ippair.sip == pair.sip && ippair.proto == pair.proto {
					return true
				}
			}
		}
		return false
	}
	for _, pol := range policyCollection.Policies {
		for _, rule := range pol.VenicePolicy.Spec.Rules {
			for _, sip := range rule.FromIPAddresses {
				for _, dip := range rule.ToIPAddresses {
					for _, proto := range rule.ProtoPorts {
						pair := ipPair{sip: sip, dip: dip, proto: proto.Protocol, ports: proto.Ports}
						if _, ok := actionCache[rule.Action]; !ok {
							actionCache[rule.Action] = []ipPair{}
						}
						//if this IP pair was already added, then don't pick it as precedence is based on order
						if !ipPairPresent(pair) {
							//Copy ports
							actionCache[rule.Action] = append(actionCache[rule.Action], pair)
						}
					}
				}
			}
		}
	}

	getPortsFromRange := func(ports string) []int {
		if ports == "any" {
			//Random port if any
			return []int{5555}
		}
		values := strings.Split(ports, "-")
		start, _ := strconv.Atoi(values[0])
		if len(values) == 1 {
			return []int{start}
		}
		end, _ := strconv.Atoi(values[1])
		portValues := []int{}
		for i := start; i <= end; i++ {
			portValues = append(portValues, i)
		}
		return portValues
	}
	for _, pair := range wpc.Pairs {
		cache, ok := actionCache[action]
		if ok {
			for _, ippair := range cache {
				if ((ippair.dip == "any") || ippair.dip == strings.Split(pair.First.iotaWorkload.Interfaces[0].GetIpPrefix(), "/")[0]) &&
					((ippair.sip == "any") || ippair.sip == strings.Split(pair.Second.iotaWorkload.Interfaces[0].GetIpPrefix(), "/")[0]) &&
					ippair.proto == proto &&
					pair.First.iotaWorkload.Interfaces[0].UplinkVlan == pair.Second.iotaWorkload.Interfaces[0].UplinkVlan {
					pair.Ports = getPortsFromRange(ippair.ports)
					pair.Proto = ippair.proto
					newCollection.Pairs = append(newCollection.Pairs, pair)
				}
			}
		}
	}

	return &newCollection
}

// Permit get allowed Workloads with proto
func (wpc *WorkloadPairCollection) Permit(policyCollection *NetworkSecurityPolicyCollection, proto string) *WorkloadPairCollection {
	return wpc.policyHelper(policyCollection, "permit", proto)
}

// Deny get Denied Workloads with proto
func (wpc *WorkloadPairCollection) Deny(policyCollection *NetworkSecurityPolicyCollection, proto string) *WorkloadPairCollection {
	return wpc.policyHelper(policyCollection, "deny", proto)
}

// Reject get rejected Workloads with proto
func (wpc *WorkloadPairCollection) Reject(policyCollection *NetworkSecurityPolicyCollection, proto string) *WorkloadPairCollection {
	return wpc.policyHelper(policyCollection, "reject", proto)
}

// ExcludeWorkloads excludes some Workloads from collection
func (wpc *WorkloadPairCollection) ExcludeWorkloads(wc *WorkloadCollection) *WorkloadPairCollection {
	if wpc.err != nil {
		return wpc
	}
	newCollection := WorkloadPairCollection{}

	for _, pair := range wpc.Pairs {
		for _, w := range wc.Workloads {
			if pair.First.iotaWorkload.WorkloadName != w.iotaWorkload.WorkloadName &&
				pair.Second.iotaWorkload.WorkloadName != w.iotaWorkload.WorkloadName {
				newCollection.Pairs = append(newCollection.Pairs, pair)
			}
		}
	}

	return &newCollection
}

// Any returna one pair from the collection in random
func (wpc *WorkloadPairCollection) Any(num int) *WorkloadPairCollection {
	if wpc.err != nil || len(wpc.Pairs) <= num {
		return wpc
	}
	newWpc := WorkloadPairCollection{Pairs: []*WorkloadPair{}}
	tmpArry := make([]*WorkloadPair, len(wpc.Pairs))
	copy(tmpArry, wpc.Pairs)
	for i := 0; i < num; i++ {
		idx := rand.Intn(len(tmpArry))
		wpair := tmpArry[idx]
		tmpArry = append(tmpArry[:idx], tmpArry[idx+1:]...)
		newWpc.Pairs = append(newWpc.Pairs, wpair)
	}

	return &newWpc
}

// ReversePairs reverses the pairs by swapping first and second entries
func (wpc *WorkloadPairCollection) ReversePairs() *WorkloadPairCollection {
	if wpc.err != nil || len(wpc.Pairs) < 1 {
		return wpc
	}
	newWpc := WorkloadPairCollection{Pairs: []*WorkloadPair{}}
	for _, pair := range wpc.Pairs {
		newPair := WorkloadPair{
			First:  pair.Second,
			Second: pair.First,
		}
		newWpc.Pairs = append(newWpc.Pairs, &newPair)
	}

	return &newWpc
}

// LocalPairs returns all workloads which are on same naples
func (wpc *WorkloadPairCollection) LocalPairs() *WorkloadPairCollection {
	if wpc.err != nil || len(wpc.Pairs) < 1 {
		return wpc
	}
	newWpc := WorkloadPairCollection{Pairs: []*WorkloadPair{}}
	for _, pair := range wpc.Pairs {
		if pair.First.NaplesUUID() == pair.Second.NaplesUUID() {
			newPair := WorkloadPair{
				Second: pair.Second,
				First:  pair.First,
			}
			newWpc.Pairs = append(newWpc.Pairs, &newPair)
		}
	}

	return &newWpc
}

// RemotePairs returns all workloads which are on different naples
func (wpc *WorkloadPairCollection) RemotePairs() *WorkloadPairCollection {
	if wpc.err != nil || len(wpc.Pairs) < 1 {
		return wpc
	}
	newWpc := WorkloadPairCollection{Pairs: []*WorkloadPair{}}
	for _, pair := range wpc.Pairs {
		if pair.First.NaplesUUID() != pair.Second.NaplesUUID() {
			newPair := WorkloadPair{
				Second: pair.Second,
				First:  pair.First,
			}
			newWpc.Pairs = append(newWpc.Pairs, &newPair)
		}
	}

	return &newWpc
}

func (wc *WorkloadCollection) AllocateHostInterfaces(tb *testbed.TestBed) error {

	var Workloads []*Workload
	type hostIntAlloc struct {
		intfs    []string
		curIndex int
	}
	hostIntfMap := make(map[string]*hostIntAlloc)

	// if there are no Workloads, nothing to do
	if len(wc.Workloads) == 0 {
		return nil
	}

	// build workload list
	for _, wrk := range wc.Workloads {
		if _, ok := hostIntfMap[wrk.NodeName()]; !ok {
			hostIntfs := tb.GetHostIntfs(wrk.NodeName(), wrk.GetNaplesUUID())
			if len(hostIntfs) == 0 {
				return fmt.Errorf("Found no host interfaces on node %v", wrk.NodeName())
			}
			hostIntfMap[wrk.NodeName()+wrk.GetNaplesUUID()] = &hostIntAlloc{intfs: hostIntfs}
		}

		Workloads = append(Workloads, wrk)
	}

	for _, wrk := range Workloads {
		hostAlloc, _ := hostIntfMap[wrk.iotaWorkload.NodeName+wrk.GetNaplesUUID()]
		wrk.iotaWorkload.Interfaces[0].ParentInterface = hostAlloc.intfs[hostAlloc.curIndex%len(hostAlloc.intfs)]
		//log.Infof("Alloc host intf %v %v", wrk.iotaWorkload.WorkloadName, wrk.iotaWorkload.ParentInterface)
		hostAlloc.curIndex++
	}
	return nil
}

// Bringup brings up all Workloads in the collection
func (wc *WorkloadCollection) Bringup(tb *testbed.TestBed) error {
	var Workloads []*iota.Workload
	// if there are no Workloads, nothing to do
	if len(wc.Workloads) == 0 {
		return nil
	}

	// build workload list
	for _, wrk := range wc.Workloads {
		Workloads = append(Workloads, wrk.iotaWorkload)
	}

	// send workload add message
	wrkLd := &iota.WorkloadMsg{
		ApiResponse: &iota.IotaAPIResponse{},
		WorkloadOp:  iota.Op_ADD,
		Workloads:   Workloads,
	}
	topoClient := iota.NewTopologyApiClient(tb.Client().Client)
	appResp, err := topoClient.AddWorkloads(context.Background(), wrkLd)
	log.Debugf("Got add workload resp: %+v, err: %v", appResp, err)
	if err != nil {
		log.Errorf("Failed to instantiate Apps. Err: %v", err)
		return fmt.Errorf("Error creating IOTA workload. err: %v", err)
	} else if appResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Failed to instantiate Apps. resp: %+v.", appResp.ApiResponse)
		return fmt.Errorf("Error creating IOTA workload. Resp: %+v", appResp.ApiResponse)
	}
	for _, wrk := range wc.Workloads {
		for _, gwrk := range appResp.Workloads {
			if gwrk.WorkloadName == wrk.iotaWorkload.WorkloadName {
				wrk.iotaWorkload.MgmtIp = gwrk.MgmtIp
				wrk.iotaWorkload.Interfaces[0].Interface = gwrk.Interfaces[0].GetInterface()
			}
		}
	}

	// copy over some binaries after Workloads are brought up
	fuzBinPath := os.Getenv("GOPATH") + "/src/github.com/pensando/sw/iota/bin/fuz"
	for _, wrk := range wc.Workloads {
		if err := tb.CopyToWorkload(wrk.iotaWorkload.NodeName, wrk.iotaWorkload.WorkloadName, []string{fuzBinPath}, "."); err != nil {
			log.Errorf("error copying fuz binary to workload: %s", err)
		}
	}

	return nil
}

// Any returna one pair from the collection in random
func (wpc *WorkloadCollection) Any(num int) *WorkloadCollection {
	if wpc.err != nil || len(wpc.Workloads) <= num {
		return wpc
	}
	newWpc := WorkloadCollection{CollectionCommon: wpc.CollectionCommon}
	tmpArry := make([]*Workload, len(wpc.Workloads))
	copy(tmpArry, wpc.Workloads)
	for i := 0; i < num; i++ {
		idx := rand.Intn(len(tmpArry))
		wpair := tmpArry[idx]
		tmpArry = append(tmpArry[:idx], tmpArry[idx+1:]...)
		newWpc.Workloads = append(newWpc.Workloads, wpair)
	}

	return &newWpc
}

// String Prints workload string
func (wpc *WorkloadCollection) String() string {

	workloads := []string{}

	for _, wl := range wpc.Workloads {
		workloads = append(workloads, wl.Name())
	}

	return strings.Join(workloads, "-")

}

// MeshPairs returns full-mesh pair of workloads
func (wc *WorkloadCollection) MeshPairs() *WorkloadPairCollection {
	if wc.HasError() {
		return &WorkloadPairCollection{CollectionCommon: CollectionCommon{Client: wc.Client, err: wc.Error()}}
	}

	collection := WorkloadPairCollection{}

	for i, wf := range wc.Workloads {
		for j, ws := range wc.Workloads {
			if i != j {
				pair := WorkloadPair{
					First:  wf,
					Second: ws,
				}
				collection.Pairs = append(collection.Pairs, &pair)
			}
		}
	}

	return &collection
}

// MeshPairs returns full-mesh pair of Workloads
func (wc *WorkloadCollection) MeshPairsWithOther(other *WorkloadCollection) *WorkloadPairCollection {
	if wc.HasError() {
		return NewWorkloadPairCollection(wc.CollectionCommon.Client, wc.CollectionCommon.Testbed)
	}

	collection := WorkloadPairCollection{}

	for _, wf := range wc.Workloads {
		for _, ws := range other.Workloads {
			pair := WorkloadPair{
				First:  wf,
				Second: ws,
			}
			collection.Pairs = append(collection.Pairs, &pair)
		}
	}

	return &collection
}

// LocalPairs returns pairs of Workloads in same host
func (wc *WorkloadCollection) LocalPairs() *WorkloadPairCollection {

	collection := NewWorkloadPairCollection(wc.Client, wc.Testbed)

	for i, wf := range wc.Workloads {
		for j, ws := range wc.Workloads {
			if i != j && wf.iotaWorkload.NodeName == ws.iotaWorkload.NodeName {
				pair := WorkloadPair{
					First:  wf,
					Second: ws,
				}
				collection.Pairs = append(collection.Pairs, &pair)
			}
		}
	}

	return collection
}

// LocalPairsWithinNetwork returns pairs of Workloads in same host and same network
func (wc *WorkloadCollection) LocalPairsWithinNetwork() *WorkloadPairCollection {

	collection := NewWorkloadPairCollection(wc.Client, wc.Testbed)

	for i, wf := range wc.Workloads {
		for j, ws := range wc.Workloads {
			if i != j && wf.iotaWorkload.NodeName == ws.iotaWorkload.NodeName &&
				wf.iotaWorkload.Interfaces[0].UplinkVlan == ws.iotaWorkload.Interfaces[0].UplinkVlan {
				pair := WorkloadPair{
					First:  wf,
					Second: ws,
				}
				collection.Pairs = append(collection.Pairs, &pair)
			}
		}
	}

	return collection
}

// RemotePairsWithinNetwork returns pairs of Workloads on different hosts and same network
func (wc *WorkloadCollection) RemotePairsWithinNetwork() *WorkloadPairCollection {

	collection := NewWorkloadPairCollection(wc.Client, wc.Testbed)

	for i, wf := range wc.Workloads {
		for j, ws := range wc.Workloads {
			if i != j && wf.iotaWorkload.NodeName != ws.iotaWorkload.NodeName &&
				wf.iotaWorkload.Interfaces[0].UplinkVlan == ws.iotaWorkload.Interfaces[0].UplinkVlan {
				pair := WorkloadPair{
					First:  wf,
					Second: ws,
				}
				collection.Pairs = append(collection.Pairs, &pair)
			}
		}
	}

	return collection
}

func (wc *WorkloadCollection) LocalPairsAcrossNetwork(workload *Workload) *WorkloadPairCollection {
	collection := NewWorkloadPairCollection(wc.Client, wc.Testbed)
	for i, wf := range wc.Workloads {
		if wf.iotaWorkload.Interfaces[0].NetworkName == workload.iotaWorkload.Interfaces[0].NetworkName &&
			wf.iotaWorkload.Interfaces[0].UplinkVlan == workload.iotaWorkload.Interfaces[0].UplinkVlan {
			for j, ws := range wc.Workloads {
				if i != j && ws.iotaWorkload.NodeName == workload.iotaWorkload.NodeName &&
					ws.iotaWorkload.Interfaces[0].UplinkVlan != workload.iotaWorkload.Interfaces[0].UplinkVlan &&
					ws.iotaWorkload.Interfaces[0].NetworkName != workload.iotaWorkload.Interfaces[0].NetworkName {
					pair := WorkloadPair{
						First:  wf,
						Second: ws,
					}
					collection.Pairs = append(collection.Pairs, &pair)
				}
			}
			return collection
		}
	}
	return collection
}

func (wc *WorkloadCollection) RemotePairsAcrossNetwork(workload *Workload) *WorkloadPairCollection {
	collection := NewWorkloadPairCollection(wc.Client, wc.Testbed)
	for i, wf := range wc.Workloads {
		if wf.iotaWorkload.Interfaces[0].NetworkName == workload.iotaWorkload.Interfaces[0].NetworkName &&
			wf.iotaWorkload.Interfaces[0].UplinkVlan == workload.iotaWorkload.Interfaces[0].UplinkVlan {
			for j, ws := range wc.Workloads {
				if i != j && ws.iotaWorkload.NodeName != workload.iotaWorkload.NodeName &&
					ws.iotaWorkload.Interfaces[0].UplinkVlan != workload.iotaWorkload.Interfaces[0].UplinkVlan &&
					ws.iotaWorkload.Interfaces[0].NetworkName != workload.iotaWorkload.Interfaces[0].NetworkName {
					pair := WorkloadPair{
						First:  wf,
						Second: ws,
					}
					collection.Pairs = append(collection.Pairs, &pair)
				}
			}
			return collection
		}
	}
	return collection
}

func (wpc *WorkloadPairCollection) WorkloadPairGetDynamicIps(tb *testbed.TestBed) error {
	cmds := []*iota.Command{}
	for _, pair := range wpc.Pairs {
		log.Infof("Workload pair %v-%v getting IPs dynamically.", pair.First.GetInterface(), pair.Second.GetInterface())
		cmdF := iota.Command{
			Mode:       iota.CommandMode_COMMAND_FOREGROUND,
			Command:    fmt.Sprintf("ifconfig %s 0 && dhclient -r %s && dhclient %s", pair.First.GetInterface(), pair.First.GetInterface(), pair.First.GetInterface()),
			EntityName: pair.First.Name(),
			NodeName:   pair.First.NodeName(),
		}
		cmds = append(cmds, &cmdF)

		cmdS := iota.Command{
			Mode:       iota.CommandMode_COMMAND_FOREGROUND,
			Command:    fmt.Sprintf("ifconfig %s 0 && dhclient -r %s && dhclient %s", pair.Second.GetInterface(), pair.Second.GetInterface(), pair.Second.GetInterface()),
			EntityName: pair.Second.Name(),
			NodeName:   pair.Second.NodeName(),
		}
		cmds = append(cmds, &cmdS)
	}

	trmode := iota.TriggerMode_TRIGGER_PARALLEL
	if !tb.HasNaplesSim() {
		trmode = iota.TriggerMode_TRIGGER_NODE_PARALLEL
	}

	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: trmode,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tb.Client().Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return fmt.Errorf("Failed to trigger dhclient on workloadpair. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	for _, cmdResp := range triggerResp.Commands {
		if cmdResp.ExitCode != 0 {
			return fmt.Errorf("Getting IP failed for workload pair. Resp: %#v", cmdResp)
		}
	}

	for _, pair := range wpc.Pairs {
		err := WorkloadUpdateDynamicIPs(pair.First, tb)
		if err != nil {
			return err
		}
		err = WorkloadUpdateDynamicIPs(pair.Second, tb)
		if err != nil {
			return err
		}
	}
	return nil
}

func WorkloadUpdateDynamicIPs(w *Workload, tb *testbed.TestBed) error {
	cmds := []*iota.Command{
		{
			Mode:       iota.CommandMode_COMMAND_FOREGROUND,
			Command:    fmt.Sprintf("ip -o -4 addr list %s | awk '{print $4}'", w.GetInterface()),
			EntityName: w.Name(),
			NodeName:   w.NodeName(),
		},
	}

	trmode := iota.TriggerMode_TRIGGER_PARALLEL
	if !tb.HasNaplesSim() {
		trmode = iota.TriggerMode_TRIGGER_NODE_PARALLEL
	}

	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: trmode,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tb.Client().Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return fmt.Errorf("Failed to update dynamic IP on workload. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	for _, cmdResp := range triggerResp.Commands {
		if cmdResp.ExitCode != 0 {
			return fmt.Errorf("Update dynamic IP failed. Resp: %#v", cmdResp)
		}
		ipPrefix := strings.Split(cmdResp.Stdout, "\r\n")
		if ipPrefix[0] == "" {
			return fmt.Errorf("Workload :%v failed to get IP", w.Name())
		}
		log.Infof("%v's dynamic IpPrefix: [%v]", w.Name(), ipPrefix[0])
		w.SetIpPrefix(ipPrefix[0])
	}

	return WorkloadUpdateDefaultRoute(w, tb)
}

func WorkloadUpdateDefaultRoute(w *Workload, tb *testbed.TestBed) error {
	cmds := []*iota.Command{
		{
			Mode:       iota.CommandMode_COMMAND_FOREGROUND,
			Command:    fmt.Sprintf("ip route del default 2>/dev/null; ip route add default via %s", w.GetGatewayIP()),
			EntityName: w.Name(),
			NodeName:   w.NodeName(),
		},
	}

	trmode := iota.TriggerMode_TRIGGER_PARALLEL
	if !tb.HasNaplesSim() {
		trmode = iota.TriggerMode_TRIGGER_NODE_PARALLEL
	}

	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: trmode,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tb.Client().Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return fmt.Errorf("Failed to update default route on workload. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	for _, cmdResp := range triggerResp.Commands {
		if cmdResp.ExitCode != 0 {
			return fmt.Errorf("Update default route. Resp: %#v", cmdResp)
		}
	}
	return nil
}
