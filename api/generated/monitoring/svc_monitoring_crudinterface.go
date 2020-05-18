// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

package monitoring

import (
	"context"

	api "github.com/pensando/sw/api"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/kvstore"
)

// Dummy vars to suppress unused imports message
var _ context.Context
var _ api.ObjectMeta
var _ kvstore.Interface

// MonitoringV1EventPolicyInterface exposes the CRUD methods for EventPolicy
type MonitoringV1EventPolicyInterface interface {
	Create(ctx context.Context, in *EventPolicy) (*EventPolicy, error)
	Update(ctx context.Context, in *EventPolicy) (*EventPolicy, error)
	UpdateStatus(ctx context.Context, in *EventPolicy) (*EventPolicy, error)
	Label(ctx context.Context, in *api.Label) (*EventPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*EventPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*EventPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*EventPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1FwlogPolicyInterface exposes the CRUD methods for FwlogPolicy
type MonitoringV1FwlogPolicyInterface interface {
	Create(ctx context.Context, in *FwlogPolicy) (*FwlogPolicy, error)
	Update(ctx context.Context, in *FwlogPolicy) (*FwlogPolicy, error)
	UpdateStatus(ctx context.Context, in *FwlogPolicy) (*FwlogPolicy, error)
	Label(ctx context.Context, in *api.Label) (*FwlogPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*FwlogPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*FwlogPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*FwlogPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1FlowExportPolicyInterface exposes the CRUD methods for FlowExportPolicy
type MonitoringV1FlowExportPolicyInterface interface {
	Create(ctx context.Context, in *FlowExportPolicy) (*FlowExportPolicy, error)
	Update(ctx context.Context, in *FlowExportPolicy) (*FlowExportPolicy, error)
	UpdateStatus(ctx context.Context, in *FlowExportPolicy) (*FlowExportPolicy, error)
	Label(ctx context.Context, in *api.Label) (*FlowExportPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*FlowExportPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*FlowExportPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*FlowExportPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1AlertInterface exposes the CRUD methods for Alert
type MonitoringV1AlertInterface interface {
	Create(ctx context.Context, in *Alert) (*Alert, error)
	Update(ctx context.Context, in *Alert) (*Alert, error)
	UpdateStatus(ctx context.Context, in *Alert) (*Alert, error)
	Label(ctx context.Context, in *api.Label) (*Alert, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*Alert, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*Alert, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*Alert, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1AlertPolicyInterface exposes the CRUD methods for AlertPolicy
type MonitoringV1AlertPolicyInterface interface {
	Create(ctx context.Context, in *AlertPolicy) (*AlertPolicy, error)
	Update(ctx context.Context, in *AlertPolicy) (*AlertPolicy, error)
	UpdateStatus(ctx context.Context, in *AlertPolicy) (*AlertPolicy, error)
	Label(ctx context.Context, in *api.Label) (*AlertPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*AlertPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*AlertPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*AlertPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1StatsAlertPolicyInterface exposes the CRUD methods for StatsAlertPolicy
type MonitoringV1StatsAlertPolicyInterface interface {
	Create(ctx context.Context, in *StatsAlertPolicy) (*StatsAlertPolicy, error)
	Update(ctx context.Context, in *StatsAlertPolicy) (*StatsAlertPolicy, error)
	UpdateStatus(ctx context.Context, in *StatsAlertPolicy) (*StatsAlertPolicy, error)
	Label(ctx context.Context, in *api.Label) (*StatsAlertPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*StatsAlertPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*StatsAlertPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*StatsAlertPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1AlertDestinationInterface exposes the CRUD methods for AlertDestination
type MonitoringV1AlertDestinationInterface interface {
	Create(ctx context.Context, in *AlertDestination) (*AlertDestination, error)
	Update(ctx context.Context, in *AlertDestination) (*AlertDestination, error)
	UpdateStatus(ctx context.Context, in *AlertDestination) (*AlertDestination, error)
	Label(ctx context.Context, in *api.Label) (*AlertDestination, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*AlertDestination, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*AlertDestination, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*AlertDestination, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1MirrorSessionInterface exposes the CRUD methods for MirrorSession
type MonitoringV1MirrorSessionInterface interface {
	Create(ctx context.Context, in *MirrorSession) (*MirrorSession, error)
	Update(ctx context.Context, in *MirrorSession) (*MirrorSession, error)
	UpdateStatus(ctx context.Context, in *MirrorSession) (*MirrorSession, error)
	Label(ctx context.Context, in *api.Label) (*MirrorSession, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*MirrorSession, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*MirrorSession, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*MirrorSession, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1TroubleshootingSessionInterface exposes the CRUD methods for TroubleshootingSession
type MonitoringV1TroubleshootingSessionInterface interface {
	Create(ctx context.Context, in *TroubleshootingSession) (*TroubleshootingSession, error)
	Update(ctx context.Context, in *TroubleshootingSession) (*TroubleshootingSession, error)
	UpdateStatus(ctx context.Context, in *TroubleshootingSession) (*TroubleshootingSession, error)
	Label(ctx context.Context, in *api.Label) (*TroubleshootingSession, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*TroubleshootingSession, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*TroubleshootingSession, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*TroubleshootingSession, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1TechSupportRequestInterface exposes the CRUD methods for TechSupportRequest
type MonitoringV1TechSupportRequestInterface interface {
	Create(ctx context.Context, in *TechSupportRequest) (*TechSupportRequest, error)
	Update(ctx context.Context, in *TechSupportRequest) (*TechSupportRequest, error)
	UpdateStatus(ctx context.Context, in *TechSupportRequest) (*TechSupportRequest, error)
	Label(ctx context.Context, in *api.Label) (*TechSupportRequest, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*TechSupportRequest, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*TechSupportRequest, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*TechSupportRequest, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1ArchiveRequestInterface exposes the CRUD methods for ArchiveRequest
type MonitoringV1ArchiveRequestInterface interface {
	Create(ctx context.Context, in *ArchiveRequest) (*ArchiveRequest, error)
	Update(ctx context.Context, in *ArchiveRequest) (*ArchiveRequest, error)
	UpdateStatus(ctx context.Context, in *ArchiveRequest) (*ArchiveRequest, error)
	Label(ctx context.Context, in *api.Label) (*ArchiveRequest, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*ArchiveRequest, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*ArchiveRequest, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*ArchiveRequest, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
	Cancel(ctx context.Context, in *CancelArchiveRequest) (*ArchiveRequest, error)
}

// MonitoringV1AuditPolicyInterface exposes the CRUD methods for AuditPolicy
type MonitoringV1AuditPolicyInterface interface {
	Create(ctx context.Context, in *AuditPolicy) (*AuditPolicy, error)
	Update(ctx context.Context, in *AuditPolicy) (*AuditPolicy, error)
	UpdateStatus(ctx context.Context, in *AuditPolicy) (*AuditPolicy, error)
	Label(ctx context.Context, in *api.Label) (*AuditPolicy, error)
	Get(ctx context.Context, objMeta *api.ObjectMeta) (*AuditPolicy, error)
	Delete(ctx context.Context, objMeta *api.ObjectMeta) (*AuditPolicy, error)
	List(ctx context.Context, options *api.ListWatchOptions) ([]*AuditPolicy, error)
	Watch(ctx context.Context, options *api.ListWatchOptions) (kvstore.Watcher, error)
	Allowed(oper apiintf.APIOperType) bool
}

// MonitoringV1Interface exposes objects with CRUD operations allowed by the service
type MonitoringV1Interface interface {
	EventPolicy() MonitoringV1EventPolicyInterface
	FwlogPolicy() MonitoringV1FwlogPolicyInterface
	FlowExportPolicy() MonitoringV1FlowExportPolicyInterface
	Alert() MonitoringV1AlertInterface
	AlertPolicy() MonitoringV1AlertPolicyInterface
	StatsAlertPolicy() MonitoringV1StatsAlertPolicyInterface
	AlertDestination() MonitoringV1AlertDestinationInterface
	MirrorSession() MonitoringV1MirrorSessionInterface
	TroubleshootingSession() MonitoringV1TroubleshootingSessionInterface
	TechSupportRequest() MonitoringV1TechSupportRequestInterface
	ArchiveRequest() MonitoringV1ArchiveRequestInterface
	AuditPolicy() MonitoringV1AuditPolicyInterface
	Watch(ctx context.Context, options *api.AggWatchOptions) (kvstore.Watcher, error)
}
