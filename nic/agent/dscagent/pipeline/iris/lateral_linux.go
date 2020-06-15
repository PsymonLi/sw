// +build iris
// +build linux

package iris

import (
	"context"
	"fmt"
	"net"
	"sort"
	"sync"
	"time"

	"github.com/mdlayher/arp"
	"github.com/pensando/netlink"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	refreshDuration      = time.Duration(time.Minute * 1)
	arpResolutionTimeout = time.Duration(time.Second * 3)
)

var lateralDB = map[string][]string{}
var doneCache = map[string]context.CancelFunc{}
var destIPToMAC sync.Map
var arpCache sync.Map
var mux sync.Mutex

// ArpClient is the global arp client for netagent
var ArpClient *arp.Client

// MgmtLink is the global management link for netagent
var MgmtLink netlink.Link

// GwCache is the global cache for ip to their gateway (if needed)
var GwCache sync.Map

// CreateLateralNetAgentObjects creates lateral objects for telmetry objects and does refcounting. This is temporary code till HAL subsumes ARP'ing for dest IPs
func CreateLateralNetAgentObjects(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner string, mgmtIP, destIP, gwIP string, tunnelOp bool) error {
	log.Infof("Creating Lateral NetAgent objects. Owner: %v MgmtIP: %v DestIP: %v gatewayIP: %v TunnelOp: %v", owner, mgmtIP, destIP, gwIP, tunnelOp)
	var (
		ep                    *netproto.Endpoint
		err                   error
		objectName            string
		tunnelName            = fmt.Sprintf("_internal-%s", destIP)
		tunnelCompositeKey    = fmt.Sprintf("tunnel|%s", tunnelName)
		collectorCompositeKey string
	)

	collectorKnown, tunnelKnown := reconcileLateralObjects(infraAPI, owner, destIP, tunnelOp)
	if collectorKnown {
		var knownCollector *netproto.Endpoint
		eDat, err := infraAPI.List("Endpoint")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
		}
		for _, o := range eDat {
			var endpoint netproto.Endpoint
			err := endpoint.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
				continue
			}
			for _, address := range endpoint.Spec.IPv4Addresses {
				if address == destIP {
					knownCollector = &endpoint
					break
				}
			}
		}

		objectName = fmt.Sprintf("_internal-%s", knownCollector.Spec.MacAddress)
		collectorCompositeKey = fmt.Sprintf("collector|%s", objectName)
	}
	if collectorKnown && tunnelKnown {
		log.Info("Collector and Tunnel already known. Nothing to do here.")
		log.Infof("Lateral object DB pre refcount increment: %v", lateralDB)

		// Add refcount here
		if tunnelOp {
			lateralDB[tunnelCompositeKey] = append(lateralDB[tunnelCompositeKey], owner)
		}

		lateralDB[collectorCompositeKey] = append(lateralDB[collectorCompositeKey], owner)
		lateralDB[destIP] = append(lateralDB[destIP], owner)

		// Dedup here to handle idempotent calls to lateral methods.
		dedupReferences(collectorCompositeKey)
		dedupReferences(tunnelCompositeKey)
		dedupReferences(destIP)

		log.Infof("Lateral object DB post refcount increment: %v", lateralDB)
		return nil
	}

	log.Infof("One or more lateral object creation needed. CreateEP: %v, CreateTunnel: %v", !collectorKnown, !tunnelKnown)

	if !collectorKnown {
		ep, err = generateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, destIP, gwIP)
		if err != nil {
			return err
		}
		if ep != nil {
			dmac := ep.Spec.MacAddress
			log.Infof("Set the composite key name with dmac: %s", dmac)
			objectName = fmt.Sprintf("_internal-%s", dmac)
			collectorCompositeKey = fmt.Sprintf("collector|%s", objectName)
		}
	}

	log.Infof("Lateral object DB pre refcount increment: %v", lateralDB)

	// Add refcount here -- using this to keep the same ip ref count
	if tunnelOp {
		lateralDB[tunnelCompositeKey] = append(lateralDB[tunnelCompositeKey], owner)
	}

	// If collector unknown, leave it for go routine to modify
	if collectorKnown {
		lateralDB[collectorCompositeKey] = append(lateralDB[collectorCompositeKey], owner)
	}
	lateralDB[destIP] = append(lateralDB[destIP], owner)

	// Dedup here to handle idempotent calls to lateral methods.
	if collectorKnown {
		dedupReferences(collectorCompositeKey)
	}
	dedupReferences(tunnelCompositeKey)
	dedupReferences(destIP)

	log.Infof("Lateral object DB post refcount increment: %v", lateralDB)

	if !collectorKnown && ep != nil {
		if err := createOrUpdateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, destIP, ep.Spec.MacAddress); err != nil {
			log.Errorf("Failed to create or update lateral endpoint IP: %s mac:%s. Err: %v", destIP, ep.Spec.MacAddress, err)
			return err
		}
	}

	if tunnelOp && !tunnelKnown {
		tun := netproto.Tunnel{
			TypeMeta: api.TypeMeta{Kind: "Tunnel"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      tunnelName,
			},
			Spec: netproto.TunnelSpec{
				Type:        "GRE",
				AdminStatus: "UP",
				Src:         mgmtIP,
				Dst:         destIP,
			},
			Status: netproto.TunnelStatus{
				TunnelID: infraAPI.AllocateID(types.TunnelID, types.TunnelOffset),
			},
		}

		if _, ok := isCollectorTunnelKnown(infraAPI, tun); ok {
			log.Infof("Lateral Tunnel: %v already present. Skipping...", tun.Name)
		} else {
			log.Infof("Creating internal tunnel %v for mirror session %v", tun.ObjectMeta, owner)
			err := createTunnelHandler(infraAPI, intfClient, tun, vrfID)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorTunnelCreateFailure, "MirrorSession: %s |  Err: %v", owner, err))
				return errors.Wrapf(types.ErrCollectorTunnelCreateFailure, "MirrorSession: %s |  Err: %v", owner, err)
			}
		}
	}
	return nil
}

