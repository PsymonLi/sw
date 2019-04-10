// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package smartnic_test

import (
	"context"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"fmt"
	mathrand "math/rand"
	"os"
	"reflect"
	"strings"
	"testing"
	"time"

	"google.golang.org/grpc/grpclog"

	"github.com/pensando/sw/api"
	api_cache "github.com/pensando/sw/api/cache"
	apicache "github.com/pensando/sw/api/client"
	"github.com/pensando/sw/api/generated/apiclient"
	cmd "github.com/pensando/sw/api/generated/cluster"
	_ "github.com/pensando/sw/api/generated/exports/apiserver"
	"github.com/pensando/sw/nic/agent/nmd"
	"github.com/pensando/sw/nic/agent/nmd/platform"
	proto "github.com/pensando/sw/nic/agent/nmd/protos"
	"github.com/pensando/sw/nic/agent/nmd/upg"
	"github.com/pensando/sw/venice/apiserver"
	apiserverpkg "github.com/pensando/sw/venice/apiserver/pkg"
	cmdapi "github.com/pensando/sw/venice/cmd/apiclient"
	"github.com/pensando/sw/venice/cmd/cache"
	cmdenv "github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/grpc"
	. "github.com/pensando/sw/venice/cmd/grpc/server/smartnic"
	cmdsvc "github.com/pensando/sw/venice/cmd/services"
	"github.com/pensando/sw/venice/cmd/services/mock"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certmgr"
	"github.com/pensando/sw/venice/utils/certs"
	esmock "github.com/pensando/sw/venice/utils/elastic/mock/curator"
	perror "github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	rmock "github.com/pensando/sw/venice/utils/resolver/mock"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
	ventrace "github.com/pensando/sw/venice/utils/trace"
	"github.com/pensando/sw/venice/utils/tsdb"
)

const (
	smartNICServerURL = "localhost:" + globals.CMDSmartNICRegistrationAPIPort
	resolverURLs      = ":" + globals.CMDResolverPort
	healthInterval    = 1 * time.Second
	deadtimeInterval  = 3 * time.Second
)

var (
	logger = log.WithContext("module", "CrudOpsTest")
	// create mock events recorder
	mr = mockevtsrecorder.NewRecorder("smartnic_test", logger)
	_  = recorder.Override(mr)
)

type testInfo struct {
	l              log.Logger
	apiServerAddr  string
	apiServer      apiserver.Server
	apiClient      apiclient.Services
	rpcServer      *rpckit.RPCServer
	rpcClient      *rpckit.RPCClient
	smartNICServer *RPCServer
}

func (t testInfo) APIClient() cmd.ClusterV1Interface {
	return t.apiClient.ClusterV1()
}

var tInfo testInfo

// returns true if it is a well-formed error response from the server
func isErrorResponse(r *grpc.RegisterNICResponse) bool {
	if r == nil || r.AdmissionResponse == nil {
		return false
	}
	phase := r.AdmissionResponse.Phase
	return phase == cmd.SmartNICStatus_REJECTED.String() || phase == cmd.SmartNICStatus_UNKNOWN.String()
}

func (t testInfo) CheckNICVersionForAdmission(nicSku string, nicVersion string) (string, string) {
	return "", ""
}

// runRPCServer creates a smartNIC server for SmartNIC service.
func createRPCServer(url, certFile, keyFile, caFile string) (*rpckit.RPCServer, error) {
	var err error

	// set cmd logger, statemgr, certmgr & quorum nodes
	cmdenv.Logger = tInfo.l
	cmdenv.QuorumNodes = []string{"localhost"}
	cmdenv.StateMgr = cache.NewStatemgr()
	cmdenv.CertMgr, err = certmgr.NewTestCertificateMgr("smartnic-test")
	if err != nil {
		return nil, fmt.Errorf("Error creating CertMgr instance: %v", err)
	}

	l := mock.NewLeaderService("testMaster")
	cmdenv.LeaderService = l
	l.Start()

	// Start CMD config watcher
	s := cmdsvc.NewSystemdService(cmdsvc.WithSysIfSystemdSvcOption(&mock.SystemdIf{}))
	cw := cmdapi.NewCfgWatcherService(tInfo.l, tInfo.apiServerAddr, cmdenv.StateMgr)
	cmdenv.MasterService = cmdsvc.NewMasterService(
		cmdsvc.WithLeaderSvcMasterOption(l),
		cmdsvc.WithSystemdSvcMasterOption(s),
		cmdsvc.WithConfigsMasterOption(&mock.Configs{}),
		cmdsvc.WithCfgWatcherMasterOption(cw),
		cmdsvc.WithElasticCuratorSvcrOption(esmock.NewMockCurator()))
	cw.Start()

	// create an RPC server.
	rpcServer, err := rpckit.NewRPCServer("smartNIC", url)
	if err != nil {
		fmt.Printf("Error creating RPC-server: %v", err)
		return nil, err
	}
	tInfo.rpcServer = rpcServer
	cmdenv.UnauthRPCServer = rpcServer

	// create and register the RPC handler for SmartNIC service
	tInfo.smartNICServer, err = NewRPCServer(tInfo, healthInterval, deadtimeInterval, getRESTPort(1), cmdenv.StateMgr, tInfo)
	if err != nil {
		fmt.Printf("Error creating SmartNIC RPC server: %v", err)
		return nil, err
	}

	// Register self as rpc handler for both NIC registration and watch/updates on the server.
	// In reality, CMD uses two different servers because watches and updates APIs are
	// exposed over TLS, whereas NIC registration is not.
	grpc.RegisterSmartNICRegistrationServer(rpcServer.GrpcServer, tInfo.smartNICServer)
	grpc.RegisterSmartNICUpdatesServer(rpcServer.GrpcServer, tInfo.smartNICServer)
	rpcServer.Start()
	cmdenv.NICService = tInfo.smartNICServer

	// Launch go routine to monitor health updates of smartNIC objects and update status
	go func() {
		tInfo.smartNICServer.MonitorHealth()
	}()

	return rpcServer, nil
}

// createHost creates the Host object based on object meta
func createHost(objName string, smartNICIDs ...cmd.SmartNICID) (*cmd.Host, error) {
	host := &cmd.Host{
		ObjectMeta: api.ObjectMeta{
			Name: objName,
		},
		Spec: cmd.HostSpec{
			SmartNICs: smartNICIDs,
		},
	}
	host.Defaults("v1")
	host.TypeMeta = api.TypeMeta{Kind: "Host"}

	hostObj, err := tInfo.apiClient.ClusterV1().Host().Create(context.Background(), host)
	if err != nil || hostObj == nil {
		return nil, fmt.Errorf("Host create failed, host: %+v error: %+v", host, err)
	}

	return hostObj, nil
}

// deleteHost deletes the Host object based on object meta name
func deleteHost(om api.ObjectMeta) error {
	_, err := tInfo.apiClient.ClusterV1().Host().Delete(context.Background(), &om)
	if err != nil {
		log.Errorf("Error deleting Host object name:%s err: %v", om.Name, err)
		return err
	}

	return nil
}

// getHost fetches the Host object based on object meta
func getHost(om api.ObjectMeta) (*cmd.Host, error) {
	hostObj, err := tInfo.apiClient.ClusterV1().Host().Get(context.Background(), &om)
	if err != nil || hostObj == nil {
		return nil, perror.NewNotFound("Host", om.Name)
	}

	return hostObj, nil
}

// createRPCServerClient creates rpc client and server for SmartNIC service
func createRPCServerClient() (*rpckit.RPCServer, *rpckit.RPCClient) {

	// start the rpc server
	rpcServer, err := createRPCServer(smartNICServerURL, "", "", "")
	if err != nil {
		fmt.Printf("Error connecting to grpc server. Err: %v", err)
		return nil, nil
	}

	// create a grpc client
	rpcClient, err := rpckit.NewRPCClient("smartNIC", smartNICServerURL)
	if err != nil {
		fmt.Printf("Error connecting to grpc server. Err: %v", err)
		rpcServer.Stop()
		return nil, nil
	}

	return rpcServer, rpcClient
}

