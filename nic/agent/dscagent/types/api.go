package types

import (
	"context"

	"github.com/pensando/sw/nic/agent/protos/dscagentproto"
	"github.com/pensando/sw/nic/agent/protos/tpmprotos"
	"github.com/pensando/sw/nic/agent/protos/tsproto"

	"github.com/pensando/sw/venice/utils/emstore"
	"github.com/pensando/sw/venice/utils/events"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/protos/netproto"
)

// ControllerAPI implements north bound APIs for DSCAgent
type ControllerAPI interface {
	HandleVeniceCoordinates(obj DistributedServiceCardStatus) error
	Start(kinds []string) error
	Stop() error
	WatchTechSupport()
	WatchEventPolicies() error
	WatchFwlogPolicies()
	GetAgentStatus(context.Context, *api.Empty) (*dscagentproto.DSCAgentStatus, error)
}

// InfraAPI implements all infra APIs that are common across pipelines
type InfraAPI interface {
	// List returns a list of objects by kind
	List(kind string) ([][]byte, error)

	// Read returns a single object of a given kind
	Read(kind, key string) ([]byte, error)

	// Store stores a single object of a given kind
	Store(kind, key string, data []byte) error

	// Delete deletes a single object
	Delete(kind, key string) error

	// AllocateID allocates id based off an offset
	AllocateID(kind emstore.ResourceType, offset int) uint64

	// GetDscName returns the DSC Name.
	GetDscName() string

	// StoreConfig stores NetAgent config
	StoreConfig(cfg DistributedServiceCardStatus)

	// GetConfig returns NetAgent config
	GetConfig() (cfg DistributedServiceCardStatus)

	// NotifyVeniceConnection updates venice connection status from netagent
	NotifyVeniceConnection(bool)

	// UpdateIfChannel updates the intf update channel
	UpdateIfChannel(evt UpdateIfEvent)

	// GetIntfUpdList returns the list of dirty interfaces
	GetIntfUpdList() []UpdateIfEvent

	// IfUpdateChannel returns the interface update channel
	IfUpdateChannel() chan error

	// Close closes the DB Write Handlers
	Close() error

	// InitDB reinits the DB (with optional purge)
	InitDB(purge bool) error
}

// PipelineAPI defines methods that all pipelines must implement.
// If the method is not applicable for a pipeline it must return nil and ErrNotImplemented
type PipelineAPI interface {
	// PipelineInit sets up the pipeline and creates default objects needed
	PipelineInit() error

	// HandleVeniceCoordinates initializes the pipeline when VeniceCoordinates are discovered
	HandleVeniceCoordinates(dsc DistributedServiceCardStatus)
	// RegisterControllerAPI allows access to ControllerAPI via the Pipeline.
	RegisterControllerAPI(controllerAPI ControllerAPI)

	// HandleVrf handles CRUDs for VRF object
	HandleVrf(oper Operation, obj netproto.Vrf) ([]netproto.Vrf, error)

	// HandleL2Segment handles CRUDs for L2Segment object
	HandleNetwork(oper Operation, obj netproto.Network) ([]netproto.Network, error)

	// HandleEndpoint handles CRUDs for Endpoint object
	HandleEndpoint(oper Operation, obj netproto.Endpoint) ([]netproto.Endpoint, error)

	// HandleInterface handles CRUDs for Interface object
	HandleInterface(oper Operation, obj netproto.Interface) ([]netproto.Interface, error)

	// HandleTunnel handles CRUDs for Tunnel object
	HandleTunnel(oper Operation, obj netproto.Tunnel) ([]netproto.Tunnel, error)

	// HandleApp handls CRUDs for App object
	HandleApp(oper Operation, obj netproto.App) ([]netproto.App, error)

	// HandleNetworkSecurityPolicy handles CRUDs for NetworkSecurityPolicy object
	HandleNetworkSecurityPolicy(oper Operation, obj netproto.NetworkSecurityPolicy) ([]netproto.NetworkSecurityPolicy, error)

	// ValidateSecurityProfile validates SecurityProfile object and returns the attachment vrf
	ValidateSecurityProfile(profile netproto.SecurityProfile) (vrf netproto.Vrf, err error)

	// HandleSecurityProfile handles CRUDs for SecurityProfile object
	HandleSecurityProfile(oper Operation, obj netproto.SecurityProfile) ([]netproto.SecurityProfile, error)

	// HandleNetworkSecurityPolicy handles CRUDs for Mirror object
	HandleMirrorSession(oper Operation, obj netproto.MirrorSession) ([]netproto.MirrorSession, error)

	// HandleNetworkSecurityPolicy handles CRUDs for Mirror object
	HandleInterfaceMirrorSession(oper Operation, obj netproto.InterfaceMirrorSession) ([]netproto.InterfaceMirrorSession, error)

	// HandleIPAMPolicy handles CRUDs for IPAM policy object
	HandleIPAMPolicy(oper Operation, obj netproto.IPAMPolicy) ([]netproto.IPAMPolicy, error)

	// HandleTraffiPolicerPolicy handles CRUDs for PolicerProfile object
	HandlePolicerProfile(oper Operation, obj netproto.PolicerProfile) ([]netproto.PolicerProfile, error)

	// HandleDSCConfig handles CRUDs for PolicerProfile object
	HandleDSCConfig(oper Operation, obj netproto.DSCConfig) ([]netproto.DSCConfig, error)

	//HandleFlowExportPolicy handles CRUDs for NetworkSecurityPolicy object
	HandleFlowExportPolicy(oper Operation, obj netproto.FlowExportPolicy) ([]netproto.FlowExportPolicy, error)

	// HandleRoutingConfig handles CRUDs for RoutingConfig object
	HandleRoutingConfig(oper Operation, obj netproto.RoutingConfig) ([]netproto.RoutingConfig, error)

	// HandleRouteTable handles CRUDs for RouteTable object
	HandleRouteTable(oper Operation, routetableObj netproto.RouteTable) ([]netproto.RouteTable, error)

	//HandleProfile handles CRUDs for Profile object
	HandleProfile(oper Operation, obj netproto.Profile) ([]netproto.Profile, error)

	// GetWatchOptions returns aggregate watch options on per kind. Currently this is used to get only local EPs
	GetWatchOptions(ctx context.Context, kind string) api.ListWatchOptions

	// HandleCPRoutingConfig handles creation of control plane route objects
	HandleCPRoutingConfig(obj DSCStaticRoute) error

	// HandleDSCInterfaceInfo handles configuring DSC interfaces served from the DB
	HandleDSCInterfaceInfo(obj DistributedServiceCardStatus)

	// HandleTechSupport captures tech support from DSC and uploads it to Venice
	HandleTechSupport(obj tsproto.TechSupportRequest) (techSupportArtifact string, err error)

	// HandleEvents relays events from pen-oper to various exporters
	HandleEvents(evtsDispatcher events.Dispatcher)

	HandleFwlogPolicyConfig(oper Operation, obj tpmprotos.FwlogPolicy) error

	// GetDSCAgentStatus returns the current agent status
	GetDSCAgentStatus(status *dscagentproto.DSCAgentStatus)

	// HandleVirtualRouterPeeringGroup handles CRUD Methods for VirtualRouterPeeringGroup object
	HandleVirtualRouterPeeringGroup(oper Operation, obj netproto.VirtualRouterPeeringGroup) ([]netproto.VirtualRouterPeeringGroup, error)
}