// DeleteLateralNetAgentObjects deletes lateral objects for telmetry objects and does refcounting. This is temporary code till HAL subsumes ARP'ing for dest IPs
func DeleteLateralNetAgentObjects(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner string, destIP, gwIP string, tunnelOp bool) error {
	log.Infof("Deleting Lateral NetAgent objects. Owner: %v DestIP: %v gatewayIP: %v TunnelOp: %v", owner, destIP, gwIP, tunnelOp)

	tunnelName := fmt.Sprintf("_internal-%s", destIP)
	tunnelCompositeKey := fmt.Sprintf("tunnel|%s", tunnelName)
	log.Infof("Lateral object destIP and tunnel DB pre refcount decrement: %v", lateralDB)

	// Remove Dependency
	counter := 0
	for _, dep := range lateralDB[tunnelCompositeKey] {
		if dep != owner {
			lateralDB[tunnelCompositeKey][counter] = dep
			counter++
		}
	}
	lateralDB[tunnelCompositeKey] = lateralDB[tunnelCompositeKey][:counter]

	counter = 0
	for _, dep := range lateralDB[destIP] {
		if dep != owner {
			lateralDB[destIP][counter] = dep
			counter++
		}
	}
	lateralDB[destIP] = lateralDB[destIP][:counter]

	log.Infof("Lateral object destIP and tunnel DB post refcount decrement: %v", lateralDB)

	// Check for pending dependencies
	if tunnelOp {
		if len(lateralDB[tunnelCompositeKey]) == 0 {
			// Find Tunnel
			t := netproto.Tunnel{
				TypeMeta: api.TypeMeta{Kind: "Tunnel"},
				ObjectMeta: api.ObjectMeta{
					Tenant:    "default",
					Namespace: "default",
					Name:      fmt.Sprintf("_internal-%s", destIP),
				},
			}
			if tun, ok := isCollectorTunnelKnown(infraAPI, t); ok {
				log.Infof("Deleting lateral tunnel %v", t.Name)
				err := deleteTunnelHandler(infraAPI, intfClient, *tun)
				if err != nil {
					log.Error(errors.Wrapf(types.ErrCollectorTunnelDeleteFailure, "MirrorSession: %s |  Err: %v", owner, err))
					return errors.Wrapf(types.ErrCollectorTunnelDeleteFailure, "MirrorSession: %s |  Err: %v", owner, err)
				}
			}
			delete(lateralDB, tunnelCompositeKey)
		} else {
			log.Infof("Disallowing deletion of lateral tunnels. Pending refs: %v", lateralDB[tunnelCompositeKey])
		}
	}

	var lateralEP *netproto.Endpoint
	eDat, err := infraAPI.List("Endpoint")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
	}
	for _, o := range eDat {
		var endpoint netproto.Endpoint
		err := endpoint.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			continue
		}
		for _, address := range endpoint.Spec.IPv4Addresses {
			if address == destIP {
				lateralEP = &endpoint
				break
			}
		}
	}

	// Remove the destIPToMAC entry for the IP and remove the static route installed via gw IP
	cleanup := func(IP, gIP string) {
		destIPToMAC.Delete(IP + "-" + gIP)
		if gIP != "" {
			_, dest, _ := net.ParseCIDR(IP + "/32")
			log.Infof("Removing route for %v via %s", dest, gIP)
			route := &netlink.Route{
				Dst: dest,
				Gw:  net.ParseIP(gIP),
			}
			if err := netlink.RouteDel(route); err != nil {
				log.Errorf("Failed to configure static route %v. Err: %v", route, err)
			}
		}
	}

	// Remove owner from lateralDB. This will remove the collector composite key as well.
	for k := range lateralDB {
		counter := 0
		for _, dep := range lateralDB[k] {
			if dep != owner {
				lateralDB[k][counter] = dep
				counter++
			}
		}
		lateralDB[k] = lateralDB[k][:counter]
	}

	arpResolverKey := destIP + "-" + gwIP
	if lateralEP == nil {
		log.Errorf("Lateral EP not found for destIP: %s", destIP)
		// Remove the control loop only if destIP is no longer referred.
		if len(lateralDB[destIP]) == 0 {
			cancel, ok := doneCache[arpResolverKey]
			if ok {
				log.Infof("Calling cancel for IP: %v gw: %v", destIP, gwIP)
				cancel()
				cleanup(destIP, gwIP)
			}
		}
		//Todo check with abhi if we have to handle idemopotncy. for idempotency case to pass retun nil
		return nil
		//return fmt.Errorf("endpoint not found Owner: %v Obj: %v", owner, destIP)
	}

	_, internalEP := GwCache.Load(destIP)
	if internalEP && len(lateralDB[destIP]) == 0 {
		if err := deleteOrUpdateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, destIP); err != nil {
			log.Errorf("Failed to delete or update lateral endpoint IP: %s. Err: %v", destIP, err)
			return err
		}
		// Cancel ARP loop if we are removing lateral objects
		cancel, ok := doneCache[arpResolverKey]
		if ok {
			log.Infof("Calling cancel for IP: %v gw: %v", destIP, gwIP)
			cancel()
			cleanup(destIP, gwIP)
		}
		delete(lateralDB, destIP)
		return nil
	}

	// if not internal ep and created by venice
	if len(lateralDB[destIP]) == 0 {
		// Cancel ARP loop if we are removing lateral objects
		cancel, ok := doneCache[arpResolverKey]
		if ok {
			log.Infof("Calling cancel for IP: %v gw: %v", destIP, gwIP)
			cancel()
			cleanup(destIP, gwIP)
		}
		delete(lateralDB, destIP)
	}
	return nil
}

