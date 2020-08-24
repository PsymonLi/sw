package services

import (
	"context"
	"encoding/binary"
	"errors"
	"fmt"
	"net"
	goruntime "runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	cmd "github.com/pensando/sw/api/generated/cluster"
	diagapi "github.com/pensando/sw/api/generated/diagnostics"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/routing"
	"github.com/pensando/sw/events/generated/eventtypes"
	pdstypes "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	msTypes "github.com/pensando/sw/nic/metaswitch/gen/agent/pds_ms"
	"github.com/pensando/sw/nic/metaswitch/rtrctl/utils"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/perseus/env"
	"github.com/pensando/sw/venice/perseus/types"
	vldtor "github.com/pensando/sw/venice/utils/apigen/validators"
	"github.com/pensando/sw/venice/utils/diagnostics"
	diagsvc "github.com/pensando/sw/venice/utils/diagnostics/service"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

type snicT struct {
	name   string
	phase  string
	ip     string
	uuid   string
	pushed bool
}

type peerState struct {
	state bool
	epoch uint64
}

func pdsStateToString(in pdstypes.BGPPeerState) string {
	switch in {
	case pdstypes.BGPPeerState_BGP_PEER_STATE_ESTABLISHED:
		return "Established"
	case pdstypes.BGPPeerState_BGP_PEER_STATE_ACTIVE:
		return "Active"
	case pdstypes.BGPPeerState_BGP_PEER_STATE_CONNECT:
		return "Connect"
	case pdstypes.BGPPeerState_BGP_PEER_STATE_IDLE:
		return "Idle"
	case pdstypes.BGPPeerState_BGP_PEER_STATE_OPENCONFIRM:
		return "OpenConfirm"
	case pdstypes.BGPPeerState_BGP_PEER_STATE_OPENSENT:
		return "OpenSent"
	}
	return "Unknown"
}

// ServiceHandlers holds all the servies to be supported
type ServiceHandlers struct {
	sync.Mutex
	updated        bool
	pegasusURL     string
	cfgWatcherSvc  types.CfgWatcherService
	pegasusClient  pdstypes.BGPSvcClient
	ifClient       pdstypes.IfSvcClient
	routeSvc       pdstypes.CPRouteSvcClient
	pegasusMon     msTypes.EpochSvcClient
	apiclient      apiclient.Services
	snicMapMu      sync.RWMutex
	snicMap        map[string]*snicT
	naplesTemplate *network.BGPNeighbor
	ctx            context.Context
	cancelFn       context.CancelFunc
	monitorPort    net.Listener
	stallMonitor   bool
	grpcSvc        *rpckit.RPCServer
	evRecorder     events.Recorder
	peerTracker    map[string]*peerState
}

// NewServiceHandlers returns a Service Handler
func NewServiceHandlers() *ServiceHandlers {
	m := ServiceHandlers{
		cfgWatcherSvc: env.CfgWatcherService,
	}
	m.cfgWatcherSvc.SetSmartNICEventHandler(m.HandleSmartNICEvent)
	m.cfgWatcherSvc.SetNetworkInterfaceEventHandler(m.HandleNetworkInterfaceEvent)
	m.cfgWatcherSvc.SetRoutingConfigEventHandler(m.HandleRoutingConfigEvent)
	m.cfgWatcherSvc.SetNodeConfigEventHandler(m.HandleNodeConfigEvent)
	m.snicMap = make(map[string]*snicT)
	m.peerTracker = make(map[string]*peerState)
	m.pegasusURL = globals.Localhost + ":" + globals.PegasusGRPCPort

	m.ctx, m.cancelFn = context.WithCancel(context.Background())
	m.initPegasus()
	var err error
	m.evRecorder, err = recorder.NewRecorder(&recorder.Config{
		Component: globals.Perseus}, log.GetDefaultInstance())
	if err != nil {
		log.Fatalf("failed to create events recorder, err: %v", err)
	}

	grpcSvc, err := rpckit.NewRPCServer(globals.Perseus, ":"+globals.PerseusGRPCPort)
	if err != nil {
		log.Fatalf("could not start the the GRPC server (%s)", err)
	}
	m.grpcSvc = grpcSvc
	routing.RegisterRoutingV1Server(grpcSvc.GrpcServer, &m)
	grpcSvc.Start()

	m.registerDebugHandlers()
	m.cfgWatcherSvc.SetHealthReportHandler(m.HandleHealthReport)

	return &m
}

// CfgAsn is the ASN for the RR config
var CfgAsn uint32

const (
	maxInitialFail = 300
	maxRunningfail = 10
)

func (m *ServiceHandlers) initPegasus() {
	m.connectToPegasus()
	go m.pollPeerState()
	m.setupLBIf()
	var doneWg sync.WaitGroup
	doneWg.Add(1)
	go func() {
		doneWg.Done()
		<-m.ctx.Done()
		m.monitorPort.Close()
		log.Errorf("Closing monitor port due to failure")
	}()
	doneWg.Add(1)
	go m.monitor(&doneWg)
	doneWg.Wait()
}

func (m *ServiceHandlers) monitor(wg *sync.WaitGroup) {
	wg.Done()
	initial := true
	failCount := 0
	var epoch uint32
	var err error

	log.Infof("waiting for %d seconds before enabling monitor", maxRunningfail*3)
	time.Sleep(maxRunningfail * time.Second * 3)
	log.Infof("starting monitor")
	m.monitorPort, err = net.Listen("tcp", "127.0.0.1:"+globals.PerseusMonitorPort)
	if err != nil {
		log.Fatalf("could not start listener on monitor por (%s)", err)
	}
	go func() {
		for {
			_, err := m.monitorPort.Accept()
			if err != nil {
				log.Errorf("failed to connect (%s)", err)
				if m.ctx.Err() != nil {
					log.Errorf("monitor port is closed, exiting (%s)", m.ctx.Err())
					return
				}
			}

		}
	}()
	ticker := time.Tick(time.Second)
	for {
		select {
		case <-ticker:
			if m.stallMonitor {
				log.Infof("got monitor stall exiting")
				m.cancelFn()
				return
			}
			if initial && (failCount > maxInitialFail) || !initial && (failCount > maxRunningfail) {
				log.Errorf("failed to get epoch from pegasus initial[%v] Count[%d]", initial, failCount)
				m.cancelFn()
				return
			}
			ctx, cancel := context.WithDeadline(m.ctx, time.Now().Add(time.Second*30))
			epResp, err := m.pegasusMon.EpochGet(ctx, &msTypes.EpochGetRequest{})
			cancel()
			if err != nil {
				failCount++
				log.Errorf("Epoch get failed (%s)", err)
				continue
			}
			if epResp.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
				failCount++
				log.Errorf("Epoch get failed (%s)(%v)", err, epResp.ApiStatus)
				continue
			}
			failCount = 0
			if initial {
				epoch = epResp.Epoch
				initial = false
			} else {
				if epoch != epResp.Epoch {
					log.Errorf("Epoch from pegasus has changed [%v]->[%v]", epoch, epResp.Epoch)
					initial = true
					var curCfg *network.RoutingConfig
					cache.Lock()
					if cache.config != nil {
						curCfg = cache.config
						cache.config = nil
					}
					cache.Unlock()
					if curCfg != nil {
						m.setupLBIf()
						err := m.configureBGP(m.ctx, curCfg)
						if err != nil {
							m.cancelFn()
							return
						}
					}
				}
			}
		}
	}
}

