package vospkg

import (
	"context"
	"fmt"
	"io"
	"os"
	"runtime"
	"strings"
	"testing"

	"github.com/golang/mock/gomock"
	minioclient "github.com/minio/minio-go/v6"
	"github.com/pkg/errors"

	mock_credentials "github.com/pensando/sw/venice/utils/objstore/minio/mock"

	"github.com/pensando/sw/venice/vos"

	"github.com/pensando/sw/api/generated/objstore"
	. "github.com/pensando/sw/venice/utils/testutils"
)

type mockBackend struct {
	bucketExists, makeBucket, removeBucket, putBucket int
	putObject, removeObject, listObject, statObject   int
	fwlogObjects                                      map[string]*fakeStoreObj
	retErr                                            error
	bExists                                           bool
	putSize                                           int64
	putErr                                            error
	objInfo                                           minioclient.ObjectInfo
	listObjFn                                         func() <-chan minioclient.ObjectInfo
	listFwLogObjFn                                    func(prefix string) <-chan minioclient.ObjectInfo
	listenObjFn                                       func() <-chan minioclient.NotificationInfo
	makeBucketFn                                      func(bucketName string, location string) error
	delBucketFn                                       func(bucketname string) error
	delObjFn                                          func(b, n string) error
	statFunc                                          func(bucketName, objectName string, opts minioclient.StatObjectOptions) (minioclient.ObjectInfo, error)
	bExistsFunc                                       func(in string) (bool, error)
	setLifecycleFunc                                  func(bucketName string, lc string) error
	fObj                                              *fakeStoreObj
}

func (f *mockBackend) BucketExists(bucketName string) (bool, error) {
	f.bucketExists++
	if f.bExistsFunc != nil {
		return f.bExistsFunc(bucketName)
	}
	return f.bExists, f.retErr
}
func (f *mockBackend) MakeBucket(bucketName string, location string) (err error) {
	f.makeBucket++
	if f.makeBucketFn != nil {
		return f.makeBucketFn(bucketName, location)
	}
	return f.retErr
}
func (f *mockBackend) RemoveBucket(bucketName string) error {
	f.removeBucket++
	if f.delBucketFn != nil {
		return f.delBucketFn(bucketName)
	}
	return f.retErr
}

func (f *mockBackend) PutObject(bucketName, objectName string, reader io.Reader, objectSize int64, opts minioclient.PutObjectOptions) (n int64, err error) {
	if strings.Contains(bucketName, fwlogsBucketName) {
		if f.fwlogObjects == nil {
			f.fwlogObjects = map[string]*fakeStoreObj{}
		}
		f.fwlogObjects[objectName] = &fakeStoreObj{objReader: reader}
	}
	f.putObject++
	return f.putSize, f.putErr
}
func (f *mockBackend) RemoveObject(bucketName, objectName string) error {
	f.removeObject++
	return f.retErr
}
func (f *mockBackend) ListObjectsV2(bucketName, objectPrefix string, recursive bool, doneCh <-chan struct{}) <-chan minioclient.ObjectInfo {
	f.listObject++
	if strings.Contains(bucketName, fwlogsBucketName) {
		return f.listFwLogObjFn(objectPrefix)
	}
	return f.listObjFn()
}
func (f *mockBackend) StatObject(bucketName, objectName string, opts minioclient.StatObjectOptions) (minioclient.ObjectInfo, error) {
	f.statObject++
	if f.statFunc != nil {
		return f.statFunc(bucketName, objectName, opts)
	}
	return f.objInfo, f.retErr
}
func (f *mockBackend) GetObjectWithContext(ctx context.Context, bucketName, objectName string, opts minioclient.GetObjectOptions) (*minioclient.Object, error) {
	return nil, nil
}
func (f *mockBackend) ListenBucketNotification(bucketName, prefix, suffix string, events []string, doneCh <-chan struct{}) <-chan minioclient.NotificationInfo {
	if f.listenObjFn != nil {
		return f.listenObjFn()
	}
	return nil
}