func startRefreshLoop(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner string, refreshCtx context.Context, destIP, gwIP string) {
	log.Infof("Starting ARP refresh loop for %s, gatewayIP:%s", destIP, gwIP)

	var oldMAC string
	ticker := time.NewTicker(refreshDuration)
	go func(refreshCtx context.Context, IP, gIP string) {
		arpResolverKey := IP + "-" + gIP
		mac := resolveIPAddress(refreshCtx, IP, gIP)
		log.Infof("Resolved MAC 1st Tick: %s", mac)
		if mac != oldMAC {
			if err := createOrUpdateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, IP, mac); err != nil {
				log.Errorf("Failed to create or update lateral endpoint IP: %s mac:%s. Err: %v", IP, mac, err)
			}
			oldMAC = mac
			utils.RaiseEvent(eventtypes.COLLECTOR_REACHABLE, fmt.Sprintf("Netflow/ERspan collector %s is reachable from DSC %s", IP, infraAPI.GetDscName()), infraAPI.GetDscName())
		} else {
			utils.RaiseEvent(eventtypes.COLLECTOR_UNREACHABLE, fmt.Sprintf("Netflow/ERspan collector %s is not reachable from DSC %s", IP, infraAPI.GetDscName()), infraAPI.GetDscName())
		}
		// Populate the ARPCache.
		log.Infof("Populate ARP %s -> %s", arpResolverKey, mac)
		destIPToMAC.Store(arpResolverKey, mac)
		for {
			select {
			case <-ticker.C:
				mac = resolveIPAddress(refreshCtx, IP, gIP)
				if mac != oldMAC {
					if err := deleteOrUpdateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, IP); err != nil {
						log.Errorf("Failed to delete or update lateral endpoint IP: %s. Err: %v", IP, err)
					}
					if len(mac) > 0 {
						if err := createOrUpdateLateralEP(infraAPI, intfClient, epClient, vrfID, owner, IP, mac); err != nil {
							log.Errorf("Failed to create or update lateral endpoint IP: %s mac:%s. Err: %v", IP, mac, err)
						}
					}
					if mac == "" {
						utils.RaiseEvent(eventtypes.COLLECTOR_UNREACHABLE, fmt.Sprintf("Netflow/ERspan collector %s is not reachable from DSC %s", IP, infraAPI.GetDscName()), infraAPI.GetDscName())
					} else if oldMAC == "" {
						utils.RaiseEvent(eventtypes.COLLECTOR_REACHABLE, fmt.Sprintf("Netflow/ERspan collector %s is reachable from DSC %s", IP, infraAPI.GetDscName()), infraAPI.GetDscName())
					}
				}
				oldMAC = mac
				// Populate the ARPCache.
				log.Infof("Populate ARP %s -> %s", arpResolverKey, mac)
				destIPToMAC.Store(arpResolverKey, mac)
			case <-refreshCtx.Done():
				log.Infof("Cancelling ARP refresh loop for IP: %v gw: %v", IP, gIP)
				return
			}
		}
	}(refreshCtx, destIP, gwIP)
}