func (m *ServiceHandlers) registerDebugHandlers() {
	diagSvc := diagsvc.GetDiagnosticsService(globals.APIServer, k8s.GetNodeName(), diagapi.ModuleStatus_Venice, log.GetDefaultInstance())
	if err := diagSvc.RegisterHandler("Debug", diagapi.DiagnosticsRequest_Stats.String(), diagsvc.NewExpVarHandler(globals.APIServer, k8s.GetNodeName(), diagapi.ModuleStatus_Venice, log.GetDefaultInstance())); err != nil {
		log.ErrorLog("method", "registerDebugHandlers", "msg", "failed to register expvar handler", "err", err)
	}

	diagSvc.RegisterCustomAction("mon-stall", func(action string, params map[string]string) (interface{}, error) {
		m.stallMonitor = !m.stallMonitor
		return fmt.Sprintf("stall monitor [%v]", m.stallMonitor), nil
	})
	diagSvc.RegisterCustomAction("stack-trace", func(action string, params map[string]string) (interface{}, error) {
		buf := make([]byte, 1<<20)
		blen := goruntime.Stack(buf, true)
		return fmt.Sprintf("=== goroutine dump ====\n %s \n=== END ===", buf[:blen]), nil
	})
	diagnostics.RegisterService(m.grpcSvc.GrpcServer, diagSvc)
}

func (m *ServiceHandlers) setSNIC(k string, t *snicT) {
	defer m.snicMapMu.Unlock()
	m.snicMapMu.Lock()
	m.snicMap[k] = t
}

