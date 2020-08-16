// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/memdb/objReceiver"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/runtime"
)

func convertNetifObj(nodeID string, current *network.NetworkInterface, agentNetif *netproto.Interface) *network.NetworkInterface {
	// convert agentNetif -> veniceNetif
	creationTime, _ := types.TimestampProto(time.Now())
	var netif *network.NetworkInterface
	if current == nil {
		netif = &network.NetworkInterface{
			TypeMeta: api.TypeMeta{
				Kind: "NetworkInterface",
			},
			ObjectMeta: agentNetif.ObjectMeta,
			Spec: network.NetworkInterfaceSpec{
				AdminStatus: agentNetif.Spec.AdminStatus,
			},
			Status: network.NetworkInterfaceStatus{
				DSC: nodeID,
				// TBD: PrimaryMac: "tbf",
			},
		}
		netif.CreationTime = api.Timestamp{Timestamp: *creationTime}
	} else {
		netif = current
	}

	switch strings.ToUpper(netif.Spec.AdminStatus) {
	case netproto.IFStatus_UP.String():
		netif.Spec.AdminStatus = network.IFStatus_UP.String()
	case netproto.IFStatus_DOWN.String():
		netif.Spec.AdminStatus = network.IFStatus_DOWN.String()
	default:
		netif.Spec.AdminStatus = network.IFStatus_UP.String()
	}

	if netif.Spec.AdminStatus == network.IFStatus_DOWN.String() {
		log.Infof("Ignoring admin status down from agent for interface %v", netif.Name)
		netif.Spec.AdminStatus = network.IFStatus_UP.String()
	}

	switch strings.ToUpper(agentNetif.Status.OperStatus) {
	case "UP":
		netif.Status.OperStatus = network.IFStatus_UP.String()
	case "DOWN":
		netif.Status.OperStatus = network.IFStatus_DOWN.String()
	default:
		netif.Status.OperStatus = network.IFStatus_UP.String() // TBD: should default be modeled to vencie user?
	}

	switch agentNetif.Spec.Type {
	case "UPLINK_ETH":
		netif.Spec.Type = network.NetworkInterfaceStatus_UPLINK_ETH.String()
		netif.Spec.IPAllocType = network.IPAllocTypes_Static.String()
		netif.Status.Type = network.NetworkInterfaceStatus_UPLINK_ETH.String()
	case "UPLINK_MGMT":
		netif.Spec.Type = network.NetworkInterfaceStatus_UPLINK_MGMT.String()
		netif.Spec.IPAllocType = network.IPAllocTypes_DHCP.String()
		netif.Status.Type = network.NetworkInterfaceStatus_UPLINK_MGMT.String()
	case "HOST_PF":
		netif.Spec.Type = network.NetworkInterfaceStatus_HOST_PF.String()
		netif.Spec.IPAllocType = network.IPAllocTypes_DHCP.String()
		netif.Status.Type = network.NetworkInterfaceStatus_HOST_PF.String()
	case "LOOPBACK":
		netif.Spec.Type = network.NetworkInterfaceStatus_LOOPBACK_TEP.String()
		netif.Spec.IPAllocType = network.IPAllocTypes_Static.String()
		netif.Status.Type = network.NetworkInterfaceStatus_LOOPBACK_TEP.String()
	default:
		netif.Spec.Type = network.NetworkInterfaceStatus_NONE.String()
		netif.Spec.IPAllocType = network.IPAllocTypes_Static.String()
		netif.Status.Type = network.NetworkInterfaceStatus_UPLINK_ETH.String()
	}

	if agentNetif.Status.IPAllocType != "" {
		netif.Spec.IPAllocType = agentNetif.Status.IPAllocType
	}
	netif.Tenant = ""
	netif.Namespace = ""
	netif.Status.IFHostStatus = &network.NetworkInterfaceHostStatus{
		HostIfName: agentNetif.Status.IFHostStatus.HostIfName,
		MACAddress: agentNetif.Status.IFHostStatus.MacAddress,
	}
	netif.Status.IFUplinkStatus = &network.NetworkInterfaceUplinkStatus{
		// TBD: LinkSpeed: "tbf",
		IPConfig: &cluster.IPConfig{
			IPAddress: agentNetif.Status.IFUplinkStatus.IPAddress,
			DefaultGW: agentNetif.Status.IFUplinkStatus.GatewayIP,
		},
	}
	if agentNetif.Spec.Type == "UPLINK_ETH" && agentNetif.Status.IFUplinkStatus.LLDPNeighbor != nil {
		netif.Status.IFUplinkStatus.LLDPNeighbor = &network.LLDPNeighbor{
			ChassisID:       agentNetif.Status.IFUplinkStatus.LLDPNeighbor.ChassisID,
			SysName:         agentNetif.Status.IFUplinkStatus.LLDPNeighbor.SysName,
			SysDescription:  agentNetif.Status.IFUplinkStatus.LLDPNeighbor.SysDescription,
			PortID:          agentNetif.Status.IFUplinkStatus.LLDPNeighbor.PortID,
			PortDescription: agentNetif.Status.IFUplinkStatus.LLDPNeighbor.PortDescription,
			MgmtAddress:     agentNetif.Status.IFUplinkStatus.LLDPNeighbor.MgmtAddress,
		}
	}

	netif.Status.DSCID = agentNetif.Status.DSCID

	return netif
}

