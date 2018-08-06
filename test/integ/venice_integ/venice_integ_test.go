// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package veniceinteg

import (
	"flag"
	"fmt"
	"os"
	"testing"
	"time"

	"golang.org/x/net/context"
	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	evtsapi "github.com/pensando/sw/api/generated/events"
	"github.com/pensando/sw/nic/agent/netagent"
	"github.com/pensando/sw/nic/agent/netagent/ctrlerif/restapi"
	"github.com/pensando/sw/nic/agent/netagent/datapath"
	"github.com/pensando/sw/nic/agent/troubleshooting"
	tshal "github.com/pensando/sw/nic/agent/troubleshooting/datapath/hal"
	testutils "github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/apigw"
	"github.com/pensando/sw/venice/apiserver"
	certsrv "github.com/pensando/sw/venice/cmd/grpc/server/certificates/mock"
	"github.com/pensando/sw/venice/cmd/grpc/service"
	"github.com/pensando/sw/venice/cmd/services/mock"
	"github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/ctrler/npm"
	"github.com/pensando/sw/venice/ctrler/tsm"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/orch"
	"github.com/pensando/sw/venice/orch/simapi"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/rpckit/tlsproviders"
	"github.com/pensando/sw/venice/utils/runtime"
	"github.com/pensando/sw/venice/utils/testenv"
	"github.com/pensando/sw/venice/utils/tsdb"

	_ "github.com/pensando/sw/api/generated/cluster/gateway"
	_ "github.com/pensando/sw/api/generated/exports/apiserver"
	_ "github.com/pensando/sw/api/generated/monitoring/gateway"
	_ "github.com/pensando/sw/api/generated/network/gateway"
	_ "github.com/pensando/sw/api/hooks/apiserver"
	_ "github.com/pensando/sw/venice/apigw/svc"

	"github.com/golang/mock/gomock"

	"reflect"

	"github.com/pensando/sw/nic/agent/netagent/protos"

	. "github.com/pensando/sw/venice/utils/testutils"

	"github.com/pensando/sw/venice/ctrler/tpm"
)

// integ test suite parameters
const (
	numIntegTestAgents  = 3
	integTestNpmURL     = "localhost:9495"
	integTestNpmRESTURL = "localhost:9496"
	integTestApisrvURL  = "localhost:8082"
	integTestAPIGWURL   = "localhost:9092"
	integTestTPMURL     = "localhost:9093"
	vchTestURL          = "localhost:19003"
	// TS Controller
	integTestTsmURL     = "localhost:9500"
	integTestTsmRestURL = "localhost:9501"
	agentDatapathKind   = "mock"
	// TLS keys and certificates used by mock CKM endpoint to generate control-plane certs
	certPath  = "../../../venice/utils/certmgr/testdata/ca.cert.pem"
	keyPath   = "../../../venice/utils/certmgr/testdata/ca.key.pem"
	rootsPath = "../../../venice/utils/certmgr/testdata/roots.pem"
)

var (
	// create events recorder
	_, _ = recorder.NewRecorder(
		&evtsapi.EventSource{NodeName: utils.GetHostname(), Component: "venice_integ_test"},
		evtsapi.GetEventTypes(), "", "/tmp")
)

// veniceIntegSuite is the state of integ test
type veniceIntegSuite struct {
	apiSrv         apiserver.Server
	apiGw          apigw.APIGateway
	certSrv        *certsrv.CertSrv
	ctrler         *npm.Netctrler
	tpm            *tpm.PolicyManager
	tsCtrler       *tsm.TsCtrler
	agents         []*netagent.Agent
	datapaths      []*datapath.Datapath
	datapathKind   datapath.Kind
	numAgents      int
	restClient     apiclient.Services
	apisrvClient   apiclient.Services
	vcHub          vchSuite
	resolverSrv    *rpckit.RPCServer
	resolverClient resolver.Interface
	userCred       *auth.PasswordCredential
}

// test args
var numAgents = flag.Int("agents", numIntegTestAgents, "Number of agents")
var datapathKind = flag.String("datapath", agentDatapathKind, "Specify the datapath type. mock | hal")

