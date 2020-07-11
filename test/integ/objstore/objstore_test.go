// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package objstoreinteg

import (
	"bytes"
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"reflect"
	"strings"
	"testing"
	"time"

	uuid "github.com/satori/go.uuid"
	. "gopkg.in/check.v1"

	"github.com/pensando/sw/venice/utils/objstore/minio"
	minioclient "github.com/pensando/sw/venice/utils/objstore/minio/client"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	testutils "github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/cmd/credentials"
	types "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	"github.com/pensando/sw/venice/utils/rpckit"
	. "github.com/pensando/sw/venice/utils/testutils"
)

// integ test suite parameters
const (
	// TLS keys and certificates used by mock CKM endpoint to generate control-plane certs
	certPath       = "../../../venice/utils/certmgr/testdata/ca.cert.pem"
	keyPath        = "../../../venice/utils/certmgr/testdata/ca.key.pem"
	rootsPath      = "../../../venice/utils/certmgr/testdata/roots.pem"
	minioURL       = "127.0.0.1:19001"
	vosDebugURL    = "127.0.0.1:9052"
	minioHealthURL = "http://127.0.0.1:19001/minio/health/live"
)

// objstoreIntegSuite is the state of integ test
type objstoreIntegSuite struct {
	restClient     apiclient.Services
	resolverClient *mockresolver.ResolverClient
	authDir        string
	tlsConfig      *tls.Config
	credsManger    minio.CredentialsManager
}

type dummyCredsManager struct {
	creds *minio.Credentials
}

func (d *dummyCredsManager) GetCredentials() (*minio.Credentials, error) {
	return d.creds, nil
}

func (d *dummyCredsManager) CreateCredentials() (*minio.Credentials, error) {
	return nil, nil
}

func TestObjStoreInteg(t *testing.T) {
	// integ test suite
	var sts = &objstoreIntegSuite{}
	var _ = Suite(sts)
	TestingT(t)
}

func (it *objstoreIntegSuite) SetUpSuite(c *C) {
	// We need a fairly high limit because all clients are collapsed into a single process
	// so they hit the same rate limiter
	rpckit.SetDefaultListenerConnectionRateLimit(50)
	err := testutils.SetupIntegTLSProvider()
	if err != nil {
		log.Fatalf("Error setting up TLS provider: %v", err)
	}

	// Now create a mock resolver
	it.resolverClient = mockresolver.New()
	penVos := types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "pen-vos",
		},
		Service: globals.VosMinio,
		Node:    "localhost",
		URL:     minioURL,
	}
	it.resolverClient.AddServiceInstance(&penVos)

	caKey, err := certs.ReadPrivateKey(keyPath)
	c.Assert(err, IsNil)
	caCert, err := certs.ReadCertificate(certPath)
	c.Assert(err, IsNil)
	trustRoots, err := certs.ReadCertificates(rootsPath)
	c.Assert(err, IsNil)
	it.authDir, err = ioutil.TempDir("/tmp", "objstore_test")
	c.Assert(err, IsNil)
	csrSigner := func(csr *x509.CertificateRequest, opts ...certs.Option) (*x509.Certificate, error) {
		return certs.SignCSRwithCA(csr, caCert, caKey, certs.WithValidityDays(1))
	}
	err = credentials.GenVosHTTPSAuth("localhost", it.authDir, csrSigner, append([]*x509.Certificate{caCert}, trustRoots[0]), trustRoots)
	c.Assert(err, IsNil)

	it.tlsConfig = &tls.Config{
		RootCAs:    certs.NewCertPool(trustRoots),
		ServerName: globals.Vos,
	}

	minioAccessKey := uuid.NewV4().String()
	minioSecretKey := uuid.NewV4().String()
	it.credsManger = &dummyCredsManager{
		creds: &minio.Credentials{
			AccessKey: minioAccessKey,
			SecretKey: minioSecretKey,
		},
	}

	// start objstore
	ctx := context.Background()
	logger := log.GetNewLogger(log.GetDefaultConfig("TestObjStoreApis"))
	testutils.StartVos(ctx, logger, it.credsManger)

	client := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: it.tlsConfig,
		},
	}

	// check health
	AssertEventually(c, func() (bool, interface{}) {
		s, err := client.Get(minioHealthURL)
		if err != nil {
			return false, s
		}
		defer s.Body.Close()
		return s.StatusCode == http.StatusOK, s
	}, "minio server is unhealthy", "1s", "60s")
}