// OnInterfaceCreateReq gets called when agent sends create request
func (sm *Statemgr) OnInterfaceCreateReq(nodeID string, agentNetif *netproto.Interface) error {

	log.Infof("interface create for %v DSC ID", agentNetif.Name)
	//Make sure we don't create inteface if DSC ID is not set
	//This is useful for upgrade from lower to higher release.
	if agentNetif.Status.DSCID == "" {
		log.Infof("Ignore interface create for %v DSC ID not set ", agentNetif.Name)
		return nil
	}
	_, err := sm.mbus.FindReceiver(agentNetif.Status.DSC)
	if err != nil {
		log.Errorf("Ignoring interface create as DSC %v (%v) not registered yet %v", agentNetif.Status.DSC, agentNetif.Status.DSCID, err)
		return err
	}

	_, err = smgrNetworkInterface.FindNetworkInterface(agentNetif.Name)
	if err == nil {
		return sm.OnInterfaceUpdateReq(nodeID, agentNetif)
	}
	log.Infof("Creating network interface %v", agentNetif.Name)
	netif := convertNetifObj(nodeID, nil, agentNetif)

	agentNetif.Status.DSC = nodeID

	agentNetif.Spec.Network = ""
	agentNetif.Spec.VrfName = ""

	// store it in local DB
	sm.mbus.AddObject(agentNetif)

	// trigger the create in ctkit, which adds the objec to apiserer
	// retry till it succeeds
	now := time.Now()
	retries := 0
	for {
		err := sm.ctrler.NetworkInterface().Create(netif)
		if err == nil {
			break
		}
		if time.Since(now) > time.Second*2 {
			log.Errorf("create interface failed (%s)", err)
			now = time.Now()
		}
		retries++
		time.Sleep(100 * time.Millisecond)

	}
	return nil
}

// compares interface's LLDP neighbor info from netagent and statemgr
// and update statemgr LLDP neighbor
func checkLLDPNeighborUpdate(netif *network.LLDPNeighbor, agentNetif *netproto.LLDPNeighbor) bool {

	if netif == nil || agentNetif == nil {
		return false
	}
	if netif.ChassisID != agentNetif.ChassisID ||
		netif.SysName != agentNetif.SysName ||
		netif.SysDescription != agentNetif.SysDescription ||
		netif.PortID != agentNetif.PortID ||
		netif.PortDescription != agentNetif.PortDescription ||
		netif.MgmtAddress != agentNetif.MgmtAddress {

		return true
	}
	return false
}

