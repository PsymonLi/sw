package validator

import (
	"fmt"
	"hash/fnv"
	"strings"
	"sync"

	"github.com/gogo/protobuf/proto"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// ValidateApp validates that an app has only a single ALG set
func ValidateApp(app netproto.App) (err error) {
	err = validateSingletonALG(app.Spec.ALG)
	if err != nil {
		return
	}

	return
}

// ValidateEndpoint performs static field validations on IP and MAC and named ref validations on network and vrf
func ValidateEndpoint(i types.InfraAPI, endpoint netproto.Endpoint) (network netproto.Network, vrf netproto.Vrf, err error) {
	// Static Field  Validations
	err = utils.ValidateIPAddresses(endpoint.Spec.IPv4Addresses...)
	if err != nil {
		return
	}

	err = utils.ValidateMacAddresses(endpoint.Spec.MacAddress)
	if err != nil {
		return
	}

	// Named reference validations
	vrf, err = ValidateVrf(i, endpoint.Tenant, endpoint.Namespace, endpoint.Spec.VrfName)
	if err != nil {
		return
	}

	nt := netproto.Network{
		TypeMeta: api.TypeMeta{
			Kind: "Network",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    endpoint.Tenant,
			Namespace: endpoint.Namespace,
			Name:      endpoint.Spec.NetworkName,
		},
	}
	log.Infof("Looking up Network: %v", nt)
	dat, err := i.Read(nt.Kind, nt.GetKey())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", nt.GetKey(), types.ErrObjNotFound))
		return network, vrf, errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", nt.GetKey(), types.ErrObjNotFound)
	}
	err = nt.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", nt.GetKey(), err))
		return network, vrf, errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", nt.GetKey(), err)
	}

	network = nt
	log.Infof("Found Network: %v", network)
	return
}

func validateInterfaceMirrorSessions(i types.InfraAPI, mirrorSessions []string, collectorMap map[uint64]int, mirrorSessionMap map[string][]uint64) error {
	for _, ms := range mirrorSessions {
		var intfMirror netproto.InterfaceMirrorSession
		dat, err := i.Read("InterfaceMirrorSession", ms)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", ms, types.ErrObjNotFound))
			return errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", ms, types.ErrObjNotFound)
		}
		err = intfMirror.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", ms, err))
			return errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", ms, err)
		}
		mirrorKey := fmt.Sprintf("%s/%s", intfMirror.Kind, intfMirror.GetKey())
		for _, c := range mirrorSessionMap[mirrorKey] {
			collectorMap[c] |= utils.MirrorDir(intfMirror.Spec.MirrorDirection)
		}
	}
	return nil
}

// ValidateInterface performs static field validations on interface type
func ValidateInterface(i types.InfraAPI, intf netproto.Interface, collectorMap map[uint64]int, mirrorSessionMap map[string][]uint64) (err error) {
	// InterfaceMirrorSession validations
	if err = validateInterfaceMirrorSessions(i, intf.Spec.MirrorSessions, collectorMap, mirrorSessionMap); err != nil {
		return
	}
	// Static Field  Validations
	switch strings.ToLower(intf.Spec.Type) {
	case "uplink_eth":
		return
	case "uplink_mgmt":
		return
	default:
		err = errors.Wrapf(types.ErrInvalidInterfaceType, "Type: %s | Err: %v", intf.Spec.Type, types.ErrBadRequest)
		return
	}
}

