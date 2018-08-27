// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"time"

	"github.com/gorilla/mux"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/httputils"
	"github.com/pensando/sw/nic/agent/nmd/protos"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/emstore"
	"github.com/pensando/sw/venice/utils/keymgr"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit/tlsproviders"
)

// NewNMD returns a new NMD instance
func NewNMD(platform PlatformAPI,
	dbPath, nodeUUID, macAddr, listenURL, certsListenURL, remoteCertsURL, mode string,
	regInterval, updInterval time.Duration) (*NMD, error) {

	var emdb emstore.Emstore
	var err error

	// Set mode and mac
	var naplesMode nmd.NaplesMode
	switch mode {
	case "classic":
		naplesMode = nmd.NaplesMode_CLASSIC_MODE
	case "managed":
		naplesMode = nmd.NaplesMode_MANAGED_MODE
	default:
		log.Errorf("Invalid mode, mode:%s", mode)
		return nil, errors.New("Invalid mode")
	}

	// open the embedded database
	if dbPath == "" {
		emdb, err = emstore.NewEmstore(emstore.MemStoreType, "")
	} else {
		emdb, err = emstore.NewEmstore(emstore.BoltDBType, dbPath)
	}

	if err != nil {
		log.Errorf("Error opening the embedded db. Err: %v", err)
		return nil, err
	}

	// construct default config
	config := nmd.Naples{
		ObjectMeta: api.ObjectMeta{
			Name: "NaplesConfig",
		},
		TypeMeta: api.TypeMeta{
			Kind: "Naples",
		},
		Spec: nmd.NaplesSpec{
			Mode:       naplesMode,
			PrimaryMac: macAddr,
			HostName:   nodeUUID,
		},
	}

	// check if naples config exists in emdb
	cfgObj, err := emdb.Read(&config)
	if cfgObj != nil && err == nil {

		// Use the persisted config moving forward
		config = *cfgObj.(*nmd.Naples)
	} else {

		// persist the default naples config
		err = emdb.Write(&config)
		if err != nil {
			log.Fatalf("Error persisting the default naples config in EmDB, err: %+v", err)
		}
	}

	// create NMD object
	nm := NMD{
		store:            emdb,
		nodeUUID:         nodeUUID,
		macAddr:          macAddr,
		platform:         platform,
		nic:              nil,
		nicRegInterval:   regInterval,
		isRegOngoing:     false,
		nicUpdInterval:   updInterval,
		isUpdOngoing:     false,
		isRestSrvRunning: false,
		listenURL:        listenURL,
		certsListenURL:   certsListenURL,
		remoteCertsURL:   remoteCertsURL,
		stopNICReg:       make(chan bool, 1),
		stopNICUpd:       make(chan bool, 1),
		config:           config,
	}

	// register NMD with the platform agent
	err = platform.RegisterNMD(&nm)
	if err != nil {
		// cleanup emstore and return
		emdb.Close()
		return nil, err
	}

	// Start the control loop based on configured Mode
	if nm.config.Spec.Mode == nmd.NaplesMode_CLASSIC_MODE {

		// Start in Classic Mode
		err = nm.StartClassicMode()
		if err != nil {
			log.Fatalf("Error starting in classic mode, err: %+v", err)
		}
	} else {

		// Start in Managed Mode
		go nm.StartManagedMode()
	}

	return &nm, nil
}

// RegisterCMD registers a CMD object
func (n *NMD) RegisterCMD(cmd CmdAPI) error {

	n.Lock()
	defer n.Unlock()

	// ensure two controller plugins dont register
	if n.cmd != nil {
		log.Fatalf("Attempt to register multiple controllers with NMD.")
	}

	// initialize cmd
	n.cmd = cmd

	return nil
}

// GenClusterKeyPair generates a (public key, private key) for this NMD instance
// to authenticate itself to other entities in the Venice cluster.
// When the instance is admitted to a cluster, it receives a corresponding
// certificate signed by CMD
func (n *NMD) GenClusterKeyPair() (*keymgr.KeyPair, error) {
	return n.tlsProvider.CreateClientKeyPair(keymgr.ECDSA384)
}

// objectKey returns object key from object meta
func objectKey(meta api.ObjectMeta) string {
	return fmt.Sprintf("%s|%s", meta.Tenant, meta.Name)
}

// GetAgentID returns UUID of the NMD
func (n *NMD) GetAgentID() string {
	return n.nodeUUID
}

// NaplesConfigHandler is the REST handler for Naples Config POST operation
func (n *NMD) NaplesConfigHandler(r *http.Request) (interface{}, error) {

	req := nmd.Naples{}
	resp := NaplesConfigResp{}
	content, err := ioutil.ReadAll(r.Body)
	if err != nil {
		log.Errorf("Failed to read request: %v", err)
		resp.ErrorMsg = err.Error()
		return resp, err
	}

	if err = json.Unmarshal(content, &req); err != nil {
		log.Errorf("Unmarshal err %s", content)
		resp.ErrorMsg = err.Error()
		return resp, err
	}

	log.Infof("Naples Config Request: %+v", req)
	err = n.UpdateNaplesConfig(req)

	if err != nil {
		resp.ErrorMsg = err.Error()
		return resp, err
	}
	log.Infof("Naples Config Response: %+v", resp)

	return resp, nil
}