func (it *objstoreIntegSuite) SetUpTest(c *C) {
	log.Infof("============================= %s starting ==========================", c.TestName())
}

func (it *objstoreIntegSuite) TearDownTest(c *C) {
	log.Infof("============================= %s completed ==========================", c.TestName())

}

func (it *objstoreIntegSuite) TearDownSuite(c *C) {
	// stop server and client
	log.Infof("Stop all Test Controllers")

	it.resolverClient.Stop()

	testutils.CleanupIntegTLSProvider()

	os.RemoveAll(it.authDir)
}

// basic test to make sure all components come up
func (it *objstoreIntegSuite) TestObjStoreApis(c *C) {
	oc, err := objstore.NewClient("default",
		"pktcap", it.resolverClient, objstore.WithCredentialsManager(it.credsManger))
	AssertOk(c, err, fmt.Sprintf("obj store client failed"))
	_, err = oc.ListObjects("")
	AssertOk(c, err, fmt.Sprintf("list objects failed"))

	obj1Data := []byte("test object")
	b := bytes.NewBuffer(obj1Data)
	meta := map[string]string{
		"Key1": "val1",
		"Key2": "val2",
		"Key3": "val3",
		"Key4": "val4",
		"Key5": "val5",
	}

	// put obj
	n, err := oc.PutObject(context.Background(), "obj1", b, meta)
	AssertOk(c, err, fmt.Sprintf("put object failed"))
	Assert(c, n > 0, fmt.Sprintf("put object returned invalid bytes"))

	// stat obj
	s, err := oc.StatObject("obj1")
	AssertOk(c, err, fmt.Sprintf("stat object failed"))
	for k, v := range meta {
		Assert(c, s.MetaData[k] == v, fmt.Sprintf("meta didn't match key %s in %+v, expected %+v", k, s.MetaData, v))
	}

	// list obj
	objList, err := oc.ListObjects("obj1")
	AssertOk(c, err, fmt.Sprintf("list object failed"))
	Assert(c, len(objList) == 1, fmt.Sprintf("too many objects in list %+v", objList))

	// get obj
	reader, err := oc.GetObject(context.Background(), "obj1")
	AssertOk(c, err, fmt.Sprintf("get object failed"))
	data, err := ioutil.ReadAll(reader)
	AssertOk(c, err, fmt.Sprintf("read object failed"))
	Assert(c, reflect.DeepEqual(data, obj1Data), fmt.Sprintf("invalid object read :%s, expected: %s", string(data), string(obj1Data)))

	// rem obj
	err = oc.RemoveObjects("obj1")
	AssertOk(c, err, fmt.Sprintf("delete object failed"))
	objList, err = oc.ListObjects("obj1")
	AssertOk(c, err, fmt.Sprintf("list object failed"))
	Assert(c, len(objList) == 0, fmt.Sprintf("too many objects in list %+v", objList))

	// putstream
	wr, err := oc.PutStreamObject(context.Background(), "obj2", meta)
	AssertOk(c, err, fmt.Sprintf("putstream object failed"))
	Assert(c, wr != nil, fmt.Sprintf("put object returned invalid writer"))

	for i := 0; i < 10; i++ {
		s, err = oc.StatObject("obj2")
		AssertOk(c, err, fmt.Sprintf("stat on stream object failed"))

		n, err := wr.Write(obj1Data)
		AssertOk(c, err, fmt.Sprintf("write object failed"))
		Assert(c, n > 0, fmt.Sprintf("invalid bytes from write %+v", n))
	}
	objList, err = oc.ListObjects("obj2")
	AssertOk(c, err, fmt.Sprintf("list object failed"))
	Assert(c, len(objList) == 11, fmt.Sprintf("too many objects in list %+v", objList))
	wr.Close()
	objList, err = oc.ListObjects("obj2")
	AssertOk(c, err, fmt.Sprintf("list object failed"))
	Assert(c, len(objList) == 10, fmt.Sprintf("too many objects in list %+v", objList))

	// get stream
	for i := 0; i < 10; i++ {
		reader, err := oc.GetStreamObjectAtOffset(context.Background(), "obj2", i)
		AssertOk(c, err, fmt.Sprintf("getstream failed for %d", i))
		_, err = ioutil.ReadAll(reader)
		AssertOk(c, err, fmt.Sprintf("reader failed for %d", i))
	}

	// remove stream
	err = oc.RemoveObjects("obj2")
	AssertOk(c, err, fmt.Sprintf("delete object failed"))
	objList, err = oc.ListObjects("obj2")
	AssertOk(c, err, fmt.Sprintf("list object failed"))
	Assert(c, len(objList) == 0, fmt.Sprintf("too many objects in list %+v", objList))
}

