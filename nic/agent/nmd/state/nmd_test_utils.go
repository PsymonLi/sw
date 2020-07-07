// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"crypto"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"fmt"
	"os"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	cmd "github.com/pensando/sw/api/generated/cluster"
	nmdapi "github.com/pensando/sw/nic/agent/nmd/api"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/cmd/grpc/server/certificates/certapi"
	roprotos "github.com/pensando/sw/venice/ctrler/rollout/rpcserver/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
	tu "github.com/pensando/sw/venice/utils/testutils"
)

// Test params
var (
	// Bolt DB file
	emDBPath = "/tmp/nmd.db"

	// NIC to be admitted
	nicKey1 = "00ae.cd01.0203"

	// NIC to be in pending state
	nicKey2 = "4444.4444.4444"

	// NIC to be rejected
	nicKey3 = "6666.6666.6666"

	logger = log.GetNewLogger(log.GetDefaultConfig("nmd_state_test"))

	// NIC registration interval
	nicRegInterval = time.Second

	// NIC update interval
	nicUpdInterval = 100 * time.Millisecond

	// create mock events recorder
	_ = recorder.Override(mockevtsrecorder.NewRecorder("nmd_state_test", logger))
)

// Mock platform agent
type mockAgent struct {
	sync.Mutex
	nicDB     map[string]*cmd.DistributedServiceCard
	rpcServer *rpckit.RPCServer
}

// RegisterNMD registers NMD with PlatformAgent
func (m *mockAgent) RegisterNMD(nmdapi nmdapi.NmdPlatformAPI) error {
	return nil
}

// CreateSmartNIC creates a smart NIC object
func (m *mockAgent) CreateSmartNIC(nic *cmd.DistributedServiceCard) error {
	m.Lock()
	defer m.Unlock()

	key := objectKey(nic.ObjectMeta)
	m.nicDB[key] = nic
	return nil
}

// UpdateSmartNIC updates a smart NIC object
func (m *mockAgent) UpdateSmartNIC(nic *cmd.DistributedServiceCard) error {
	m.Lock()
	defer m.Unlock()

	key := objectKey(nic.ObjectMeta)
	m.nicDB[key] = nic
	return nil
}

// DeleteSmartNIC deletes a smart NIC object
func (m *mockAgent) DeleteSmartNIC(nic *cmd.DistributedServiceCard) error {
	m.Lock()
	defer m.Unlock()

	key := objectKey(nic.ObjectMeta)
	delete(m.nicDB, key)
	return nil
}

func (m *mockAgent) GetPlatformCertificate(nic *cmd.DistributedServiceCard) ([]byte, error) {
	return nil, nil
}

func (m *mockAgent) GetPlatformSigner(nic *cmd.DistributedServiceCard) (crypto.Signer, error) {
	return nil, nil
}

// StartServer starts the RPC server used by NMD.
// It is needed because NMD will not proceed if it cannot connect
func (m *mockAgent) StartServer(url string) error {
	s, err := rpckit.NewRPCServer(globals.Netagent, globals.Localhost+":"+globals.AgentGRPCPort, rpckit.WithTLSProvider(nil))
	m.rpcServer = s
	return err
}

func (m *mockAgent) StopServer() {
	if m.rpcServer != nil {
		m.rpcServer.Stop()
		m.rpcServer = nil
	}
}

type mockCtrler struct {
	sync.Mutex
	nicDB                     map[string]*cmd.DistributedServiceCard
	numUpdateSmartNICReqCalls int
	smartNICWatcherRunning    bool
}