func (m *ServiceHandlers) getSNIC(k string) (*snicT, bool) {
	defer m.snicMapMu.RUnlock()
	m.snicMapMu.RLock()
	r, ok := m.snicMap[k]
	return r, ok
}

func (m *ServiceHandlers) delSNIC(k string) {
	defer m.snicMapMu.Unlock()
	m.snicMapMu.Lock()
	delete(m.snicMap, k)
}

func (m *ServiceHandlers) configurePeers() {
	defer m.snicMapMu.RUnlock()
	m.snicMapMu.RLock()
	for _, nic := range m.snicMap {
		m.configurePeer(nic, false)
	}
}

func (m *ServiceHandlers) configurePeer(nic *snicT, deleteOp bool) {
	log.Infof("configurePeer: delete [%v] - snic %+v CfgAsn %d", deleteOp, nic, CfgAsn)
	if nic.phase != "admitted" || nic.ip == "" || nic.uuid == "" || CfgAsn == 0 {
		// wait for all information to be right
		// might have to handle case where nic was admitted previously
		// TODO
		log.Infof("ignoring configure peer")
		return
	}
	if cache.config == nil {
		log.Infof("Routing config is not valid ignore peer config request")
		return
	}
	keepalive, holdtime := cache.getTimers()
	uid := cache.getUUID()
	ctx := context.TODO()
	if deleteOp != true {
		// call grpc api to configure ms
		peerReq := pdstypes.BGPPeerRequest{}
		peer := pdstypes.BGPPeerSpec{
			Id:           uid,
			PeerAddr:     ip2PDSType(nic.ip),
			LocalAddr:    ip2PDSType(""),
			RemoteASN:    CfgAsn,
			State:        pdstypes.AdminState_ADMIN_STATE_ENABLE,
			SendComm:     true,
			SendExtComm:  true,
			ConnectRetry: 60,
			KeepAlive:    keepalive,
			HoldTime:     holdtime,
			RRClient:     pdstypes.BGPPeerRRClient_BGP_PEER_RR_CLIENT,
		}
		if m.naplesTemplate != nil {
			peer.Password = []byte(m.naplesTemplate.Password)
			peer.Ttl = m.naplesTemplate.MultiHop
			if m.naplesTemplate.Shutdown {
				peer.State = pdstypes.AdminState_ADMIN_STATE_DISABLE
			} else {
				peer.State = pdstypes.AdminState_ADMIN_STATE_ENABLE
			}
		}
		log.Infof("Add create peer [%+v]", peer)
		peerReq.Request = append(peerReq.Request, &peer)
		if nic.pushed {
			presp, err := m.pegasusClient.BGPPeerUpdate(ctx, &peerReq)
			if err != nil || presp.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
				log.Errorf("Peer update Request returned (%v)[%+v]", err, presp)
			}
		} else {
			presp, err := m.pegasusClient.BGPPeerCreate(ctx, &peerReq)
			if err != nil || presp.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
				log.Errorf("Peer create Request returned (%v)[%+v]", err, presp)
			} else {
				nic.pushed = true
			}
		}

	} else {
		if nic.pushed {
			unkLocal := &pdstypes.IPAddress{
				Af: pdstypes.IPAF_IP_AF_INET,
			}
			peerDReq := pdstypes.BGPPeerDeleteRequest{}
			peer := pdstypes.BGPPeerKeyHandle{
				IdOrKey: &pdstypes.BGPPeerKeyHandle_Key{Key: &pdstypes.BGPPeerKey{PeerAddr: ip2PDSType(nic.ip), LocalAddr: unkLocal}},
			}
			log.Infof("Add Delete peer [%+v]", peer)
			peerDReq.Request = append(peerDReq.Request, &peer)
			presp, err := m.pegasusClient.BGPPeerDelete(ctx, &peerDReq)
			if err != nil || presp.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
				log.Errorf("Peer delete Request returned (%v)[%+v]", err, presp)
			} else {
				log.Infof("Peer delete request succeeded (%s)[%v]", err, presp.ApiStatus)
				nic.pushed = false
			}
		}
	}

	if !deleteOp {
		peerAfReq := pdstypes.BGPPeerAfRequest{
			Request: []*pdstypes.BGPPeerAfSpec{
				{
					Id:          uid,
					PeerAddr:    ip2PDSType(nic.ip),
					LocalAddr:   ip2PDSType(""),
					NexthopSelf: false,
					DefaultOrig: false,
					Afi:         pdstypes.BGPAfi_BGP_AFI_L2VPN,
					Safi:        pdstypes.BGPSafi_BGP_SAFI_EVPN,
				},
			},
		}
		presp, err := m.pegasusClient.BGPPeerAfCreate(ctx, &peerAfReq)
		if err != nil || presp.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
			log.Errorf("Peer AF create Request returned (%v)[%+v]", err, presp)
		}
	}

	m.updated = true
}