// Hook up gocheck into the "go test" runner.
func TestVeniceInteg(t *testing.T) {
	// integ test suite
	var sts = &veniceIntegSuite{}

	var _ = Suite(sts)
	TestingT(t)
}

func (it *veniceIntegSuite) SetUpSuite(c *C) {
	// test parameters
	it.numAgents = *numAgents
	it.datapathKind = datapath.Kind(*datapathKind)

	logger := log.GetNewLogger(log.GetDefaultConfig("venice_integ"))
	tsdb.Init(&tsdb.DummyTransmitter{}, tsdb.Options{})

	// start certificate server
	certSrv, err := certsrv.NewCertSrv("localhost:0", certPath, keyPath, rootsPath)
	c.Assert(err, IsNil)
	it.certSrv = certSrv
	log.Infof("Created cert endpoint at %s", globals.CMDCertAPIPort)

	// instantiate a CKM-based TLS provider and make it default for all rpckit clients and servers
	tlsProvider := func(svcName string) (rpckit.TLSProvider, error) {
		p, err := tlsproviders.NewDefaultCMDBasedProvider(certSrv.GetListenURL(), svcName)
		if err != nil {
			return nil, err
		}
		return p, nil
	}
	testenv.EnableRpckitTestMode()
	rpckit.SetTestModeDefaultTLSProvider(tlsProvider)

	// Now create a mock resolver
	m := mock.NewResolverService()
	resolverHandler := service.NewRPCHandler(m)
	resolverServer, err := rpckit.NewRPCServer(globals.Cmd, "localhost:0", rpckit.WithTracerEnabled(true))
	c.Assert(err, IsNil)
	types.RegisterServiceAPIServer(resolverServer.GrpcServer, resolverHandler)
	resolverServer.Start()
	it.resolverSrv = resolverServer

	// populate the mock resolver with apiserver instance.
	apiSrvSi := types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "pen-apiserver-test",
		},
		Service: globals.APIServer,
		Node:    "localhost",
		URL:     integTestApisrvURL,
	}
	m.AddServiceInstance(&apiSrvSi)

	npmSi := types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "pen-npm-test",
		},
		Service: globals.Npm,
		Node:    "localhost",
		URL:     integTestNpmURL,
	}
	m.AddServiceInstance(&npmSi)

	tsmSi := types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "pen-tsm-test",
		},
		Service: globals.Tsm,
		Node:    "localhost",
		URL:     integTestTsmURL,
	}
	m.AddServiceInstance(&tsmSi)

	// start API server
	it.apiSrv, _, err = testutils.StartAPIServer(integTestApisrvURL, &store.Config{
		Type:    store.KVStoreTypeMemkv,
		Servers: []string{""},
		Codec:   runtime.NewJSONCodec(runtime.GetDefaultScheme()),
	}, logger)
	c.Assert(err, IsNil)

	// start API gateway
	it.apiGw, _, err = testutils.StartAPIGateway(integTestAPIGWURL, map[string]string{}, []string{resolverServer.GetListenURL()}, logger)
	c.Assert(err, IsNil)

	// create a controller
	rc := resolver.New(&resolver.Config{Name: globals.Npm, Servers: []string{resolverServer.GetListenURL()}})
	it.resolverClient = rc
	ctrler, err := npm.NewNetctrler(integTestNpmURL, integTestNpmRESTURL, globals.APIServer, "", rc)
	c.Assert(err, IsNil)
	it.ctrler = ctrler

	// create a trouble shooting controller
	tsmrc := resolver.New(&resolver.Config{Name: globals.Tsm, Servers: []string{resolverServer.GetListenURL()}})
	// The resolver client will be used by trouble shooting net agent...
	tsCtrler, err := tsm.NewTsCtrler(integTestTsmURL, integTestTsmRestURL, globals.APIServer, tsmrc)
	c.Assert(err, IsNil)
	it.tsCtrler = tsCtrler

	log.Infof("Creating %d/%d agents", it.numAgents, *numAgents)

	// create agents
	for i := 0; i < it.numAgents; i++ {
		// mock datapath
		dp, aerr := datapath.NewHalDatapath(it.datapathKind)
		c.Assert(aerr, IsNil)
		it.datapaths = append(it.datapaths, dp)

		// set tenant create expectations for mock clients
		if it.datapathKind.String() == "mock" {
			dp.Hal.MockClients.MockTnclient.EXPECT().VrfCreate(gomock.Any(), gomock.Any()).Return(nil, nil)
		}

		// Create netagent
		agent, aerr := netagent.NewAgent(dp, fmt.Sprintf("/tmp/agent_%d.db", i), fmt.Sprintf("dummy-uuid-%d", i), globals.Npm, rc, state.AgentMode_MANAGED)
		c.Assert(aerr, IsNil)

		tsdp, aerr := tshal.NewHalDatapath("mock")
		c.Assert(aerr, IsNil)
		//it.datapaths = append(it.datapaths, dp)

		log.Infof("creating troubleshooting subagent")
		tsa, aerr := troubleshooting.NewTsAgent(tsdp, fmt.Sprintf("/tmp/TsAgent_%d.db", i), fmt.Sprintf("dummy-uuid-%d", i), globals.Tsm, tsmrc, state.AgentMode_MANAGED, agent.NetworkAgent)
		c.Assert(aerr, IsNil)
		if tsa == nil {
			c.Fatalf("cannot create troubleshooting agent. Err: %v", err)
		}
		log.Infof("created troubleshooting subagent")

		// create new RestServer instance. Not started yet.
		restServer, err := restapi.NewRestServer(agent.NetworkAgent, tsa.TroubleShootingAgent, "")
		c.Assert(err, IsNil)
		if restServer == nil {
			c.Fatalf("cannot create REST server . Err: %v", err)
		}
		agent.RestServer = restServer
		it.agents = append(it.agents, agent)

	}

	// REST Client
	restcl, err := apiclient.NewRestAPIClient(integTestAPIGWURL)
	if err != nil {
		c.Fatalf("cannot create REST client. Err: %v", err)
	}
	it.restClient = restcl

	// create api server client
	l := log.GetNewLogger(log.GetDefaultConfig("VeniceIntegTest"))
	apicl, err := apiclient.NewGrpcAPIClient("integ_test", globals.APIServer, l, rpckit.WithBalancer(balancer.New(rc)))
	if err != nil {
		c.Fatalf("cannot create grpc client")
	}
	it.apisrvClient = apicl
	time.Sleep(time.Millisecond * 100)

	// create tpm
	rs := resolver.New(&resolver.Config{Name: globals.Tpm, Servers: []string{resolverServer.GetListenURL()}})
	pm, err := tpm.NewPolicyManager(integTestTPMURL, rs)
	c.Assert(err, IsNil)
	it.tpm = pm

	it.userCred = &auth.PasswordCredential{
		Username: testutils.TestLocalUser,
		Password: testutils.TestLocalPassword,
		Tenant:   testutils.TestTenant,
	}
	err = testutils.SetupAuth(integTestApisrvURL, true, false, it.userCred, logger)
	c.Assert(err, IsNil)

	it.vcHub.SetUp(c, it.numAgents)
}

