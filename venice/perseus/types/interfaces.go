package types

import (
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/venice/utils/kvstore"
)

// SmartNICEventHandler handles watch events for SmartNIC object
type SmartNICEventHandler func(et kvstore.WatchEventType, nic *cluster.DistributedServiceCard)

// NetworkInterfaceEventHandler handles watch events for Network Interface object
type NetworkInterfaceEventHandler func(et kvstore.WatchEventType, nwintf *network.NetworkInterface)

// RoutingConfigEventHandler handles watch events for RoutingConfig objects
type RoutingConfigEventHandler func(et kvstore.WatchEventType, nwintf *network.RoutingConfig)

// NodeConfigEventHandler handles watch events for Node objects
type NodeConfigEventHandler func(et kvstore.WatchEventType, nwintf *cluster.Node)

// HealthReport reports health of a module
type HealthReport struct {
	Healthy bool
	Reason  string
}

// HealthReportHandler handles health reports from a module.
type HealthReportHandler func(in HealthReport)

// CfgWatcherService watches for changes to config from API Server
type CfgWatcherService interface {
	// Start the service
	Start()
	// Stop the service
	Stop()

	// SetSmartNICEventHandler sets the handler to handle events related to SmartNIC object
	SetSmartNICEventHandler(SmartNICEventHandler)

	// SetNetworkInterfaceEventHandler sets the handler to handle events related to network interface object
	SetNetworkInterfaceEventHandler(NetworkInterfaceEventHandler)

	// SetRoutingConfigEventHandler sets the handler to handle events related to RoutingConfig object
	SetRoutingConfigEventHandler(RoutingConfigEventHandler)

	// SetNodeConfigEventHandler sets the handler to handle events related to Node object
	SetNodeConfigEventHandler(NodeConfigEventHandler)

	// SetHealthReportHandler sets the handler to report health events
	SetHealthReportHandler(HealthReportHandler)

	// APIClient returns a valid interface once the APIServer is good and
	// accepting requests
	APIClient() cluster.ClusterV1Interface
}