func (m *ServiceHandlers) handleCreateUpdateSmartNICObject(evtNIC *cmd.DistributedServiceCard) {
	snic, ok := m.getSNIC(evtNIC.ObjectMeta.Name)
	if !ok {
		snic = &snicT{}
	}
	snic.name = evtNIC.ObjectMeta.Name
	snic.phase = evtNIC.Status.AdmissionPhase
	snic.uuid = evtNIC.UUID
	// snic.pushed = false
	m.setSNIC(evtNIC.ObjectMeta.Name, snic)
	m.configurePeer(snic, false)
}

func (m *ServiceHandlers) handleDeleteSmartNICObject(evtNIC *cmd.DistributedServiceCard) {
	snic, ok := m.getSNIC(evtNIC.ObjectMeta.Name)
	if !ok {
		log.Infof("got delete DSC [%v] but not found", evtNIC.Name)
		return
	}
	if snic.pushed == true {
		m.configurePeer(snic, true)
	}
	m.delSNIC(evtNIC.ObjectMeta.Name)
}

func (m *ServiceHandlers) handleCreateUpdateNetIntfObject(evtIntf *network.NetworkInterface) {
	snic, ok := m.getSNIC(evtIntf.Status.DSC)
	if !ok {
		snic = &snicT{}
	}
	snic.name = evtIntf.Status.DSC
	oldip := snic.ip
	if evtIntf.Spec.IPConfig != nil {
		ip, _, err := net.ParseCIDR(evtIntf.Spec.IPConfig.IPAddress)
		if err != nil {
			log.Errorf("could not parse IP CIDR (%s)", err)
			return
		}
		if ip.IsUnspecified() {
			log.Infof("IP is unspecified [%v]", ip.String())
			snic.ip = ""
		} else {
			log.Infof("IP is [%v]", ip.String())
			snic.ip = ip.String()
		}
	} else {
		snic.ip = ""
	}
	if oldip != "" && oldip != snic.ip {
		ip := snic.ip
		snic.ip = oldip
		m.configurePeer(snic, true)
		snic.ip = ip
	}
	m.setSNIC(evtIntf.Status.DSC, snic)
	m.configurePeer(snic, false)
}

func (m *ServiceHandlers) handleBGPConfigChange() {
	m.snicMapMu.RLock()
	for _, nic := range m.snicMap {
		m.configurePeer(nic, false)
	}
	m.snicMapMu.RUnlock()
}

func (m *ServiceHandlers) handleBGPConfigDelete() {
	m.snicMapMu.RLock()
	for _, nic := range m.snicMap {
		m.configurePeer(nic, true)
	}
	m.snicMapMu.RUnlock()
}

func (m *ServiceHandlers) handleDeleteNetIntfObject(evtIntf *network.NetworkInterface) {
	snic, ok := m.getSNIC(evtIntf.Status.DSC)
	if !ok {
		log.Infof("got delete for Netif [%v] but DSC [%v] not found", evtIntf.Name, evtIntf.Status.DSC)
		return
	}
	if snic.pushed == true {
		m.configurePeer(snic, true)
	}
	m.delSNIC(evtIntf.Status.DSC)
}

func isEvtTypeCreatedupdated(et kvstore.WatchEventType) bool {
	if et == "Created" || et == "Updated" {
		return true
	}
	return false
}

func isEvtTypeDeleted(et kvstore.WatchEventType) bool {
	if et == "Deleted" {
		return true
	}
	return false
}

// HandleSmartNICEvent handles SmartNIC updates
func (m *ServiceHandlers) HandleSmartNICEvent(et kvstore.WatchEventType, evtNIC *cmd.DistributedServiceCard) {
	log.Infof("HandleSmartNICEvent called: NIC: %+v event type: %v\n", *evtNIC, et)
	if isEvtTypeCreatedupdated(et) {
		m.handleCreateUpdateSmartNICObject(evtNIC)
	} else if isEvtTypeDeleted(et) {
		m.handleDeleteSmartNICObject(evtNIC)
	} else {
		log.Fatalf("unexpected event received")
	}
	return
}

