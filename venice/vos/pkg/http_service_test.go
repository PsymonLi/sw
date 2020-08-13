package vospkg

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/go-martini/martini"
	"github.com/gorilla/mux"
	minioclient "github.com/minio/minio-go/v6"
	"github.com/pkg/errors"

	"github.com/pensando/sw/venice/utils/objstore/minio"

	"github.com/pensando/sw/api/generated/objstore"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/vos"

	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestNewHandler(t *testing.T) {
	h, err := newHTTPHandler(nil, nil, nil)
	AssertOk(t, err, "newHTTPHandler returned error")
	Assert(t, h != nil, "handler is nil")
	ctx, cancel := context.WithCancel(context.Background())
	// should start the server and return
	minioCredentials := &minio.Credentials{
		AccessKey: "dummy",
		SecretKey: "dummy",
	}
	h.start(ctx, "0", minioCredentials, nil)
	cancel()
}

func createUploadReq(uri string, params map[string]string, pname, fname, method string, content []byte) (*http.Request, error) {
	body := &bytes.Buffer{}
	writer := multipart.NewWriter(body)
	part, err := writer.CreateFormFile(pname, fname)
	if err != nil {
		return nil, err
	}
	part.Write(content)

	for key, val := range params {
		_ = writer.WriteField(key, val)
	}
	err = writer.Close()
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequest(method, uri, body)
	req.Header.Set("Content-Type", writer.FormDataContentType())
	return req, err
}

func TestUploadHandler(t *testing.T) {
	metadata := map[string]string{
		"test1": "One",
		"test2": "Two",
	}

	req, err := createUploadReq("/test/", metadata, "badname", "file.test", "POST", []byte{})
	AssertOk(t, err, "failted to create request (%s)", err)
	wr := httptest.NewRecorder()
	fb := &mockBackend{}
	inst := &instance{}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst}
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusBadRequest, "invalid response for bad request [%v]", wr.Code)
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "POST", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusOK, "invalid response for bad request [%v][%v]", wr.Code, wr.Body.String())
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "GET", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusMethodNotAllowed, "invalid response for bad request [%v][%v]", wr.Code, wr.Body.String())
	httphdr := http.Header{}
	fb.statObject = 0
	tn := time.Now().Format(time.RFC3339Nano)
	for k, v := range metadata {
		httphdr[metaPrefix+k] = []string{v}
	}
	httphdr[metaPrefix+metaCreationTime] = []string{tn}
	sObjs := []*minioclient.ObjectInfo{
		nil,
		{
			ETag:     "abcdef",
			Key:      "file1",
			Metadata: httphdr,
			Size:     1024,
		},
	}
	fb.statObject = 0
	fb.statFunc = func(bucketName, objectName string, opts minioclient.StatObjectOptions) (minioclient.ObjectInfo, error) {
		if fb.statObject <= len(sObjs) {
			if sObjs[fb.statObject-1] != nil {
				return *sObjs[fb.statObject-1], nil
			}
			return minioclient.ObjectInfo{}, errors.New("not found")
		}
		return minioclient.ObjectInfo{}, fmt.Errorf("out of range [%d]", fb.statObject)
	}
	fb.putSize = 1024
	fb.putErr = nil
	fb.statObject = 0
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "POST", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusOK, "invalid response for good request [%v][%v]", wr.Code, wr.Body.String())
	fb.putSize = 1024
	fb.putErr = errors.New("some error")
	fb.statObject = 0
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "POST", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusInternalServerError, "invalid response for good request [%v][%v]", wr.Code, wr.Body.String())
	sObjs = []*minioclient.ObjectInfo{
		nil,
		nil,
	}
	fb.putErr = nil
	fb.statObject = 0
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "POST", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusInternalServerError, "invalid response for good request [%v][%v]", wr.Code, wr.Body.String())

	fb.statObject = 0
	fb.putSize = 2048
	sObjs = []*minioclient.ObjectInfo{
		nil,
		{
			ETag:     "abcdef",
			Key:      "file1",
			Metadata: httphdr,
			Size:     1024,
		},
	}
	wr = httptest.NewRecorder()
	req, err = createUploadReq("/test/", metadata, "file", "file.test", "POST", []byte{})
	AssertOk(t, err, "failed to create request (%s)", err)
	srv.uploadHandler(wr, req, "images")
	Assert(t, wr.Code == http.StatusInternalServerError, "invalid response for good request [%v][%v]", wr.Code, wr.Body.String())
}

