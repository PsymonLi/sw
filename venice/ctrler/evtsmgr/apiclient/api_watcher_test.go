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
	"github.com/pensando/sw/venice/ctrler/evtsmgr/memdb"
	"github.com/pensando/sw/venice/utils/kvstore"
	mockkvs "github.com/pensando/sw/venice/utils/kvstore/mock"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
)

type mockMonitoringV1 struct {
	mAlert            monitoring.MonitoringV1AlertInterface
	mAlertPolicy      monitoring.MonitoringV1AlertPolicyInterface
	mAlertDestination monitoring.MonitoringV1AlertDestinationInterface
	mEventPolicy      monitoring.MonitoringV1EventPolicyInterface
	mStatsAlertPolicy monitoring.MonitoringV1StatsAlertPolicyInterface
}

func (m *mockMonitoringV1) EventPolicy() monitoring.MonitoringV1EventPolicyInterface {
	return m.mEventPolicy
}
func (m *mockMonitoringV1) FwlogPolicy() monitoring.MonitoringV1FwlogPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) FlowExportPolicy() monitoring.MonitoringV1FlowExportPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) Alert() monitoring.MonitoringV1AlertInterface {
	return m.mAlert
}
func (m *mockMonitoringV1) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return m.mAlertPolicy
}
func (m *mockMonitoringV1) StatsAlertPolicy() monitoring.MonitoringV1StatsAlertPolicyInterface {
	return m.mStatsAlertPolicy
}
func (m *mockMonitoringV1) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return m.mAlertDestination
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

	evtsMgr := &ConfigWatcher{logger: log.SetConfig(log.GetDefaultConfig(t.Name())), memDb: memdb.NewMemDb()}
	opts := &api.ListWatchOptions{}

	mMonitoring := &mockMonitoringV1{}
	mAlert := mockapi.NewMockMonitoringV1AlertInterface(ctrl)
	mAlertPolicy := mockapi.NewMockMonitoringV1AlertPolicyInterface(ctrl)
	mAlertDestination := mockapi.NewMockMonitoringV1AlertDestinationInterface(ctrl)
	mEventPolicy := mockapi.NewMockMonitoringV1EventPolicyInterface(ctrl)
	mStatsAlertPolicy := mockapi.NewMockMonitoringV1StatsAlertPolicyInterface(ctrl)
	mMonitoring.mAlert = mAlert
	mMonitoring.mAlertPolicy = mAlertPolicy
	mMonitoring.mAlertDestination = mAlertDestination
	mMonitoring.mEventPolicy = mEventPolicy
	mMonitoring.mStatsAlertPolicy = mStatsAlertPolicy

	mapi := mockapi.NewMockServices(ctrl)
	evtsMgr.apiClient = mapi

	specFieldChangeSelector := &api.ListWatchOptions{FieldChangeSelector: []string{"Spec"}}

	// fail alert policy WATCH
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(nil, fmt.Errorf("alert policy watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err := evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil, "missed alert policy watch failure")

	// fail alert WATCH
	mWatcher := mockkvs.NewMockWatcher(ctrl)
	mWatcher.EXPECT().EventChan().Return(nil).Times(1)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(nil, fmt.Errorf("alert watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(2)
	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil, "missed alert watch failure")

	// fail alert destination WATCH
	mWatcher = mockkvs.NewMockWatcher(ctrl)
	mWatcher.EXPECT().EventChan().Return(nil).Times(2)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(nil, fmt.Errorf("alert destination watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(3)
	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil, "missed alert destination watch failure")

	// fail event policy WATCH
	mWatcher = mockkvs.NewMockWatcher(ctrl)
	mWatcher.EXPECT().EventChan().Return(nil).Times(3)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(nil, fmt.Errorf("event policy watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(4)
	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil, "missed event policy watch failure")

	// fail stats alert policy WATCH
	mWatcher = mockkvs.NewMockWatcher(ctrl)
	mWatcher.EXPECT().EventChan().Return(nil).Times(4)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(nil, fmt.Errorf("stats alert policy watch failed")).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil, "missed stats alert policy watch failure")

	// fail on invalid alert policy WATCH event
	mAPWatcher := mockkvs.NewMockWatcher(ctrl) // alert policy watch chan
	apCh := make(chan *kvstore.WatchEvent)
	defer close(apCh)
	mAPWatcher.EXPECT().EventChan().Return(apCh).Times(1)
	mWatcher = mockkvs.NewMockWatcher(ctrl) // shared watch chan between alert destination, alerts, event policy and stats alert policy
	mWatcher.EXPECT().EventChan().Return(nil).Times(4)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mAPWatcher, nil).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	go func() { // send some watch events for alert policy
		tick := time.NewTimer(1 * time.Second)
		for {
			select {
			case <-tick.C:
				apCh <- &kvstore.WatchEvent{Type: "INVALID EVENT"}
				return
			default:
				key := CreateAlphabetString(5)
				aObj := &monitoring.AlertPolicy{TypeMeta: api.TypeMeta{Kind: "AlertPolicy"},
					ObjectMeta: api.ObjectMeta{Name: fmt.Sprintf("test-%s", key), Tenant: fmt.Sprintf("test-ten%s", key)}}
				apCh <- &kvstore.WatchEvent{Type: kvstore.Created, Key: key, Object: aObj}
				apCh <- &kvstore.WatchEvent{Type: kvstore.Updated, Key: key, Object: aObj}
				apCh <- &kvstore.WatchEvent{Type: kvstore.Deleted, Key: key, Object: aObj}
			}
			time.Sleep(50 * time.Millisecond)
		}
	}()

	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid watch event type"), "missed invalid watch event") // this should fail on the final invalid event

	// fail on invalid alert WATCH event
	mWatcher = mockkvs.NewMockWatcher(ctrl) // shared watch chan between alert policy, alert destination, event policy and stats alert policy
	mWatcher.EXPECT().EventChan().Return(nil).Times(4)
	mAWatcher := mockkvs.NewMockWatcher(ctrl) // alert watch chan
	aCh := make(chan *kvstore.WatchEvent)
	defer close(aCh)
	mAWatcher.EXPECT().EventChan().Return(aCh).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mAWatcher, nil).Times(1)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	go func() { // send some watch events for alert
		tick := time.NewTimer(1 * time.Second)
		for {
			key := CreateAlphabetString(5)
			apObj := &monitoring.AlertPolicy{TypeMeta: api.TypeMeta{Kind: "Alert"},
				ObjectMeta: api.ObjectMeta{Name: fmt.Sprintf("test-%s", key), Tenant: fmt.Sprintf("test-ten%s", key)}}

			select {
			case <-tick.C:
				aCh <- &kvstore.WatchEvent{Type: "INVALID EVENT", Key: key, Object: apObj}
				return
			default:
				aCh <- &kvstore.WatchEvent{Type: kvstore.Created, Key: key, Object: apObj}
				aCh <- &kvstore.WatchEvent{Type: kvstore.Updated, Key: key, Object: apObj}
				aCh <- &kvstore.WatchEvent{Type: kvstore.Deleted, Key: key, Object: apObj}
			}
			time.Sleep(50 * time.Millisecond)
		}
	}()

	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid alert policy watch event"),
		"missed invalid alert policy watch event") // this should fail on the final invalid event

	// fail on invalid alert destination WATCH event
	mWatcher = mockkvs.NewMockWatcher(ctrl) // shared watch chan between alert, alert policy, event policy and stats alert policy
	mWatcher.EXPECT().EventChan().Return(nil).Times(4)
	mADWatcher := mockkvs.NewMockWatcher(ctrl) // alert watch chan
	adCh := make(chan *kvstore.WatchEvent)
	defer close(adCh)
	mADWatcher.EXPECT().EventChan().Return(adCh).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mADWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	go func() { // send some watch events for alert
		tick := time.NewTimer(1 * time.Second)
		for {
			key := CreateAlphabetString(5)
			adObj := &monitoring.AlertDestination{TypeMeta: api.TypeMeta{Kind: "AlertDestination"},
				ObjectMeta: api.ObjectMeta{Name: fmt.Sprintf("test-%s", key), Tenant: fmt.Sprintf("test-ten%s", key)}}

			select {
			case <-tick.C:
				adCh <- &kvstore.WatchEvent{Type: "INVALID EVENT", Key: key, Object: adObj}
				return
			default:
				adCh <- &kvstore.WatchEvent{Type: kvstore.Created, Key: key, Object: adObj}
				adCh <- &kvstore.WatchEvent{Type: kvstore.Updated, Key: key, Object: adObj}
				adCh <- &kvstore.WatchEvent{Type: kvstore.Deleted, Key: key, Object: adObj}
			}
			time.Sleep(50 * time.Millisecond)
		}
	}()

	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid alert destination watch event"),
		"missed invalid alert destination watch event") // this should fail on the final invalid event

	// fail on invalid event policy WATCH event
	mWatcher = mockkvs.NewMockWatcher(ctrl) // shared watch chan between alert, alert policy, alert destination and stats alert policy
	mWatcher.EXPECT().EventChan().Return(nil).Times(4)
	mEPWatcher := mockkvs.NewMockWatcher(ctrl) // alert watch chan
	epCh := make(chan *kvstore.WatchEvent)
	defer close(epCh)
	mEPWatcher.EXPECT().EventChan().Return(epCh).Times(1)
	mAlert.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mAlertDestination.EXPECT().Watch(ctx, opts).Return(mWatcher, nil).Times(1)
	mEventPolicy.EXPECT().Watch(ctx, opts).Return(mEPWatcher, nil).Times(1)
	mStatsAlertPolicy.EXPECT().Watch(ctx, specFieldChangeSelector).Return(mWatcher, nil).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	go func() { // send some watch events for alert
		tick := time.NewTimer(1 * time.Second)
		for {
			key := CreateAlphabetString(5)
			epObj := &monitoring.EventPolicy{TypeMeta: api.TypeMeta{Kind: "EventPolicy"},
				ObjectMeta: api.ObjectMeta{Name: fmt.Sprintf("test-%s", key), Tenant: fmt.Sprintf("test-ten%s", key)}}
			select {
			case <-tick.C:
				epCh <- &kvstore.WatchEvent{Type: "INVALID EVENT", Key: key, Object: epObj}
				return
			default:
				epCh <- &kvstore.WatchEvent{Type: kvstore.Created, Key: key, Object: epObj}
				epCh <- &kvstore.WatchEvent{Type: kvstore.Updated, Key: key, Object: epObj}
				epCh <- &kvstore.WatchEvent{Type: kvstore.Deleted, Key: key, Object: epObj}
			}
			time.Sleep(50 * time.Millisecond)
		}
	}()

	err = evtsMgr.processEvents(parentCtx)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid event policy watch event"),
		"missed event policy watch event") // this should fail on the final invalid event
}