// OnInterfaceUpdateReq gets called when agent sends update request
func (sm *Statemgr) OnInterfaceUpdateReq(nodeID string, agentNetif *netproto.Interface) error {

	log.Infof("interface update for %v DSC ID", agentNetif.Name)
	obj, err := smgrNetworkInterface.FindNetworkInterface(agentNetif.Name)
	if err != nil {
		log.Errorf("Error finding interface %v for update", agentNetif.Name)
		return err
	}

	obj.NetworkInterfaceState.Lock()
	//For now Update only operation status or LLDP update
	update := false
	opStatus := strings.ToUpper(agentNetif.Status.OperStatus)
	if obj.NetworkInterfaceState.Status.OperStatus != opStatus {
		log.Infof("Updating network interface state %v : %v", agentNetif.Name, opStatus)
		update = true
	}

	if agentNetif.Status.DSCID != obj.NetworkInterfaceState.Status.DSCID {
		update = true
	}
	if agentNetif.Spec.Type == netproto.InterfaceSpec_UPLINK_ETH.String() {
		if checkLLDPNeighborUpdate(obj.NetworkInterfaceState.Status.IFUplinkStatus.LLDPNeighbor, agentNetif.Status.IFUplinkStatus.LLDPNeighbor) == true {
			log.Infof("Updating network interface LLDPNeighbor %v : %v", agentNetif.Name, obj.NetworkInterfaceState.Status.IFUplinkStatus.LLDPNeighbor)
			update = true
		}
	}

	if agentNetif.Spec.Type == netproto.InterfaceSpec_HOST_PF.String() {
		if obj.NetworkInterfaceState.Status.IFHostStatus.HostIfName != agentNetif.Status.IFHostStatus.HostIfName ||
			obj.NetworkInterfaceState.Status.IFHostStatus.MACAddress != agentNetif.Status.IFHostStatus.MacAddress {
			log.Infof("Updating network interface host if status %v : %v", agentNetif.Name, agentNetif.Status.IFHostStatus)
			update = true
		}
	}
	if update {
		obj.NetworkInterfaceState.NetworkInterface = *(convertNetifObj(nodeID, &obj.NetworkInterfaceState.NetworkInterface,
			agentNetif))
	}
	obj.NetworkInterfaceState.Unlock()
	if update == false {
		return nil
	}

	// retry till it succeeds
	now := time.Now()
	retries := 0
	for {
		err := obj.Write()
		if err == nil {
			break
		}
		if time.Since(now) > time.Second*2 {
			log.Errorf("update  interface failed (%s)", err)
			now = time.Now()
		}
		retries++
		time.Sleep(100 * time.Millisecond)

	}

	return nil
}

// OnInterfaceDeleteReq gets called when agent sends delete request
func (sm *Statemgr) OnInterfaceDeleteReq(nodeID string, agentNetif *netproto.Interface) error {
	obj, err := sm.FindObject("NetworkInterface", agentNetif.ObjectMeta.Tenant, agentNetif.ObjectMeta.Namespace, agentNetif.ObjectMeta.Name)
	if err != nil {
		return err
	}

	// delete the networkInterface
	ctkitNetif, ok := obj.(*ctkit.NetworkInterface)
	if !ok {
		return ErrIncorrectObjectType
	}
	return sm.ctrler.NetworkInterface().Delete(&ctkitNetif.NetworkInterface)
}

// OnInterfaceOperUpdate gets called when agent sends create request
func (sm *Statemgr) OnInterfaceOperUpdate(nodeID string, agentNetif *netproto.Interface) error {
	eps, err := smgrNetworkInterface.FindNetworkInterface(agentNetif.Name)
	if err != nil {
		return err
	}

	eps.updateNodeVersion(nodeID, agentNetif.ObjectMeta.GenerationID)
	return nil
}

// OnInterfaceOperDelete is called when agent sends delete request
func (sm *Statemgr) OnInterfaceOperDelete(nodeID string, agentNetif *netproto.Interface) error {
	return nil
}

