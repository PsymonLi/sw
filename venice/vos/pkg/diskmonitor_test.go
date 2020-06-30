package vospkg

import (
	"context"
	"os"
	"os/exec"
	"sync"
	"testing"
	"time"

	minioclient "github.com/minio/minio-go/v6"
	"google.golang.org/grpc/metadata"

	"github.com/pensando/sw/api"
	. "github.com/pensando/sw/venice/utils/testutils"
	vosinternalprotos "github.com/pensando/sw/venice/vos/protos"
)

type fakeDiskUpdateWatchServer struct {
	ctx context.Context
}

var testChannel = make(chan interface{}, 100)

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
	tempDir := "./default.fwlogs/data"
	os.MkdirAll(tempDir, os.ModePerm)
	exec.Command("/bin/sh", "-c", "cp * "+tempDir+"/.").Output()

	// Remove dummy dirs and files
	defer func() {
		os.RemoveAll(tempDir)
	}()

	// Test static threshold
	paths := new(sync.Map)
	c := DiskMonitorConfig{
		TenantName:               "default",
		CombinedThresholdPercent: 0.00001,
		CombinedBuckets:          []string{"fwlogs"},
	}
	paths.Store("", c)
	cancelFunc, err := inst.createDiskUpdateWatcher(paths, time.Second*2, []string{"./"})
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
	cancelFunc, err = inst.createDiskUpdateWatcher(paths, time.Second*2, []string{"./"})
	Assert(t, err == nil, "failed to create disk update watcher")
	time.Sleep(time.Second * 3)

	opts := api.ListWatchOptions{}
	opts.ObjectMeta = api.ObjectMeta{
		ResourceVersion: "10",
	}
	err = srv.WatchDiskThresholdUpdates(&opts, fw)
	Assert(t, err != nil, "filtering is not supported")

}