// HandleNetworkInterfaceEvent handles SmartNIC updates
func (m *ServiceHandlers) HandleNetworkInterfaceEvent(et kvstore.WatchEventType, evtIntf *network.NetworkInterface) {
	log.Infof("HandleNetworkInterfaceEvent called: Intf: %+v event type: %v\n", *evtIntf, et)
	if isEvtTypeCreatedupdated(et) {
		m.handleCreateUpdateNetIntfObject(evtIntf)
	} else if isEvtTypeDeleted(et) {
		m.handleDeleteNetIntfObject(evtIntf)
	} else {
		log.Fatalf("unexpected event received")
	}
	return
}

// HandleHealthReport handles health reports
func (m *ServiceHandlers) HandleHealthReport(in types.HealthReport) {
	if !in.Healthy {
		log.Errorf("got unhealthy report, triggering restart[%+v]", in)
		m.stallMonitor = true
	}
}

func (m *ServiceHandlers) pollPeerState() {
	log.Infof("Starting BGP Peer polling")
	tick := time.Tick(time.Second * 5)
	var epoch, ups, downs uint64
	for {
		select {
		case <-m.ctx.Done():
			log.Infof("got cancel, exiting BGP Peer polling")
			return
		case <-tick:
			epoch++
			resp, err := m.pegasusClient.BGPPeerGet(context.Background(), &pdstypes.BGPPeerGetRequest{})
			if err == nil && resp.ApiStatus == pdstypes.ApiStatus_API_STATUS_OK {
				if epoch%12 == 0 {
					log.Infof("epoch [%v] got [%d] peers : ups [%d] downs[%d]", epoch, len(resp.Response), ups, downs)
				}
				for _, peer := range resp.Response {
					p, ok := m.peerTracker[pdsIPToString(peer.Spec.PeerAddr)]
					if !ok {
						state := peerStateToBool(peer.Status)
						m.peerTracker[pdsIPToString(peer.Spec.PeerAddr)] = &peerState{epoch: epoch, state: state}
						if state {
							recorder.Event(eventtypes.BGP_SESSION_ESTABLISHED, fmt.Sprintf("Peer %v", pdsIPToString(peer.Spec.PeerAddr)), nil)
							log.Infof("Peer EVENT: Peer up [%v]", pdsIPToString(peer.Spec.PeerAddr))
							ups++
						} else {
							recorder.Event(eventtypes.BGP_SESSION_DOWN, fmt.Sprintf("Peer %v", pdsIPToString(peer.Spec.PeerAddr)), nil)
							log.Infof("Peer EVENT: Peer down [%v]", pdsIPToString(peer.Spec.PeerAddr))
							downs++
						}
					} else {
						if p.state != peerStateToBool(peer.Status) {
							if peerStateToBool(peer.Status) {
								recorder.Event(eventtypes.BGP_SESSION_ESTABLISHED, fmt.Sprintf("Peer %v", pdsIPToString(peer.Spec.PeerAddr)), nil)
								log.Infof("Peer EVENT: Peer up [%v]", pdsIPToString(peer.Spec.PeerAddr))
								ups++
							} else {
								recorder.Event(eventtypes.BGP_SESSION_DOWN, fmt.Sprintf("Peer %v", pdsIPToString(peer.Spec.PeerAddr)), nil)
								log.Infof("Peer EVENT: Peer down [%v]", pdsIPToString(peer.Spec.PeerAddr))
								downs++
							}
						}
						p.epoch = epoch
						p.state = peerStateToBool(peer.Status)
						m.peerTracker[pdsIPToString(peer.Spec.PeerAddr)] = p
					}
				}
				delKeys := []string{}
				for k, p := range m.peerTracker {
					if p.epoch != epoch {
						recorder.Event(eventtypes.BGP_SESSION_DOWN, fmt.Sprintf("Peer %v, State Deleted", k), nil)
						log.Infof("Peer EVENT: Peer deleted [%v]", k)
						downs++
						delKeys = append(delKeys, k)
					}
				}
				for _, k := range delKeys {
					delete(m.peerTracker, k)
				}
			}
		}
	}

}

func pdsIPToString(in *pdstypes.IPAddress) string {
	if in == nil {
		return ""
	}
	if in.Af == pdstypes.IPAF_IP_AF_INET {
		ip := make(net.IP, 4)
		binary.BigEndian.PutUint32(ip, in.GetV4Addr())
		return ip.String()
	}
	return ""
}

func peerStateToBool(p *pdstypes.BGPPeerStatus) bool {
	if p == nil {
		return false
	}
	return p.Status == pdstypes.BGPPeerState_BGP_PEER_STATE_ESTABLISHED
}