// NetworkInterfaceStateFromObj conerts from memdb object to network state
func NetworkInterfaceStateFromObj(obj runtime.Object) (*NetworkInterfaceState, error) {
	switch obj.(type) {
	case *ctkit.NetworkInterface:
		sgobj := obj.(*ctkit.NetworkInterface)
		switch sgobj.HandlerCtx.(type) {
		case *NetworkInterfaceState:
			sgs := sgobj.HandlerCtx.(*NetworkInterfaceState)
			return sgs, nil
		default:
			return nil, ErrIncorrectObjectType
		}

	default:
		return nil, ErrIncorrectObjectType
	}
}

// FindNetworkInterface finds mirror session state
func (sm *Statemgr) FindNetworkInterface(tenant, name string) (*NetworkInterfaceState, error) {
	// find the object
	obj, err := sm.FindObject("NetworkInterface", tenant, "default", name)
	if err != nil {
		return nil, err
	}

	return NetworkInterfaceStateFromObj(obj)
}

//GetNetworkInterfaceWatchOptions gets options
func (sm *Statemgr) GetNetworkInterfaceWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"ObjectMeta.Labels", "Spec", "Status"}
	return &opts
}

// OnNetworkInterfaceReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnNetworkInterfaceReconnect() {
	return
}

type labelInterfaces struct {
	label map[string]string
	intfs map[string]mirrorObjectState
}

// SmNetworkInterface is statemanager struct for NetworkInterface
type SmNetworkInterface struct {
	featureMgrBase
	sm *Statemgr
}

// NetworkInterfaceState is a wrapper for NetworkInterface object
type NetworkInterfaceState struct {
	NetworkInterfaceState  *ctkit.NetworkInterface `json:"-"` // NetworkInterface object
	pushObject             memdb.PushObjectHandle
	netProtoMirrorSessions []string
	mirrorSessions         []string
	smObjectTracker
	markedForDelete bool
}

var smgrNetworkInterface *SmNetworkInterface

// CompleteRegistration is the callback function statemgr calls after init is done
func (sma *SmNetworkInterface) CompleteRegistration() {
	// if featureflags.IsOVerlayRoutingEnabled() == false {
	// 	return
	// }
	initSmNetworkInterface()

	log.Infof("Got CompleteRegistration for SmNetworkInterface")
	sma.sm.SetNetworkInterfaceReactor(smgrNetworkInterface)
	//Send network interface selectively.
	sma.sm.EnableSelectivePushForKind("Interface")
}

func initSmNetworkInterface() {
	mgr := MustGetStatemgr()
	smgrNetworkInterface = &SmNetworkInterface{
		sm: mgr,
	}
	mgr.Register("statemgrnetif", smgrNetworkInterface)
}

func init() {
	initSmNetworkInterface()
}

// NewNetworkInterfaceState creates a new NetworkInterfaceState
func NewNetworkInterfaceState(intf *ctkit.NetworkInterface, sma *SmNetworkInterface) (*NetworkInterfaceState, error) {
	ifcfg := &NetworkInterfaceState{
		NetworkInterfaceState: intf,
	}
	ifcfg.smObjectTracker.init(ifcfg)
	intf.HandlerCtx = ifcfg
	return ifcfg, nil
}

func convertIFTypeToAgentProto(in string) string {
	switch in {
	case network.IFType_NONE.String():
		return netproto.InterfaceSpec_NONE.String()
	case network.IFType_HOST_PF.String():
		return netproto.InterfaceSpec_HOST_PF.String()
	case network.IFType_UPLINK_ETH.String():
		return netproto.InterfaceSpec_UPLINK_ETH.String()
	case network.IFType_UPLINK_MGMT.String():
		return netproto.InterfaceSpec_UPLINK_MGMT.String()
	case network.IFType_LOOPBACK_TEP.String():
		return netproto.InterfaceSpec_LOOPBACK.String()
	default:
		return "unknown"
	}
}