func resolveIPAddress(ctx context.Context, destIP, gwIP string) string {
	addrs, err := netlink.AddrList(MgmtLink, netlink.FAMILY_V4)
	if err != nil || len(addrs) == 0 {
		log.Errorf("Failed to list management interface addresses. Err: %v", err)
		// Temporary hack in Release A. generateLateralEP is expected to return errors only on ARP resolution failures.
		// Hence masking just logging the err
		return ""
	}
	mgmtSubnet := addrs[0].IPNet

	// Check if in the same subnet
	if !mgmtSubnet.Contains(net.ParseIP(destIP)) {
		if gwIP == "" {
			routes, err := netlink.RouteGet(net.ParseIP(destIP))
			if err != nil || len(routes) == 0 {
				log.Errorf("No routes found for the dest IP %s. Err: %v", destIP, err)
				return ""
			}

			// Pick the first route. Dest IP in not in the local subnet. Use GW IP as the destIP for ARP'ing
			if routes[0].Gw == nil {
				log.Errorf("Default gateway not configured for destIP %s.", destIP)
				return ""
			}
			gwIP = routes[0].Gw.String()
		}
		log.Infof("Dest IP %s not in management subnet %v. Using Gateway IP: %v  as destIP for ARPing...", destIP, mgmtSubnet, gwIP)
		// Update destIP to be GwIP
		GwCache.Store(destIP, gwIP)
		destIP = gwIP
	} else {
		GwCache.Store(destIP, destIP)
	}

	dIP := net.ParseIP(destIP)
	mac, err := resolveWithDeadline(ctx, dIP)
	if err != nil {
		log.Errorf("Could not resolve %s: %v", destIP, err)
		cachedMac, ok := arpCache.Load(destIP)
		if ok {
			log.Infof("Cached mac for %v:%v", destIP, cachedMac)
			mac = cachedMac.(string)
		}
	}

	return mac
}