func (it *veniceIntegSuite) SetUpTest(c *C) {
	log.Infof("============================= %s starting ==========================", c.TestName())
}

func (it *veniceIntegSuite) TearDownTest(c *C) {
	log.Infof("============================= %s completed ==========================", c.TestName())
	// Remove persisted agent db files
	for i := 0; i < it.numAgents; i++ {
		// mock datapath
		os.Remove(fmt.Sprintf("/tmp/agent_%d.db", i))
		os.Remove(fmt.Sprintf("/tmp/TsAgent_%d.db", i))
	}

}

func (it *veniceIntegSuite) TearDownSuite(c *C) {
	// stop the agents
	for _, ag := range it.agents {
		ag.Stop()
	}
	it.agents = []*netagent.Agent{}
	it.datapaths = []*datapath.Datapath{}

	// stop server and client
	log.Infof("Stop all Test Controllers")
	it.tpm.Stop()
	it.ctrler.Stop()
	it.ctrler = nil
	it.tsCtrler.Stop()
	it.tsCtrler = nil
	it.apiSrv.Stop()
	it.apiGw.Stop()
	it.certSrv.Stop()
	it.vcHub.TearDown()
	it.resolverClient.Stop()
	it.resolverSrv.Stop()
	it.apisrvClient.Close()
}