func convertAgentIFToAPIProto(in string) string {
	switch in {
	case netproto.InterfaceSpec_NONE.String():
		return network.IFType_NONE.String()
	case netproto.InterfaceSpec_HOST_PF.String():
		return network.IFType_HOST_PF.String()
	case netproto.InterfaceSpec_UPLINK_ETH.String():
		return network.IFType_UPLINK_ETH.String()
	case netproto.InterfaceSpec_UPLINK_MGMT.String():
		return network.IFType_UPLINK_MGMT.String()
	case netproto.InterfaceSpec_L3.String():
		return network.IFType_NONE.String()
	case netproto.InterfaceSpec_LOOPBACK.String():
		return network.IFType_LOOPBACK_TEP.String()
	default:
		return "unknown"
	}
}

func convertNetworkInterfaceObject(ifcfg *NetworkInterfaceState) *netproto.Interface {
	agIf := &netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:          globals.DefaultTenant,
			Namespace:       globals.DefaultNamespace,
			Name:            ifcfg.NetworkInterfaceState.Name,
			GenerationID:    ifcfg.NetworkInterfaceState.GenerationID,
			ResourceVersion: ifcfg.NetworkInterfaceState.ResourceVersion,
			UUID:            ifcfg.NetworkInterfaceState.UUID,
		},
		Spec: netproto.InterfaceSpec{
			Type:               convertIFTypeToAgentProto(ifcfg.NetworkInterfaceState.Spec.Type),
			AdminStatus:        ifcfg.NetworkInterfaceState.Spec.AdminStatus,
			Speed:              ifcfg.NetworkInterfaceState.Spec.Speed,
			MTU:                ifcfg.NetworkInterfaceState.Spec.MTU,
			VrfName:            ifcfg.NetworkInterfaceState.Spec.AttachTenant,
			Network:            ifcfg.NetworkInterfaceState.Spec.AttachNetwork,
			ConnectionTracking: ifcfg.NetworkInterfaceState.Spec.ConnectionTracking,
			MirrorSessions:     ifcfg.mirrorSessions,
			TxPolicer:          ifcfg.NetworkInterfaceState.Spec.TxPolicer,
		},
		Status: netproto.InterfaceStatus{
			DSC: ifcfg.NetworkInterfaceState.Status.DSC,
		},
	}

	if ifcfg.NetworkInterfaceState.Spec.IPConfig != nil {
		agIf.Spec.IPAddress = ifcfg.NetworkInterfaceState.Spec.IPConfig.IPAddress
	}
	agIf.Spec.Type = convertIFTypeToAgentProto(ifcfg.NetworkInterfaceState.Status.Type)
	if ifcfg.NetworkInterfaceState.Spec.Pause != nil {
		//Pause is not currently supported
		log.Infof("Pause configuration not supported")
	}
	return agIf
}

// RoutingConfigStateFromObj converts from memdb object to RoutingConfig state
func networkInterfaceStateFromObj(obj runtime.Object) (*NetworkInterfaceState, error) {
	switch obj.(type) {
	case *ctkit.NetworkInterface:
		ifcfg := obj.(*ctkit.NetworkInterface)
		switch ifcfg.HandlerCtx.(type) {
		case *NetworkInterfaceState:
			state := ifcfg.HandlerCtx.(*NetworkInterfaceState)
			return state, nil
		default:
			return nil, fmt.Errorf("incorrect object type")
		}
	default:
		return nil, fmt.Errorf("incorrect object type")
	}
}

func interfaceMirroringAllowed(intfType string) bool {

	switch intfType {
	case network.IFType_UPLINK_ETH.String():
		return true
	}

	return false
}

func (sma *SmNetworkInterface) findInterfacesByLabel(label map[string]string) ([]*NetworkInterfaceState, error) {

	networkStates := []*NetworkInterfaceState{}

	objs, _ := smgrMirrorInterface.findMirroredObjectsByLabel("NetworkInterface", label)
	for _, obj := range objs {
		networkStates = append(networkStates, obj.(*NetworkInterfaceState))
	}
	return networkStates, nil
}
func (sma *SmNetworkInterface) getInterfacesMatchingSelector(selectors []*labels.Selector) ([]*NetworkInterfaceState, error) {

	networkStates := []*NetworkInterfaceState{}

	objs, _ := smgrMirrorInterface.getMirroredObjectsMatchingSelector("NetworkInterface", selectors)
	for _, obj := range objs {
		networkStates = append(networkStates, obj.(*NetworkInterfaceState))
	}
	return networkStates, nil
}

