// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/pprof"
	"os"
	"os/exec"
	"strings"
	"time"

	"github.com/gorilla/mux"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/httputils"
	nmdapi "github.com/pensando/sw/nic/agent/nmd/api"
	"github.com/pensando/sw/nic/agent/nmd/cmdif"
	"github.com/pensando/sw/nic/agent/nmd/protos"
	"github.com/pensando/sw/venice/cmd/grpc"
	roprotos "github.com/pensando/sw/venice/ctrler/rollout/rpcserver/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/emstore"
	"github.com/pensando/sw/venice/utils/keymgr"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit/tlsproviders"
)

// NewNMDOption is a functional option type that allows dependency injection in NMD constructor
// This is mostly intended for testing, as in the normal code flow dependencies are instantiated
// based on mode
type NewNMDOption func(*NMD)

// WithCMDAPI returns a functional option used to pass a CMD API implementation to NewNMD
func WithCMDAPI(cmd nmdapi.CmdAPI) NewNMDOption {
	return func(nmd *NMD) {
		nmd.cmd = cmd
	}
}

// WithRolloutAPI returns a functional option used to pass a Rollout API implementation to NewNMD
func WithRolloutAPI(ro nmdapi.RolloutCtrlAPI) NewNMDOption {
	return func(nmd *NMD) {
		nmd.rollout = ro
	}
}

// NewNMD returns a new NMD instance
func NewNMD(platform nmdapi.PlatformAPI, upgmgr nmdapi.UpgMgrAPI, resolverClient resolver.Interface,
	dbPath, nodeUUID, macAddr, listenURL, certsListenURL, remoteCertsURL, cmdRegURL, cmdUpdURL, mode string,
	regInterval, updInterval time.Duration, opts ...NewNMDOption) (*NMD, error) {

	var emdb emstore.Emstore
	var err error

	// Set mode and mac
	var naplesMode nmd.MgmtMode
	switch mode {
	case "host":
		naplesMode = nmd.MgmtMode_HOST
	case "network":
		naplesMode = nmd.MgmtMode_NETWORK
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
			PrimaryMAC: macAddr,
			Hostname:   nodeUUID,
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
		store:              emdb,
		nodeUUID:           nodeUUID,
		macAddr:            macAddr,
		platform:           platform,
		upgmgr:             upgmgr,
		resolverClient:     resolverClient,
		nic:                nil,
		nicRegInitInterval: regInterval,
		nicRegInterval:     regInterval,
		isRegOngoing:       false,
		nicUpdInterval:     updInterval,
		isUpdOngoing:       false,
		isRestSrvRunning:   false,
		listenURL:          listenURL,
		certsListenURL:     certsListenURL,
		remoteCertsURL:     remoteCertsURL,
		cmdRegURL:          cmdRegURL,
		cmdUpdURL:          cmdUpdURL,
		stopNICReg:         make(chan bool, 1),
		stopNICUpd:         make(chan bool, 1),
		config:             config,
		completedOps:       make(map[roprotos.SmartNICOpSpec]bool),
	}

	for _, o := range opts {
		o(&nm)
	}

	// register NMD with the platform agent
	err = platform.RegisterNMD(&nm)
	if err != nil {
		// cleanup emstore and return
		emdb.Close()
		return nil, err
	}

	if upgmgr != nil {
		err = upgmgr.RegisterNMD(&nm)
		if err != nil {
			log.Fatalf("Error Registering NMD with upgmgr, err: %+v", err)
			// cleanup emstore and return
			emdb.Close()
			return nil, err
		}
	}

	//// Start the control loop based on configured Mode
	//if nm.config.Spec.Mode == nmd.MgmtMode_HOST {
	//	// Start in Classic Mode
	//	err = nm.StartClassicMode()
	//	if err != nil {
	//		log.Fatalf("Error starting in classic mode, err: %+v", err)
	//	}
	//}else {
	//	// Start in Managed Mode
	//	go nm.StartManagedMode()
	//}

	return &nm, nil
}