// basic test to make sure all components come up
func (it *veniceIntegSuite) TestVeniceIntegBasic(c *C) {
	var intervals []string
	// Slightly relax AssertEventually timeouts when agent is using the hal datapath.
	if it.datapathKind == "hal" {
		intervals = []string{"30ms", "30s"}
	} else {
		intervals = []string{"10ms", "10s"}
	}

	// create a network using REST api
	nw, err := it.createNetwork("default", "default", "test", "10.1.1.0/24", "10.1.1.254")
	AssertOk(c, err, "Error creating network")

	// verify network gets created in agent
	AssertEventually(c, func() (bool, interface{}) {
		_, cerr := it.agents[0].NetworkAgent.FindNetwork(nw.ObjectMeta)
		return (cerr == nil), nil
	}, "Network not found in agent", intervals...)

	// delete the network
	_, err = it.deleteNetwork("default", "test")
	AssertOk(c, err, "Error deleting network")

	// verify network is removed from agent
	AssertEventually(c, func() (bool, interface{}) {
		_, cerr := it.agents[0].NetworkAgent.FindNetwork(nw.ObjectMeta)
		return (cerr != nil), nil
	}, "Network still found in agent", intervals...)
}

// Verify basic vchub functionality
func (it *veniceIntegSuite) TestVeniceIntegVCH(c *C) {
	// setup vchub client
	rpcClient, err := rpckit.NewRPCClient("venice_integ_test", vchTestURL, rpckit.WithRemoteServerName(globals.VCHub))
	defer rpcClient.Close()
	c.Assert(err, IsNil)

	vcHubClient := orch.NewOrchApiClient(rpcClient.ClientConn)
	// verify number of smartnics
	filter := &orch.Filter{}
	AssertEventually(c, func() (bool, interface{}) {
		nicList, err := vcHubClient.ListSmartNICs(context.Background(), filter)
		if err == nil && len(nicList.GetItems()) == it.numAgents {
			return true, nil
		}

		return false, nil
	}, "Unable to find expected snics")

	// add a nwif and verify it is seen by client.
	addReq := &simapi.NwIFSetReq{
		WLName:    "Venice-TestVM",
		IPAddr:    "22.2.2.3",
		Vlan:      "301",
		PortGroup: "userNet101",
	}

	snicMac := it.vcHub.snics[0]
	addResp, err := it.vcHub.vcSim.CreateNwIF(snicMac, addReq)
	c.Assert(err, IsNil)

	AssertEventually(c, func() (bool, interface{}) {
		nwifList, err := vcHubClient.ListNwIFs(context.Background(), filter)
		if err != nil {
			return false, nil
		}

		for _, nwif := range nwifList.GetItems() {
			s := nwif.GetStatus()
			if s.MacAddress != addResp.MacAddr || s.Network != "userNet101" || s.SmartNIC_ID != snicMac || s.WlName != "Venice-TestVM" {
				continue
			}

			return true, nil
		}

		return false, nil
	}, "Unable to find expected nwif")

	// delete and verify
	it.vcHub.vcSim.DeleteNwIF(snicMac, addResp.UUID)
	AssertEventually(c, func() (bool, interface{}) {
		nwifList, err := vcHubClient.ListNwIFs(context.Background(), filter)
		if err != nil {
			return false, nil
		}

		for _, nwif := range nwifList.GetItems() {
			s := nwif.GetStatus()
			if s.MacAddress == addResp.MacAddr {
				return false, nil
			}
		}

		return true, nil
	}, "Deleted nwif still exists")

}