// ValidateNetwork performs named reference validations on vrf and unique VLAN IDs
func ValidateNetwork(i types.InfraAPI, oper types.Operation, network netproto.Network) (uplinkIDs []uint64, vrf netproto.Vrf, err error) {
	// Named reference validations
	var (
		intfs []netproto.Interface
		dat   [][]byte
	)
	log.Infof("validating VRF for network [%v/%v/%v]", network.Tenant, network.Namespace, network.Spec.VrfName)
	vrf, err = ValidateVrf(i, network.Tenant, network.Namespace, network.Spec.VrfName)
	if err != nil {
		return
	}

	// Check for duplicate vlans
	if network.Spec.VlanID != 0 {
		if oper != types.Delete {
			dat, _ = i.List(network.Kind)

			for _, o := range dat {
				var nt netproto.Network
				err := nt.Unmarshal(o)
				if err != nil {
					log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", nt.GetKey(), err))
					continue
				}
				if nt.Spec.VlanID == network.Spec.VlanID {
					log.Error(errors.Wrapf(types.ErrDuplicateVLANID, "Found duplicate VLAN: %d in Networks %v and %v", network.Spec.VlanID, nt.GetKey(), network.GetKey()))
					return nil, vrf, errors.Wrapf(types.ErrDuplicateVLANID, "Found duplicate VLAN: %d in Networks %v and %v", network.Spec.VlanID, nt.GetKey(), network.GetKey())
				}
			}
		}
	}

	intf := &netproto.Interface{
		TypeMeta: api.TypeMeta{Kind: "Interface"},
	}

	dat, err = i.List(intf.Kind)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound))
		return nil, vrf, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound)
	}

	for _, o := range dat {
		var intf netproto.Interface
		err := intf.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			continue
		}
		intfs = append(intfs, intf)
	}

	for _, intf := range intfs {
		if intf.Spec.Type == "UPLINK_ETH" {
			uplinkIDs = append(uplinkIDs, intf.Status.InterfaceID)
		}
	}
	return
}

// ValidateNetworkSecurityPolicy validates network security policy and generates rule id to app mapping
func ValidateNetworkSecurityPolicy(i types.InfraAPI, nsp netproto.NetworkSecurityPolicy) (vrf netproto.Vrf, ruleIDLut *sync.Map, err error) {
	// Static Validations

	var ruleIDAppMapping sync.Map

	if nsp.Spec.AttachTenant == false && len(nsp.Spec.AttachGroup) == 0 {
		err := errors.Wrapf(types.ErrMissingPolicyAttachmentPoint, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrBadRequest)
		return vrf, nil, err
	}

	if nsp.Spec.AttachTenant == true && len(nsp.Spec.AttachGroup) != 0 {
		err := errors.Wrapf(types.ErrPolicyDualAttachment, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrBadRequest)
		return vrf, nil, err
	}

	// Named reference validations
	vrf, err = ValidateVrf(i, nsp.Tenant, nsp.Namespace, nsp.Spec.VrfName)
	if err != nil {
		return
	}

	for idx, r := range nsp.Spec.Rules {
		var dat []byte
		ruleHash := nsp.Spec.Rules[idx].ID
		// Calculate the hash only if npm has not set it. Else use whatever is already set
		if ruleHash == 0 {
			ruleHash = generateRuleHash(&r, nsp.GetKey())
			nsp.Spec.Rules[idx].ID = ruleHash
		}

		if len(r.AppName) > 0 {
			app := netproto.App{
				TypeMeta: api.TypeMeta{
					Kind: "App",
				},
				ObjectMeta: api.ObjectMeta{
					Tenant:    nsp.Tenant,
					Namespace: nsp.Namespace,
					Name:      r.AppName,
				},
			}

			dat, err = i.Read(app.Kind, app.GetKey())
			if err != nil {
				log.Error(errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrObjNotFound))
				return vrf, nil, errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrObjNotFound)
			}
			err = app.Unmarshal(dat)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "App: %s | Err: %v", app.GetKey(), err))
				return vrf, nil, errors.Wrapf(types.ErrUnmarshal, "App: %s | Err: %v", app.GetKey(), err)
			}

			if len(r.Dst.ProtoPorts) != 0 || len(r.Src.ProtoPorts) != 0 {
				log.Error(errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrDuplicateRuleinAppAndInlineRules))
				return vrf, nil, errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrDuplicateRuleinAppAndInlineRules)
			}

			ruleIDAppMapping.Store(idx, &app)
		}
		if r.Src != nil {
			for _, p := range r.Src.ProtoPorts {
				if p.Protocol == "icmp" && p.Port != "" {
					log.Error(errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrICMPConfiguredWithPort))
					return vrf, nil, errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrICMPConfiguredWithPort)
				}
			}
		}
		if r.Dst != nil {
			for _, p := range r.Dst.ProtoPorts {
				if p.Protocol == "icmp" && p.Port != "" {
					log.Error(errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrICMPConfiguredWithPort))
					return vrf, nil, errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrICMPConfiguredWithPort)
				}
			}
		}
	}

	return vrf, &ruleIDAppMapping, nil
}