func (f *mockBackend) GetStoreObject(ctx context.Context, bucketName, objectName string, opts minioclient.GetObjectOptions) (vos.StoreObject, error) {
	if strings.Contains(bucketName, fwlogsBucketName) {
		obj, ok := f.fwlogObjects[objectName]
		if ok {
			return obj, nil
		}
		return nil, fmt.Errorf("object not found")
	}
	return f.fObj, f.retErr
}

func (f *mockBackend) SetBucketLifecycleWithContext(ctx context.Context, bucketName, lifecycle string) error {
	if f.setLifecycleFunc != nil {
		return f.setLifecycleFunc(bucketName, lifecycle)
	}
	return nil
}

type fakeStoreObj struct {
	closeErr, statErr error
	statObjInfo       minioclient.ObjectInfo
	objReader         io.Reader
	readFn            func([]byte) (int, error)
}

// Close is a mock implementation
func (f *fakeStoreObj) Close() (err error) {
	return f.closeErr
}

// Stat is a mock implementation
func (f *fakeStoreObj) Stat() (minioclient.ObjectInfo, error) {
	return f.statObjInfo, f.statErr

}

// Read is a mock implementation
func (f *fakeStoreObj) Read(b []byte) (n int, err error) {
	if f.readFn != nil {
		return f.readFn(b)
	}

	if f.objReader != nil {
		return f.objReader.Read(b)
	}

	return 0, nil
}

func TestCreateBuckets(t *testing.T) {
	fb := &mockBackend{}
	bmap := make(map[string]int)
	var makeErr error
	fb.makeBucketFn = func(name, loc string) error {
		if v, ok := bmap[name]; ok {
			bmap[name] = v + 1
		} else {
			bmap[name] = 1
		}
		return makeErr
	}
	maxCreateBucketRetries = 1
	inst := &instance{}
	inst.Init(fb)
	err := inst.createDefaultBuckets(fb)
	AssertOk(t, err, "create buckets failed")
	// +1 for accounting for meta buckets created for flowlogs
	Assert(t, fb.bucketExists == len(objstore.Buckets_name)+1, "did not find correct number of calls [%v][%v]", fb.bucketExists, len(objstore.Buckets_name))
	Assert(t, fb.makeBucket == len(objstore.Buckets_name)+1, "did not find correct number of calls [%v][%v]", fb.makeBucket, len(objstore.Buckets_name))
	for k := range objstore.Buckets_value {
		tenantName := "default"
		if v1, ok := bmap[tenantName+"."+k]; !ok {
			t.Errorf("did not find key [%v]", k)
		} else {
			if v1 != 1 {
				t.Errorf("duplicate calls to create buckets [%d]", v1)
			}
		}
	}

	fb.bExists = true
	fb.makeBucket = 0
	err = inst.createDefaultBuckets(fb)
	AssertOk(t, err, "create buckets should have failed")
	Assert(t, fb.makeBucket == 0, "no calls to MakeBucket expected")

	fb.bExists = false
	makeErr = errors.New("someError")
	err = inst.createDefaultBuckets(fb)
	Assert(t, err != nil, "create buckets should have failed")

	err = inst.createDefaultBuckets(fb)
	Assert(t, err != nil, "create buckets should have failed")
	inst.Close()
}