//TrackedDSCs tracked dscs
func (nw *NetworkInterfaceState) TrackedDSCs() []string {

	mgr := MustGetStatemgr()
	trackedDSCs := []string{}

	dsc, err := mgr.FindDistributedServiceCard("default", nw.NetworkInterfaceState.Status.DSC)
	if err != nil {
		log.Errorf("Error looking up DSC %v", nw.NetworkInterfaceState.Status.DSC)
	} else {
		trackedDSCs = append(trackedDSCs, dsc.DistributedServiceCard.DistributedServiceCard.Name)
	}
	return trackedDSCs
}

//Name name of object
func (nw *NetworkInterfaceState) Name() string {
	return nw.NetworkInterfaceState.Name
}

//MirrorAllowed MirrorAllowed or not
func (nw *NetworkInterfaceState) MirrorAllowed() bool {
	return interfaceMirroringAllowed(nw.NetworkInterfaceState.Spec.Type)
}

//Key Key of object
func (nw *NetworkInterfaceState) Key() string {
	return nw.NetworkInterfaceState.GetKey()
}

//Kind kind of object
func (nw *NetworkInterfaceState) Kind() string {
	return nw.NetworkInterfaceState.Kind
}

//MarkedForDelete marked for delete
func (nw *NetworkInterfaceState) MarkedForDelete() bool {
	return nw.markedForDelete
}

//Update udpate object
func (nw *NetworkInterfaceState) Update() error {
	err := smgrNetworkInterface.sm.UpdatePushObjectToMbus(nw.NetworkInterfaceState.MakeKey(string(apiclient.GroupNetwork)),
		nw, references(nw.NetworkInterfaceState))
	if err != nil {
		log.Errorf("error updating  push object %v", err)
	}
	return err
}

//Lock  lock
func (nw *NetworkInterfaceState) Lock() {
	nw.NetworkInterfaceState.Lock()
}

//Unlock unlock
func (nw *NetworkInterfaceState) Unlock() {
	nw.NetworkInterfaceState.Unlock()
}

//Labels gets labels
func (nw *NetworkInterfaceState) Labels() map[string]string {
	return nw.NetworkInterfaceState.ObjectMeta.Labels
}

//DSC gets DSC
func (nw *NetworkInterfaceState) DSC() string {
	return nw.NetworkInterfaceState.Status.DSC
}

//GetMirrorSessions get current mirror sessions
func (nw *NetworkInterfaceState) GetMirrorSessions() []string {
	return nw.mirrorSessions
}

//SetMirrorSessions sets current mirror sessions
func (nw *NetworkInterfaceState) SetMirrorSessions(msessions []string) {

	nw.mirrorSessions = msessions

}

//GetNetProtoMirrorSessions get current mirror sessions
func (nw *NetworkInterfaceState) GetNetProtoMirrorSessions() []string {

	return nw.netProtoMirrorSessions
}

//SetNetProtoMirrorSessions sets current mirror sessions
func (nw *NetworkInterfaceState) SetNetProtoMirrorSessions(msessions []string) {
	nw.netProtoMirrorSessions = msessions
}

// FindNetworkInterface finds network interface
func (sma *SmNetworkInterface) FindNetworkInterface(name string) (*NetworkInterfaceState, error) {
	// find the object
	obj, err := sma.sm.FindObject("NetworkInterface", "", "", name)
	if err != nil {
		return nil, err
	}

	return NetworkInterfaceStateFromObj(obj)
}

// GetKey returns the key of Network
func (nw *NetworkInterfaceState) GetKey() string {
	return nw.NetworkInterfaceState.GetKey()
}

// GetKind returns the kind
func (nw *NetworkInterfaceState) GetKind() string {
	return nw.NetworkInterfaceState.GetKind()
}