// RegisterCMD registers a CMD object
func (n *NMD) RegisterCMD(cmd nmdapi.CmdAPI) error {
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

// UnRegisterCMD ensures that nmd cleans up its old cmd information.
func (n *NMD) UnRegisterCMD() error {
	n.Lock()
	defer n.Unlock()
	log.Infof("Received UnRegisterCMD message.")
	n.cmd = nil
	return nil
}

// UpdateCMDClient updates the cmd client with the resolver information obtained by DHCP
func (n *NMD) UpdateCMDClient(resolverURLs []string) error {
	if n.cmd != nil {
		n.cmd.Stop()
		n.UnRegisterCMD()
	}

	var cmdResolverURL []string
	// Ensure ResolverURLs are updated
	for _, res := range resolverURLs {
		cmdResolverURL = append(cmdResolverURL, fmt.Sprintf("%s:%s", res, globals.CMDGRPCAuthPort))
	}

	resolverClient := resolver.New(&resolver.Config{Name: "NMD", Servers: cmdResolverURL})

	// TODO Move this to resolver client at least for cmdUpdatesURL
	// Use the first resolverURL as registration and updatesURL

	cmdRegistrationURL := fmt.Sprintf("%s:%s", resolverURLs[0], globals.CMDGRPCUnauthPort)
	cmdUpdatesURL := fmt.Sprintf("%s:%s", resolverURLs[0], globals.CMDGRPCAuthPort)

	newCMDClient, err := cmdif.NewCmdClient(n, cmdRegistrationURL, cmdUpdatesURL, resolverClient)
	if err != nil {
		log.Errorf("Failed to update CMD Client. Err: %v", err)
		return err
	}
	n.cmd = newCMDClient
	log.Infof("Updated cmd client with newer cmd resolver URLs: %v", resolverURLs)

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

// GetPrimaryMAC returns primaryMac of NMD
func (n *NMD) GetPrimaryMAC() string {
	return n.config.Spec.PrimaryMAC
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

// NaplesRolloutHandler is the REST handler for Naples Config POST operation
func (n *NMD) NaplesRolloutHandler(r *http.Request) (interface{}, error) {
	snicRollout := roprotos.SmartNICRollout{}
	resp := NaplesConfigResp{}
	content, err := ioutil.ReadAll(r.Body)
	if err != nil {
		log.Errorf("Failed to read request: %v", err)
		resp.ErrorMsg = err.Error()
		return resp, err
	}

	if err = json.Unmarshal(content, &snicRollout); err != nil {
		log.Errorf("Unmarshal err %s", content)
		resp.ErrorMsg = err.Error()
		return resp, err
	}

	log.Infof("Naples Rollout Config Request: %+v", snicRollout)
	err = n.CreateUpdateSmartNICRollout(&snicRollout)

	if err != nil {
		resp.ErrorMsg = err.Error()
		return resp, err
	}
	log.Infof("Naples Rollout Config Response: %+v", resp)

	return resp, nil
}

// NaplesFileUploadHandler is the REST handler for Naples File Upload POST operation
func NaplesFileUploadHandler(w http.ResponseWriter, r *http.Request) {
	// parse and validate file and post parameters
	file, fileHeader, err := r.FormFile("uploadFile")
	if err != nil {
		renderError(w, err.Error(), http.StatusBadRequest)
		return
	}
	defer file.Close()
	uploadPath := r.FormValue("uploadPath")
	if uploadPath == "" {
		renderError(w, "Upload Path Not Specified\n", http.StatusBadRequest)
		return
	}
	fileBytes, err := ioutil.ReadAll(file)
	if err != nil {
		renderError(w, err.Error(), http.StatusBadRequest)
		return
	}

	fileNameSlice := strings.Split(fileHeader.Filename, "/")

	newPath := uploadPath + fileNameSlice[len(fileNameSlice)-1]

	// write file
	newFile, err := os.Create(newPath)
	if err != nil {
		renderError(w, err.Error(), http.StatusInternalServerError)
		return
	}
	defer newFile.Close() // idempotent, okay to call twice
	if _, err := newFile.Write(fileBytes); err != nil || newFile.Close() != nil {
		renderError(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Write([]byte("File Copied Successfully\n"))
	if r.MultipartForm != nil {
		r.MultipartForm.RemoveAll()
	}
}

func renderError(w http.ResponseWriter, message string, statusCode int) {
	w.WriteHeader(http.StatusBadRequest)
	w.Write([]byte(message))
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

	if n.GetConfigMode() == nmd.MgmtMode_NETWORK {

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

	// if no URL was specified, just return (used during unit/integ tests)
	if n.listenURL == "" {
		log.Errorf("Empty listenURL for REST server")
		return errors.New("ListenURL cannot be empty")
	}

	// setup the routes
	router := mux.NewRouter()
	t1 := router.Methods("POST").Subrouter()
	t1.HandleFunc(ConfigURL, httputils.MakeHTTPHandler(n.NaplesConfigHandler))
	t1.HandleFunc(RolloutURL, httputils.MakeHTTPHandler(n.NaplesRolloutHandler))
	t1.HandleFunc(UpdateURL, NaplesFileUploadHandler)

	t2 := router.Methods("GET").Subrouter()
	t2.HandleFunc(ConfigURL, httputils.MakeHTTPHandler(n.NaplesGetHandler))
	t2.HandleFunc(CmdEXECUrl, httputils.MakeHTTPHandler(NaplesCmdExecHandler))
	t2.HandleFunc("/api/{*}", unknownAction)
	t2.HandleFunc("/debug/pprof/", pprof.Index)
	t2.HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	t2.HandleFunc("/debug/pprof/profile", pprof.Profile)
	t2.HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	t2.HandleFunc("/debug/pprof/trace", pprof.Trace)
	t2.HandleFunc("/debug/pprof/allocs", pprof.Handler("allocs").ServeHTTP)
	t2.HandleFunc("/debug/pprof/block", pprof.Handler("block").ServeHTTP)
	t2.HandleFunc("/debug/pprof/heap", pprof.Handler("heap").ServeHTTP)
	t2.HandleFunc("/debug/pprof/mutex", pprof.Handler("mutex").ServeHTTP)
	t2.HandleFunc("/debug/pprof/goroutine", pprof.Handler("goroutine").ServeHTTP)
	t2.HandleFunc("/debug/pprof/threadcreate", pprof.Handler("threadcreate").ServeHTTP)

	router.Methods("DELETE").Subrouter().HandleFunc(CoresURL+"{*}", httputils.MakeHTTPHandler(NaplesCoreDeleteHandler))

	router.PathPrefix(MonitoringURL + "logs/").Handler(http.StripPrefix(MonitoringURL+"logs/", http.FileServer(http.Dir(globals.PenCtlLogDir))))
	router.PathPrefix(MonitoringURL + "obfl/").Handler(http.StripPrefix(MonitoringURL+"obfl/", http.FileServer(http.Dir(globals.ObflLogDir))))
	router.PathPrefix(MonitoringURL + "events/").Handler(http.StripPrefix(MonitoringURL+"events/", http.FileServer(http.Dir(globals.EventsDir))))
	router.PathPrefix(CoresURL).Handler(http.StripPrefix(CoresURL, http.FileServer(http.Dir(globals.CoresDir))))
	router.PathPrefix(UpdateURL).Handler(http.StripPrefix(UpdateURL, http.FileServer(http.Dir(globals.UpdateDir))))

	// create listener
	listener, err := net.Listen("tcp", n.listenURL)
	if err != nil {
		log.Errorf("Error starting listener. Err: %v", err)
		return err
	}

	// Init & create a http server
	n.listener = listener
	n.httpServer = &http.Server{Addr: n.listenURL, Handler: router}
	n.isRestSrvRunning = true

	log.Infof("Started NMD Rest server at %s", n.GetListenURL())
	os.Setenv("PATH", os.Getenv("PATH")+":/platform/bin:/nic/bin:/platform/tools:/nic/tools")

	// Launch the server
	go n.httpServer.Serve(listener)

	return nil
}

// StopRestServer stops the http server
func (n *NMD) StopRestServer(shutdown bool) error {
	n.Lock()
	defer n.Unlock()

	// TODO: This code has to go once we have Delphi integrated and we should use Delphi instead
	var postData interface{}
	var resp interface{}
	netutils.HTTPPost("http://localhost:8888/revproxy/stop", &postData, &resp)

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

// GetNMDRolloutURL returns the REST URL
func (n *NMD) GetNMDRolloutURL() string {
	return "http://" + n.GetListenURL() + RolloutURL
}

// GetNMDCmdExecURL returns the REST URL
func (n *NMD) GetNMDCmdExecURL() string {
	return "http://" + n.GetListenURL() + CmdEXECUrl
}

// GetGetNMDUploadURL returns the REST URL
func (n *NMD) GetGetNMDUploadURL() string {
	return "http://" + n.GetListenURL() + UpdateURL
}

func (n *NMD) initTLSProvider() error {
	// Instantiate a KeyMgr to store the cluster certificate and a TLS provider
	// to use it to connect to other cluster components.
	// Keys are not persisted. They will be refreshed next time we access the cluster
	// or when they expire (TBD).
	tlsProvider, err := tlsproviders.NewDefaultKeyMgrBasedProvider("")
	if err != nil {
		return errors.Wrapf(err, "Error instantiating tls provider")
	}
	n.tlsProvider = tlsProvider
	return nil
}

func (n *NMD) setClusterCredentials(resp *grpc.NICAdmissionResponse) error {
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
			trustRoots = append(trustRoots, cert)
		}
	}
	n.tlsProvider.SetTrustRoots(trustRoots)

	// Persist trust roots so that we remember what is the last Venice cluster we connected to
	// and we can authenticate offline credentials signed by Venice CA.
	err = certs.SaveCertificates(clusterTrustRootsFile, trustRoots)
	if err != nil {
		return fmt.Errorf("Error storing cluster trust roots in %s: %v", clusterTrustRootsFile, err)
	}

	return nil
}

// RegisterROCtrlClient registers client of RolloutController to NMD
func (n *NMD) RegisterROCtrlClient(rollout nmdapi.RolloutCtrlAPI) error {

	n.Lock()
	defer n.Unlock()

	// ensure two clients dont register
	if n.rollout != nil {
		log.Fatalf("Attempt to register multiple rollout clients with NMD.")
	}

	// initialize rollout
	n.rollout = rollout

	return nil
}

// NaplesCoreDeleteHandler is the REST handler for Naples delete core file operation
func NaplesCoreDeleteHandler(r *http.Request) (interface{}, error) {
	resp := NaplesConfigResp{}
	file := globals.CoresDir + "/" + strings.TrimPrefix(r.URL.Path, CoresURL)
	err := os.Remove(file)
	if err != nil {
		resp.ErrorMsg = err.Error()
		return resp, err
	}
	return resp, nil
}

func naplesExecCmd(req *nmd.NaplesCmdExecute) (string, error) {
	parts := strings.Fields(req.Opts)
	if req.Executable == "/bin/date" && req.Opts != "" {
		parts = strings.SplitN(req.Opts, " ", 2)
	}
	cmd := exec.Command(req.Executable, parts...)
	stdoutStderr, err := cmd.CombinedOutput()
	if err != nil {
		return string(fmt.Sprintf(err.Error()) + ":" + string(stdoutStderr)), err
	}
	return string(stdoutStderr), nil
}

func naplesPkgVerify(pkgName string) (string, error) {
	v := &nmd.NaplesCmdExecute{
		Executable: "/nic/tools/fwupdate",
		Opts:       strings.Join([]string{"-p ", "/update/" + pkgName, " -v"}, ""),
	}
	return naplesExecCmd(v)
}

func naplesPkgInstall(pkgName string) (string, error) {
	v := &nmd.NaplesCmdExecute{
		Executable: "/nic/tools/fwupdate",
		Opts:       strings.Join([]string{"-p ", "/update/" + pkgName, " -i all"}, ""),
	}
	return naplesExecCmd(v)
}

func naplesSetBootImg() (string, error) {
	v := &nmd.NaplesCmdExecute{
		Executable: "/nic/tools/fwupdate",
		Opts:       strings.Join([]string{"-s ", "altfw"}, ""),
	}
	return naplesExecCmd(v)
}

func naplesDelBootImg(pkgName string) (string, error) {
	v := &nmd.NaplesCmdExecute{
		Executable: "rm",
		Opts:       strings.Join([]string{"-rf ", "/update/" + pkgName}, ""),
	}
	return naplesExecCmd(v)
}

func naplesHostDisruptiveUpgrade(pkgName string) (string, error) {
	if resp, err := naplesPkgInstall(pkgName); err != nil {
		return resp, err
	}
	if resp, err := naplesSetBootImg(); err != nil {
		return resp, err
	}
	if resp, err := naplesDelBootImg(pkgName); err != nil {
		return resp, err
	}
	return "", nil
}

//NaplesCmdExecHandler is the REST handler to execute any binary on naples and return the output
func NaplesCmdExecHandler(r *http.Request) (interface{}, error) {
	req := nmd.NaplesCmdExecute{}
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

	log.Infof("Naples Cmd Execute Request: %+v env: [%s]", req, os.Environ())
	return naplesExecCmd(&req)
}
