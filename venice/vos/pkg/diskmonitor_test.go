package vospkg

import (
	"context"
	"os"
	"os/exec"
	"strings"
	"sync"
	"testing"
	"time"

	minioclient "github.com/minio/minio-go/v6"
	"google.golang.org/grpc/metadata"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
	vosinternalprotos "github.com/pensando/sw/venice/vos/protos"
)

var (
	logger             = log.GetNewLogger(log.GetDefaultConfig("vos-diskmonitor-test"))
	testChannel        = make(chan interface{}, 100)
	mockEventsRecorder = mockevtsrecorder.NewRecorder("vos-diskmonitor-test", logger)
	_                  = recorder.Override(mockEventsRecorder)
)

type fakeDiskUpdateWatchServer struct {
	ctx context.Context
}

// Send implements a mock interface
func (f *fakeDiskUpdateWatchServer) Send(event *vosinternalprotos.DiskUpdate) error {
	testChannel <- event
	return nil
}

// SetHeader implements a mock interface
func (f *fakeDiskUpdateWatchServer) SetHeader(metadata.MD) error { return nil }

// SendHeader implements a mock interface
func (f *fakeDiskUpdateWatchServer) SendHeader(metadata.MD) error { return nil }

// SetTrailer implements a mock interface
func (f *fakeDiskUpdateWatchServer) SetTrailer(metadata.MD) {}

// Context implements a mock interface
func (f *fakeDiskUpdateWatchServer) Context() context.Context {
	return f.ctx
}

// SendMsg implements a mock interface
func (f *fakeDiskUpdateWatchServer) SendMsg(m interface{}) error { return nil }

// RecvMsg implements a mock interface
func (f *fakeDiskUpdateWatchServer) RecvMsg(m interface{}) error { return nil }

func TestDiskUpdateOps(t *testing.T) {
	checkEventCountHelper := func() int {
		eventCount := 0
		for _, ev := range mockEventsRecorder.GetEvents() {
			if ev.EventType == eventtypes.FLOWLOGS_DISK_THRESHOLD_EXCEEDED.String() &&
				ev.Category == "system" &&
				ev.Severity == "critical" &&
				strings.Contains(ev.Message, "Flow logs disk usage threshold exceeded") &&
				strings.Contains(ev.Message, "current threshold 2.000000e-04") {
				eventCount++
			}
		}
		return eventCount
	}
	fb := &mockBackend{}
	inst := &instance{}
	inst.Init(fb)
	srv := grpcBackend{client: fb, instance: inst}
	ctx := context.Background()

	// Add a fake listobject function.
	// The objs array is empty because diskupdates are not objects.
	// This list function is needed to pass through the watchstream's
	// Dequeue function.
	objCh := make(chan minioclient.ObjectInfo)
	fb.listObjFn = func() <-chan minioclient.ObjectInfo {
		return objCh
	}
	objs := []minioclient.ObjectInfo{}
	go func() {
		for _, v := range objs {
			objCh <- v
		}
		close(objCh)
	}()

	cctx, cancel := context.WithCancel(ctx)
	defer cancel()

	fw := &fakeDiskUpdateWatchServer{ctx: cctx}
	go srv.WatchDiskThresholdUpdates(&api.ListWatchOptions{}, fw)

	// Create dummy dirs and files
	tempDir := "./default.fwlogs"
	os.MkdirAll(tempDir, os.ModePerm)
	exec.Command("/bin/sh", "-c", "cp * "+tempDir+"/.").Output()

	tempMetaDir := "./default.meta-fwlogs"
	os.MkdirAll(tempMetaDir, os.ModePerm)
	exec.Command("/bin/sh", "-c", "cp * "+tempMetaDir+"/.").Output()

	tempIndexDir := "./default.indexmeta"
	os.MkdirAll(tempIndexDir, os.ModePerm)
	exec.Command("/bin/sh", "-c", "cp * "+tempIndexDir+"/.").Output()

	tempRawlogsDir := "./default.rawlogs"
	os.MkdirAll(tempRawlogsDir, os.ModePerm)
	exec.Command("/bin/sh", "-c", "cp * "+tempRawlogsDir+"/.").Output()

	// Remove dummy dirs and files
	defer func() {
		os.RemoveAll(tempDir)
		os.RemoveAll(tempMetaDir)
		os.RemoveAll(tempIndexDir)
		os.RemoveAll(tempRawlogsDir)
	}()

	// Test static threshold
	paths := new(sync.Map)
	c := DiskMonitorConfig{
		TenantName:               "",
		CombinedThresholdPercent: 0.0002,
		CombinedBuckets:          []string{"fwlogs", "meta-fwlogs", "indexmeta", "rawlogs"},
	}
	paths.Store("", c)
	cancelFunc, err := inst.createDiskUpdateWatcher(paths,
		time.Second*2, []string{"./"}, time.Minute*4)
	Assert(t, err == nil, "failed to create disk update watcher")

	// Start monitor disks
	storeWatcher := inst.watcherMap[diskUpdateWatchPath]
	Assert(t, storeWatcher != nil, "store watcher is nil")

	event := <-testChannel
	Assert(t, event != nil, "received event is nil")
	diskUpdate, ok := event.(*vosinternalprotos.DiskUpdate)
	Assert(t, ok == true, "event is not a disk update")
	Assert(t, diskUpdate.Status.Path == "./", "diskupdate path is not correct", diskUpdate.Status.Path)
	Assert(t, diskUpdate.Status.Size_ != 0, "diskupdate size is 0")
	Assert(t, diskUpdate.Status.UsedByNamespace != 0, "diskupdate used is 0")
	Assert(t, ((float64(diskUpdate.Status.UsedByNamespace)/float64(diskUpdate.Status.Size_))*100) >= float64(0.0001),
		"incorrect threshold notification")

	// Verify event
	AssertEventually(t, func() (bool, interface{}) {
		eventCount := checkEventCountHelper()
		return eventCount == 1, nil
	}, "failed to find flow logs disk threshold exceeded event", "2s", "60s")

	// Verify event count
	// Sleep for 1 minute, the event count should still be the same
	time.Sleep(time.Minute * 2)
	eventCount := checkEventCountHelper()
	Assert(t, eventCount == 1, "only 1 event should have got raised")

	// Now wait for 1 more minite, after that event count should go up.
	time.Sleep(time.Minute * 2)
	eventCount = checkEventCountHelper()
	Assert(t, eventCount == 2, "2 events should have got raised", eventCount)

	// Now wait for 2 more minute, there should still be only 2 events
	time.Sleep(time.Minute * 2)
	eventCount = checkEventCountHelper()
	Assert(t, eventCount == 2, "2 events should have got raised", eventCount)

	cancelFunc()

	// Cover dynamic threshold.
	// We cant test events for dynamic threshold becuase its dependent on disk capacity.
	// Calling this function for getting code coverage
	paths = new(sync.Map)
	c = DiskMonitorConfig{
		TenantName:               "default",
		CombinedThresholdPercent: float64(-1),
		CombinedBuckets:          []string{"fwlogs"},
	}
	paths.Store("", c)
	cancelFunc, err = inst.createDiskUpdateWatcher(paths,
		time.Second*2, []string{"./"}, time.Minute*1)
	Assert(t, err == nil, "failed to create disk update watcher")
	time.Sleep(time.Second * 3)

	opts := api.ListWatchOptions{}
	opts.ObjectMeta = api.ObjectMeta{
		ResourceVersion: "10",
	}
	err = srv.WatchDiskThresholdUpdates(&opts, fw)
	Assert(t, err != nil, "filtering is not supported")

}