//GetGenerationID get genration ID
func (nw *NetworkInterfaceState) GetGenerationID() string {
	return nw.NetworkInterfaceState.GenerationID
}

// Write writes the object to api server
func (nw *NetworkInterfaceState) Write() error {
	var err error

	nw.NetworkInterfaceState.Lock()
	defer nw.NetworkInterfaceState.Unlock()

	if nw.markedForDelete {
		return nil
	}
	nw.NetworkInterfaceState.Status.MirroSessions = nw.mirrorSessions
	err = nw.NetworkInterfaceState.Write()

	return err
}

//GetDBObject get db object
func (nw *NetworkInterfaceState) GetDBObject() memdb.Object {
	return convertNetworkInterfaceObject(nw)
}

//PushObject get push object
func (nw *NetworkInterfaceState) PushObject() memdb.PushObjectHandle {
	return nw.pushObject
}

// OnNetworkInterfaceCreate creates a NetworkInterface
func (sma *SmNetworkInterface) OnNetworkInterfaceCreate(ctkitNetif *ctkit.NetworkInterface) error {
	log.Info("received OnNetworkInterfaceCreate", ctkitNetif.Spec)
	ifcfg, err := NewNetworkInterfaceState(ctkitNetif, sma)
	if err != nil {
		log.Errorf("error creating network interface state")
	}

	smgrMirrorInterface.UpdateLabelMap(ifcfg)

	receiver, err := sma.sm.mbus.FindReceiver(ifcfg.NetworkInterfaceState.Status.DSC)
	if err != nil {
		log.Errorf("error finding receiver for %v %v", ifcfg.NetworkInterfaceState.Status.DSC, err)
		return kvstore.NewTxnFailedError()
	}

	pushObj, err := sma.sm.AddPushObjectToMbus(ctkitNetif.MakeKey(string(apiclient.GroupNetwork)), ifcfg, references(ctkitNetif),
		[]objReceiver.Receiver{receiver})

	if err != nil {
		log.Errorf("error adding push object %v", err)
		return err
	}
	ifcfg.pushObject = pushObj

	if interfaceMirroringAllowed(ctkitNetif.NetworkInterface.Spec.Type) && len(ctkitNetif.NetworkInterface.Labels) != 0 {
		smgrMirrorInterface.processLabelMirrorUpdate(ifcfg)
	}

	return nil
}

// OnNetworkInterfaceUpdate handles update event
func (sma *SmNetworkInterface) OnNetworkInterfaceUpdate(ctkitNetif *ctkit.NetworkInterface, nctkitNetif *network.NetworkInterface) error {
	log.Info("received OnNetworkInterfaceUpdate", ctkitNetif.Spec, nctkitNetif.Spec)

	// update old state
	currIntf, err := networkInterfaceStateFromObj(ctkitNetif)
	if err != nil {
		log.Errorf("error finding exisiting network interface state %v", err)
		return err
	}

	//Update labels only if changed.
	if interfaceMirroringAllowed(ctkitNetif.NetworkInterface.Spec.Type) &&
		!reflect.DeepEqual(ctkitNetif.Labels, nctkitNetif.Labels) {
		smgrMirrorInterface.ClearLabelMap(currIntf)
		ctkitNetif.ObjectMeta = nctkitNetif.ObjectMeta
		smgrMirrorInterface.UpdateLabelMap(currIntf)
		smgrMirrorInterface.processLabelMirrorUpdate(currIntf)
	}
	ctkitNetif.ObjectMeta = nctkitNetif.ObjectMeta
	ctkitNetif.Spec = nctkitNetif.Spec

	err = sma.sm.UpdatePushObjectToMbus(ctkitNetif.MakeKey(string(apiclient.GroupNetwork)), currIntf, references(ctkitNetif))
	if err != nil {
		log.Errorf("error updating  push object %v", err)
		return err
	}

	return nil
}