// Create NMD and Agent
func createNMD(t *testing.T, dbPath, priMac, restURL string) (*nmd.Agent, error) {
	CreateFruJSON(priMac)

	// create a platform agent
	pa, err := platform.NewNaplesPlatformAgent()
	if err != nil {
		log.Fatalf("Error creating platform agent. Err: %v", err)
	}

	uc, err := upg.NewNaplesUpgradeClient(nil)
	if err != nil {
		log.Fatalf("Error creating Upgrade client . Err: %v", err)
	}

	r := resolver.New(&resolver.Config{Name: t.Name(), Servers: strings.Split(resolverURLs, ",")})
	// create the new NMD
	ag, err := nmd.NewAgent(pa,
		uc,
		dbPath,
		priMac,
		priMac,
		smartNICServerURL,
		restURL,
		"", // no local certs endpoint
		"", // no remote certs endpoint
		"host",
		globals.NicRegIntvl*time.Second,
		globals.NicUpdIntvl*time.Second,
		r)
	if err != nil {
		t.Errorf("Error creating NMD. Err: %v", err)
	}

	// Fake IPConfig
	ipConfig := &cmd.IPConfig{
		IPAddress: "1.2.3.4",
	}

	// Ensure the NMD's rest server is started
	nmdHandle := ag.GetNMD()
	nmdHandle.CreateMockIPClient(nil)

	cfg := nmdHandle.GetNaplesConfig()
	cfg.Spec.Controllers = []string{"localhost"}
	cfg.Spec.NetworkMode = proto.NetworkMode_INBAND.String()
	cfg.Spec.IPConfig = ipConfig
	cfg.Spec.MgmtVlan = 0
	cfg.Spec.Hostname = priMac

	nmdHandle.SetNaplesConfig(cfg.Spec)
	nmdHandle.IPClient.Update()

	return ag, err
}

// stopAgent stops NMD server and deletes emDB file
func stopNMD(t *testing.T, ag *nmd.Agent, dbPath string) {

	ag.Stop()
	err := os.Remove(dbPath)
	if err != nil {
		log.Errorf("Error deleting emDB file: %s, err: %v", dbPath, err)
	}
}

func getHostID(index int) string {
	return fmt.Sprintf("4444.4444.%02x%02x", index/256, index%256)
}

func getRESTPort(index int) string {
	return fmt.Sprintf("%d", 15000+index)
}

func getRESTUrl(index int) string {
	return fmt.Sprintf("localhost:%s", getRESTPort(index))
}

func getDBPath(index int) string {
	return fmt.Sprintf("/tmp/nmd-%d.db", index)
}

func verifySmartNICObj(t *testing.T, name string, exists bool, phase, host string) {
	var nicObj *cmd.SmartNIC
	var err error
	ometa := api.ObjectMeta{Name: name}

	f := func() (bool, interface{}) {
		nicObj, err = tInfo.smartNICServer.GetSmartNIC(ometa)
		if exists {
			if err != nil {
				log.Errorf("Error getting NIC object: %s", name)
				return false, nil
			}
			if nicObj.Status.AdmissionPhase != phase {
				log.Errorf("Got incorrect smartNIC object phase, expected: %s obtained: %s",
					phase, nicObj.Status.AdmissionPhase)
				return false, nil
			}
			if nicObj.Status.Host != host {
				log.Errorf("Got incorrect smartNIC object host, expected: %s obtained: %s",
					host, nicObj.Status.Host)
			}
			return true, nil
		} else {
			if err != nil || nicObj == nil {
				return true, nil
			}
			log.Errorf("SmartNIC object should not exist for name: %s err: %v, obj: %+v", name, err, nicObj)
			return false, nil
		}
	}
	AssertEventually(t, f, fmt.Sprintf("Failed to verify smartNIC object, name: %v, presence: %v, phase: %v", name, exists, phase), "50ms", "20s")

	// Cross-check the local (cached) copy
	var nicState *cache.SmartNICState
	g := func() (bool, interface{}) {
		nicState, err = cmdenv.StateMgr.FindSmartNIC("", name)
		if nicObj != nil {
			if nicState == nil {
				log.Infof("Did not find cached object for nic %s: %+v", name, nicObj)
				return false, nil
			}
			nicObj, err = tInfo.smartNICServer.GetSmartNIC(ometa)
			AssertOk(t, err, "Error reading ApiServer object")
			eq := reflect.DeepEqual(nicObj, nicState.SmartNIC)
			if !eq {
				log.Infof("ApiServer object does not match local cached object. ApiServer: %+v, Cache: %+v", nicObj, nicState.SmartNIC)
				return false, nil
			}
			return true, nil
		} else {
			if nicState != nil {
				log.Infof("Found stale cached object for nic %s: %+v", name, nicState)
				return false, nil
			}
			return true, nil
		}
	}
	AssertEventually(t, g, fmt.Sprintf("ApiServer/StateMgr mismatch:\n%+v\n%+v\n", nicObj, nicState), "50ms", "20s")
}

func verifyHostObj(t *testing.T, hostName, nicName string) {
	f := func() (bool, interface{}) {
		ometa := api.ObjectMeta{Name: hostName}
		hostObj, err := getHost(ometa)
		if err != nil {
			log.Errorf("Error getting Host object: %s", hostName)
			return false, nil
		}
		log.Infof("\nExpected NIC name: %s, host: %+v \n", nicName, hostObj)
		if nicName == "" && len(hostObj.Status.AdmittedSmartNICs) == 0 {
			return true, nil
		}
		if len(hostObj.Status.AdmittedSmartNICs) > 1 {
			log.Fatalf("Each Host can refer to at most 1 SmartNIC, found: %+v", hostObj)
		}
		for ii := range hostObj.Status.AdmittedSmartNICs {
			if hostObj.Status.AdmittedSmartNICs[ii] == nicName {
				return true, nil
			}
		}
		return false, nil
	}
	AssertEventually(t, f, fmt.Sprintf("Failed to verify Host object %s", hostName), "50ms", "20s")
}

func verifyWatchAPIIsInvoked(t *testing.T, client grpc.SmartNICUpdatesClient, name, phase string) {
	stream, err := client.WatchNICs(context.Background(), &api.ObjectMeta{Name: name})
	AssertOk(t, err, fmt.Sprintf("Error watching smartNIC, name: %s, err: %v", name, err))

	evt, err := stream.Recv()
	AssertOk(t, err, fmt.Sprintf("Error receiving from stream, mac: %s err: %v", name, err))
	Assert(t, evt.Nic.Name == name, fmt.Sprintf("Got incorrect smartNIC watch event: expected name: %s, actual name: %s", name, evt.Nic.Name))
	Assert(t, evt.Nic.Status.AdmissionPhase == phase, fmt.Sprintf("Got incorrect smartNIC watch event: expected phase: %s actual phase: %v", phase, evt.Nic.Status.AdmissionPhase))
}

func validateNICSpecConflictEvent(events *[]mockevtsrecorder.Event, nic, host1, host2 string) bool {
	for _, ev := range *events {
		if ev.EventType == cmd.HostSmartNICSpecConflict {
			msg := ev.Message
			return strings.Contains(msg, nic) && strings.Contains(msg, host1) && strings.Contains(msg, host2)
		}
	}
	return false
}

// Set cluster admission policy to auto-admit or manual
func setClusterAutoAdmitNICs(t *testing.T, autoAdmit bool) {
	refObj, err := tInfo.smartNICServer.GetCluster()
	AssertOk(t, err, "Error getting Cluster object")

	clRef := cmd.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name:            "testCluster",
			ResourceVersion: refObj.ObjectMeta.ResourceVersion,
		},
		Spec: cmd.ClusterSpec{
			AutoAdmitNICs: autoAdmit,
		},
	}
	clObj, err := tInfo.apiClient.ClusterV1().Cluster().Update(context.Background(), &clRef)
	Assert(t, err == nil && clObj != nil, fmt.Sprintf("Error updating cluster auto-admit status, response: %+v", clObj))
}

// setSmartNICAdmit sets the "Admit" attribute on an existing SmartNICObj
func setSmartNICAdmit(t *testing.T, meta *api.ObjectMeta, admit bool) {
	f := func() (bool, interface{}) {
		nicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), meta)
		nicObj.Spec.Admit = admit
		AssertOk(t, err, "Error getting NIC object")

		resp, err := tInfo.apiClient.ClusterV1().SmartNIC().Update(context.Background(), nicObj)
		if err != nil {
			log.Errorf("Failed to set Admit on NIC, name: %s req: %+v resp: %+v, err: %v", meta.Name, nicObj, resp, err)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f, fmt.Sprintf("Failed to set Admit for smartNIC %s", meta.Name))
}

// setSmartNICHostname sets the "Hostname" attribute on an existing SmartNIC object
func setSmartNICHostname(t *testing.T, meta *api.ObjectMeta, hostname string) {
	f := func() (bool, interface{}) {
		nicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), meta)
		nicObj.Spec.Hostname = hostname
		AssertOk(t, err, "Error getting NIC object")

		resp, err := tInfo.apiClient.ClusterV1().SmartNIC().Update(context.Background(), nicObj)
		if err != nil {
			log.Errorf("Failed to set Hostname on NIC, name: %s req: %+v resp: %+v, err: %v", meta.Name, nicObj, resp, err)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f, fmt.Sprintf("Failed to set Hostname for smartNIC %s", meta.Name))
}