type monitorSvc struct{}

// AutoWatchSvcRoutingV1 is a service watcher
func (m *ServiceHandlers) AutoWatchSvcRoutingV1(*api.AggWatchOptions, routing.RoutingV1_AutoWatchSvcRoutingV1Server) error {
	return fmt.Errorf("not implemented")
}

func getPeerCfgFromCache(peer string) (*network.BGPNeighbor, error) {
	if cache.config.Spec.BGPConfig != nil {
		for _, n := range cache.config.Spec.BGPConfig.Neighbors {
			if n.IPAddress == peer {
				return n, nil
			}
		}
		return nil, fmt.Errorf("peer not in cache")
	}
	return nil, fmt.Errorf("BGPConfig not in cache")
}

func validateListRoutesReq(in *routing.RouteFilter) error {
	if strings.Compare(in.Type, "ipv4-unicast") != 0 && strings.Compare(in.Type, "l2vpn-evpn") != 0 {
		return fmt.Errorf("Unknown route-type. valid options are: ipv4-unicast/l2vpn-evpn")
	}
	if in.ExtComm != "" {
		if utils.ExtCommToBytes(in.ExtComm) == nil {
			return fmt.Errorf("Invalid extended community ID %s", in.ExtComm)
		}
	}
	if in.Vnid != "" {
		_, ok := strconv.ParseUint(in.Vnid, 10, 32)
		if ok != nil {
			return fmt.Errorf("Invalid vni-id %s", in.Vnid)
		}
	}
	if in.RType != "" {
		t, ok := strconv.ParseUint(in.RType, 10, 32)
		if ok != nil {
			return fmt.Errorf("Invalid route type %s. <1-5> are valid types", in.RType)
		}
		if t < 1 || t > 5 {
			return fmt.Errorf("Invalid route type %s. <1-5> are valid types", in.RType)
		}
	}
	if len(in.NHop) != 0 && vldtor.IPAddr(in.NHop) != nil {
		return fmt.Errorf("Invalid nextHop address %v", in.NHop)
	}
	return nil
}

// ListRoutes lists routes
func (m *ServiceHandlers) ListRoutes(ctx context.Context, in *routing.RouteFilter) (*routing.RouteList, error) {
	log.Infof("got call for ListRoutes [%+v]", in)
	var req pdstypes.BGPNLRIPrefixGetRequest
	if err := validateListRoutesReq(in); err != nil {
		return nil, err
	}

	var rType uint32
	var vnid uint32
	var nhip *pdstypes.IPAddress
	ec := []byte{}
	filter := false

	if in.ExtComm != "" {
		filter = true
		ec = utils.ExtCommToBytes(in.ExtComm)
	}
	if in.Vnid != "" {
		filter = true
		v, _ := strconv.ParseUint(in.Vnid, 10, 32)
		vnid = uint32(v)
	}
	if in.RType != "" {
		filter = true
		t, _ := strconv.ParseUint(in.RType, 10, 32)
		rType = uint32(t)
	}
	if in.NHop != "" {
		filter = true
		nhip = utils.IPAddrStrToPdsIPAddr(in.NHop)
	}

	if filter == true {
		req = pdstypes.BGPNLRIPrefixGetRequest{
			RequestsOrFilter: &pdstypes.BGPNLRIPrefixGetRequest_Filter{
				Filter: &pdstypes.BGPNLRIPrefixFilter{
					ExtComm:   ec,
					Vnid:      vnid,
					RouteType: rType,
					NextHop:   nhip,
				},
			},
		}
	}

	respMsg, err := m.pegasusClient.BGPNLRIPrefixGet(context.Background(), &req)
	if err != nil {
		return nil, fmt.Errorf("Getting Routes failed (%s)", err)
	}
	if respMsg.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
		return nil, errors.New("Operation failed with error")
	}
	routes := &routing.RouteList{}
	routes.TypeMeta.Kind = "RouteList"
	for _, p := range respMsg.Response {
		nlri := utils.NewBGPNLRIPrefixStatus(p.Status, true)

		//Print AFI/SAFI info
		afi := strings.TrimPrefix(nlri.Afi.String(), "BGP_AFI_")
		safi := strings.TrimPrefix(nlri.Safi.String(), "BGP_SAFI_")

		//Lets check if its right afi/safi
		afisafi := strings.ToLower(afi) + "-" + strings.ToLower(safi)
		if strings.Compare(in.Type, afisafi) != 0 {
			continue
		}
		route := &routing.Route{
			Status: routing.RouteStatus{
				Prefix:           nlri.Prefix.String(),
				PrefixLen:        fmt.Sprint(nlri.PrefixLen),
				ASPath:           nlri.ASPathStr,
				PathOrigId:       nlri.PathOrigId,
				NextHopAddr:      nlri.NextHopAddr,
				RouteSource:      nlri.RouteSource,
				IsActive:         nlri.IsActive,
				ReasonNotBest:    nlri.ReasonNotBest,
				PeerAddr:         nlri.PeerAddr,
				BestRoute:        nlri.BestRoute,
				EcmpRoute:        nlri.EcmpRoute,
				FlapStatsFlapcnt: nlri.FlapStatsFlapcnt,
				FlapStatsSupprsd: nlri.FlapStatsSupprsd,
				Stale:            nlri.Stale,
				FlapStartTime:    nlri.FlapStartTime,
				ExtComm:          nlri.ExtComm,
			},
		}
		route.TypeMeta.Kind = "Route"
		routes.Items = append(routes.Items, route)
	}
	return routes, nil
}

