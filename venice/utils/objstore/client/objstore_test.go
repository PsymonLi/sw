package objstore

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"os"
	"strings"
	"testing"
	"testing/iotest"

	"github.com/golang/mock/gomock"
	"github.com/gorilla/mux"

	minio "github.com/pensando/sw/venice/utils/objstore/minio"
	mock_credentials "github.com/pensando/sw/venice/utils/objstore/minio/mock"

	vlog "github.com/pensando/sw/venice/utils/log"

	"github.com/pensando/sw/api"
	types "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	mockmc "github.com/pensando/sw/venice/utils/objstore/client/mock"
	minioclient "github.com/pensando/sw/venice/utils/objstore/minio/client"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	tu "github.com/pensando/sw/venice/utils/testutils"
)

var utLog = vlog.WithContext("pkg", "objstore-test")

func listBuckets(w http.ResponseWriter, r *http.Request) {
	resp := `{<?xml version="1.0" encoding="UTF-8"?>
<ListAllMyBucketsResult xmlns="http://s3.amazonaws.com/doc/2006-03-01/"><Owner><ID>02d6176db174dc93cb1b899f7c6078f08654445fe8cf1b6ce98d8855f66bdbf4</ID><DisplayName></DisplayName></Owner><Buckets><Bucket><Name>tenant1-pktcap</Name><CreationDate>2018-06-05T00:44:14.880Z</CreationDate></Bucket><Bucket><Name>tenant1.pktcap</Name><CreationDate>2018-06-05T07:05:22.601Z</CreationDate></Bucket></Buckets></ListAllMyBucketsResult>} `
	w.WriteHeader(http.StatusOK)
	w.Write([]byte(resp))
	w.Header().Set("Content-Type", "application/xml")
}

func minioServer(l net.Listener) {
	router := mux.NewRouter()
	router.HandleFunc(fmt.Sprintf("/"), listBuckets).Methods("GET")
	utLog.Infof("minio started @%+v", l.Addr().(*net.TCPAddr).String())
	go http.Serve(l, router)
}

func TestNewClient(t *testing.T) {
	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	c := gomock.NewController(t)
	defer c.Finish()

	r := mockresolver.New()
	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	_, err = NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.Assert(t, err != nil, "failed test client error ")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.VosMinio,
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add objstore sercvice")

	_, err = NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed newclient ")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.VosMinio,
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1001",
	})
	tu.AssertOk(t, err, "failed to add 127.0.0.1:1001 service")

	_, err = NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.Assert(t, err != nil, "failed test invalid client address")

	// test two service instance
	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server1",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1001",
	})
	tu.AssertOk(t, err, "failed to add server1 service")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")
	_, err = NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed test multiple objstore address")

	// negative case - Client initialization fails when credentials manager fails to get minio credentials
	testErrMsg := "test error message"
	mockCredMgrWithError := mock_credentials.NewMockCredentialsManager(c)
	mockCredMgrWithError.EXPECT().GetCredentials().Return(nil, errors.New(testErrMsg))
	_, err = NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredMgrWithError))
	tu.AssertError(t, err, "new client initialization expected to fail when credential manager fails")
	tu.Assert(t, strings.Contains(err.Error(), testErrMsg), "new client initialization failed with unexpected error")

	// negative case - Client initialization fails when unable to connect to api-server
	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},

		ObjectMeta: api.ObjectMeta{
			Name: globals.APIServer,
		},
		Service: globals.APIServer,
		URL:     "localhost:1234",
	})
	tu.AssertOk(t, err, "failed to add service "+globals.APIServer)
	_, err = NewClient("ten1", "svc1", r, retryOpt)
	tu.AssertError(t, err, "new client initialization expected to fail when api-server can not be resolved")
	tu.Assert(t, strings.Contains(err.Error(), "Failed to initialize API client"), "new client initialization failed with unexpected error")
}

func setupMockCredsManager(c *gomock.Controller) *mock_credentials.MockCredentialsManager {
	mockCredsManager := mock_credentials.NewMockCredentialsManager(c)
	mockCredsManager.EXPECT().GetCredentials().Return(&minio.Credentials{
		AccessKey: "testAccessKey",
		SecretKey: "testSecretKey",
	}, nil).AnyTimes()
	return mockCredsManager
}

func TestGetObjStoreAddr(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	r := mockresolver.New()
	client := &client{
		resolverClient: r,
	}

	addr, err := client.getObjStoreAddr()
	tu.Assert(t, err != nil, "failed to test invalid objstore service")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.VosMinio,
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1001",
	})
	tu.AssertOk(t, err, "failed to add objstore service")

	addr, err = client.getObjStoreAddr()
	tu.AssertOk(t, err, "failed to get objstore address")
	tu.Assert(t, len(addr) == 1, fmt.Sprintf("failed to resolve url, got %+v", addr))
}