// ValidateTunnel performs static field validations on srcIP, DstIP and named reference validation on vrf
func ValidateTunnel(i types.InfraAPI, tunnel netproto.Tunnel) (vrf netproto.Vrf, err error) {
	// Static Field  Validations
	err = utils.ValidateIPAddresses(tunnel.Spec.Src)
	if err != nil {
		return
	}

	err = utils.ValidateIPAddresses(tunnel.Spec.Dst)
	if err != nil {
		return
	}

	// Named reference validations
	vrf, err = ValidateVrf(i, tunnel.Tenant, tunnel.Namespace, tunnel.Spec.VrfName)
	if err != nil {
		return
	}
	return
}

// ValidateMirrorSession performs named reference validation on vrf and max mirror session check
func ValidateMirrorSession(i types.InfraAPI, mirror netproto.MirrorSession, oper types.Operation, mirrorSessions int) (vrf netproto.Vrf, err error) {
	uniqueCreates := 0

	switch oper {
	case types.Create:
		uniqueCreates += len(mirror.Spec.Collectors)
	case types.Update:
		var existingMirror netproto.MirrorSession
		dat, errr := i.Read(mirror.Kind, mirror.GetKey())
		if errr != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			err = errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
			return
		}
		err = existingMirror.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			err = errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
			return
		}

		uniqueCreates += len(mirror.Spec.Collectors) - len(existingMirror.Spec.Collectors)
	case types.Delete:
	}

	if mirrorSessions+uniqueCreates > types.MaxMirrorSessions {
		log.Error(errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrMaxMirrorSessionsExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrMaxMirrorSessionsExceeded)
		return
	}

	vrf, err = ValidateVrf(i, mirror.Tenant, mirror.Namespace, mirror.Spec.VrfName)
	return
}

// ValidateInterfaceMirrorSession performs named reference validation on vrf and max mirror session check
func ValidateInterfaceMirrorSession(i types.InfraAPI, mirror netproto.InterfaceMirrorSession, oper types.Operation, mirrorSessions int) (vrf netproto.Vrf, err error) {
	uniqueCreates := 0

	switch oper {
	case types.Create:
		uniqueCreates += len(mirror.Spec.Collectors)
	case types.Update:
		var existingMirror netproto.InterfaceMirrorSession
		dat, errr := i.Read(mirror.Kind, mirror.GetKey())
		if errr != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			err = errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
			return
		}
		err = existingMirror.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			err = errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
			return
		}

		uniqueCreates += len(mirror.Spec.Collectors) - len(existingMirror.Spec.Collectors)
	case types.Delete:
	}

	if mirrorSessions+uniqueCreates > types.MaxMirrorSessions {
		log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrMaxMirrorSessionsExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrMaxMirrorSessionsExceeded)
		return
	}

	vrf, err = ValidateVrf(i, mirror.Tenant, mirror.Namespace, mirror.Spec.VrfName)
	return
}

// ValidateFlowExportPolicy performs static field validations on srcIP, DstIP and named reference validation on vrf
func ValidateFlowExportPolicy(i types.InfraAPI, netflow netproto.FlowExportPolicy, oper types.Operation, netflowSessions int) (vrf netproto.Vrf, err error) {
	// Named reference validations
	if len(netflow.Spec.Exports) > types.MaxCollectorsPerFlow {
		log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrMaxCollectorsPerFlowExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrMaxCollectorsPerFlowExceeded)
		return
	}
	uniqueCreates := 0

	switch oper {
	case types.Create:
		uniqueCreates += len(netflow.Spec.Exports)
	case types.Update:
		var existingNetflow netproto.FlowExportPolicy
		dat, errr := i.Read(netflow.Kind, netflow.GetKey())
		if errr != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound))
			err = errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound)
			return
		}
		err = existingNetflow.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			err = errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
			return
		}

		uniqueCreates += len(netflow.Spec.Exports) - len(existingNetflow.Spec.Exports)
	case types.Delete:
	}

	if netflowSessions+uniqueCreates > types.MaxCollectors {
		log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrMaxCollectorsExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrMaxCollectorsExceeded)
		return
	}

	vrf, err = ValidateVrf(i, netflow.Tenant, netflow.Namespace, netflow.Spec.VrfName)
	return
}