// setHostSmartNICIDs sets the SmartNICs attribute on an existing Host object
func setHostSmartNICIDs(t *testing.T, meta *api.ObjectMeta, smartnics ...cmd.SmartNICID) {
	f := func() (bool, interface{}) {
		hostObj, err := tInfo.apiClient.ClusterV1().Host().Get(context.Background(), meta)
		hostObj.Spec.SmartNICs = smartnics
		AssertOk(t, err, "Error getting Host object")

		resp, err := tInfo.apiClient.ClusterV1().Host().Update(context.Background(), hostObj)
		if err != nil {
			log.Errorf("Failed to set SmartNIC IDs on Host, name: %s req: %+v resp: %+v, err: %v", meta.Name, hostObj, resp, err)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f, fmt.Sprintf("Failed to set SmartNIC IDs for Host %s", meta.Name))
}

// doPhase1Exchange performs the first phase of the admission sequence.
// It forms the admission request, send it to the server and receive
// the response with the challenge.
// It returns the original request that was send (containing the NIC object) and
// if the request is accepted by the server, it returns the challenge response to be sent in Phase 2,
// otherwise it returns the error message sent by the server
func doPhase1Exchange(t *testing.T, stream grpc.SmartNICRegistration_RegisterNICClient, mac, nicHostname string, validCert, expectChallenge bool) (*grpc.NICAdmissionRequest, *grpc.AuthenticationResponse, *grpc.NICAdmissionResponse) {
	// create a platform agent and NMD instance to access factory key/cert
	pa, err := platform.NewNaplesPlatformAgent()
	AssertOk(t, err, "Error creating platform agent. Err: %v", err)

	// Construct SmartNIC object with valid cert/signature
	ometa := api.ObjectMeta{Name: mac}
	nic := cmd.SmartNIC{
		TypeMeta:   api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: ometa,
		Spec: cmd.SmartNICSpec{
			Hostname: nicHostname,
			IPConfig: &cmd.IPConfig{
				IPAddress: "0.0.0.0/0",
			},
			MgmtMode:    cmd.SmartNICSpec_NETWORK.String(),
			NetworkMode: cmd.SmartNICSpec_OOB.String(),
		},
		Status: cmd.SmartNICStatus{
			AdmissionPhase: "UNKNOWN",
			SerialNum:      "TestNIC",
			PrimaryMAC:     mac,
		},
	}

	cert := []byte("invalid")
	if validCert {
		cert, err = pa.GetPlatformCertificate(&nic)
		if err != nil {
			log.Fatalf("Error getting platform cert. NIC: %+v, error: %v", nic, err)
		}
	}

	// Create cluster cert key and CSR
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	AssertOk(t, err, "Error creating private key")
	csr, err := certs.CreateCSR(key, nil, []string{mac}, nil)
	AssertOk(t, err, "Error creating CSR")

	admReq := &grpc.RegisterNICRequest{
		AdmissionRequest: &grpc.NICAdmissionRequest{
			Nic:                    nic,
			Cert:                   cert,
			ClusterCertSignRequest: csr.Raw,
		},
	}
	err = stream.Send(admReq)
	AssertOk(t, err, "Error sending request")

	log.Infof("doPhase1Exchange NIC: %s, expectChallenge: %v", mac, expectChallenge)
	if expectChallenge {
		// receive the authentication request
		authReq, err := stream.Recv()
		AssertOk(t, err, "Error receiving response")
		Assert(t, authReq.AuthenticationRequest != nil, "expected auth request")

		// prepare response
		signer, err := pa.GetPlatformSigner(&nic)
		AssertOk(t, err, "Error getting platform signer")
		claimantRandom, challengeResponse, err := certs.GeneratePoPChallengeResponse(signer, authReq.AuthenticationRequest.Challenge)
		AssertOk(t, err, "Error getting challenge response")

		authResp := &grpc.RegisterNICRequest{
			AuthenticationResponse: &grpc.AuthenticationResponse{
				ClaimantRandom:    claimantRandom,
				ChallengeResponse: challengeResponse,
			},
		}
		return admReq.AdmissionRequest, authResp.AuthenticationResponse, nil
	} else {
		// receive the error response
		errResp, err := stream.Recv()
		AssertOk(t, err, "Error receiving response")
		Assert(t, errResp.AdmissionResponse != nil, "expected auth request")
		return admReq.AdmissionRequest, nil, errResp.AdmissionResponse
	}
}

// doRegisterNIC is a utility function that performs registration for the specified NIC
// and returns the phase. It aborts in case of error.
func doRegisterNIC(t *testing.T, client grpc.SmartNICRegistrationClient, mac, nicHostname string) *grpc.NICAdmissionResponse {
	stream, err := client.RegisterNIC(context.Background())
	AssertOk(t, err, "Error creating stream")
	_, challengeResp, errResp := doPhase1Exchange(t, stream, mac, nicHostname, true, true)
	Assert(t, errResp == nil, fmt.Sprintf("Server returned unexpected error: %+v", err))
	err = stream.Send(&grpc.RegisterNICRequest{AuthenticationResponse: challengeResp})
	AssertOk(t, err, "Error sending request")
	admResp, err := stream.Recv()
	AssertOk(t, err, "Error receiving response")
	return admResp.AdmissionResponse
}

// TestRegisterSmartNICByNaples tests RegisterNIC
// functionality initiated by User
func TestRegisterSmartNICByNaples(t *testing.T) {

	// Init required components
	testSetup()
	defer testTeardown()

	testCases := []struct {
		name      string
		mac       string
		autoAdmit bool
		validCert bool
		expected  string
		condition cmd.SmartNICCondition
		hostName  string
	}{
		{
			"TestAutoAdmitValidNIC",
			"4444.4444.0001",
			true,
			true,
			cmd.SmartNICStatus_ADMITTED.String(),
			cmd.SmartNICCondition{
				Type:               cmd.SmartNICCondition_HEALTHY.String(),
				Status:             cmd.ConditionStatus_TRUE.String(),
				LastTransitionTime: time.Now().UTC().Format(time.RFC3339),
			},
			"esx-1",
		},
		{
			"TestAutoAdmitInvalidNIC",
			"4444.4444.0002",
			true,
			false,
			cmd.SmartNICStatus_REJECTED.String(),
			cmd.SmartNICCondition{},
			"esx-2",
		},
		{
			"TestManualValidNIC",
			"4444.4444.0003",
			false,
			true,
			cmd.SmartNICStatus_PENDING.String(),
			cmd.SmartNICCondition{
				Type:               cmd.SmartNICCondition_HEALTHY.String(),
				Status:             cmd.ConditionStatus_FALSE.String(),
				LastTransitionTime: time.Now().UTC().Format(time.RFC3339),
			},
			"esx-3",
		},
		{
			"TestManualValidApprovedNIC",
			"4444.4444.0004",
			false,
			true,
			cmd.SmartNICStatus_PENDING.String(),
			cmd.SmartNICCondition{
				Type:               cmd.SmartNICCondition_HEALTHY.String(),
				Status:             cmd.ConditionStatus_TRUE.String(),
				LastTransitionTime: time.Now().UTC().Format(time.RFC3339),
			},
			"esx-4",
		},
		{
			"TestManualInvalidNIC",
			"4444.4444.0005",
			false,
			false,
			cmd.SmartNICStatus_REJECTED.String(),
			cmd.SmartNICCondition{},
			"esx-5",
		},
	}

	// Pre-create some Hosts to simulate the case of prior
	// Host creation by Orchestrator (VC-hub)
	for ii := range testCases {
		host, err := createHost(testCases[ii].hostName, cmd.SmartNICID{MACAddress: testCases[ii].mac})
		AssertOk(t, err, "Error creating Host object")
		defer deleteHost(host.ObjectMeta)
	}

	// Execute the testcases
	for ii, tc := range testCases {

		t.Run(tc.name, func(t *testing.T) {

			// create API clients
			smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
			smartNICUpdatesRPCClient := grpc.NewSmartNICUpdatesClient(tInfo.rpcClient.ClientConn)

			setClusterAutoAdmitNICs(t, tc.autoAdmit)

			// register NIC call
			stream, err := smartNICRegistrationRPCClient.RegisterNIC(context.Background())
			AssertOk(t, err, "Error creating stream")

			expectChallenge := tc.expected == cmd.SmartNICStatus_ADMITTED.String() || tc.expected == cmd.SmartNICStatus_PENDING.String()
			admReq, challengeResp, errResp := doPhase1Exchange(t, stream, tc.mac, tc.hostName, tc.validCert, expectChallenge)
			nic := admReq.Nic
			ometa := nic.ObjectMeta

			if !expectChallenge {
				Assert(t, errResp != nil, "Server did not return expected error")
				verifySmartNICObj(t, tc.mac, false, "", "")
				return
			}

			// respond
			Assert(t, errResp == nil, fmt.Sprintf("Server returned unexpected error: %+v", err))
			err = stream.Send(&grpc.RegisterNICRequest{AuthenticationResponse: challengeResp})
			AssertOk(t, err, "Error sending request")

			// receive the admission response
			admResp, err := stream.Recv()
			AssertOk(t, err, "Error receiving response")

			t.Logf("Testcase: %s MAC: %s expected: %v obtained: %v err: %v", tc.name, tc.mac, tc.expected, admResp, err)

			AssertOk(t, err, "Error registering NIC")

			// verify smartNIC is created
			verifySmartNICObj(t, tc.mac, true, tc.expected, testCases[ii].hostName)

			var hostStatusAdmittedSmartNICs string
			if tc.expected == cmd.SmartNICStatus_ADMITTED.String() {
				hostStatusAdmittedSmartNICs = tc.mac
			}

			// verify Host object is created and populated
			verifyHostObj(t, tc.hostName, hostStatusAdmittedSmartNICs)

			// verify watch api is invoked
			verifyWatchAPIIsInvoked(t, smartNICUpdatesRPCClient, tc.mac, tc.expected)

			// Verify UpdateNIC RPC
			f3 := func() (bool, interface{}) {
				ometa = api.ObjectMeta{Name: tc.mac}
				nic = cmd.SmartNIC{
					TypeMeta:   api.TypeMeta{Kind: "SmartNIC"},
					ObjectMeta: ometa,
					Status: cmd.SmartNICStatus{
						AdmissionPhase: tc.expected,
						Conditions: []cmd.SmartNICCondition{
							tc.condition,
						},
					},
				}

				req := &grpc.UpdateNICRequest{
					Nic: nic,
				}

				_, err := smartNICUpdatesRPCClient.UpdateNIC(context.Background(), req)
				if err != nil {
					t.Logf("Testcase: %s Failed to update NIC, mac: %s req: %+v err: %+v", tc.name, tc.mac, req.Nic, err)
					return false, nil
				}

				nicObj, err := tInfo.smartNICServer.GetSmartNIC(ometa)
				AssertOk(t, err, fmt.Sprintf("Error getting NIC object for mac:%s", tc.mac))

				if nicObj.Status.Conditions[0].Type != tc.condition.Type || nicObj.Status.Conditions[0].Status != tc.condition.Status {
					t.Logf("Testcase: %s,  Condition expected:\n%+v\nobtained:%+v", tc.name, tc.condition, nicObj.Status.Conditions[0])
					return false, nil
				}

				return true, nil
			}
			AssertEventually(t, f3, fmt.Sprintf("Failed to verify update for smartNIC"))

			// Verify NIC health status goes to UNKNOWN after deadtimeInterval
			f4 := func() (bool, interface{}) {
				nicObj, err := tInfo.smartNICServer.GetSmartNIC(ometa)
				AssertOk(t, err, fmt.Sprintf("Error getting NIC object for mac:%s", tc.mac))
				Assert(t, nicObj.ObjectMeta.Name == tc.mac,
					fmt.Sprintf("Got incorrect smartNIC object, expected: %s obtained: %s", nicObj.ObjectMeta.Name, tc.mac))
				if nicObj.Status.AdmissionPhase == cmd.SmartNICStatus_ADMITTED.String() &&
					(len(nicObj.Status.Conditions) == 0 || nicObj.Status.Conditions[0].Type != tc.condition.Type || nicObj.Status.Conditions[0].Status != cmd.ConditionStatus_UNKNOWN.String()) {
					t.Logf("Testcase: %s, Phase: %s, Condition expected:\n%+v\nobtained:%+v", tc.name, nicObj.Status.AdmissionPhase, cmd.ConditionStatus_UNKNOWN, nicObj.Status.Conditions)
					return false, nil
				}
				return true, nil
			}
			AssertEventually(t, f4, fmt.Sprintf("Failed to verify NIC health status going to UNKNOWN"))

			// Verify Host object has its status updated with list of registered NICs
			verifyHostObj(t, tc.hostName, hostStatusAdmittedSmartNICs)

			// Verify Deletion of SmartNIC object
			f5 := func() (bool, interface{}) {
				ometa = api.ObjectMeta{Name: tc.mac}
				err = tInfo.smartNICServer.DeleteSmartNIC(ometa)
				if err != nil {
					return false, nil
				}
				return true, nil
			}
			AssertEventually(t, f5, fmt.Sprintf("Failed to verify deletion of smartNIC object"))

			// Verify Deletion of Host object
			f6 := func() (bool, interface{}) {
				ometa = api.ObjectMeta{Name: tc.hostName}
				err = deleteHost(ometa)
				if err != nil {
					return false, nil
				}
				return true, nil
			}
			AssertEventually(t, f6, fmt.Sprintf("Failed to verify deletion of Host object"))
		})
	}
}

// TestRegisterSmartNICTimeouts tests that RegisterNIC server times out properly if client is stuck
func TestRegisterSmartNICTimeouts(t *testing.T) {

	// Init required components
	testSetup()
	defer testTeardown()

	baseMac := "4444.4444.00"

	// set server-side timeout to a small value to speed-up tests
	srvTimeout := 1000
	SetNICRegTimeout(time.Duration(srvTimeout) * time.Millisecond)

	for i := 0; i < 50; i++ {
		mac := fmt.Sprintf("%s%02d", baseMac, i)
		hostName := fmt.Sprintf("esxt-%d", i)
		host, err := createHost(hostName, cmd.SmartNICID{MACAddress: mac})
		AssertOk(t, err, "Error creating Host object")
		defer deleteHost(host.ObjectMeta)

		// create API client
		smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
		// register NIC call
		stream, err := smartNICRegistrationRPCClient.RegisterNIC(context.Background())
		AssertOk(t, err, "Error creating stream")

		minSleep := 400 // 200ms margin to avoid cases where sleep time and timout are too close
		sleepTimeDelta := mathrand.Intn(srvTimeout-minSleep) + minSleep
		expectSuccess := (sleepTimeDelta % 2) == 0
		var sleepTime time.Duration
		if expectSuccess {
			sleepTime = time.Duration(srvTimeout-sleepTimeDelta) * time.Millisecond
		} else {
			sleepTime = time.Duration(srvTimeout+sleepTimeDelta) * time.Millisecond
		}
		log.Infof("NIC %s, sleepTime: %v, expectSuccess: %v", mac, sleepTime, expectSuccess)

		_, challengeResp, errResp := doPhase1Exchange(t, stream, host.Spec.SmartNICs[0].MACAddress, "", true, true)
		Assert(t, errResp == nil, fmt.Sprintf("Server returned unexpected error: %+v", err))

		time.Sleep(sleepTime)

		// Server-side failure can be detected either by Send() or Recv()
		err = stream.Send(&grpc.RegisterNICRequest{AuthenticationResponse: challengeResp})
		if !expectSuccess && err != nil {
			// failed as expected
			verifySmartNICObj(t, mac, false, "", "")
			continue
		}

		_, err = stream.Recv()
		if !expectSuccess {
			Assert(t, err != nil, fmt.Sprintf("request did not fail as expected: sleeptime: %v, err %v", sleepTime, err))
			verifySmartNICObj(t, mac, false, "", "")
			continue
		}

		// verify smartNIC is created
		verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostName)

		err = tInfo.smartNICServer.DeleteSmartNIC(api.ObjectMeta{Name: mac})
		AssertOk(t, err, "Error deleting NIC")
	}
}