func TestDownloadHandler(t *testing.T) {
	wr := httptest.NewRecorder()
	fb := &mockBackend{}
	inst := &instance{}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst}
	var cbErr1, cbErr2 error
	var cbcalled1, cbcalled2 int
	// Add plugin funcs
	cbFunc1 := func(ctx context.Context, oper vos.ObjectOper, in *objstore.Object, client vos.BackendClient) error {
		cbcalled1++
		return cbErr1
	}
	cbFunc2 := func(ctx context.Context, oper vos.ObjectOper, in *objstore.Object, client vos.BackendClient) error {
		cbcalled2++
		return cbErr2
	}
	inst.RegisterCb("images", vos.PreOp, vos.Download, cbFunc1)
	inst.RegisterCb("images", vos.PostOp, vos.Download, cbFunc2)

	req, err := http.NewRequest("GET", "/download/image", nil)
	AssertOk(t, err, "failed to create request")

	wr = httptest.NewRecorder()
	params := martini.Params(map[string]string{"_1": ""})
	srv.downloadHandler(params, wr, req)
	Assert(t, wr.Code == http.StatusBadRequest, "expecting failure due to no path")

	wr = httptest.NewRecorder()
	fso := &fakeStoreObj{}
	fb.fObj = fso
	fb.retErr = errors.New("some error")
	req, err = http.NewRequest("GET", "/download/image/some/path", nil)
	AssertOk(t, err, "failed to create request")
	params["_1"] = "/some/path"
	srv.downloadHandler(params, wr, req)
	Assert(t, wr.Code == http.StatusNotFound, "expecting failure due to not found got [%v]", wr.Code)

	wr = httptest.NewRecorder()

	cbErr1 = fb.retErr
	fb.retErr = nil
	wr = httptest.NewRecorder()
	srv.downloadHandler(params, wr, req)
	Assert(t, wr.Code == http.StatusPreconditionFailed, "expecting failure due to not found got [%v]", wr.Code)

	cnt := 0
	testStr := []byte("aabababababababaabababababaababababababaabababababa")
	readErr := io.EOF
	fso.readFn = func(in []byte) (int, error) {
		in = testStr
		var err error
		if cnt == 3 {
			err = readErr
		}

		if cnt > 3 {
			return 0, io.EOF
		}
		cnt++
		return len(testStr), err
	}
	wr = httptest.NewRecorder()
	cbcalled1, cbcalled2, cnt = 0, 0, 0
	cbErr1 = nil
	srv.downloadHandler(params, wr, req)
	AssertOk(t, err, "DownloadFile failed")
	Assert(t, cbcalled1 == 1, "exepecting 1 call for preop got [%d]", cbcalled1)
	Assert(t, cbcalled2 == 1, "exepecting 1 call for postop got [%d]", cbcalled2)
	Assert(t, wr.Code == http.StatusOK, "expecting downlod to succeed got  [%v]", wr.Code)

	cbErr2 = errors.New("some error")
	wr = httptest.NewRecorder()
	cbcalled1, cbcalled2, cnt = 0, 0, 0
	srv.downloadHandler(params, wr, req)
}

func TestMetricsHandler(t *testing.T) {
	fb := &mockBackend{}
	inst := &instance{}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst}
	req, err := http.NewRequest("GET", "/debug/minio/metrics", nil)
	AssertOk(t, err, "failed to create request")

	wr := httptest.NewRecorder()
	handler := srv.minioMetricsHandler("dummy", "dummy")
	handler(wr, req)
	Assert(t, wr.Code == http.StatusInternalServerError, "expecting failure due to metrics path not implemented in mock backend")
}