func TestPutObject(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)
	b := &bytes.Buffer{}

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	// success case
	mc.EXPECT().PutObject(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = mockClient.PutObject(context.Background(), "obj1", b, nil)
	tu.AssertOk(t, err, "putobj failed")

	mc.EXPECT().PutObjectOfSize(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = mockClient.PutObjectOfSize(context.Background(), "obj1", b, 0, nil)
	tu.AssertOk(t, err, "putobj failed")

	mc.EXPECT().PutObjectExplicit(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = mockClient.PutObjectExplicit(context.Background(), "testService", "obj1", b, 0, nil, "")
	tu.AssertOk(t, err, "putobj failed")

	// failure
	mc.EXPECT().PutObject(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = mockClient.PutObject(context.Background(), "obj1", b, nil)
	tu.Assert(t, err != nil, "putobj succeeded ")

	mc.EXPECT().PutObjectOfSize(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = mockClient.PutObjectOfSize(context.Background(), "obj1", b, 0, nil)
	tu.Assert(t, err != nil, "putobj succeeded ")

	mc.EXPECT().PutObjectExplicit(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = mockClient.PutObjectExplicit(context.Background(), "testService", "obj1", b, 0, nil, "")
	tu.Assert(t, err != nil, "putobj succeeded ")

	// connect error
	mc.EXPECT().PutObject(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.PutObject(context.Background(), "obj1", b, nil)
	tu.Assert(t, err != nil, "putobj succeeded")

	mc.EXPECT().PutObjectOfSize(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.PutObjectOfSize(context.Background(), "obj1", b, 0, nil)
	tu.Assert(t, err != nil, "putobj succeeded")

	mc.EXPECT().PutObjectExplicit(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.PutObjectExplicit(context.Background(), "testService", "obj1", b, 0, nil, "")
	tu.Assert(t, err != nil, "putobj succeeded")

	mc.EXPECT().FPutObjectExplicit(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.FPutObjectExplicit(context.Background(), "testService", "obj1", "filepath", nil, "")
	tu.Assert(t, err != nil, "fputobj succeeded")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().PutObject(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.PutObject(context.Background(), "obj1", b, nil)
	tu.Assert(t, err != nil, "putobj succeeded")
}

func TestGetObject(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	// success case
	mc.EXPECT().GetObject(gomock.Any(), gomock.Any()).Return(nil, nil).Times(1)
	_, err = mockClient.GetObject(context.Background(), "obj1")
	tu.AssertOk(t, err, "getobj failed")

	// failure
	mc.EXPECT().GetObject(gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("failed")).Times(1)
	_, err = mockClient.GetObject(context.Background(), "obj1")
	tu.Assert(t, err != nil, "getobj didn't fail on error")

	// connect error
	mc.EXPECT().GetObject(gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.GetObject(context.Background(), "obj1")
	tu.Assert(t, err != nil, "getobj didn't fail on connect error")

	mc.EXPECT().GetObjectExplicit(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.GetObjectExplicit(context.Background(), "svc1", "obj1")
	tu.Assert(t, err != nil, "getobj didn't fail on connect error")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().GetObject(gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.GetObject(context.Background(), "obj1")
	tu.Assert(t, err != nil, "getobj didn't fail on connect error")
}

func TestStatObject(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	objStats := &minioclient.ObjectStats{
		Size: 100,
	}

	// success case
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(gomock.Any()).Return(objStats, nil).Times(1)
	s, err := mockClient.StatObject("obj1")
	tu.AssertOk(t, err, "statobj failed")
	tu.Assert(t, s.Size == objStats.Size, fmt.Sprintf("object size didn't match"))

	// failure
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(gomock.Any()).Return(objStats, fmt.Errorf("failed")).Times(1)
	_, err = mockClient.StatObject("obj1")
	tu.Assert(t, err != nil, "statobj didn't fail on error")

	// connect error
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(gomock.Any()).Return(objStats, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.StatObject("obj1")
	tu.Assert(t, err != nil, "statobj didn't fail on connect error")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(gomock.Any()).Return(objStats, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.StatObject("obj1")
	tu.Assert(t, err != nil, "statobj didn't fail on connect error")
}

func TestListObjectsAndBuckets(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	objlist := []string{"obj1", "obj2"}

	// success case
	mc.EXPECT().ListObjects(gomock.Any()).Return(objlist, nil).Times(1)
	s, err := mockClient.ListObjects("obj1")
	tu.AssertOk(t, err, "listobj failed")
	tu.Assert(t, len(objlist) == len(s), fmt.Sprintf("list objects didn't match"))

	mc.EXPECT().ListObjectsExplicit(gomock.Any(), gomock.Any(), gomock.Any()).Return(objlist, nil).Times(1)
	s, err = mockClient.ListObjectsExplicit("svc1", "obj1", true)
	tu.AssertOk(t, err, "listobjsexplicit failed")
	tu.Assert(t, len(objlist) == len(s), fmt.Sprintf("listobjsexplicit objects didn't match"))

	// failure
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, fmt.Errorf("failed")).Times(1)
	_, err = mockClient.ListObjects("obj1")
	tu.Assert(t, err != nil, "listobj didn't fail on error")

	// connect error
	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.ListObjects("obj1")
	tu.Assert(t, err != nil, "listobj didn't fail on connect error")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().ListObjects(gomock.Any()).Return(objlist, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.ListObjects("obj1")
	tu.Assert(t, err != nil, "listobj didn't fail on connect error")

	mockClient.client = mc
	mc.EXPECT().ListBuckets(gomock.Any()).Return(objlist, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.ListBuckets(context.Background())
	tu.Assert(t, err != nil, "ListBuckets didn't fail on connect error")

	mc.EXPECT().ListBuckets(gomock.Any()).Return([]string{"bucket1"}, nil).Times(1)
	s, err = mockClient.ListBuckets(context.Background())
	tu.AssertOk(t, err, "ListBuckets failed")
	tu.Assert(t, len(s) == 1, fmt.Sprintf("ListBuckets result didn't match"))
}

func TestRemoveObjects(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	// success case
	mc.EXPECT().RemoveObjects(gomock.Any()).Return(nil).Times(1)
	err = mockClient.RemoveObjects("obj1")
	tu.AssertOk(t, err, "remobj failed")

	// failure
	mc.EXPECT().RemoveObjects(gomock.Any()).Return(fmt.Errorf("failed")).Times(1)
	err = mockClient.RemoveObjects("obj1")
	tu.Assert(t, err != nil, "remobj didn't fail on error")

	// connect error
	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().RemoveObjects(gomock.Any()).Return(fmt.Errorf("%s", connectErr)).Times(1)
	err = mockClient.RemoveObjects("obj1")
	tu.Assert(t, err != nil, "remobj didn't fail on connect error")

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1003",
	})

	mockClient.client = mc
	mc.EXPECT().RemoveObjects(gomock.Any()).Return(fmt.Errorf("%s", connectErr)).Times(1)
	err = mockClient.RemoveObjects("obj1")
	tu.Assert(t, err != nil, "remobj didn't fail on connect error")

	mockClient.client = mc
	mc.EXPECT().RemoveObject(gomock.Any()).Return(fmt.Errorf("%s", connectErr)).Times(1)
	err = mockClient.RemoveObject("obj1")
	tu.Assert(t, err != nil, "remobj didn't fail on connect error")

	mockClient.client = mc
	objectCh := make(chan string)
	errCh := make(<-chan interface{})
	mc.EXPECT().RemoveObjectsWithContext(gomock.Any(), gomock.Any(), gomock.Any()).Return(errCh).Times(1)
	removeObjectErrCh := mockClient.RemoveObjectsWithContext(context.Background(), "dummy", objectCh)
	tu.Assert(t, removeObjectErrCh != nil, "remobj didn't fail on connect error")
}

func TestSetServiceLifecycle(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:         mc,
		bucketName:     "ten1:svc1",
		resolverClient: r,
	}

	// success case
	mc.EXPECT().SetServiceLifecycleWithContext(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(1)
	err = mockClient.SetServiceLifecycleWithContext(context.Background(), "svc1", Lifecycle{true, "", 30})
	tu.AssertOk(t, err, "SetServiceLifecycleWithContext failed")
}

func TestSelectObjectContent(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server2",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")

	retryOpt := WithConnectRetries(1)
	mockCredsManager := setupMockCredsManager(c)
	oc, err := NewClient("ten1", "svc1", r, retryOpt, WithCredentialsManager(mockCredsManager))
	tu.AssertOk(t, err, "failed create new client")
	tu.Assert(t, oc != nil, "new client nil")

	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:             mc,
		bucketName:         "ten1:svc1",
		resolverClient:     r,
		credentialsManager: setupMockCredsManager(c),
	}

	// failure case
	mc.EXPECT().SelectObjectContentExplicit(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, fmt.Errorf("%s", connectErr)).Times(1)
	_, err = mockClient.SelectObjectContentExplicit(context.Background(), "svc1", "obj1", "", InputSerializationType(CSVGZIP), CSV)
	tu.Assert(t, err != nil, "SelectObjectContentExplicit passed, expected to fail")
}

func TestGetStreamObjectAtOffset(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server1",
		},
		Service: globals.VosMinio,
		URL:     l.Addr().(*net.TCPAddr).String(),
	})
	tu.AssertOk(t, err, "failed to add server2 service")
	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:         mc,
		bucketName:     "ten1:svc1",
		resolverClient: r,
	}

	// success case
	mc.EXPECT().GetObject(gomock.Any(), "obj1/obj1.0").Return(nil, nil).Times(1)
	_, err = mockClient.GetStreamObjectAtOffset(context.Background(), "obj1", 0)
	tu.AssertOk(t, err, "getobjectatoffset failed")

	// not in progress
	mc.EXPECT().GetObject(gomock.Any(), "obj1/obj1.0").Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(nil, fmt.Errorf("failed")).Times(1)
	_, err = mockClient.GetStreamObjectAtOffset(context.Background(), "obj1", 0)
	tu.Assert(t, err != nil, "getobjectatoffset didn't fail on error")

	// in progress
	objStats := &minioclient.ObjectStats{
		Size: 100,
	}
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToStreamObjName("obj1", 0))).Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().ListObjects(gomock.Any()).Return(nil, nil).Times(1)
	mc.EXPECT().StatObject(fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(objStats, nil).Times(1)
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToStreamObjName("obj1", 0))).Return(nil, nil).Times(1)

	_, err = mockClient.GetStreamObjectAtOffset(context.Background(), "obj1", 0)
	tu.AssertOk(t, err, "getobjectatoffset failed for in-progress streams")
}

func TestPutStreamObject(t *testing.T) {
	c := gomock.NewController(t)
	defer c.Finish()

	l, err := net.Listen("tcp", "127.0.0.1:")
	tu.AssertOk(t, err, "failed listen")
	minioServer(l)
	defer l.Close()

	r := mockresolver.New()

	err = r.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "server1",
		},
		Service: globals.VosMinio,
		URL:     "127.0.0.1:1001",
	})
	tu.AssertOk(t, err, "failed to add server2 service")
	mc := mockmc.NewMockobjStoreBackend(c)

	mockClient := &client{
		client:         mc,
		bucketName:     "ten1:svc1",
		resolverClient: r,
	}

	// success case
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	wr, err := mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed")

	mc.EXPECT().PutObject(gomock.Any(), objNameToStreamObjName("obj1", 0), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = wr.Write([]byte("test"))
	tu.AssertOk(t, err, "write stream failed")

	mc.EXPECT().PutObject(gomock.Any(), objNameToStreamObjName("obj1", 1), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = wr.Write([]byte("test"))
	tu.AssertOk(t, err, "write stream failed")

	mc.EXPECT().RemoveObjects(objNameToInProgress("obj1")).Return(nil).Times(1)
	err = wr.Close()
	tu.AssertOk(t, err, "failed to close stream")

	// putstream failure
	mockClient.client = mc
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.Assert(t, err != nil, fmt.Sprintf("putObjectStream didn't fail on error"))

	// write fail
	mockClient.client = mc
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	wr, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed")

	mc.EXPECT().PutObject(gomock.Any(), objNameToStreamObjName("obj1", 0), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = wr.Write([]byte("test"))
	tu.Assert(t, err != nil, fmt.Sprintf("write stream didn't fail on error"))

	// progress file error
	mockClient.client = mc
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(nil, fmt.Errorf("failed")).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	wr, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed")

	mc.EXPECT().PutObject(gomock.Any(), objNameToStreamObjName("obj1", 0), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), fmt.Errorf("failed")).Times(1)
	_, err = wr.Write([]byte("test"))
	tu.Assert(t, err != nil, fmt.Sprintf("write stream didn't fail on error"))

	// advance offset
	mockClient.client = mc
	buff := bytes.NewBufferString("10")
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(ioutil.NopCloser(buff), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	wr, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed")
	mc.EXPECT().PutObject(gomock.Any(), objNameToStreamObjName("obj1", 10), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = wr.Write([]byte("test"))
	tu.AssertOk(t, err, fmt.Sprintf("write stream to offset failed"))

	//  offset read error
	mockClient.client = mc
	buff = bytes.NewBufferString("cd")
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(ioutil.NopCloser(buff), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed to update offset ")

	//  timeout error
	mockClient.client = mc
	buff = bytes.NewBufferString("")
	tread := iotest.TimeoutReader(buff)
	ioutil.ReadAll(tread)
	mc.EXPECT().GetObject(gomock.Any(), fmt.Sprintf("%s", objNameToInProgress("obj1"))).Return(ioutil.NopCloser(tread), nil).Times(1)
	mc.EXPECT().PutObject(gomock.Any(), objNameToInProgress("obj1"), gomock.Any(), gomock.Any()).Return(int64(0), nil).Times(1)
	_, err = mockClient.PutStreamObject(context.Background(), "obj1", nil)
	tu.AssertOk(t, err, "putObjectStream failed on timeout")
}

func TestMain(m *testing.M) {
	os.Exit(m.Run())
}