func TestRegisterSmartNICProtocolErrors(t *testing.T) {

	// Init required components
	testSetup()
	defer testTeardown()

	nicHostname := "4444.4444.0001"
	mac := "4444.4444.0001"

	// cluster certificate key and csr
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	AssertOk(t, err, "Error creating private key")
	csr, err := certs.CreateCSR(key, nil, []string{nicHostname}, nil)
	AssertOk(t, err, "Error creating CSR")

	// Construct SmartNIC object with valid cert/signature
	ometa := api.ObjectMeta{Name: mac}
	nic := cmd.SmartNIC{
		TypeMeta:   api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: ometa,
		Spec: cmd.SmartNICSpec{
			Hostname: nicHostname,
			IPConfig: &cmd.IPConfig{
				IPAddress: "0.0.0.0/0",
			},
			MgmtMode:    cmd.SmartNICSpec_NETWORK.String(),
			NetworkMode: cmd.SmartNICSpec_OOB.String(),
		},
		Status: cmd.SmartNICStatus{
			AdmissionPhase: cmd.SmartNICStatus_REGISTERING.String(),
			SerialNum:      "TestNIC",
			PrimaryMAC:     mac,
		},
	}

	// create a platform agent and NMD instance to access factory key/cert
	pa, err := platform.NewNaplesPlatformAgent()
	AssertOk(t, err, "Error creating platform agent. Err: %v", err)

	// platform key and certificate
	cert, err := pa.GetPlatformCertificate(&nic)
	AssertOk(t, err, "Error getting platform cert")

	// create API client
	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)

	invalidAdmReqs := []grpc.RegisterNICRequest{
		{},
		{
			AdmissionRequest: &grpc.NICAdmissionRequest{
				Nic:                    cmd.SmartNIC{},
				Cert:                   cert,
				ClusterCertSignRequest: csr.Raw,
			},
		},
		{
			AdmissionRequest: &grpc.NICAdmissionRequest{
				Nic:                    nic,
				Cert:                   nil,
				ClusterCertSignRequest: csr.Raw,
			},
		},
		{
			AdmissionRequest: &grpc.NICAdmissionRequest{
				Nic:                    nic,
				Cert:                   cert,
				ClusterCertSignRequest: nil,
			},
		},
	}

	for i, req := range invalidAdmReqs {
		stream, err := smartNICRegistrationRPCClient.RegisterNIC(context.Background())
		AssertOk(t, err, "Error creating stream")
		err = stream.Send(&req)
		AssertOk(t, err, "Error sending request")
		resp, err := stream.Recv()
		AssertOk(t, err, "Error receiving response")
		Assert(t, resp.AuthenticationRequest == nil, "Authentication request despite invalid admission request %v: %v", i, resp)
		Assert(t, isErrorResponse(resp), fmt.Sprintf("No error with invalid request %v: %v", i, resp))
		verifySmartNICObj(t, mac, false, "", "")
	}

	// now test protocols errors in Phase 2
	invalidChallengeResp := []grpc.RegisterNICRequest{
		{},
		{
			AuthenticationResponse: nil,
		},
		{
			AuthenticationResponse: &grpc.AuthenticationResponse{},
		},
		{
			AuthenticationResponse: &grpc.AuthenticationResponse{
				ChallengeResponse: []byte("invalid"),
			},
		},
	}

	for i, chResp := range invalidChallengeResp {
		stream, err := smartNICRegistrationRPCClient.RegisterNIC(context.Background())
		AssertOk(t, err, "Error creating stream")
		_, _, errResp := doPhase1Exchange(t, stream, mac, nicHostname, true, true)
		Assert(t, errResp == nil, fmt.Sprintf("Server returned unexpected error, index: %d, resp: %+v", i, errResp))
		err = stream.Send(&chResp)
		AssertOk(t, err, "Error sending request")
		resp, err := stream.Recv()
		AssertOk(t, err, "Error receiving response")
		Assert(t, isErrorResponse(resp), fmt.Sprintf("Unexpected positive response, index: %d, resp: %+v", i, resp))
		verifySmartNICObj(t, mac, false, "", "")
	}
}