func (m *mockCtrler) RegisterSmartNICReq(nic *cmd.DistributedServiceCard) (grpc.RegisterNICResponse, error) {
	m.Lock()
	defer m.Unlock()

	key := objectKey(nic.ObjectMeta)
	m.nicDB[key] = nic
	if strings.HasPrefix(nic.Spec.ID, nicKey1) {
		// we don't have the actual csr from the NIC request, so we just make up
		// a certificate on the spot
		key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
		if err != nil {
			return grpc.RegisterNICResponse{}, fmt.Errorf("Error generating CA key: %v", err)
		}
		cert, err := certs.SelfSign("nmd_state_test_ca", key, certs.WithValidityDays(1))
		if err != nil {
			return grpc.RegisterNICResponse{}, fmt.Errorf("Error generating CA cert: %v", err)
		}
		resp := grpc.RegisterNICResponse{
			AdmissionResponse: &grpc.NICAdmissionResponse{
				Phase: cmd.DistributedServiceCardStatus_ADMITTED.String(),
				ClusterCert: &certapi.CertificateSignResp{
					Certificate: &certapi.Certificate{
						Certificate: cert.Raw,
					},
				},
				CaTrustChain: &certapi.CaTrustChain{
					Certificates: []*certapi.Certificate{
						{
							Certificate: cert.Raw,
						},
					},
				},
				TrustRoots: &certapi.TrustRoots{
					Certificates: []*certapi.Certificate{
						{
							Certificate: cert.Raw,
						},
					},
				},
			},
		}
		return resp, nil
	}

	if strings.HasPrefix(nic.Spec.ID, nicKey2) {
		return grpc.RegisterNICResponse{
			AdmissionResponse: &grpc.NICAdmissionResponse{
				Phase: cmd.DistributedServiceCardStatus_PENDING.String(),
			},
		}, nil
	}

	log.Infof("REGITERING return REJECTED")

	return grpc.RegisterNICResponse{
		AdmissionResponse: &grpc.NICAdmissionResponse{
			Phase:  cmd.DistributedServiceCardStatus_REJECTED.String(),
			Reason: string("Invalid Cert"),
		},
	}, nil
}

func (m *mockCtrler) UpdateSmartNICReq(nic *cmd.DistributedServiceCard) error {
	m.Lock()
	defer m.Unlock()
	m.numUpdateSmartNICReqCalls++
	key := objectKey(nic.ObjectMeta)
	m.nicDB[key] = nic
	return nil
}

func (m *mockCtrler) WatchSmartNICUpdates() {
	m.Lock()
	defer m.Unlock()
	m.smartNICWatcherRunning = true
}
func (m *mockCtrler) StopWatch() {

}

func (m *mockCtrler) Stop() {
	m.Lock()
	defer m.Unlock()
	m.smartNICWatcherRunning = false
	m.numUpdateSmartNICReqCalls = 0
}

func (m *mockCtrler) IsSmartNICWatcherRunning() bool {
	return m.smartNICWatcherRunning
}

func (m *mockCtrler) GetNIC(meta *api.ObjectMeta) *cmd.DistributedServiceCard {
	m.Lock()
	defer m.Unlock()

	key := objectKey(*meta)
	return m.nicDB[key]
}

func (m *mockCtrler) GetNumUpdateSmartNICReqCalls() int {
	m.Lock()
	defer m.Unlock()
	return m.numUpdateSmartNICReqCalls
}

type mockRolloutCtrler struct {
	sync.Mutex
	status                 []roprotos.DSCOpStatus
	smartNICWatcherRunning bool
}

func (f *mockRolloutCtrler) UpdateDSCRolloutStatus(status *roprotos.DSCRolloutStatusUpdate) error {
	f.status = status.Status.OpStatus
	log.Errorf("Got status %#v", f.status)
	return nil
}

func (f *mockRolloutCtrler) WatchDSCRolloutUpdates() error {
	f.Lock()
	defer f.Unlock()
	f.smartNICWatcherRunning = true
	return nil
}

func (f *mockRolloutCtrler) Stop() {
	f.Lock()
	defer f.Unlock()
	f.smartNICWatcherRunning = false
}

func (f *mockRolloutCtrler) IsSmartNICWatcherRunning() bool {
	f.Lock()
	defer f.Unlock()
	return f.smartNICWatcherRunning
}

type mockUpgAgent struct {
	n         nmdapi.NmdRolloutAPI
	forceFail bool
}