// ListNeighbors lists neighbors
func (m *ServiceHandlers) ListNeighbors(ctx context.Context, in *routing.NeighborFilter) (*routing.NeighborList, error) {
	log.Infof("got call for ListNeighbors [%+v]", in)
	respMsg, err := m.pegasusClient.BGPPeerGet(context.Background(), &pdstypes.BGPPeerGetRequest{})
	if err != nil {
		return nil, fmt.Errorf("Getting Peers failed (%s)", err)
	}
	if respMsg.ApiStatus != pdstypes.ApiStatus_API_STATUS_OK {
		return nil, errors.New("Operation failed with error")
	}
	neighbors := &routing.NeighborList{}
	neighbors.TypeMeta.Kind = "NeighborList"
	for _, p := range respMsg.Response {
		peer := utils.NewBGPPeer(p)
		var n *network.BGPNeighbor
		if ne, ok := getPeerCfgFromCache(peer.Spec.PeerAddr); ok != nil {
			log.Infof("did not get peer back from cache [%+v]", peer)
			n = ne
		}
		neighbor := &routing.Neighbor{
			Status: routing.NeighborStatus{
				Status:             peer.Status.Status,
				PrevStatus:         peer.Status.PrevStatus,
				LastErrorRcvd:      peer.Status.LastErrorRcvd,
				LastErrorSent:      peer.Status.LastErrorSent,
				LocalAddr:          peer.Status.LocalAddr,
				HoldTime:           peer.Status.HoldTime,
				KeepAlive:          peer.Status.KeepAlive,
				CapsSent:           peer.Status.CapsSent,
				CapsRcvd:           peer.Status.CapsRcvd,
				CapsNeg:            peer.Status.CapsNeg,
				SelLocalAddrType:   peer.Status.SelLocalAddrType,
				InNotifications:    peer.Status.InNotifications,
				OutNotifications:   peer.Status.OutNotifications,
				InUpdates:          peer.Status.InUpdates,
				OutUpdates:         peer.Status.OutUpdates,
				InKeepalives:       peer.Status.InKeepalives,
				OutKeepalives:      peer.Status.OutKeepalives,
				InRefreshes:        peer.Status.InRefreshes,
				OutRefreshes:       peer.Status.OutRefreshes,
				InTotalMessages:    peer.Status.InTotalMessages,
				OutTotalMessages:   peer.Status.OutTotalMessages,
				FsmEstTransitions:  peer.Status.FsmEstTransitions,
				ConnectRetryCount:  peer.Status.ConnectRetryCount,
				Peergr:             peer.Status.Peergr,
				StalePathTime:      peer.Status.StalePathTime,
				OrfEntryCount:      peer.Status.OrfEntryCount,
				RcvdMsgElpsTime:    peer.Status.RcvdMsgElpsTime,
				RouteRefrSent:      peer.Status.RouteRefrSent,
				RouteRefrRcvd:      peer.Status.RouteRefrRcvd,
				InPrfxes:           peer.Status.InPrfxes,
				OutPrfxes:          peer.Status.OutPrfxes,
				OutUpdateElpsTime:  peer.Status.OutUpdateElpsTime,
				OutPrfxesDenied:    peer.Status.OutPrfxesDenied,
				OutPrfxesImpWdr:    peer.Status.OutPrfxesImpWdr,
				OutPrfxesExpWdr:    peer.Status.OutPrfxesExpWdr,
				InPrfxesImpWdr:     peer.Status.InPrfxesImpWdr,
				InPrfxesExpWdr:     peer.Status.InPrfxesExpWdr,
				ReceivedHoldTime:   peer.Status.ReceivedHoldTime,
				FsmEstablishedTime: peer.Status.FsmEstablishedTime,
				InUpdatesElpsTime:  peer.Status.InUpdatesElpsTime,
				InOpens:            peer.Status.InOpens,
				OutOpens:           peer.Status.OutOpens,
				PeerIndex:          peer.Status.PeerIndex,
			},
		}
		if n != nil {
			neighbor.Spec = network.BGPNeighbor{
				Shutdown:              n.Shutdown,
				DSCAutoConfig:         n.DSCAutoConfig,
				IPAddress:             n.IPAddress,
				RemoteAS:              n.RemoteAS,
				MultiHop:              n.MultiHop,
				KeepaliveInterval:     n.KeepaliveInterval,
				Holdtime:              n.Holdtime,
				EnableAddressFamilies: n.EnableAddressFamilies,
			}
		} else {
			neighbor.Spec.Shutdown = true
			if peer.Spec.State == "ENABLE" {
				neighbor.Spec.Shutdown = false
			}
			neighbor.Spec = network.BGPNeighbor{
				IPAddress: peer.Spec.PeerAddr,
				RemoteAS: api.BgpAsn{
					ASNumber: peer.Spec.RemoteASN,
				},
				MultiHop:              peer.Spec.Ttl,
				KeepaliveInterval:     peer.Spec.KeepAlive,
				Holdtime:              peer.Spec.HoldTime,
				EnableAddressFamilies: []string{network.BGPAddressFamily_L2vpnEvpn.String()},
			}
		}
		neighbor.ObjectMeta.Name = neighbor.Spec.IPAddress + "-" + strings.Join(neighbor.Spec.EnableAddressFamilies, "-")
		neighbor.TypeMeta.Kind = "Neighbor"
		neighbors.Items = append(neighbors.Items, neighbor)
	}
	return neighbors, nil
}