func TestCredentialsHandler(t *testing.T) {
	fb := &mockBackend{}
	inst := &instance{}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst}
	req, err := http.NewRequest("GET", "/debug/minio/credentials", nil)
	AssertOk(t, err, "failed to create request")

	wr := httptest.NewRecorder()
	testAccessKey := "minioTestAccessKey"
	testOldAccessKey := "minioTestOldAccessKey"
	testSecretKey := "minioTestSecretKey"
	testOldSecretKey := "minioTestOldSecretKey"
	minioCredentials := &minio.Credentials{
		AccessKey:    testAccessKey,
		SecretKey:    testSecretKey,
		OldAccessKey: testOldAccessKey,
		OldSecretKey: testOldSecretKey,
	}
	handler := srv.minioCredentialsHandler(minioCredentials)
	handler(wr, req)
	bodyBytes, err := ioutil.ReadAll(wr.Body)
	AssertOk(t, err, "Body should have been read successfully")
	minioCreds := map[string]string{}
	err = json.Unmarshal(bodyBytes, &minioCreds)
	AssertOk(t, err, "Credentials should have been unmarshalled successfully")
	assertKey(t, minioCreds, globals.MinioAccessKeyName, testAccessKey)
	assertKey(t, minioCreds, globals.MinioSecretKeyName, testSecretKey)
	assertKey(t, minioCreds, globals.MinioOldAccessKeyName, testOldAccessKey)
	assertKey(t, minioCreds, globals.MinioOldSecretKeyName, testOldSecretKey)
}

func assertKey(t *testing.T, minioCreds map[string]string, keyName, expectedValue string) {
	actualValue, ok := minioCreds[keyName]
	Assert(t, ok, "%s should be present", keyName)
	AssertEquals(t, expectedValue, actualValue, "actual key different from expected")
}

func TestDebugConfigHandler(t *testing.T) {
	paths := new(sync.Map)
	c := DiskMonitorConfig{
		TenantName:               "",
		CombinedThresholdPercent: 40.00,
		CombinedBuckets:          []string{"/root/dummy_path1", "/root/dummy_path2"},
	}
	paths.Store("", c)
	fb := &mockBackend{}
	inst := &instance{bucketDiskThresholds: paths}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst}
	router := mux.NewRouter()
	router.Methods("GET", "POST").Subrouter().Handle("/debug/config", http.HandlerFunc(srv.debugConfigHandler()))
	go http.ListenAndServe(fmt.Sprintf("127.0.0.1:%s", globals.VosHTTPPort), router)
	time.Sleep(time.Second * 2)
	getHelper := func(uri string) map[string]interface{} {
		resp, err := http.Get(uri)
		AssertOk(t, err, "error is getting %s", uri)
		defer resp.Body.Close()
		body, err := ioutil.ReadAll(resp.Body)
		AssertOk(t, err, "error is reading response of %s", uri)
		config := map[string]interface{}{}
		err = json.Unmarshal(body, &config)
		AssertOk(t, err, "error is unmarshaling response of %s", uri)
		return config
	}

	uri := strings.TrimSpace(fmt.Sprintf("http://127.0.0.1:%s", globals.VosHTTPPort) + "/debug/config")
	config := getHelper(uri)
	th, ok := config[bucketDiskThresholdKey]
	Assert(t, ok, "bucketDiskThreshold config is not present in /debug/config response, response:", th)
	ths, ok := th.(map[string]interface{})
	Assert(t, ok, "bucketDiskThreshold type is not map[string]interface{}, response:", th, reflect.TypeOf(th).String())
	Assert(t, len(ths) == 1, "incorrect number of disk entries, response:", th)
	for tenantName, dmc := range ths {
		Assert(t, tenantName == "", "incorrect tenantName", tenantName)
		for key, value := range dmc.(map[string]interface{}) {
			if key == "buckets" {
				Assert(t, len(value.([]interface{})) == 2, "incorrect number of buckets", key)
				for _, b := range value.([]interface{}) {
					temp := b.(string)
					if temp != "/root/dummy_path1" && temp != "/root/dummy_path2" {
						Assert(t, false, "incorrect path found")
					}
				}
			} else if key == "tenant" {
				Assert(t, value.(string) == "", "incorrect tenant name")
			} else if key == "threshold" {
				Assert(t, value.(float64) == 40.00, "incorrect threshold", value.(float64))
			} else {
				Assert(t, false, "unknown key found in response ", key)
			}
		}
	}
	reqBody, err := json.Marshal(map[string]interface{}{
		bucketDiskThresholdKey: map[string]DiskMonitorConfig{
			"": {
				TenantName:               "",
				CombinedBuckets:          []string{"/root/dummy_path3", "/root/dummy_path4"},
				CombinedThresholdPercent: 80.00,
			},
		},
	})

	AssertOk(t, err, "error is marshaling POST request for /debug/config")
	req, err := http.NewRequest("POST", uri, bytes.NewBuffer(reqBody))
	AssertOk(t, err, "error is creating POST request for /debug/config")
	req.Header.Set("Content-Type", "application/json; charset=utf-8")
	client := &http.Client{}
	resp, err := client.Do(req)
	AssertOk(t, err, "error is POSTing request for /debug/config")
	defer resp.Body.Close()
	Assert(t, resp.Status == "200 OK", "incorrect response status")
	// Veirfy that indexing is disabled
	config = getHelper(uri)
	th, ok = config[bucketDiskThresholdKey]
	Assert(t, ok, "bucketDiskThreshold config is not present in /debug/config response, response:", th)
	ths, ok = th.(map[string]interface{})
	Assert(t, ok, "bucketDiskThreshold type is not map[string]interface{}, response:", th)
	Assert(t, len(ths) == 1, "incorrect number of disk entries, response:", th)
	for tenantName, dmc := range ths {
		Assert(t, tenantName == "", "incorrect tenantName", tenantName)
		for key, value := range dmc.(map[string]interface{}) {
			if key == "buckets" {
				Assert(t, len(value.([]interface{})) == 2, "incorrect number of buckets", key)
				for _, b := range value.([]interface{}) {
					temp := b.(string)
					if temp != "/root/dummy_path3" && temp != "/root/dummy_path4" {
						Assert(t, false, "incorrect path found")
					}
				}
			} else if key == "tenant" {
				Assert(t, value.(string) == "", "incorrect tenant name")
			} else if key == "threshold" {
				Assert(t, value.(float64) == 80.00, "incorrect threshold", value.(float64))
			} else {
				Assert(t, false, "unknown key found in response ", key)
			}
		}
	}
}