func TestUpdateSmartNIC(t *testing.T) {

	// Init required components
	testSetup()
	defer testTeardown()

	// Verify create nic
	nic := cmd.SmartNIC{
		TypeMeta:   api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: api.ObjectMeta{Name: "2222.2222.2222"},
		Spec: cmd.SmartNICSpec{
			IPConfig: &cmd.IPConfig{
				IPAddress: "10.1.1.1/24",
			},
			MgmtMode:    cmd.SmartNICSpec_NETWORK.String(),
			NetworkMode: cmd.SmartNICSpec_OOB.String(),
		},
		Status: cmd.SmartNICStatus{
			AdmissionPhase: "UNKNOWN",
		},
	}
	nicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Create(context.Background(), &nic)
	AssertOk(t, err, "Failed to create nic object, 2222.2222.222")

	// Verify update nic
	nic = cmd.SmartNIC{
		TypeMeta:   nicObj.TypeMeta,
		ObjectMeta: nicObj.ObjectMeta,
		Status: cmd.SmartNICStatus{
			AdmissionPhase: nicObj.Status.AdmissionPhase,
			Conditions: []cmd.SmartNICCondition{
				{
					Type:   cmd.SmartNICCondition_HEALTHY.String(),
					Status: cmd.ConditionStatus_FALSE.String(),
				},
			},
		},
	}
	nicObj, err = tInfo.smartNICServer.UpdateSmartNIC(&nic)
	AssertOk(t, err, "Failed to update nic object, 2222.2222.222")

	// delete nic
	err = tInfo.smartNICServer.DeleteSmartNIC(nic.ObjectMeta)
	AssertOk(t, err, "Failed to delete nic object, 2222.2222.222")
}

func TestDeleteSmartNIC(t *testing.T) {

	testSetup()
	defer testTeardown()

	// Verify Delete SmartNIC object
	ometa := api.ObjectMeta{Name: "1111.1111.1111"}
	err := tInfo.smartNICServer.DeleteSmartNIC(ometa)
	Assert(t, err != nil, "SmartNIC object - 1111.1111.1111 - should not exist")
}