// TestOptions tests the Vos's option functions
func TestOptions(t *testing.T) {
	inst := &instance{}
	f := WithBootupArgs([]string{"dummyarg"})
	Assert(t, f != nil, "bootup option function is nil")
	f(inst)
	Assert(t, inst.bootupArgs[0] == "dummyarg", "bootup arg is missing")

	f = WithBucketDiskThresholds(GetBucketDiskThresholds())
	Assert(t, f != nil, "bucket diskthreshold option function is nil")
	f(inst)
	Assert(t, len(inst.bucketDiskThresholds) == 2, "fwlogs bucket diskthreshold is missing")

	dth := GetBucketDiskThresholds()
	Assert(t, len(dth) == 2, "incorrect disk threshold arguments, expected /disk1/default.fwlogs, /disk2/default.fwlogs")
	th, ok := dth["/disk1/default.fwlogs"]
	Assert(t, th == 50.00, "incorrect threshold")
	Assert(t, ok, "fwlogs bucket threshold missing")
	th, ok = dth["/disk2/default.fwlogs"]
	Assert(t, th == 50.00, "incorrect threshold")
	Assert(t, ok, "fwlogs bucket threshold missing")
}

func TestGoMaxProcsSetting(t *testing.T) {
	// test GomaxProcsSetting
	os.Setenv("CPU_DIVISION_FACTOR", "2")
	defer os.Unsetenv("CPU_DIVISION_FACTOR")
	updateGoMaxProcs()
	Assert(t, runtime.NumCPU()/2 == runtime.GOMAXPROCS(-1),
		"GOMAXPROCS setting not updated correclty, current value:", runtime.GOMAXPROCS(-1))
}

func TestAPIThrottlingSetting(t *testing.T) {
	os.Setenv("MINIO_API_REQUESTS_MAX", "2")
	updateAPIThrottlingParams()
	Assert(t, os.Getenv("MINIO_API_REQUESTS_MAX") == "2",
		"MINIO_API_REQUESTS_MAX setting not updated correclty, current value:", os.Getenv("MINIO_API_REQUESTS_MAX"))
	os.Unsetenv("MINIO_API_REQUESTS_MAX")

	// Since the env variable is unset now, the MINIO_API_REQUESTS_MAX should get set to default value
	updateAPIThrottlingParams()
	Assert(t, os.Getenv("MINIO_API_REQUESTS_MAX") == "60",
		"MINIO_API_REQUESTS_MAX setting not updated correclty, current value:", os.Getenv("MINIO_API_REQUESTS_MAX"))
}

func TestSetBucketLifecycle(t *testing.T) {
	fb := &mockBackend{}
	bmap := make(map[string]int)
	lcmap := make(map[string]string) // key = bucketName, value == lifecycle
	var makeErr error
	fb.makeBucketFn = func(name, loc string) error {
		if v, ok := bmap[name]; ok {
			bmap[name] = v + 1
		} else {
			bmap[name] = 1
		}
		return makeErr
	}
	fb.setLifecycleFunc = func(bucketName, lc string) error {
		if lc != "" {
			lcmap[bucketName] = lc
		}
		return nil
	}
	maxCreateBucketRetries = 1
	inst := &instance{}
	inst.Init(fb)
	err := inst.createDefaultBuckets(fb)
	AssertOk(t, err, "create buckets failed")
	Assert(t, len(lcmap) == 2, "lifecycle not set for fwlogs bucket", len(lcmap))
}

func TestNew(t *testing.T) {

	// negative case - vos constructor fails when unexpected type is sent over cert manager channel
	testCertMgrChannel := make(chan interface{}, 1)
	testCertMgrChannel <- errors.New("unexpected data")
	_, err := New(context.Background(), false, "testURL", testCertMgrChannel)
	AssertError(t, err, "VOS constructor is expected to fail")

	// negative case - vos constructor fails when credentials creation fails
	c := gomock.NewController(t)
	defer c.Finish()
	mockCredentialManager := mock_credentials.NewMockCredentialsManager(c)
	mockCredentialManager.EXPECT().GetCredentials().Return(nil, errors.New("failed to get credentials")).AnyTimes()
	testCertMgrChannel = make(chan interface{}, 1)
	testCertMgrChannel <- mockCredentialManager
	_, err = New(context.Background(), false, "testURL", testCertMgrChannel)
	AssertError(t, err, "VOS constructor is expected to fail")
}