// ResolveWatch looks for ARP replies and updates the ARP cache
func ResolveWatch() {
	ticker := time.NewTicker(time.Second * 1)
loop:
	for {
		select {
		case <-ticker.C:
			if ArpClient != nil {
				break loop
			}
		}
	}

	log.Infof("Starting ARP Watch loop")
	// Loop and wait for replies
	for {
		p, _, err := ArpClient.Read()
		if err != nil {
			continue
		}

		if p.Operation != arp.OperationReply {
			continue
		}

		log.Infof("Got ARP reply for %v, mac: %s", p.SenderIP.String(), p.SenderHardwareAddr.String())
		arpCache.Store(p.SenderIP.String(), p.SenderHardwareAddr.String())
	}
}

func resolveWithDeadline(ctx context.Context, IP net.IP) (string, error) {
	resolveCtx, cancel := context.WithCancel(ctx)
	defer cancel()
	arpChan := make(chan string, 1)

	go func(arpChan chan string, IP net.IP) {
		// Delete for arp Expiry to take effect
		log.Infof("Delete arpCache for %s", IP.String())
		arpCache.Delete(IP.String())
		err := ArpClient.Request(IP)
		if err != nil {
			log.Errorf("Failed to resolve MAC for %v, %v", IP.String(), err)
			return
		}
		// Check for arp reply, every 10 millisecond till 1 second
		for i := 0; i < 100; i++ {
			d, ok := arpCache.Load(IP.String())
			if !ok {
				time.Sleep(time.Millisecond * 10)
				continue
			}
			arpChan <- d.(string)
			break
		}
	}(arpChan, IP)

	select {
	case <-time.After(arpResolutionTimeout):
		cancel()
		return "", resolveCtx.Err()
	case mac := <-arpChan:
		return mac, nil
	}
}

func updateArpClient(mgmtIntf *net.Interface, mgmtLink netlink.Link) {
	if mgmtIntf == nil {
		return
	}
	// Check for idempotency and close older ARP clients
	if ArpClient != nil {
		ArpClient.Close()
	}
	client, err := arp.Dial(mgmtIntf)
	if err != nil {
		return
	}
	ArpClient = client
	MgmtLink = mgmtLink
}

// IPWatch starts a watch for IP on bond0 interface and updates ArpClient and MgmtIntf
func IPWatch(infraAPI types.InfraAPI) {
	var mgmtIP string
	mgmtIntf, mgmtLink, err := utils.GetMgmtInfo(infraAPI.GetConfig())
	if err != nil {
		log.Errorf("Failed to get the mgmt information. config: %v: %v", infraAPI.GetConfig(), err)
	} else {
		mgmtIP = utils.GetMgmtIP(mgmtLink)
	}
	updateArpClient(mgmtIntf, mgmtLink)
	go func() {
		ticker := time.NewTicker(time.Second * 1)
		for {
			select {
			case <-ticker.C:
				var ip string
				mgmtIntf, mgmtLink, err := utils.GetMgmtInfo(infraAPI.GetConfig())
				if err == nil {
					ip = utils.GetMgmtIP(mgmtLink)
					if ip != "" && ip != mgmtIP {
						log.Infof("bond0 IP changed from %s to %s. Updating ArpClient", mgmtIP, ip)
						updateArpClient(mgmtIntf, mgmtLink)
						mgmtIP = ip
					}
				}
			}
		}
	}()
}

