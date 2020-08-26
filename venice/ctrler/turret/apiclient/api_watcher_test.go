package apiclient

import (
	"context"
	"fmt"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	mockapi "github.com/pensando/sw/api/mock"
	"github.com/pensando/sw/venice/ctrler/turret/memdb"
	"github.com/pensando/sw/venice/utils/kvstore"
	mockkvs "github.com/pensando/sw/venice/utils/kvstore/mock"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
)

type mockMonitoringV1 struct {
	mStatsAlertPolicy monitoring.MonitoringV1StatsAlertPolicyInterface
}

func (m *mockMonitoringV1) EventPolicy() monitoring.MonitoringV1EventPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) FwlogPolicy() monitoring.MonitoringV1FwlogPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) FlowExportPolicy() monitoring.MonitoringV1FlowExportPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) Alert() monitoring.MonitoringV1AlertInterface {
	return nil
}
func (m *mockMonitoringV1) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) StatsAlertPolicy() monitoring.MonitoringV1StatsAlertPolicyInterface {
	return m.mStatsAlertPolicy
}
func (m *mockMonitoringV1) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return nil
}
func (m *mockMonitoringV1) MirrorSession() monitoring.MonitoringV1MirrorSessionInterface {
	return nil
}
func (m *mockMonitoringV1) Watch(ctx context.Context, options *api.AggWatchOptions) (kvstore.Watcher, error) {
	return nil, nil
}
func (m *mockMonitoringV1) TroubleshootingSession() monitoring.MonitoringV1TroubleshootingSessionInterface {
	return nil
}
func (m *mockMonitoringV1) TechSupportRequest() monitoring.MonitoringV1TechSupportRequestInterface {
	return nil
}

func (m *mockMonitoringV1) ArchiveRequest() monitoring.MonitoringV1ArchiveRequestInterface {
	return nil
}

func (m *mockMonitoringV1) AuditPolicy() monitoring.MonitoringV1AuditPolicyInterface {
	return nil
}

// TestAPIWatchEvents test API server watch on alert policy and alert objects
func TestAPIWatchEvents(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	parentCtx := context.Background()
	ctx, cancel := context.WithCancel(parentCtx)
	defer cancel()

	configWatcher := &ConfigWatcher{logger: log.SetConfig(log.GetDefaultConfig(t.Name())), memDb: memdb.NewMemDb()}

	mMonitoring := &mockMonitoringV1{}
	mStatsAlertPolicy := mockapi.NewMockMonitoringV1StatsAlertPolicyInterface(ctrl)
	mMonitoring.mStatsAlertPolicy = mStatsAlertPolicy

	mapi := mockapi.NewMockServices(ctrl)
	configWatcher.apiClient = mapi

	specFieldChangeSelector := &api.ListWatchOptions{FieldChangeSelector: []string{"Spec"}}

	// fail stats alert policy WATCH
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(nil, fmt.Errorf("stats alert policy watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err := configWatcher.processEvents(parentCtx)
	Assert(t, err != nil, "missed stats alert policy watch failure")

	// fail on invalid stats alert policy WATCH event
	statsAPCh := make(chan *kvstore.WatchEvent)
	defer close(statsAPCh)
	mStatsAPWatcher := mockkvs.NewMockWatcher(ctrl)

	mStatsAPWatcher.EXPECT().EventChan().Return(statsAPCh).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mStatsAPWatcher, nil).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	go func() { // send some watch events for stats alert policy
		tick := time.NewTimer(1 * time.Second)
		for {
			select {
			case <-tick.C:
				statsAPCh <- &kvstore.WatchEvent{Type: "INVALID EVENT"}
				return
			default:
				key := CreateAlphabetString(5)
				aObj := &monitoring.StatsAlertPolicy{TypeMeta: api.TypeMeta{Kind: "StatsAlertPolicy"},
					ObjectMeta: api.ObjectMeta{Name: fmt.Sprintf("test-%s", key), Tenant: fmt.Sprintf("test-ten%s", key)}}
				statsAPCh <- &kvstore.WatchEvent{Type: kvstore.Created, Key: key, Object: aObj}
				statsAPCh <- &kvstore.WatchEvent{Type: kvstore.Updated, Key: key, Object: aObj}
				statsAPCh <- &kvstore.WatchEvent{Type: kvstore.Deleted, Key: key, Object: aObj}
			}
			time.Sleep(50 * time.Millisecond)
		}
	}()

	err = configWatcher.processEvents(parentCtx)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid watch event type"), "missed invalid watch event") // this should fail on the final invalid event
}