// NaplesGetHandler is the REST handler for Naples Config GET operation
func (n *NMD) NaplesGetHandler(r *http.Request) (interface{}, error) {

	cfg := &n.config
	log.Infof("Naples Get Response: %+v", cfg)
	return cfg, nil
}

// Stop stops the NMD
func (n *NMD) Stop() error {

	log.Errorf("Stopping NMD")

	n.StopClassicMode(true)

	if n.GetConfigMode() == nmd.NaplesMode_MANAGED_MODE {

		// Cleanup Managed mode tasks, if any
		n.StopManagedMode()
	}

	// Close the embedded object store
	err := n.store.Close()

	return err
}

// Catchall http handler
func unknownAction(w http.ResponseWriter, r *http.Request) {
	log.Infof("Unknown REST URL %q", r.URL.Path)
	w.WriteHeader(503)
}

// StartRestServer creates a new HTTP server serving REST api
func (n *NMD) StartRestServer() error {

	// If RestServer is already running, return
	if n.GetRestServerStatus() == true {
		log.Warnf("REST server is already running")
		return errors.New("REST server already running")
	}

	// if no URL was specified, just return (used during unit/integ tests)
	if n.listenURL == "" {
		log.Errorf("Empty listenURL for REST server")
		return errors.New("ListenURL cannot be empty")
	}

	// setup the routes
	router := mux.NewRouter()
	t1 := router.Methods("POST").Subrouter()
	t1.HandleFunc(ConfigURL, httputils.MakeHTTPHandler(n.NaplesConfigHandler))

	t2 := router.Methods("GET").Subrouter()
	t2.HandleFunc(ConfigURL, httputils.MakeHTTPHandler(n.NaplesGetHandler))
	t2.HandleFunc("/api/{*}", unknownAction)

	// create listener
	listener, err := net.Listen("tcp", n.listenURL)
	if err != nil {
		log.Errorf("Error starting listener. Err: %v", err)
		return err
	}

	// Init & create a http server
	n.Lock()
	n.listener = listener
	n.httpServer = &http.Server{Addr: n.listenURL, Handler: router}
	n.isRestSrvRunning = true
	n.Unlock()

	log.Infof("Started NMD Rest server at %s", n.GetListenURL())

	// Launch the server
	go n.httpServer.Serve(listener)

	return nil
}

// StopRestServer stops the http server
func (n *NMD) StopRestServer(shutdown bool) error {
	n.Lock()
	defer n.Unlock()

	if shutdown && n.httpServer != nil {
		err := n.httpServer.Close()
		if err != nil {
			log.Errorf("Failed to stop NMD Rest server at %s", n.listenURL)
			return err
		}
		n.httpServer = nil
		log.Infof("Stopped REST server at %s", n.listenURL)

	}
	n.isRestSrvRunning = false

	return nil
}

// GetNMDUrl returns the REST URL
func (n *NMD) GetNMDUrl() string {
	return "http://" + n.GetListenURL() + ConfigURL
}

func (n *NMD) initTLSProvider() error {
	// Instantiate a KeyMgr to store the cluster certificate and a TLS provider
	// to use it to connect to other cluster components
	tlsProvider, err := tlsproviders.NewDefaultKeyMgrBasedProvider(globals.Nmd + "-cluster")
	if err != nil {
		return errors.Wrapf(err, "Error instantiating tls provider")
	}
	n.tlsProvider = tlsProvider
	return nil
}

func (n *NMD) setClusterCredentials(resp *grpc.RegisterNICResponse) error {
	certMsg := resp.GetClusterCert()
	if certMsg == nil {
		return fmt.Errorf("No certificate found in registration response message")
	}
	cert, err := x509.ParseCertificate(certMsg.GetCertificate().Certificate)
	if err != nil {
		return fmt.Errorf("Error parsing cluster certificate: %v", err)
	}
	err = n.tlsProvider.SetClientCertificate(cert)
	if err != nil {
		return fmt.Errorf("Error storing cluster certificate: %v", err)
	}
	var caTrustChain []*x509.Certificate
	if resp.GetCaTrustChain() != nil {
		for i, c := range resp.GetCaTrustChain().GetCertificates() {
			cert, err := x509.ParseCertificate(c.Certificate)
			if err != nil {
				log.Errorf("Error parsing CA trust chain certificate index %d: %v", i, err)
				// continue anyway
			}
			caTrustChain = append(caTrustChain, cert)
		}
	}
	n.tlsProvider.SetCaTrustChain(caTrustChain)

	var trustRoots []*x509.Certificate
	if resp.GetTrustRoots() != nil {
		for i, c := range resp.GetTrustRoots().GetCertificates() {
			cert, err := x509.ParseCertificate(c.Certificate)
			if err != nil {
				log.Errorf("Error parsing trust roots certificate index %d: %v", i, err)
				// continue anyway
			}
			trustRoots = append(caTrustChain, cert)
		}
	}
	n.tlsProvider.SetTrustRoots(trustRoots)
	return nil
}
