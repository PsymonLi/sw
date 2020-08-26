package memdb

import (
	"sync"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/runtime"
)

// event types
const (
	CreateEvent = memdb.CreateEvent
	UpdateEvent = memdb.UpdateEvent
	DeleteEvent = memdb.DeleteEvent
)

// MemDb in-memory database to store policy objects and alerts
type MemDb struct {
	sync.RWMutex
	*memdb.Memdb
}

// local cache alerts group
type alertsGroup struct {
	grpByEventURI                 map[string]string
	grpByEventMessageAndObjectRef map[string]string
}

// FilterFn represents various filters that can be applied before returning the list response
type FilterFn func(obj runtime.Object) bool

// WithTenantFilter returns a fn() which returns true if the object matches the given tenant
func WithTenantFilter(tenant string) FilterFn {
	return func(obj runtime.Object) bool {
		objm := obj.(runtime.ObjectMetaAccessor)
		return objm.GetObjectMeta().GetTenant() == tenant
	}
}

// WithNameSpaceFilter returns a fn() which returns true if the object matches the given tenant
func WithNameSpaceFilter(namespace string) FilterFn {
	return func(obj runtime.Object) bool {
		objm := obj.(runtime.ObjectMetaAccessor)
		return objm.GetObjectMeta().GetNamespace() == namespace
	}
}

// WithMetricKindFilter returns a fn() which returns true if the stats alert policy metrics kind the given kind
func WithMetricKindFilter(kind string) FilterFn {
	return func(obj runtime.Object) bool {
		switch obj.(type) {
		case *monitoring.StatsAlertPolicy:
			return obj.(*monitoring.StatsAlertPolicy).Spec.Metric.Kind == kind
		}
		return false
	}
}

// WithEnabledFilter returns a fn() which returns true if the alert policy object matches the given enabled flag
func WithEnabledFilter(enabled bool) FilterFn {
	return func(obj runtime.Object) bool {
		switch obj.(type) {
		case *monitoring.StatsAlertPolicy:
			return obj.(*monitoring.StatsAlertPolicy).Spec.Enable == enabled
		}
		return false
	}
}

// GetStatsAlertPolicies returns the list of alert policies that matches all the given filters
func (m *MemDb) GetStatsAlertPolicies(filters ...FilterFn) []*monitoring.StatsAlertPolicy {
	var statsAlertPolicies []*monitoring.StatsAlertPolicy
	for _, policy := range m.ListObjects("StatsAlertPolicy", nil) {
		ap := *(policy.(*monitoring.StatsAlertPolicy))

		matched := 0
		for _, filter := range filters { // run through filters
			if !filter(&ap) {
				break
			}
			matched++
		}

		if matched == len(filters) {
			var criteria monitoring.MeasurementCriteria
			criteria = *ap.Spec.GetMeasurementCriteria()
			ap.Spec.MeasurementCriteria = &criteria
			statsAlertPolicies = append(statsAlertPolicies, &ap)
		}
	}

	return statsAlertPolicies
}

// WatchStatsAlertPolicy returns the watcher to watch for stats alert policy events
func (m *MemDb) WatchStatsAlertPolicy() *memdb.Watcher {
	watcher := memdb.Watcher{Name: "stats-alert-policy"}
	watcher.Channel = make(chan memdb.Event, memdb.WatchLen)
	m.WatchObjects("StatsAlertPolicy", &watcher)
	return &watcher
}

// StopWatchStatsAlertPolicy stops the stats alert policy watcher
func (m *MemDb) StopWatchStatsAlertPolicy(watcher *memdb.Watcher) {
	m.StopWatchObjects("StatsAlertPolicy", watcher)
}

// NewMemDb creates a new mem DB
func NewMemDb() *MemDb {
	amemDb := &MemDb{
		Memdb: memdb.NewMemdb(),
	}

	return amemDb
}