func deleteOrUpdateLateralEP(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner, destIP string) error {
	// Make sure only one go routine is updating EP at a point
	mux.Lock()
	defer mux.Unlock()

	var lateralEP *netproto.Endpoint
	eDat, err := infraAPI.List("Endpoint")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
	}
	for _, o := range eDat {
		var endpoint netproto.Endpoint
		err := endpoint.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			continue
		}
		for _, address := range endpoint.Spec.IPv4Addresses {
			if address == destIP {
				lateralEP = &endpoint
				break
			}
		}
	}

	if lateralEP == nil {
		log.Errorf("Lateral EP not found for destIP: %s owner: %s", destIP, owner)
		return nil
	}

	objName := fmt.Sprintf("_internal-%s", lateralEP.Spec.MacAddress)
	collectorCompositeKey := fmt.Sprintf("collector|%s", objName)
	log.Infof("Lateral object endpoint DB pre refcount decrement: %v owner: %s", lateralDB, owner)
	counter := 0
	for _, dep := range lateralDB[collectorCompositeKey] {
		if dep != owner {
			lateralDB[collectorCompositeKey][counter] = dep
			counter++
		}
	}
	lateralDB[collectorCompositeKey] = lateralDB[collectorCompositeKey][:counter]

	log.Infof("Lateral object endpoint DB post refcount decrement: %v", lateralDB)
	_, internalEP := GwCache.Load(destIP)
	if internalEP {
		// Check for pending dependencies
		if len(lateralDB[collectorCompositeKey]) == 0 {
			log.Infof("Deleting lateral endpoint %v", lateralEP.ObjectMeta.Name)
			err := deleteEndpointHandler(infraAPI, epClient, intfClient, *lateralEP, vrfID, types.UntaggedCollVLAN)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrCollectorEPDeleteFailure, "MirrorSession: %s |  Err: %v", owner, err))
				return errors.Wrapf(types.ErrCollectorEPDeleteFailure, "MirrorSession: %s |  Err: %v", owner, err)
			}
			delete(lateralDB, collectorCompositeKey)
		} else {
			for idx, dep := range lateralEP.Spec.IPv4Addresses {
				if dep == destIP {
					log.Infof("Updating endpoint: %s", lateralEP.GetKey())
					ep := &netproto.Endpoint{
						TypeMeta: api.TypeMeta{Kind: "Endpoint"},
						ObjectMeta: api.ObjectMeta{
							Tenant:    "default",
							Namespace: "default",
							Name:      lateralEP.Name,
						},
						Spec: netproto.EndpointSpec{
							NetworkName: lateralEP.Spec.NetworkName,
							NodeUUID:    lateralEP.Spec.NodeUUID,
							MacAddress:  lateralEP.Spec.MacAddress,
						},
					}
					ep.Spec.IPv4Addresses = append(ep.Spec.IPv4Addresses, lateralEP.Spec.IPv4Addresses[:idx]...)
					ep.Spec.IPv4Addresses = append(ep.Spec.IPv4Addresses, lateralEP.Spec.IPv4Addresses[idx+1:]...)
					if err = updateEndpointHandler(infraAPI, epClient, intfClient, *ep, vrfID, types.UntaggedCollVLAN); err != nil {
						log.Error(errors.Wrapf(types.ErrCollectorEPUpdateFailure, "MirrorSession: %s |  Err: %v", owner, err))
						return errors.Wrapf(types.ErrCollectorEPUpdateFailure, "MirrorSession: %s |  Err: %v", owner, err)
					}
					break
				}
			}
		}
		return nil
	}
	if len(lateralDB[collectorCompositeKey]) == 0 {
		delete(lateralDB, collectorCompositeKey)
	}
	log.Infof("Disallowing deletion of lateral endpoints. Pending refs: %v or could be venice known EP", lateralDB[lateralEP.Name])
	return nil
}