// ValidateVrf validates VRF object. On empty VrfNames, it uses default vrf with id == 65
func ValidateVrf(i types.InfraAPI, tenant, namespace, name string) (vrf netproto.Vrf, err error) {
	// Pick default vrf is unspecified or specified default
	if len(name) == 0 || name == types.DefaultVrf {
		if len(tenant) == 0 {
			tenant = types.DefaultTenant
		}
		namespace = types.DefaultNamespace
		name = types.DefaultVrf
	}

	if namespace == "" {
		namespace = types.DefaultNamespace
	}

	v := netproto.Vrf{
		TypeMeta: api.TypeMeta{
			Kind: "Vrf",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    tenant,
			Namespace: namespace,
			Name:      name,
		},
	}
	dumpVrfs := func() {
		rs, err := i.List(v.Kind)
		if err != nil {
			log.Infof("did not find any vrfs (%s)", err)
			return
		}
		for _, r := range rs {
			vf := netproto.Vrf{}
			log.Infof("unmarshal is %v/%v", r, vf.Unmarshal(r))
		}
	}
	log.Infof("Trying to find VRF [%v/%v/%v][%+v]", tenant, namespace, name, v)
	dat, err := i.Read(v.Kind, v.GetKey())
	if err != nil {
		dumpVrfs()
		log.Error(errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", v.GetKey(), types.ErrObjNotFound))
		return vrf, errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", v.GetKey(), types.ErrObjNotFound)
	}
	err = vrf.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", v.GetKey(), err))
		return vrf, errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", v.GetKey(), err)
	}

	return
}

// ValidateSecurityProfile validates SecurityProfile object and returns the attachment vrf
func ValidateSecurityProfile(i types.InfraAPI, profile netproto.SecurityProfile) (vrf netproto.Vrf, err error) {
	vrf, err = ValidateVrf(i, profile.Tenant, profile.Namespace, types.DefaultVrf)
	return
}

// ValidateRoutingConfig validates RoutingConfig object
func ValidateRoutingConfig(i types.InfraAPI, rtCfg netproto.RoutingConfig) error {
	return nil
}

// ValidateRouteTable validates RotueTable object
func ValidateRouteTable(i types.InfraAPI, rtCfg netproto.RouteTable) error {
	return nil
}

// ValidateProfile validates Profile object
func ValidateProfile(profile netproto.Profile) error {
	if _, ok := netproto.ProfileSpec_Fwd_Mode_value[strings.ToUpper(profile.Spec.FwdMode)]; !ok {
		return errors.Wrapf(types.ErrInvalidProfileFwdMode, "Type: %s | Err: %v", strings.ToUpper(profile.Spec.FwdMode), types.ErrBadRequest)
	}
	if _, ok := netproto.ProfileSpec_Policy_Mode_value[strings.ToUpper(profile.Spec.PolicyMode)]; !ok {
		return errors.Wrapf(types.ErrInvalidProfilePolicyMode, "Type: %s | Err: %v", strings.ToUpper(profile.Spec.PolicyMode), types.ErrBadRequest)
	}
	return nil
}

// ValidateIPAMPolicyExists validates IPAMPolicy
func ValidateIPAMPolicyExists(i types.InfraAPI, tenant, namespace, name string) (ipam netproto.IPAMPolicy, err error) {
	policy := netproto.IPAMPolicy{
		TypeMeta: api.TypeMeta{Kind: "IPAMPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    tenant,
			Namespace: namespace,
			Name:      name,
		},
	}
	dat, err := i.Read(policy.Kind, policy.GetKey())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound))
		err = errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound)
		return
	}
	err = ipam.Unmarshal(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
		err = errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err)
		return
	}
	return
}

func validateSingletonALG(alg *netproto.ALG) (err error) {
	var algMapper int
	if alg == nil {
		return
	}
	switch {
	case alg.DNS != nil:
		algMapper = setBit(algMapper, types.AlgDNS)

	case alg.FTP != nil:
		algMapper = setBit(algMapper, types.AlgFTP)
	case alg.ICMP != nil:
		algMapper = setBit(algMapper, types.AlgICMP)
	case alg.MSRPC != nil:
		algMapper = setBit(algMapper, types.AlgMSRPC)
	case alg.RTSP != nil:
		algMapper = setBit(algMapper, types.AlgRTSP)
	case alg.SIP != nil:
		algMapper = setBit(algMapper, types.AlgSIP)
	case alg.SUNRPC != nil:
		algMapper = setBit(algMapper, types.AlgSUNRPC)
	case alg.TFTP != nil:
		algMapper = setBit(algMapper, types.AlgTFTP)
	}

	if (algMapper & -algMapper) != algMapper {
		return types.ErrMultipleALGInApp
	}

	return
}