// TestSmartNICConfigByUser tests the following scenario & actions
// - SmartNIC config by user with Mgmt-IP
// - CMD would post naples config via REST to initiate nic registration
//   in managed mode
// - NMD would next do NIC registration
// - CMD would validate NIC and admit NIC into cluster.
func TestSmartNICConfigByUser(t *testing.T) {
	// Init required components
	testSetup()
	defer testTeardown()

	hostID := getHostID(1)
	dbPath := getDBPath(1)
	restURL := getRESTUrl(1)
	testMac := "4444.4444.4401"

	// Cleanup any prior DB files
	os.Remove(dbPath)

	// create Agent and NMD
	ag, err := createNMD(t, dbPath, testMac, restURL)
	defer stopNMD(t, ag, dbPath)
	Assert(t, (err == nil && ag != nil), "Failed to create agent", err)

	nm := ag.GetNMD()

	// Validate default host mode
	f1 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		if cfg.Spec.Mode == proto.MgmtMode_HOST.String() && nm.GetListenURL() != "" &&
			nm.GetUpdStatus() == false && nm.GetRegStatus() == false && nm.GetRestServerStatus() == true {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f1, "Failed to verify mode is in host")

	// Create Host in Venice
	hostName := "esxc"
	host, err := createHost(hostName, cmd.SmartNICID{MACAddress: testMac})
	AssertOk(t, err, "Error creating Host object")
	defer deleteHost(host.ObjectMeta)

	// Create SmartNIC object in Venice
	nic := cmd.SmartNIC{
		TypeMeta: api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: api.ObjectMeta{
			Name: testMac,
		},
		Spec: cmd.SmartNICSpec{
			Admit: true,
			IPConfig: &cmd.IPConfig{
				IPAddress: "127.0.0.1/32",
			},
			Hostname:    hostID,
			MgmtMode:    cmd.SmartNICSpec_NETWORK.String(),
			NetworkMode: cmd.SmartNICSpec_OOB.String(),
		},
		Status: cmd.SmartNICStatus{
			AdmissionPhase: "UNKNOWN",
		},
	}

	_, err = tInfo.apiClient.ClusterV1().SmartNIC().Create(context.Background(), &nic)
	if err != nil {
		t.Errorf("Failed to created smartnic: %+v, err: %v", nic, err)
	}

	// Verify the Naples received the config and switched to Managed Mode
	f4 := func() (bool, interface{}) {

		// validate the mode is managed
		cfg := nm.GetNaplesConfig()
		log.Infof("NaplesConfig: %v", cfg)
		if cfg.Spec.Mode != proto.MgmtMode_NETWORK.String() {
			log.Errorf("Failed to switch to managed mode")
			return false, nil
		}

		// Fetch smartnic object
		nic, err := nm.GetSmartNIC()
		if nic == nil || err != nil {
			log.Errorf("NIC not found in nicDB, mac:%s", hostID)
			return false, nil
		}

		// Verify NIC is admitted
		if nic.Status.AdmissionPhase != cmd.SmartNICStatus_ADMITTED.String() {
			log.Errorf("NIC is not admitted %+v", nic)
			return false, nil
		}

		// Verify Update NIC task is running
		if nm.GetUpdStatus() == false {
			log.Errorf("Update NIC is not in progress: %+v", nm)
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f4, "Failed to verify mode is in Managed Mode", string("1s"), string("60s"))

	// Validate SmartNIC object state is updated on Venice
	verifySmartNICObj(t, testMac, true, cmd.SmartNICStatus_ADMITTED.String(), hostName)

	// Validate Host object is created
	verifyHostObj(t, "esxc", testMac)

	// Verify Deletion of SmartNIC object
	f7 := func() (bool, interface{}) {
		ometa := api.ObjectMeta{Name: testMac}
		err = tInfo.smartNICServer.DeleteSmartNIC(ometa)
		if err != nil {
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f7, fmt.Sprintf("Failed to verify deletion of smartNIC object"))

	// Verify update of Host object
	f8 := func() (bool, interface{}) {
		meta := api.ObjectMeta{
			Name: "esxc",
		}
		hostObj, err := tInfo.apiClient.ClusterV1().Host().Get(context.Background(), &meta)
		if err != nil || hostObj == nil {
			log.Errorf("Failed to GET Host object, name:%s, %v", meta.Name, err)
			return false, nil
		}
		for ii := range hostObj.Status.AdmittedSmartNICs {
			if hostObj.Status.AdmittedSmartNICs[ii] == testMac {
				return false, nil
			}
		}
		return true, nil
	}
	AssertEventually(t, f8, fmt.Sprintf("Failed to verify update of Host object"))
}

func TestSmartNICConfigByUserErrorCases(t *testing.T) {
	// Init required components
	testSetup()
	defer testTeardown()

	hostID := getHostID(1)
	dbPath := getDBPath(1)
	restURL := getRESTUrl(1)
	testMac := "4444.4444.4401"

	// Cleanup any prior DB files
	os.Remove(dbPath)

	// create Agent and NMD
	ag, err := createNMD(t, dbPath, testMac, restURL)
	defer stopNMD(t, ag, dbPath)
	Assert(t, (err == nil && ag != nil), "Failed to create agent", err)

	nm := ag.GetNMD()

	// Validate default host mode
	f1 := func() (bool, interface{}) {

		cfg := nm.GetNaplesConfig()
		if cfg.Spec.Mode == proto.MgmtMode_HOST.String() && nm.GetListenURL() != "" &&
			nm.GetUpdStatus() == false && nm.GetRegStatus() == false && nm.GetRestServerStatus() == true {
			return true, nil
		}
		return false, nil
	}
	AssertEventually(t, f1, "Failed to verify mode is in host")

	// Delete the SmartNIC object if it exists
	_, err = tInfo.apiClient.ClusterV1().SmartNIC().Delete(context.Background(), &api.ObjectMeta{Name: hostID})

	// Create SmartNIC object in Venice
	nic := cmd.SmartNIC{
		TypeMeta: api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: api.ObjectMeta{
			Name: hostID,
		},
		Spec: cmd.SmartNICSpec{
			IPConfig: &cmd.IPConfig{
				IPAddress: "remotehost", // unreachable hostname for testing error case
			},
			Hostname:    hostID,
			MgmtMode:    cmd.SmartNICSpec_NETWORK.String(),
			NetworkMode: cmd.SmartNICSpec_OOB.String(),
		},
		Status: cmd.SmartNICStatus{
			AdmissionPhase: "UNKNOWN",
		},
	}

	_, err = tInfo.apiClient.ClusterV1().SmartNIC().Create(context.Background(), &nic)
	if err != nil {
		t.Fatalf("Failed to create smartnic: %+v, err: %v", nic, err)
	}

	// Verify SmartNIC object has UNREACHABLE condition
	f2 := func() (bool, interface{}) {

		meta := api.ObjectMeta{
			Name: hostID,
		}
		nicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), &meta)
		if err != nil || nicObj == nil || len(nicObj.Status.Conditions) == 0 ||
			nicObj.Status.Conditions[0].Type != cmd.SmartNICCondition_UNREACHABLE.String() ||
			nicObj.Status.Conditions[0].Status != cmd.ConditionStatus_TRUE.String() {
			log.Errorf("Failed to validate SmartNIC condition, nicObj: %+v, err: %v",
				nicObj, err)
			return false, nil
		}

		return true, nil
	}
	AssertEventually(t, f2, "Failed to verify SmartNIC object has UNREACHABLE condition set",
		string("10ms"), string("30s"))

	// Verify Deletion of SmartNIC object
	f3 := func() (bool, interface{}) {
		ometa := api.ObjectMeta{Name: hostID}
		err = tInfo.smartNICServer.DeleteSmartNIC(ometa)
		if err != nil {
			return false, nil
		}
		return true, nil
	}
	AssertEventually(t, f3, fmt.Sprintf("Failed to verify deletion of smartNIC object"))
}

func TestManualAdmission(t *testing.T) {
	testSetup()
	defer testTeardown()

	hostName := "esx000"
	mac := "4444.4444.0001"

	// Create Host in Venice
	host, err := createHost(hostName, cmd.SmartNICID{MACAddress: mac})
	AssertOk(t, err, "Error creating Host object")
	defer deleteHost(host.ObjectMeta)

	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
	smartNICUpdatesRPCClient := grpc.NewSmartNICUpdatesClient(tInfo.rpcClient.ClientConn)

	setClusterAutoAdmitNICs(t, false)

	r := doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))

	// verify smartNIC is created
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")
	// verify watch api is invoked
	verifyWatchAPIIsInvoked(t, smartNICUpdatesRPCClient, mac, cmd.SmartNICStatus_PENDING.String())

	nicObjMeta := &api.ObjectMeta{
		Name: mac,
	}

	// Admit NIC
	setSmartNICAdmit(t, nicObjMeta, true)

	// Register again. This time we should go straight to ADMITTED because Admit=true.
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostName)

	// verify Host object Status is populated correctly
	verifyHostObj(t, hostName, mac)

	// Register again. Previous admission should be preserved because Admit=true
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostName)

	// De-admit NIC
	setSmartNICAdmit(t, nicObjMeta, false)

	// verify Host object Status is cleared correctly
	verifyHostObj(t, hostName, "")

	// Register again. This time we should go to PENDING because Admit=false.
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")

	// Delete NIC object
	err = tInfo.smartNICServer.DeleteSmartNIC(*nicObjMeta)
	AssertOk(t, err, "Error deleting smartnic object")
	verifySmartNICObj(t, mac, false, "", "")

	// Register again. This time we should go to PENDING because no pre-existing object is there and AutoAdmit=false.
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")

	// Now register with different MAC and same hostname, we should get back a "reject" response
	mac2 := "4444.4444.0002"
	nicObjMeta2 := &api.ObjectMeta{
		Name: mac2,
	}
	mr.ClearEvents()
	numExpectedRejectEvents := 0

	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_REJECTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_REJECTED.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_REJECTED.String(), "")
	numExpectedRejectEvents++

	// Mark NIC2 for admission, we should still get rejected
	setSmartNICAdmit(t, nicObjMeta2, true)
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_REJECTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_REJECTED.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_REJECTED.String(), "")
	numExpectedRejectEvents++

	// Now try to register with different hostname and check that it gets admitted
	hostName2 := "XXX"
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName2)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_ADMITTED.String(), hostName2)

	// check that events got generated
	numRejectEvents := 0
	for _, ev := range mr.GetEvents() {
		if ev.EventType == cmd.NICRejected && strings.Contains(ev.Message, mac2) {
			numRejectEvents++
		}
	}
	Assert(t, numRejectEvents == numExpectedRejectEvents,
		"Unexpected number of REJECT events. Have: %d, want: %d, events:%+v", numRejectEvents, numExpectedRejectEvents, mr.GetEvents())

	err = tInfo.smartNICServer.DeleteSmartNIC(*nicObjMeta)
	AssertOk(t, err, "Error deleting smartnic object")
	err = tInfo.smartNICServer.DeleteSmartNIC(*nicObjMeta2)
	AssertOk(t, err, "Error deleting smartnic object")
}