// basic test to make sure all components come up
func (it *objstoreIntegSuite) TestObjStoreAdminApis(c *C) {
	ctx := context.Background()
	mc, err := minioclient.NewAdminClient(it.resolverClient, minioclient.WithCredentialsManager(it.credsManger))
	AssertOk(c, err, fmt.Sprintf("objstore admin client failed"))
	Assert(c, mc != nil, fmt.Sprintf("objstore admin client is nil"))

	time.Sleep(time.Second * 3)

	// DataUsageInfo
	AssertEventually(c, func() (bool, interface{}) {
		info, err := mc.DataUsageInfo(ctx)
		if err != nil {
			log.Errorf("objstore DataUsageInfo err %+v", err)
		}
		if info == "" {
			log.Infof("objstore DataUsageInfo empty result")
		}
		return err == nil && info != "", nil
	}, "objstore DataUsageInfo API failed", "500ms", "30s")

	// IsCluserOnline
	AssertEventually(c, func() (bool, interface{}) {
		online, err := mc.IsClusterOnline(ctx)
		if err != nil {
			log.Errorf("objstore admin IsClusterOnline failed err %+v", err)
		}
		if !online {
			log.Infof("objstore admin IsClusterOnline false")
		}
		return err == nil && online, nil
	}, "objstore IsClusterOnline API failed", "500ms", "30s")

	// ServerInfo
	AssertEventually(c, func() (bool, interface{}) {
		info, err := mc.ClusterInfo(ctx)
		if err != nil {
			log.Errorf("objstore admin ServerInfo failed err %+v", err)
		}
		if info == "" {
			log.Infof("objstore admin ServerInfo empty result")
		}
		return err == nil && info != "", nil
	}, "objstore ServerInfo API failed", "500ms", "30s")
}