type peerHealth struct {
	peer     string
	estab    bool
	internal bool
}

// HealthZ returns the health of the RR
func (m *ServiceHandlers) HealthZ(ctx context.Context, in *routing.EmptyReq) (*routing.Health, error) {
	log.Infof("got call for HealthZ [%+v]", in)
	ret := &routing.Health{}

	if cache.config != nil && cache.config.Spec.BGPConfig != nil {
		peerMap := make(map[string]peerHealth)
		ret.Status.RouterID = cache.config.Spec.BGPConfig.RouterId
		for _, p := range cache.config.Spec.BGPConfig.Neighbors {
			if !p.DSCAutoConfig {
				peerMap[p.IPAddress] = peerHealth{peer: p.IPAddress}
			}
		}
		m.snicMapMu.RLock()
		for _, p := range m.snicMap {
			if p.phase == cmd.DistributedServiceCardStatus_ADMITTED.String() && p.ip != "" && p.uuid != "" {
				peerMap[p.ip] = peerHealth{peer: p.ip, internal: true}
			}
		}
		m.snicMapMu.RUnlock()
		resp, err := m.pegasusClient.BGPPeerGet(context.Background(), &pdstypes.BGPPeerGetRequest{})
		if err == nil && resp.ApiStatus == pdstypes.ApiStatus_API_STATUS_OK {
			for _, p := range resp.Response {
				s, ok := peerMap[pdsIPToString(p.Spec.PeerAddr)]
				if !ok {
					ret.Status.UnexpectedPeers++
				} else {
					s.estab = peerStateToBool(p.Status)
				}
				peerMap[pdsIPToString(p.Spec.PeerAddr)] = s
			}
		} else {
			return ret, fmt.Errorf("error getting peers state (%s)", err)
		}
		var internal, internalEstab, external, externalEstab int32
		for _, s := range peerMap {
			if s.internal {
				internal++
				if s.estab {
					internalEstab++
				} else {
					ret.Status.InternalPeers.DownPeers = append(ret.Status.InternalPeers.DownPeers, s.peer)
				}
			} else {
				external++
				if s.estab {
					externalEstab++
				} else {
					ret.Status.ExternalPeers.DownPeers = append(ret.Status.ExternalPeers.DownPeers, s.peer)
				}
			}
		}
		ret.Status.ExternalPeers.Configured = external
		ret.Status.ExternalPeers.Established = externalEstab
		ret.Status.InternalPeers.Configured = internal
		ret.Status.InternalPeers.Established = internalEstab
	}

	return ret, nil
}