func createOrUpdateLateralEP(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner, destIP, dmac string) error {
	// Make sure only one go routine is updating EP at a point
	mux.Lock()
	defer mux.Unlock()

	objectName := fmt.Sprintf("_internal-%s", dmac)
	collectorCompositeKey := fmt.Sprintf("collector|%s", objectName)

	lateralDB[collectorCompositeKey] = append(lateralDB[collectorCompositeKey], owner)
	dedupReferences(collectorCompositeKey)

	ep := &netproto.Endpoint{
		TypeMeta: api.TypeMeta{Kind: "Endpoint"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      objectName,
		},
		Spec: netproto.EndpointSpec{
			NetworkName:   types.InternalDefaultUntaggedNetwork,
			NodeUUID:      "REMOTE",
			IPv4Addresses: []string{destIP},
			MacAddress:    dmac,
		},
	}
	// Test for already existing collector EP
	// FindEndpoint with matching Mac if found update else create
	var knownEp *netproto.Endpoint
	eDat, err := infraAPI.List("Endpoint")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
	}
	for _, o := range eDat {
		var endpoint netproto.Endpoint
		err := endpoint.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			continue
		}
		if endpoint.Spec.MacAddress == ep.Spec.MacAddress {
			knownEp = &endpoint
			break
		}
	}
	if knownEp != nil {
		log.Infof("Updating internal endpoint %v", ep.ObjectMeta)
		ep := &netproto.Endpoint{
			TypeMeta: api.TypeMeta{Kind: "Endpoint"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    knownEp.ObjectMeta.Tenant,
				Namespace: knownEp.ObjectMeta.Namespace,
				Name:      knownEp.ObjectMeta.Name,
			},
			Spec: netproto.EndpointSpec{
				NetworkName:   knownEp.Spec.NetworkName,
				NodeUUID:      knownEp.Spec.NodeUUID,
				IPv4Addresses: knownEp.Spec.IPv4Addresses,
				MacAddress:    knownEp.Spec.MacAddress,
			},
		}
		ep.Spec.IPv4Addresses = append(ep.Spec.IPv4Addresses, destIP)
		if err = updateEndpointHandler(infraAPI, epClient, intfClient, *ep, vrfID, types.UntaggedCollVLAN); err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorEPUpdateFailure, "MirrorSession: %s |  Err: %v", owner, err))
			return errors.Wrapf(types.ErrCollectorEPUpdateFailure, "MirrorSession: %s |  Err: %v", owner, err)
		}
	} else {
		log.Infof("Creating internal endpoint %v", ep.ObjectMeta)
		err := createEndpointHandler(infraAPI, epClient, intfClient, *ep, vrfID, types.UntaggedCollVLAN)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrCollectorEPCreateFailure, "MirrorSession: %s |  Err: %v", owner, err))
			return errors.Wrapf(types.ErrCollectorEPCreateFailure, "MirrorSession: %s |  Err: %v", owner, err)
		}
	}
	return nil
}

func reconcileLateralObjects(infraAPI types.InfraAPI, owner string, destIP string, createTunnel bool) (epKnown, tunnelKnown bool) {
	// Find EP
	log.Infof("Testing for known collector with DestIP: %v, TunnelOp: %v", destIP, createTunnel)
	var (
		tDat [][]byte
		eDat [][]byte
		err  error
	)
	tDat, err = infraAPI.List("Tunnel")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
	}

	eDat, err = infraAPI.List("Endpoint")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
	}

	if createTunnel == true {
		var knownTunnel *netproto.Tunnel
		var knownCollector *netproto.Endpoint

		for _, o := range tDat {
			var tunnel netproto.Tunnel
			err := tunnel.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err))
				continue
			}
			if tunnel.Spec.Dst == destIP {
				knownTunnel = &tunnel
				break
			}
		}

		for _, o := range eDat {
			var endpoint netproto.Endpoint
			err := endpoint.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
				continue
			}
			for _, address := range endpoint.Spec.IPv4Addresses {
				if address == destIP {
					knownCollector = &endpoint
					break
				}
			}
		}

		if knownTunnel != nil {
			// Found a precreated tunnel. Add refcount
			tunnelKnown = true
		}

		if knownCollector != nil {
			// Found a precreated tunnel. Add refcount
			epKnown = true
		}
		return
	}

	var knownCollector *netproto.Endpoint

	// Find EP and add refcount
	for _, o := range eDat {
		var endpoint netproto.Endpoint
		err := endpoint.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			continue
		}
		for _, address := range endpoint.Spec.IPv4Addresses {
			if address == destIP {
				knownCollector = &endpoint
				break
			}
		}
	}

	if knownCollector != nil {
		// Found a precreated ep. Add refcount
		epKnown = true
		tunnelKnown = true // Mark tunnel as known here so that it doesn't get created
	}
	return
}