// test tenant watch
func (it *veniceIntegSuite) TestTenantWatch(c *C) {
	// create watch
	client, err := it.initGrpcClient()
	AssertOk(c, err, "failed to init grpc client")
	defer client.Close()
	kvWatch, err := client.ClusterV1().Tenant().Watch(context.Background(), &api.ListWatchOptions{})
	AssertOk(c, err, "failed to watch tenants")
	tenChan := kvWatch.EventChan()
	defer kvWatch.Stop()

	for j := 0; j < 2; j++ {
		log.Infof("########################## tpm tenant test:%d ###################", j)
		tenantName := fmt.Sprintf("vpc-%d", j)
		// create a tenant
		_, err := it.createTenant(tenantName)
		AssertOk(c, err, fmt.Sprintf("failed to create tenant %s", tenantName))
		defer it.deleteTenant(tenantName)

		AssertEventually(c, func() (bool, interface{}) {
			_, err := it.getTenant(tenantName)
			return err == nil, err
		}, fmt.Sprintf("failed to find tenant %s ", tenantName))

		AssertEventually(c, func() (bool, interface{}) {
			select {
			case _, ok := <-tenChan:
				return ok, nil
			default:
				return false, nil
			}
		}, fmt.Sprintf("failed to receive watch event for tenant %s ", tenantName))
	}
}

// test tpm
func (it *veniceIntegSuite) TestTelemetryPolicyMgr(c *C) {
	tenantName := fmt.Sprintf("tenant-100")
	// create a tenant
	_, err := it.createTenant(tenantName)
	AssertOk(c, err, "Error creating tenant")

	defer it.deleteTenant(tenantName)

	AssertEventually(c, func() (bool, interface{}) {
		tn, err := it.getTenant(tenantName)
		return err == nil, tn
	}, fmt.Sprintf("failed to find tenant %s ", tenantName))

	AssertEventually(c, func() (bool, interface{}) {
		sp, err := it.getStatsPolicy(tenantName)
		if err == nil {
			Assert(c, reflect.DeepEqual(sp.GetSpec(), tpm.DefaultStatsSpec),
				fmt.Sprintf("stats spec didn't match: got %+v, expectd %+v",
					sp.GetSpec(), tpm.DefaultStatsSpec))
			return true, nil
		}
		return false, nil
	}, "failed to find stats policy")

	AssertEventually(c, func() (bool, interface{}) {
		fp, err := it.getFwlogPolicy(tenantName)
		if err == nil {
			Assert(c, reflect.DeepEqual(fp.GetSpec(), tpm.DefaultFwlogSpec),
				fmt.Sprintf("fwlog spec didn't match: got %+v, expectd %+v", fp.GetSpec(), tpm.DefaultFwlogSpec))
			return true, nil
		}
		return false, nil
	}, "failed to find fwlog policy")

	_, err = it.deleteTenant(tenantName)
	AssertOk(c, err, "Error deleting tenant")

	AssertEventually(c, func() (bool, interface{}) {
		_, err := it.getStatsPolicy(tenantName)
		return err != nil, nil

	}, "failed to delete stats policy")

	AssertEventually(c, func() (bool, interface{}) {
		_, err := it.getFwlogPolicy(tenantName)
		return err != nil, nil

	}, "failed to delete fwlog policy")
}

func (it *veniceIntegSuite) initGrpcClient() (apiclient.Services, error) {
	for i := 0; i < 3; i++ {
		rs := resolver.New(&resolver.Config{Name: globals.Tpm, Servers: []string{it.resolverSrv.GetListenURL()}})
		// create a grpc client
		client, apiErr := apiclient.NewGrpcAPIClient(globals.Cmd, globals.APIServer, log.WithContext("pkg", "TPM-IT-GRPC-API"),
			rpckit.WithBalancer(balancer.New(rs)))
		if apiErr == nil {
			return client, nil
		}
		time.Sleep(1 * time.Second)
	}
	return nil, fmt.Errorf("failed to connect to {%s}, exhausted all attempts)", globals.APIGw)
}