// Test that a NIC that was previously rejected is automatically admitted when
// the reason for the rejection is addressed and AutoAdmitNICs = true
func TestAutoAdmitRejectedNICs(t *testing.T) {
	testSetup()
	defer testTeardown()

	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
	setClusterAutoAdmitNICs(t, true)

	// Crete and admit first NIC
	hostName := "esx000"
	mac := "4444.4444.0001"
	nicObjMeta := &api.ObjectMeta{
		Name: mac,
	}

	r := doRegisterNIC(t, smartNICRegistrationRPCClient, mac, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostName)

	mr.ClearEvents()
	numExpectedRejectEvents := 0

	// Create second NIC and try to register with conflicting hostname
	mac2 := "4444.4444.0002"
	hostName2 := "esx001"
	nicObjMeta2 := &api.ObjectMeta{
		Name: mac2,
	}

	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName)
	Assert(t, r.Phase == cmd.SmartNICStatus_REJECTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_REJECTED.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_REJECTED.String(), "")
	numExpectedRejectEvents++

	// Register again. This time with different hostname
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName2)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_ADMITTED.String(), "")

	// De-admit NIC
	setSmartNICAdmit(t, nicObjMeta2, false)

	// Register again. This time we should go to PENDING because Admit=false.
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac2, hostName2)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))
	verifySmartNICObj(t, mac2, true, cmd.SmartNICStatus_PENDING.String(), "")

	numRejectEvents := 0
	for _, ev := range mr.GetEvents() {
		if ev.EventType == cmd.NICRejected && strings.Contains(ev.Message, mac2) {
			numRejectEvents++
		}
	}
	Assert(t, numRejectEvents == numExpectedRejectEvents,
		"Unexpected number of REJECT events. Have: %d, want: %d, events:%+v", numRejectEvents, numExpectedRejectEvents, mr.GetEvents())

	// Clean up
	err := tInfo.smartNICServer.DeleteSmartNIC(*nicObjMeta)
	AssertOk(t, err, "Error deleting smartnic object")
	err = tInfo.smartNICServer.DeleteSmartNIC(*nicObjMeta2)
	AssertOk(t, err, "Error deleting smartnic object")
}

func TestReadmitSmartNIC(t *testing.T) {
	testSetup()
	defer testTeardown()

	hostObjName := "esx011"
	mac := "4444.4444.0011"
	nicHostname := "naples"

	// Create Host in Venice
	host, err := createHost(hostObjName, cmd.SmartNICID{MACAddress: mac})
	AssertOk(t, err, "Error creating Host object")

	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
	smartNICUpdatesRPCClient := grpc.NewSmartNICUpdatesClient(tInfo.rpcClient.ClientConn)

	setClusterAutoAdmitNICs(t, false)
	r := doRegisterNIC(t, smartNICRegistrationRPCClient, mac, nicHostname)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))

	// verify smartNIC is created
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")
	// verify watch api is invoked
	verifyWatchAPIIsInvoked(t, smartNICUpdatesRPCClient, mac, cmd.SmartNICStatus_PENDING.String())

	nicObjMeta := &api.ObjectMeta{
		Name: mac,
	}

	// Once the SmartNIC object is created, behavior should be same regardless of whether AutoAdmit = true or false.
	// If AutoAdmit = true but user sets NIC Spec.Admit = false, the NIC must be de-admitted and not re-admitted
	// until user re-sets Spec.Admit = true.

	for _, autoAdmit := range []bool{true, false} {
		setClusterAutoAdmitNICs(t, autoAdmit)
		for i := 0; i < 20; i++ {
			// Admit NIC
			setSmartNICAdmit(t, nicObjMeta, true)

			// Register again. This time we should go straight to ADMITTED because Admit=true.
			r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, nicHostname)
			Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
				fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
			verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostObjName)

			// verify Host object is created
			verifyHostObj(t, hostObjName, mac)

			// De-Admit NIC
			setSmartNICAdmit(t, nicObjMeta, false)

			// verify it goes to pending
			verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")

			// verify Host object is cleared
			verifyHostObj(t, hostObjName, "")
		}
	}

	// Try to re-admit with different spec.
	// The new values supplied by NAPLES should override those in Venice,
	// except for parameters that are exclusively owned by Venice.
	// Right now "Admit" is the only parameter owned by Venice.
	nicHostname = "hello-world"

	// Create a new host object matching the new nic hostname
	deleteHost(host.ObjectMeta)
	host, err = createHost(hostObjName, cmd.SmartNICID{Name: nicHostname})
	AssertOk(t, err, "Error creating Host object")
	defer deleteHost(host.ObjectMeta)

	// Set admit = false on ApiServer so the NIC does not get admitted immediately
	setSmartNICAdmit(t, nicObjMeta, false)
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_PENDING.String(), "")
	verifyHostObj(t, hostObjName, "")
	refNicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), nicObjMeta)
	AssertOk(t, err, "Error reading SmartNIC object from ApiServer")

	// Try to register. Old parameters should not be overridden yet because NIC is not admitted
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, nicHostname)
	Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))
	curNicObj, err := tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), nicObjMeta)
	AssertOk(t, err, "Error reading SmartNIC object from ApiServer")
	Assert(t, reflect.DeepEqual(refNicObj.Spec, curNicObj.Spec),
		"NIC registration modified Spec parameters even though NIC did not get admitted. Ref: %+v, Cur: %+v", refNicObj, curNicObj)

	// Admit the SmartNIC and verify that new parameters are honored
	setSmartNICAdmit(t, nicObjMeta, true)
	r = doRegisterNIC(t, smartNICRegistrationRPCClient, mac, nicHostname)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		"Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r)
	// NIC and Host object should pick up the new host name and form the pairing
	verifySmartNICObj(t, mac, true, cmd.SmartNICStatus_ADMITTED.String(), hostObjName)
	verifyHostObj(t, hostObjName, mac)
	// Hostname should have been overridden, Admit should not
	curNicObj, err = tInfo.apiClient.ClusterV1().SmartNIC().Get(context.Background(), nicObjMeta)
	AssertOk(t, err, "Error reading SmartNIC object from ApiServer")
	Assert(t, curNicObj.Spec.Hostname == nicHostname && curNicObj.Spec.Hostname != refNicObj.Spec.Hostname,
		"New NIC parameter (hostname) not honored: %+v", curNicObj)
	Assert(t, curNicObj.Spec.Admit == true, "Old NIC parameter (Admit) should not have been overriden: %+v", curNicObj)
}