// OnNetworkInterfaceDelete deletes an networkInterface
func (sma *SmNetworkInterface) OnNetworkInterfaceDelete(ctkitNetif *ctkit.NetworkInterface) error {
	// see if we already have it

	obj, err := sma.sm.FindObject("NetworkInterface", ctkitNetif.Tenant, "", ctkitNetif.Name)
	if err != nil {
		log.Errorf("Can not find the Network Interface %s|%s", ctkitNetif.Tenant, ctkitNetif.Name)
		return fmt.Errorf("NetworkInterface not found")
	}

	ifcfg, err := networkInterfaceStateFromObj(obj)
	if err != nil {
		log.Errorf("Can not find the Network Interface %s|%s (%s)", ctkitNetif.Tenant, ctkitNetif.Name, err)
		return err
	}

	ifcfg.markedForDelete = true
	smgrMirrorInterface.ClearLabelMap(ifcfg)
	smgrNetworkInterface.Lock()
	smgrMirrorInterface.processLabelMirrorUpdate(ifcfg)
	smgrNetworkInterface.Unlock()
	return sma.sm.DeletePushObjectToMbus(ctkitNetif.MakeKey(string(apiclient.GroupNetwork)),
		ifcfg, references(ctkitNetif))
}

// OnNetworkInterfaceReconnect is called when ctkit reconnects to apiserver
func (sma *SmNetworkInterface) OnNetworkInterfaceReconnect() {
	return
}

// GetNetworkInterfaceWatchOptions is a dummy handler used in init if no one registers the handler
func (sma *SmNetworkInterface) GetNetworkInterfaceWatchOptions() *api.ListWatchOptions {
	opts := &api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"ObjectMeta.Labels", "Spec", "Status"}
	return opts
}

// ListNetworkInterfaces lists all network interfaces
func (sm *Statemgr) ListNetworkInterfaces() ([]*NetworkInterfaceState, error) {
	objs := sm.ListObjects("NetworkInterface")

	var nwIntfs []*NetworkInterfaceState
	for _, obj := range objs {
		ns, err := NetworkInterfaceStateFromObj(obj)
		if err != nil {
			return nwIntfs, err
		}

		nwIntfs = append(nwIntfs, ns)
	}

	return nwIntfs, nil
}

func (sma *SmNetworkInterface) deleteDSCInterfaces(dsc *cluster.DistributedServiceCard) {

	// see if we already have it
	nwIntfs, err := sma.sm.ListNetworkInterfaces()
	if err != nil {
		log.Errorf("Can not list network interfaces")
	}

	log.Infof("Deleting interfaces as DSC %v is decommissioned/deleted", dsc.Name)
	for _, nwIntf := range nwIntfs {
		if nwIntf.NetworkInterfaceState.Status.DSC == dsc.Status.PrimaryMAC {
			nwIntf.NetworkInterfaceState.Lock()
			nwIntf.markedForDelete = true
			nwIntf.NetworkInterfaceState.Unlock()
			now := time.Now()
			for {
				err := sma.sm.ctrler.NetworkInterface().Delete(&nwIntf.NetworkInterfaceState.NetworkInterface)
				if err == nil {
					break
				}
				if time.Since(now) > time.Second*2 {
					log.Errorf("delete interface failed (%s)", err)
					return
				}
				time.Sleep(100 * time.Millisecond)
			}
		}
	}
}

//ProcessDSCCreate create
func (sma *SmNetworkInterface) ProcessDSCCreate(dsc *cluster.DistributedServiceCard) {

}

//ProcessDSCUpdate update
func (sma *SmNetworkInterface) ProcessDSCUpdate(dsc *cluster.DistributedServiceCard, ndsc *cluster.DistributedServiceCard) {

	//Process only if it is deleted or decomissioned
	if sma.sm.dscDecommissioned(ndsc) {
		sma.deleteDSCInterfaces(ndsc)
		return
	}

}

//ProcessDSCDelete delete
func (sma *SmNetworkInterface) ProcessDSCDelete(dsc *cluster.DistributedServiceCard) {
	sma.deleteDSCInterfaces(dsc)

}