func TestMinioDebugHandlers(t *testing.T) {
	// Actual tests are happening in integ.
	// Just call the APIs here to get the coverage
	paths := new(sync.Map)
	paths.Store("/root/dummy_path1", 40.00)
	paths.Store("/root/dummy_path2", 30.00)
	fb := &mockBackend{}
	inst := &instance{bucketDiskThresholds: paths}
	inst.Init(fb)
	srv := httpHandler{client: fb, instance: inst, adminClient: fb}
	router := mux.NewRouter()
	router.Methods("GET").Subrouter().Handle("/debug/minio/clusterinfo", http.HandlerFunc(srv.minioClusterInfoHandler()))
	router.Methods("GET").Subrouter().Handle("/debug/minio/datausageinfo", http.HandlerFunc(srv.minioDataUsageInfoHandler()))
	router.Methods("GET").Subrouter().Handle("/debug/minio/restartservices", http.HandlerFunc(srv.minioRestartServicesHandler()))
	router.Methods("GET").Subrouter().Handle("/debug/minio/recoverconfigmigration", http.HandlerFunc(srv.minioRecoverFromConfigMigration()))
	go http.ListenAndServe(fmt.Sprintf("127.0.0.1:9053"), router)
	time.Sleep(time.Second * 4)

	baseURL := "http://127.0.0.1:9053"
	getHelper := func(url string) map[string]interface{} {
		resp, err := http.Get(url)
		AssertOk(t, err, "error is getting "+url)
		defer resp.Body.Close()
		body, err := ioutil.ReadAll(resp.Body)
		AssertOk(t, err, "error is reading response of "+url)
		config := map[string]interface{}{}
		err = json.Unmarshal(body, &config)
		AssertOk(t, err, "error is unmarshaling response of "+url)
		return config
	}

	// Get ClusterInfo
	config := getHelper(baseURL + "/debug/minio/clusterinfo")
	v, ok := config["dummyInfo"]
	Assert(t, ok, "dummyInfo is not present in /debug/minio/clusterinfo response, response:", config)
	Assert(t, v.(string) == "dummyValue", "cluster is not online, returned value", v.(string))

	// Get DataUsageInfo
	config = getHelper(baseURL + "/debug/minio/datausageinfo")
	v, ok = config["dummyInfo"]
	Assert(t, ok, "dummyInfo is not present in /debug/minio/clusterinfo response, response:", config)
	Assert(t, v.(string) == "dummyValue", "cluster is not online, returned value", v.(string))

	// Restart services
	_, err := http.Get(baseURL + "/debug/minio/restartservices")
	AssertOk(t, err, "error is restarting services")

	_, err = http.Get(baseURL + "/debug/minio/recoverconfigmigration")
	AssertOk(t, err, "error is recovering minio")
}