// basic test to make sure all components come up
func (it *objstoreIntegSuite) TestObjStoreDebugRESTApis(c *C) {
	time.Sleep(time.Second * 5)

	baseURL := "http://" + vosDebugURL

	getHelper := func(url string) map[string]interface{} {
		resp, err := http.Get(url)
		AssertOk(c, err, "error is getting "+url)
		defer resp.Body.Close()
		body, err := ioutil.ReadAll(resp.Body)
		AssertOk(c, err, "error is reading response of "+url)
		fmt.Println("response", string(body[:]))
		config := map[string]interface{}{}
		err = json.Unmarshal(body, &config)
		AssertOk(c, err, "error is unmarshaling response of "+url)
		return config
	}

	/***** Test Admin APIs *********/
	// Get ClusterInfo
	AssertEventually(c, func() (bool, interface{}) {
		config := getHelper(baseURL + "/debug/minio/clusterinfo")
		v, ok := config["mode"]
		if ok && v.(string) == "online" {
			return true, nil
		}
		if !ok {
			log.Errorf("mode is not present in /debug/minio/clusterinfo response, response: %s", config)
		} else if v.(string) != "online" {
			log.Infof("cluster is not online, returned value: %s", v.(string))
		}
		return false, nil
	}, "objstore /debug/minio/clusterinfo API failed", "500ms", "30s")

	AssertEventually(c, func() (bool, interface{}) {
		config := getHelper(baseURL + "/debug/minio/datausageinfo")
		_, ok := config["bucketsCount"]
		if ok {
			return true, nil
		}
		log.Errorf("bucketsCount is not present in /debug/minio/datausageinfo response, response: %s", config)
		return false, nil
	}, "objstore /debug/minio/datausageinfo API failed", "500ms", "30s")

	/***** Test debug/config APIs *********/
	AssertEventually(c, func() (bool, interface{}) {
		config := getHelper(baseURL + "/debug/config")
		_, ok := config["bucketLifecycle"]
		if ok {
			return true, nil
		}
		log.Errorf("bucketLifecycle is not present in /debug/config response, response: %s", config)
		return false, nil
	}, "objstore /debug/config API failed", "500ms", "30s")

	oc, err := objstore.NewClient("default",
		"fwlogs", it.resolverClient, objstore.WithCredentialsManager(it.credsManger))
	AssertOk(c, err, fmt.Sprintf("obj store client failed"))
	_, err = oc.ListObjects("")
	AssertOk(c, err, fmt.Sprintf("list objects failed"))

	obj1Data := []byte("test object")
	b := bytes.NewBuffer(obj1Data)
	meta := map[string]string{
		"Key1": "val1",
	}

	// put obj
	n, err := oc.PutObjectOfSize(context.Background(), "obj1", b, int64(len(obj1Data)), meta)
	AssertOk(c, err, fmt.Sprintf("put object failed"))
	Assert(c, n > 0, fmt.Sprintf("put object returned invalid bytes"))

	// Set Bucket lifecycle
	lifecycle := `<LifecycleConfiguration>
					<Rule>
					<ID>expire-bucket</ID>
					<Prefix></Prefix>
					<Status>Enabled</Status>
					<Expiration>
						<Date>%s</Date>
					</Expiration>
					</Rule>
					</LifecycleConfiguration>`
	y, m, d := time.Now().Date()
	ts := time.Date(y, m, d, 0, 0, 0, 0, time.UTC)
	tsString := ts.Format(time.RFC3339)
	lc := fmt.Sprintf(lifecycle, tsString)
	reqMap := map[string]interface{}{}
	lcMap := map[string]string{}
	lcMap["default.fwlogs"] = lc
	reqMap["bucketLifecycle"] = lcMap
	reqBody, err := json.Marshal(reqMap)
	AssertOk(c, err, "error is marshaling POST request for /debug/config")
	req, err := http.NewRequest("POST", baseURL+"/debug/config", bytes.NewBuffer(reqBody))
	AssertOk(c, err, "error is creating POST request for /debug/config")
	req.Header.Set("Content-Type", "application/json; charset=utf-8")
	client := &http.Client{}
	resp, err := client.Do(req)
	AssertOk(c, err, "error is POSTing request for /debug/config")
	defer resp.Body.Close()
	_, err = ioutil.ReadAll(resp.Body)
	AssertOk(c, err, "error is reading response body for /debug/config")
	Assert(c, resp.Status == "200 OK", "incorrect response status")

	// Now get lifecycle
	config := getHelper(baseURL + "/debug/config")
	lcConfigInterface, ok := config["bucketLifecycle"]
	Assert(c, ok, "bucketLifecycle key is not present in /debug/config response")
	fmt.Println("Shrey lcConfig", lcConfigInterface)
	lcConfig := lcConfigInterface.(map[string]interface{})
	defaultFwlogsLc, ok := lcConfig["default.fwlogs"]
	dateSubstringToMatch := fmt.Sprintf("<Date>%s</Date>", tsString)
	Assert(c, ok, "lifecycle config is not present for default.fwlogs bucket")
	Assert(c, strings.Contains(defaultFwlogsLc.(string), dateSubstringToMatch), "fetched lc config is not same to the one that was posted")
}