// setBit sets a particular bit position to 1. This is used for ALG Mapper
func setBit(n int, pos uint) int {
	n |= 1 << pos
	return n
}

// generateRuleHash generates a fnv 64 hash based on the policy rule and the NetworkSecurityPolicy Key.
// Ensures cluster wide uniqueness
//generateRuleHash generates rule hash for policy rule
func generateRuleHash(r *netproto.PolicyRule, key string) uint64 {
	h := fnv.New64()
	rule, _ := r.Marshal()
	rule = append(rule, []byte(key)...)
	h.Write(rule)
	return h.Sum64()
}

// ValidateNwAttach checks if this subnet is attached to any host-pfs and returns attached host-pfs
func ValidateNwAttach(i types.InfraAPI, tenant, namespace, name string) []netproto.Interface {
	var intfs []netproto.Interface
	data, err := i.List("Interface")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Interfaces not found: Err: %v", types.ErrObjNotFound))
		return intfs
	}

	for _, intf := range data {
		var nwIf netproto.Interface
		err := proto.Unmarshal(intf, &nwIf)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %v | Err: %v", intf, err))
			continue
		}

		if nwIf.Spec.Type != netproto.InterfaceSpec_HOST_PF.String() {
			continue
		}

		if nwIf.Spec.VrfName == tenant && nwIf.Spec.Network == name {
			intfs = append(intfs, nwIf)
		}
	}

	return intfs
}

// ValidateIPAMPolicy performs static field validations on server IP and named reference validation on underlay vp
func ValidateIPAMPolicy(i types.InfraAPI, policy netproto.IPAMPolicy, oper types.Operation, serverIPToKeys map[string]int) (vrf netproto.Vrf, err error) {
	// Named reference validations
	if len(policy.Spec.DHCPRelay.Servers) > types.MaxServerIPPerPolicy {
		log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrMaxServerIPPerPolicyExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrMaxServerIPPerPolicyExceeded)
		return
	}

	uniqueCreates := 0

	switch oper {
	case types.Create:
		// Get the count of unique creates
		for _, srv := range policy.Spec.DHCPRelay.Servers {
			if _, ok := serverIPToKeys[srv.IPAddress]; !ok {
				uniqueCreates++
			}
		}
	case types.Update:
		// Get the existing policy object and consider valid deletes i.e
		// objects that would get deleted (ones having single reference). Add the unique
		// creates to the get the actual increase in server IPs number if any.
		var existingPolicy netproto.IPAMPolicy
		dat, errr := i.Read(policy.Kind, policy.GetKey())
		if errr != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound))
			err = errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound)
			return
		}
		err = existingPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
			err = errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err)
			return
		}

		// Track if the server IP needs to deleted
		deleteKey := map[string]bool{}
		for _, srv := range existingPolicy.Spec.DHCPRelay.Servers {
			deleteKey[srv.IPAddress] = false
			// If referenced by a singular key then could be removed
			if size, ok := serverIPToKeys[srv.IPAddress]; ok && size == 1 {
				deleteKey[srv.IPAddress] = true
			}
		}

		for _, srv := range policy.Spec.DHCPRelay.Servers {
			deleteKey[srv.IPAddress] = false
		}

		for k, del := range deleteKey {
			if del {
				uniqueCreates--
				continue
			}
			// For keys that are not to be deleted, check if they are new entries
			if _, ok := serverIPToKeys[k]; !ok {
				uniqueCreates++
			}
		}
	case types.Delete:
	}

	if len(serverIPToKeys)+uniqueCreates > types.MaxDHCPServers {
		log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrMaxDHCPServersExceeded))
		err = errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrMaxDHCPServersExceeded)
		return
	}

	vrf, err = ValidateVrf(i, types.DefaultTenant, types.DefaultNamespace, types.DefaulUnderlaytVrf)
	if err != nil {
		return
	}
	return
}
