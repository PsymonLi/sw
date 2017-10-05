// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/ctrler/npm/rpcserver/netproto"
	"github.com/pensando/sw/venice/utils/emstore"
	"sync"
)

// IntfInfo has the interface names to be plumbed into container
type IntfInfo struct {
	ContainerIntfName string //  Name of container side of the interface
	SwitchIntfName    string // Name of switch side of the interface
}

// NetAgent is the network agent instance
type NetAgent struct {
	sync.Mutex                                          // global lock for the agent
	store            emstore.Emstore                    // embedded db
	nodeUUID         string                             // Node's UUID
	datapath         NetDatapathAPI                     // network datapath
	ctrlerif         CtrlerAPI                          // controller object
	networkDB        map[string]*netproto.Network       // Network object db
	endpointDB       map[string]*netproto.Endpoint      // Endpoint object db
	secgroupDB       map[string]*netproto.SecurityGroup // security group object db
	currentNetworkID uint32                             // poor man's id allocation FIXME:
	currentSgID      uint32                             // poor man's id allocation FIXME:
}

// CtrlerAPI is the API provided by controller modules to netagent
type CtrlerAPI interface {
	EndpointCreateReq(epinfo *netproto.Endpoint) (*netproto.Endpoint, error) // sends an endpoint create request
	EndpointAgeoutNotif(epinfo *netproto.Endpoint) error                     // sends an endpoint ageout notification
	EndpointDeleteReq(epinfo *netproto.Endpoint) (*netproto.Endpoint, error) // sends an endpoint delete request
}

// CtrlerIntf is the API provided by netagent for the controllers
type CtrlerIntf interface {
	RegisterCtrlerIf(ctrlerif CtrlerAPI) error
	GetAgentID() string
	CreateNetwork(nt *netproto.Network) error                   // create a network
	UpdateNetwork(nt *netproto.Network) error                   // update a network
	DeleteNetwork(nt *netproto.Network) error                   // delete a network
	ListNetwork() []*netproto.Network                           // lists all networks
	FindNetwork(meta api.ObjectMeta) (*netproto.Network, error) // finds a network
	CreateEndpoint(ep *netproto.Endpoint) (*IntfInfo, error)    // create an endpoint
	UpdateEndpoint(ep *netproto.Endpoint) error                 // update an endpoint
	DeleteEndpoint(ep *netproto.Endpoint) error                 // delete an endpoint
	ListEndpoint() []*netproto.Endpoint                         // list all endpoints
	CreateSecurityGroup(nt *netproto.SecurityGroup) error       // create a sg
	UpdateSecurityGroup(nt *netproto.SecurityGroup) error       // update a sg
	DeleteSecurityGroup(nt *netproto.SecurityGroup) error       // delete a sg
	ListSecurityGroup() []*netproto.SecurityGroup               // list all sgs
}

// PluginIntf is the API provided by the netagent to plugins
type PluginIntf interface {
	EndpointCreateReq(epinfo *netproto.Endpoint) (*netproto.Endpoint, *IntfInfo, error) // Creates an endpoint
	EndpointDeleteReq(epinfo *netproto.Endpoint) error                                  // deletes an endpoint
}

// NetDatapathAPI is the API provided by datapath modules
type NetDatapathAPI interface {
	SetAgent(ag DatapathIntf) error
	CreateLocalEndpoint(ep *netproto.Endpoint, nt *netproto.Network, sgs []*netproto.SecurityGroup) (*IntfInfo, error) // Creates a local endpoint in datapath
	UpdateLocalEndpoint(ep *netproto.Endpoint, nt *netproto.Network, sgs []*netproto.SecurityGroup) error              // Updates a local endpoint in datapath
	DeleteLocalEndpoint(ep *netproto.Endpoint) error                                                                   // deletes a local endpoint in datapath
	CreateRemoteEndpoint(ep *netproto.Endpoint, nt *netproto.Network, sgs []*netproto.SecurityGroup) error             // Creates a remote endpoint in datapath
	UpdateRemoteEndpoint(ep *netproto.Endpoint, nt *netproto.Network, sgs []*netproto.SecurityGroup) error             // Updates a remote endpoint in datapath
	DeleteRemoteEndpoint(ep *netproto.Endpoint) error                                                                  // deletes a remote endpoint in datapath
	CreateNetwork(nw *netproto.Network) error                                                                          // creates a network
	UpdateNetwork(nw *netproto.Network) error                                                                          // updates a network in datapath
	DeleteNetwork(nw *netproto.Network) error                                                                          // deletes a network from datapath
	CreateSecurityGroup(sg *netproto.SecurityGroup) error                                                              // creates a security group
	UpdateSecurityGroup(sg *netproto.SecurityGroup) error                                                              // updates a security group
	DeleteSecurityGroup(sg *netproto.SecurityGroup) error                                                              // deletes a security group
}

// DatapathIntf is the API provided by the netagent to datapaths
type DatapathIntf interface {
	EndpointCreateReq(epinfo *netproto.Endpoint) (*netproto.Endpoint, *IntfInfo, error) // Creates an endpoint
	EndpointDeleteReq(epinfo *netproto.Endpoint) error                                  // deletes an endpoint
}