// dedupReferences does inplace dedup of reference slices
func dedupReferences(compositeKey string) {
	log.Infof("Lateral DB: %v pre dedup: %v", compositeKey, lateralDB[compositeKey])
	sort.Strings(lateralDB[compositeKey])
	if len(lateralDB[compositeKey]) < 2 {
		return
	}
	marker := 0
	for idx := 1; idx < len(lateralDB[compositeKey]); idx++ {
		if lateralDB[compositeKey][marker] == lateralDB[compositeKey][idx] {
			continue
		}
		marker++
		lateralDB[compositeKey][marker] = lateralDB[compositeKey][idx]
	}
	lateralDB[compositeKey] = lateralDB[compositeKey][:marker+1]
}

func generateLateralEP(infraAPI types.InfraAPI, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, vrfID uint64, owner, destIP, gwIP string) (*netproto.Endpoint, error) {
	var dMAC string
	epIP := destIP

	log.Infof("Resolving for IP: %s, gw: %v", destIP, gwIP)

	arpResolverKey := destIP + "-" + gwIP
	// Make sure only one loop for an IP
	mac, ok := destIPToMAC.Load(arpResolverKey)
	if !ok {
		// Cache the handler for outer done. This needs to be called during the object deletes
		refreshCtx, done := context.WithCancel(context.Background())
		doneCache[arpResolverKey] = done

		if gwIP != "" {
			_, dest, _ := net.ParseCIDR(destIP + "/32")
			log.Infof("Installing route for %v via %s", dest, gwIP)
			route := &netlink.Route{
				Dst: dest,
				Gw:  net.ParseIP(gwIP),
			}
			if err := netlink.RouteAdd(route); err != nil {
				log.Errorf("Failed to configure static route %v. Err: %v", route, err)
			}
		}
		go startRefreshLoop(infraAPI, intfClient, epClient, vrfID, owner, refreshCtx, destIP, gwIP)
		// Give the routine a chance to run
		time.Sleep(time.Second * 3)
		return nil, nil
	}

	dMAC = mac.(string)

	if len(dMAC) == 0 {
		log.Errorf("failed to resolve MAC in first try for %s. Will keep retrying ...", destIP)
		return nil, nil
	}
	objectName := fmt.Sprintf("_internal-%s", dMAC)

	ep := &netproto.Endpoint{
		TypeMeta: api.TypeMeta{Kind: "Endpoint"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      objectName,
		},
		Spec: netproto.EndpointSpec{
			NetworkName:   types.InternalDefaultUntaggedNetwork,
			NodeUUID:      "REMOTE",
			IPv4Addresses: []string{epIP},
			MacAddress:    dMAC,
		},
	}

	return ep, nil
}

func isCollectorEPKnown(infraAPI types.InfraAPI, collectorEP netproto.Endpoint) (ep netproto.Endpoint, known bool) {
	dat, err := infraAPI.Read(collectorEP.Kind, collectorEP.GetKey())
	if err != nil {
		return
	}
	err = ep.Unmarshal(dat)
	known = true
	return
}

func isCollectorTunnelKnown(infraAPI types.InfraAPI, collectorTunnel netproto.Tunnel) (*netproto.Tunnel, bool) {
	var knownTunnel netproto.Tunnel
	dat, err := infraAPI.Read(collectorTunnel.Kind, collectorTunnel.GetKey())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", collectorTunnel.GetKey(), types.ErrObjNotFound))
		return nil, false
	}

	err = knownTunnel.Unmarshal(dat)
	if err != nil {
		return nil, false
	}

	return &knownTunnel, true
}