func (f *mockUpgAgent) RegisterNMD(n nmdapi.NmdRolloutAPI) error {
	f.n = n
	return nil
}
func (f *mockUpgAgent) StartDisruptiveUpgrade(firmwarePkgName string) error {
	if f.forceFail {
		go f.n.UpgFailed(&[]string{"ForceFailDisruptive"})
	} else {
		go f.n.UpgSuccessful()
	}
	return nil
}
func (f *mockUpgAgent) StartUpgOnNextHostReboot(firmwarePkgName string) error {
	if f.forceFail {
		go f.n.UpgFailed(&[]string{"ForceFailUpgOnNextHostReboot"})
	} else {
		go f.n.UpgSuccessful()
	}
	return nil
}
func (f *mockUpgAgent) IsUpgClientRegistered() error {
	return nil
}
func (f *mockUpgAgent) IsUpgradeInProgress() bool {
	return false
}
func (f *mockUpgAgent) StartPreCheckDisruptive(version string) error {
	if f.forceFail {
		go f.n.UpgNotPossible(&[]string{"ForceFailpreCheckDisruptive"})
	} else {
		go f.n.UpgPossible()
	}
	return nil
}
func (f *mockUpgAgent) StartPreCheckForUpgOnNextHostReboot(version string) error {
	if f.forceFail {
		go f.n.UpgNotPossible(&[]string{"ForceFailpreCheckForUpgOnNextHostReboot"})
	} else {
		go f.n.UpgPossible()
	}
	return nil
}

// createNMD creates a NMD server
func createNMD(t *testing.T, dbPath, mode, nodeID string) (*NMD, *mockAgent, *mockCtrler, *mockUpgAgent, *mockRolloutCtrler) {
	// Start a fake delphi hub
	log.Info("Creating NMD")
	// Hardcode the MacAddress for now. If we want multiple instances of fru.json with unique MAC, then a getMac() function can be written.
	err := tu.CreateFruJSON("00:AE:CD:01:02:03")
	if err != nil {
		log.Errorf("Error creating /tmp/fru.json file. Err: %v", err)
		return nil, nil, nil, nil, nil
	}

	ag := &mockAgent{
		nicDB: make(map[string]*cmd.DistributedServiceCard),
	}
	ct := &mockCtrler{
		nicDB: make(map[string]*cmd.DistributedServiceCard),
	}
	roC := &mockRolloutCtrler{}
	upgAgt := &mockUpgAgent{}

	err = ag.StartServer(globals.Localhost + ":" + globals.AgentGRPCPort)
	if err != nil {
		log.Errorf("Error starting mock agent server: %v", err)
		return nil, nil, nil, nil, nil
	}

	// create new NMD
	nm, err := NewNMD(nil,
		//ag,
		//upgAgt,
		//nil, // no resolver
		dbPath,
		//nodeID,
		"localhost:0",
		"", // no revproxy endpoint
		//"", // no local certs endpoint
		//"", // no remote certs endpoint
		//"", // no cmd registration endpoint
		//mode,
		nicRegInterval,
		nicUpdInterval,
		WithCMDAPI(ct),
		WithRolloutAPI(roC))

	if err != nil {
		log.Errorf("Error creating NMD. Err: %v", err)
		return nil, nil, nil, nil, nil
	}
	tu.Assert(t, nm.GetAgentID() == nodeID, "Failed to match nodeUUID", nm)

	// Ensure the NMD's rest server is started
	nm.CreateMockIPClient()
	cfg := nm.GetNaplesConfig()

	if cfg.Spec.IPConfig == nil {
		cfg.Spec.IPConfig = &cmd.IPConfig{}
	}

	if mode == "network" {
		// Add a fake controller to the spec so that mock IPClient starts managed mode
		cfg.Spec.Controllers = []string{"127.0.0.1"}
	}

	nm.SetNaplesConfig(cfg.Spec)
	err = nm.UpdateNaplesConfig(nm.GetNaplesConfig())
	nm.Upgmgr = upgAgt
	nm.cmd = ct
	nm.Platform = ag

	return nm, ag, ct, upgAgt, roC
}

// stopNMD stops NMD server and optionally deleted emDB file
func stopNMD(t *testing.T, nm *NMD, ma *mockAgent, cleanupDB bool) {
	if nm != nil {
		nm.Stop()
	}

	if ma != nil {
		ma.StopServer()
	}

	if cleanupDB {
		err := os.Remove(emDBPath)
		if err != nil {
			t.Fatalf("Error deleting emDB file, err: %v", err)
		}
	}
}