func TestHostNICPairing(t *testing.T) {
	testSetup()
	defer testTeardown()

	numHostsAndNICs := 20

	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
	setClusterAutoAdmitNICs(t, false)

	hostName := func(i int) string {
		return fmt.Sprintf("host-%d", i)
	}

	nicHostname := func(i int) string {
		return fmt.Sprintf("naples-%d", i)
	}

	nicMAC := func(i int) string {
		return fmt.Sprintf("00ae.cd00.%02d00", i)
	}

	// Test pairing via MAC, host objects created before NIC registers
	for i := 0; i < numHostsAndNICs; i++ {
		_, err := createHost(hostName(i), cmd.SmartNICID{MACAddress: nicMAC(i)})
		AssertOk(t, err, "Error creating host object %d", i)
		defer deleteHost(api.ObjectMeta{Name: hostName(i)})
	}

	// Create NIC objects but do not admit yet
	for i := 0; i < numHostsAndNICs; i++ {
		r := doRegisterNIC(t, smartNICRegistrationRPCClient, nicMAC(i), nicHostname(i))
		Assert(t, r.Phase == cmd.SmartNICStatus_PENDING.String(),
			fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_PENDING.String(), r))
	}
	// Verify smartNIC are created and not paired yet
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), true, cmd.SmartNICStatus_PENDING.String(), "")
		verifyHostObj(t, hostName(i), "")
		defer tInfo.smartNICServer.DeleteSmartNIC(api.ObjectMeta{Name: nicMAC(i)})
	}

	// Admit NICs
	for i := 0; i < numHostsAndNICs; i++ {
		setSmartNICAdmit(t, &api.ObjectMeta{Name: nicMAC(i)}, true)
		r := doRegisterNIC(t, smartNICRegistrationRPCClient, nicMAC(i), nicHostname(i))
		Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
			fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	}
	// Verify smartNIC are admitted and paired with corresponding hosts
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), true, cmd.SmartNICStatus_ADMITTED.String(), hostName(i))
		verifyHostObj(t, hostName(i), nicMAC(i))
	}

	// Delete host objects
	for i := 0; i < numHostsAndNICs; i++ {
		err := deleteHost(api.ObjectMeta{Name: hostName(i)})
		AssertOk(t, err, "Error deleting Host object %s", hostName(i))
	}
	// Verify NIC pairing gets removed
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), true, cmd.SmartNICStatus_ADMITTED.String(), "")
	}

	// Re-create the host objects, this time pairing using NIC hostnames
	for i := 0; i < numHostsAndNICs; i++ {
		_, err := createHost(hostName(i), cmd.SmartNICID{Name: nicHostname(i)})
		AssertOk(t, err, "Error creating host object %d", i)
	}
	// Verify pairings are re-established
	for i := 0; i < numHostsAndNICs; i++ {
		verifyHostObj(t, hostName(i), nicMAC(i))
	}

	// Change the NIC hostnames
	for i := 0; i < numHostsAndNICs; i++ {
		setSmartNICHostname(t, &api.ObjectMeta{Name: nicMAC(i)}, nicHostname(i)+"-foo")
	}
	// Verify the pairings are removed again
	for i := 0; i < numHostsAndNICs; i++ {
		verifyHostObj(t, hostName(i), "")
	}

	// Change the SmartNIC IDs on the host object to match
	for i := 0; i < numHostsAndNICs; i++ {
		setHostSmartNICIDs(t, &api.ObjectMeta{Name: hostName(i)}, cmd.SmartNICID{Name: nicHostname(i) + "-foo"})
	}
	// Verify the pairings are re-established
	for i := 0; i < numHostsAndNICs; i++ {
		verifyHostObj(t, hostName(i), nicMAC(i))
	}

	// De-admit the NICs
	for i := 0; i < numHostsAndNICs; i++ {
		setSmartNICAdmit(t, &api.ObjectMeta{Name: nicMAC(i)}, false)
	}
	// Verify the pairings are cleared
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), true, cmd.SmartNICStatus_PENDING.String(), "")
		verifyHostObj(t, hostName(i), "")
	}

	// Switch back hosts to pair using MAC
	for i := 0; i < numHostsAndNICs; i++ {
		setHostSmartNICIDs(t, &api.ObjectMeta{Name: hostName(i)}, cmd.SmartNICID{MACAddress: nicMAC(i)})
	}

	// Admit NICs
	for i := 0; i < numHostsAndNICs; i++ {
		setSmartNICAdmit(t, &api.ObjectMeta{Name: nicMAC(i)}, true)
		r := doRegisterNIC(t, smartNICRegistrationRPCClient, nicMAC(i), nicHostname(i))
		Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
			fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))
	}
	// Verify smartNIC are admitted and paired with corresponding hosts
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), true, cmd.SmartNICStatus_ADMITTED.String(), hostName(i))
		verifyHostObj(t, hostName(i), nicMAC(i))
	}

	// Delete the NIC objects
	for i := 0; i < numHostsAndNICs; i++ {
		err := tInfo.smartNICServer.DeleteSmartNIC(api.ObjectMeta{Name: nicMAC(i)})
		AssertOk(t, err, "Error deleting smartnic object")
	}
	// Verify pairings are cleared
	for i := 0; i < numHostsAndNICs; i++ {
		verifySmartNICObj(t, nicMAC(i), false, "", "")
		verifyHostObj(t, hostName(i), "")
	}

	// Check that there are no conflict events
	for _, ev := range mr.GetEvents() {
		if ev.EventType == cmd.HostSmartNICSpecConflict {
			log.Fatalf("Found unexpected NIC conflict events: %+v", mr.GetEvents())
		}
	}
}

func TestHostNICPairingConflicts(t *testing.T) {
	testSetup()
	defer testTeardown()

	smartNICRegistrationRPCClient := grpc.NewSmartNICRegistrationClient(tInfo.rpcClient.ClientConn)
	setClusterAutoAdmitNICs(t, true)

	nicHostname := "naples-1"
	nicMAC := "00ae.cd00.1111"
	host1Name := "host1"
	host2Name := "host2"

	_, err := createHost(host1Name, cmd.SmartNICID{MACAddress: nicMAC})
	AssertOk(t, err, "Error creating host object")

	r := doRegisterNIC(t, smartNICRegistrationRPCClient, nicMAC, nicHostname)
	Assert(t, r.Phase == cmd.SmartNICStatus_ADMITTED.String(),
		fmt.Sprintf("Error in registration response. Expected phase: %s, got: %s", cmd.SmartNICStatus_ADMITTED.String(), r))

	// verify smartNIC is admitted and paired with "host1"
	verifySmartNICObj(t, nicMAC, true, cmd.SmartNICStatus_ADMITTED.String(), host1Name)
	verifyHostObj(t, host1Name, nicMAC)

	// Create a second hosts referring to the same nic
	mr.ClearEvents()
	_, err = createHost(host2Name, cmd.SmartNICID{Name: nicHostname})
	AssertOk(t, err, "Error creating host object")

	// Verify pairing did not change on host1 and NIC
	verifyHostObj(t, host1Name, nicMAC)
	verifyHostObj(t, host2Name, "")
	verifySmartNICObj(t, nicMAC, true, cmd.SmartNICStatus_ADMITTED.String(), host1Name)

	ev := mr.GetEvents()
	Assert(t, validateNICSpecConflictEvent(&ev, nicMAC, host1Name, host2Name),
		"Did not find expected NIC conflict event, got: %+v", ev)

	// delete host1, the pairing should be updated without further events
	mr.ClearEvents()
	err = deleteHost(api.ObjectMeta{Name: host1Name})
	AssertOk(t, err, "Error deleting Host object")
	verifyHostObj(t, host2Name, nicMAC)
	verifySmartNICObj(t, nicMAC, true, cmd.SmartNICStatus_ADMITTED.String(), host2Name)
	ev = mr.GetEvents()
	Assert(t, !validateNICSpecConflictEvent(&ev, nicMAC, host1Name, host2Name), "Found unexpected events: %+v", ev)
}

func testSetup() {

	// Disable open trace
	ventrace.DisableOpenTrace()
	grpclog.SetLoggerV2(logger)

	// Init tsdb
	ctx, cancel := context.WithCancel(context.Background())
	tsdb.Init(ctx, &tsdb.Opts{ClientName: "smart-nic-test", ResolverClient: &rmock.ResolverClient{}})
	defer cancel()

	// Create api server
	apiServerAddress := ":0"
	tInfo.l = logger
	scheme := runtime.GetDefaultScheme()
	srvConfig := apiserver.Config{
		GrpcServerPort: apiServerAddress,
		DebugMode:      false,
		Logger:         logger,
		Version:        "v1",
		Scheme:         scheme,
		Kvstore: store.Config{
			Type:  store.KVStoreTypeMemkv,
			Codec: runtime.NewJSONCodec(scheme),
		},
		GetOverlay: api_cache.GetOverlay,
		IsDryRun:   api_cache.IsDryRun,
	}

	tInfo.apiServer = apiserverpkg.MustGetAPIServer()
	go tInfo.apiServer.Run(srvConfig)
	tInfo.apiServer.WaitRunning()
	addr, err := tInfo.apiServer.GetAddr()
	if err != nil {
		os.Exit(-1)
	}

	// Create api client
	tInfo.apiServerAddr = addr
	apiCl, err := apicache.NewGrpcUpstream("smartnic_test", tInfo.apiServerAddr, tInfo.l)
	if err != nil {
		fmt.Printf("Cannot create gRPC client - %v", err)
		os.Exit(-1)
	}
	tInfo.apiClient = apiCl

	// create gRPC server for smartNIC service and gRPC client
	tInfo.rpcServer, tInfo.rpcClient = createRPCServerClient()
	if tInfo.rpcServer == nil || tInfo.rpcClient == nil {
		fmt.Printf("Err creating rpc server & client")
		os.Exit(-1)
	}

	// Check if no cluster exists to start with - negative test
	_, err = tInfo.smartNICServer.GetCluster()
	if err == nil {
		fmt.Printf("Unexpected cluster object found, err: %s", err)
		os.Exit(-1)
	}

	// Create test cluster object
	clRef := &cmd.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name: "testCluster",
		},
		Spec: cmd.ClusterSpec{
			AutoAdmitNICs: true,
		},
	}
	_, err = tInfo.apiClient.ClusterV1().Cluster().Create(context.Background(), clRef)
	if err != nil {
		fmt.Printf("Error creating Cluster object, %v", err)
		os.Exit(-1)
	}
}

func testTeardown() {
	mr.ClearEvents()

	// delete cluster object
	clRef := &cmd.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name: "testCluster",
		},
	}
	_, err := tInfo.apiClient.ClusterV1().Cluster().Delete(context.Background(), &clRef.ObjectMeta)
	if err != nil {
		fmt.Printf("Error deleting Cluster object, %v", err)
		os.Exit(-1)
	}

	// stop the rpc client and server
	tInfo.rpcClient.Close()
	tInfo.rpcServer.Stop()

	// stop the apiServer
	tInfo.apiServer.Stop()

	// close the CMD certmgr
	if cmdenv.CertMgr != nil {
		cmdenv.CertMgr.Close()
	}
}

func TestMain(m *testing.M) {
	// Run tests
	rcode := m.Run()

	os.Exit(rcode)
}
