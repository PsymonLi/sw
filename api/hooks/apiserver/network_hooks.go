// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package impl

import (
	"context"
	"errors"
	"fmt"
	"math"
	"net"
	"strings"
	"sync"
	"time"

	"github.com/gogo/protobuf/types"
	"github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/apiserver/pkg"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/featureflags"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
)

type networkHooks struct {
	svc       apiserver.Service
	logger    log.Logger
	vnidMapMu sync.Mutex
	vnidMap   map[uint32]string
	netwMapMu sync.Mutex
	netwMap   map[string]map[string]*net.IPNet
}

func (h *networkHooks) validateIPAMPolicyConfig(i interface{}, ver string, ignStatus, ignoreSpec bool) []error {
	cfg, ok := i.(network.IPAMPolicy)

	if ok == false {
		return []error{errors.New("Invalid input configuration")}
	}

	// in the current implementation only one dhcp server configuration is supported even though the model
	// defines it as a slice for future enhancements
	if len(cfg.Spec.DHCPRelay.Servers) > 1 {
		return []error{errors.New("Only one DHCP server configuration is supported")}
	}
	return nil
}

func validateImportExportRTs(rd *network.RouteDistinguisher, name string) (ret []error) {
	switch strings.ToLower(rd.Type) {
	case strings.ToLower(network.RouteDistinguisher_Type0.String()):
		if rd.AdminValue.Value > math.MaxUint16 {
			ret = append(ret, fmt.Errorf("%s Route Target %s admin value cannot be greater than %d", name, network.RouteDistinguisher_Type0.String(), math.MaxUint16))
		}
		if rd.AdminValue.Format != api.ASNFormatRD {
			ret = append(ret, fmt.Errorf("%s Route Target %s admin value should be an ASN", name, network.RouteDistinguisher_Type0.String()))
		}
	case strings.ToLower(network.RouteDistinguisher_Type1.String()):
		if rd.AssignedValue > math.MaxUint16 {
			ret = append(ret, fmt.Errorf("%s Route Target %s assigned value cannot be greater than %d", name, network.RouteDistinguisher_Type1.String(), math.MaxUint16))
		}
	case strings.ToLower(network.RouteDistinguisher_Type2.String()):
		if rd.AssignedValue > math.MaxUint16 {
			ret = append(ret, fmt.Errorf("%s Route Target %s assigned value cannot be greater than %d", name, network.RouteDistinguisher_Type2.String(), math.MaxUint16))
		}
		if rd.AdminValue.Format != api.ASNFormatRD {
			ret = append(ret, fmt.Errorf("%s Route Target %s admin value should be an ASN", name, network.RouteDistinguisher_Type2.String()))
		}
	}
	return
}

func (h *networkHooks) validateNetworkConfig(i interface{}, ver string, ignStatus, ignoreSpec bool) []error {
	var ret []error

	in := i.(network.Network)
	if in.Spec.Type == network.NetworkType_Routed.String() {
		if in.Spec.IPv4Subnet == "" || in.Spec.IPv4Gateway == "" {
			ret = append(ret, fmt.Errorf("IP Subnet information must be specified for routed networks"))
		}
		if in.Spec.IPv6Gateway != "" || in.Spec.IPv6Subnet != "" {
			ret = append(ret, fmt.Errorf("IPv6 not supported"))
		}
	}

	if in.Spec.RouteImportExport != nil {
		if in.Spec.RouteImportExport.RDAuto && in.Spec.RouteImportExport.RD != nil {
			ret = append(ret, fmt.Errorf("rd and rd-auto specified, only one of them can be specified"))
		}
		if in.Spec.RouteImportExport.AddressFamily == network.BGPAddressFamily_IPv4Unicast.String() {
			ret = append(ret, fmt.Errorf("Route Import Export of address family ipv4 unicast cannot be specified for Network"))
		}
		if in.Spec.RouteImportExport.AddressFamily == network.BGPAddressFamily_L2vpnEvpn.String() {
			if in.Spec.VxlanVNI == 0 {
				ret = append(ret, fmt.Errorf("For address family %s VxlanVNI must be specified", network.BGPAddressFamily_L2vpnEvpn.String()))
			}
			if in.Spec.VirtualRouter == "" {
				ret = append(ret, fmt.Errorf("For address family %s VirtualRouter must be specified", network.BGPAddressFamily_L2vpnEvpn.String()))
			}
			if in.Spec.VirtualRouter == "default" {
				ret = append(ret, fmt.Errorf("For address family %s VirtualRouter has to be non-default", network.BGPAddressFamily_L2vpnEvpn.String()))
			}
		}

		for _, e := range in.Spec.RouteImportExport.ExportRTs {
			errs := validateImportExportRTs(e, "Export")
			if errs != nil {
				ret = append(ret, errs...)
			}
		}

		for _, e := range in.Spec.RouteImportExport.ImportRTs {
			errs := validateImportExportRTs(e, "Import")
			if len(errs) != 0 {
				ret = append(ret, errs...)
			}
		}
	}

	if len(in.Spec.IngressSecurityPolicy) > 2 {
		ret = append(ret, fmt.Errorf("maximum of 2 ingress security policies are allowed"))
	}
	if len(in.Spec.EgressSecurityPolicy) > 2 {
		ret = append(ret, fmt.Errorf("maximum of 2 egress security policies are allowed"))
	}

	return ret
}

func (h *networkHooks) validateRoutingConfig(i interface{}, ver string, ignStatus, ignoreSpec bool) []error {
	var ret []error
	in, ok := i.(network.RoutingConfig)
	if !ok {
		return nil
	}
	if in.Spec.BGPConfig == nil {
		return nil
	}
	var autoCfg, evpn, ipv4 bool
	autoCfg = in.Spec.BGPConfig.DSCAutoConfig
	// validate Holdtime and Keepalive timers
	if in.Spec.BGPConfig.Holdtime != 0 && in.Spec.BGPConfig.KeepaliveInterval == 0 || in.Spec.BGPConfig.Holdtime == 0 && in.Spec.BGPConfig.KeepaliveInterval != 0 {
		ret = append(ret, fmt.Errorf("inconsistent holdtime and keepalive-interval values, either both should be zero or both should be non-zero"))
	}
	if in.Spec.BGPConfig.Holdtime != 0 {
		if in.Spec.BGPConfig.Holdtime < 3 {
			ret = append(ret, fmt.Errorf("holdtime cannot be smaller than 3secs"))
		} else {
			if in.Spec.BGPConfig.KeepaliveInterval*3 > in.Spec.BGPConfig.Holdtime {
				ret = append(ret, fmt.Errorf("holdtime should be 3 times keepalive-interval or more"))
			}
		}
	}
	if in.Spec.BGPConfig.RouterId != "" && in.Spec.BGPConfig.DSCAutoConfig {
		ret = append(ret, fmt.Errorf("router id cannot be specified when dsc-auto-config is true"))
	}
	peerMap := make(map[string]bool)
	for _, n := range in.Spec.BGPConfig.Neighbors {
		if len(n.EnableAddressFamilies) != 1 {
			ret = append(ret, fmt.Errorf("there should be one address family %v", n.EnableAddressFamilies))
		}
		if n.DSCAutoConfig {
			if n.IPAddress != "" {
				ret = append(ret, fmt.Errorf("peer IP Address not allowed when dsc-auto-config is true"))
			}
			switch n.EnableAddressFamilies[0] {
			case network.BGPAddressFamily_L2vpnEvpn.String():
				if evpn {
					ret = append(ret, fmt.Errorf("only one auto-config peer per address family [l2vpn-evpn] allowed"))
				} else {
					evpn = true
				}
				if n.RemoteAS != in.Spec.BGPConfig.ASNumber {
					ret = append(ret, fmt.Errorf("EVPN auto-peering allowed only for iBGP peers"))
				}
			case network.BGPAddressFamily_IPv4Unicast.String():
				if !autoCfg {
					ret = append(ret, fmt.Errorf("dsc-auto-config peer only allowed when BGP config is also dsc-auto-config"))
				}
				if ipv4 {
					ret = append(ret, fmt.Errorf("only one auto-config peer per address family [ipv4-unicast] allowed"))
				} else {
					ipv4 = true
				}
				if n.RemoteAS == in.Spec.BGPConfig.ASNumber {
					ret = append(ret, fmt.Errorf("ipv4-unicast auto-peering allowed only for eBGP peers"))
				}
			}
		} else {
			if n.IPAddress == "" {
				ret = append(ret, fmt.Errorf("IPAddress should be specified if DSCAutoConfig is false"))
			} else {
				if _, ok := peerMap[n.IPAddress]; ok {
					ret = append(ret, fmt.Errorf("duplicate peer in spec [%v]", n.IPAddress))
				}
				peerMap[n.IPAddress] = true
			}
		}
		// validate Holdtime and Keepalive timers
		if n.Holdtime != 0 && n.KeepaliveInterval == 0 || n.Holdtime == 0 && n.KeepaliveInterval != 0 {
			ret = append(ret, fmt.Errorf("inconsistent holdtime and keepalive-interval values, either both should be zero or both should be non-zero"))
		}
		if n.Holdtime != 0 {
			if n.Holdtime < 3 {
				ret = append(ret, fmt.Errorf("holdtime cannot be smaller than 3secs"))
			} else {
				if n.KeepaliveInterval*3 > n.Holdtime {
					ret = append(ret, fmt.Errorf("holdtime should be 3 times keepalive-interval or more"))
				}
			}
		}
	}
	return ret
}

// Checks that for any orch info deletion, no workloads from the orch info are using this network
func (h *networkHooks) routingConfigPreCommit(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	in, ok := i.(network.RoutingConfig)
	if !ok {
		h.logger.ErrorLog("method", "routingConfigPreCommit", "msg", fmt.Sprintf("invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}

	switch oper {
	case apiintf.CreateOper:
		if !featureflags.IsOVerlayRoutingEnabled() {
			return i, true, fmt.Errorf("not licensed to enable overlay routing")
		}
	case apiintf.UpdateOper:
		existingObj := &network.RoutingConfig{}
		err := kv.Get(ctx, key, existingObj)
		if err != nil {
			return i, true, fmt.Errorf("Failed to get existing object: %s", err)
		}
		if existingObj.Spec.BGPConfig != nil && in.Spec.BGPConfig != nil {
			if existingObj.Spec.BGPConfig.ASNumber != in.Spec.BGPConfig.ASNumber {
				return i, true, fmt.Errorf("Change in local ASN not allowed, delete and recreate")
			}
		}
	}
	return i, true, nil
}

func (h *networkHooks) checkNetworkMutableFields(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	in, ok := i.(network.Network)
	if !ok {
		h.logger.ErrorLog("method", "checkNetworkMutableFields", "msg", fmt.Sprintf("API server network hook called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}

	existingNw := &network.Network{}
	err := kv.Get(ctx, key, existingNw)
	if err != nil {
		log.Errorf("did not find obj [%v] on update (%s)", key, err)
		return i, true, err
	}
	if in.Spec.Type != existingNw.Spec.Type {
		return i, true, fmt.Errorf("cannot modify type of network [%v->%v]", existingNw.Spec.Type, in.Spec.Type)
	}
	if in.Spec.VirtualRouter != existingNw.Spec.VirtualRouter {
		return i, true, fmt.Errorf("cannot modify Virtual Router [%v->%v]", existingNw.Spec.Type, in.Spec.Type)
	}
	if in.Spec.VlanID != existingNw.Spec.VlanID {
		return i, true, fmt.Errorf("cannot modify VlanID of a network")
	}
	if in.Spec.VxlanVNI != existingNw.Spec.VxlanVNI {
		return i, true, fmt.Errorf("cannot modify VxlanVNI of a network")
	}
	return i, true, nil
}

func (h *networkHooks) checkNetworkCreateConfig(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	in, ok := i.(network.Network)
	if !ok {
		h.logger.ErrorLog("method", "checkNetworkCreateConfig", "msg", fmt.Sprintf("API server network hook called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}

	// make sure that vlanID is unique (network:vlan relationship is 1:1)
	nw := &network.Network{
		ObjectMeta: api.ObjectMeta{
			Tenant: globals.DefaultTenant,
		},
	}
	// CAVEAT: List approach has a small timing window where this check does not work on
	// back-to-back operations
	// create-create with same vlan will not detect the error until 1st transaction is committed
	// delete-create will report false error if 1st transaction is not committed
	// NPM and other controllers which act on network objects shuold perform their own checks
	var networks network.NetworkList
	nwKey := nw.MakeKey(string(apiclient.GroupNetwork))
	err := kv.List(ctx, nwKey, &networks)
	if err != nil {
		return i, true, fmt.Errorf("Error retrieving networks: %s", err)
	}
	for _, exNw := range networks.Items {
		if exNw.Name == in.Name {
			// self-check: it is possible (e.g. in multi-threaded env) that obj with same name
			// got committed to DB when this hook is called.. ignore that object.
			// Commit for this obj will fail later as expected
			continue
		}
		if exNw.Spec.VlanID == in.Spec.VlanID {
			return i, true, fmt.Errorf("Network vlanID must be unique, already used by %s", exNw.Name)
		}
	}
	return i, true, nil
}

// Checks that for any orch info deletion, no workloads from the orch info are using this network
func (h *networkHooks) networkOrchConfigPrecommit(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	in, ok := i.(network.Network)
	if !ok {
		h.logger.ErrorLog("method", "networkOrchConfigPrecommit", "msg", fmt.Sprintf("API server network hook called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}

	if in.Tenant != globals.DefaultTenant {
		// Since orch config can only live in default tenant scope,
		// this hook check is not needed if tenant is different
		return i, true, nil
	}

	existingObj := &network.Network{}
	err := kv.Get(ctx, key, existingObj)
	if err != nil {
		return i, true, fmt.Errorf("Failed to get existing object: %s", err)
	}

	// Build new network's orch info map
	orchMap := map[string](map[string]bool){}

	for _, config := range in.Spec.Orchestrators {
		if _, ok := orchMap[config.Name]; !ok {
			orchMap[config.Name] = map[string]bool{}
		}
		entry := orchMap[config.Name]
		entry[config.Namespace] = true
	}

	// check if any orch or namespace got removed
	orchRemoved := false
	for _, config := range existingObj.Spec.Orchestrators {
		entry, ok := orchMap[config.Name]
		if !ok {
			// orchestrator is removed
			orchRemoved = true
			break
		} else if _, ok := entry[config.Namespace]; !ok {
			if _, ok := entry[utils.ManageAllDcs]; !ok {
				// namespace is removed and new object does not use all_namespaces
				orchRemoved = true
				break
			}
		}
	}
	// avoid checking workloads if no orch config changed
	if !orchRemoved {
		return i, true, nil
	}
	// Fetch workloads
	var workloads workload.WorkloadList
	wl := workload.Workload{
		ObjectMeta: api.ObjectMeta{
			Tenant: in.Tenant,
		},
	}
	listKey := wl.MakeKey(string(apiclient.GroupWorkload))
	err = kv.List(ctx, listKey, &workloads)
	if err != nil {
		return i, true, fmt.Errorf("Error retrieving workloads: %s", err)
	}

	// check workloads
	for _, wl := range workloads.Items {
		// check if workload was created by orchestrator
		orch, ok := utils.GetOrchNameFromObj(wl.Labels)
		if !ok {
			continue
		}
		namespace, ok := utils.GetOrchNamespaceFromObj(wl.Labels)
		if !ok {
			continue
		}
		// check if the workload has any interface on this network
		networkUsed := false
		for _, inf := range wl.Spec.Interfaces {
			if inf.Network == in.Name {
				networkUsed = true
				break
			}
		}
		if !networkUsed {
			continue
		}
		// check if this workload belongs to an orch or DC that is not configured anymore (removed)
		if entry, ok := orchMap[orch]; ok {
			// check if namespace is stil configured
			if _, ok := entry[namespace]; ok {
				continue
			} else if _, ok := entry[utils.ManageAllDcs]; ok {
				continue
			} else {
				// Cannot remove DC from the the networr
				return i, true, fmt.Errorf("Cannot remove orchestrator %s namespace %s, as workloads from this orchestrator namespace are using this network", orch, namespace)
			}
		} else {
			// Cannot remove the orchestrator from the network
			return i, true, fmt.Errorf("Cannot remove orchestrator info %s, as workloads from this orchestrator are using this network", orch)
		}
	}
	return i, true, nil
}

func (h *networkHooks) validateVirtualrouterConfig(i interface{}, ver string, ignStatus, ignoreSpec bool) []error {
	var ret []error

	in := i.(network.VirtualRouter)
	if in.Spec.Type == network.VirtualRouterSpec_Infra.String() {
		if in.Spec.VxLanVNI != 0 {
			ret = append(ret, fmt.Errorf("VxLAN VNI cannot be specified for an Infra Virtual Router"))
		}
		if in.Spec.RouteImportExport != nil {
			ret = append(ret, fmt.Errorf("Route Import Export cannot be specified for an Infra Virtual Router"))
		}
	}
	if in.Spec.RouteImportExport != nil && in.Spec.RouteImportExport.AddressFamily == network.BGPAddressFamily_IPv4Unicast.String() {
		ret = append(ret, fmt.Errorf("Route Import Export of address family ipv4 unicast cannot be specified for Virtual Router"))
	}
	return ret
}
func (h *networkHooks) validateNetworkIntfConfig(i interface{}, ver string, ignStatus, ignoreSpec bool) []error {
	var ret []error

	in := i.(network.NetworkInterface)
	log.Infof("Got Newtwork interface [%+v]", in)
	if in.Spec.Type != network.IFType_HOST_PF.String() && in.Spec.AttachNetwork != "" && in.Spec.AttachTenant != "" {
		ret = append(ret, fmt.Errorf("attach-tenant and attach-network can only be specified for HOST_PFs"))
	}

	if in.Spec.AdminStatus == network.IFStatus_DOWN.String() {
		ret = append(ret, fmt.Errorf("Interface Down operation is not supported"))
	}

	if in.Spec.AttachNetwork != "" && in.Spec.AttachTenant == "" {
		ret = append(ret, fmt.Errorf("attach-network needs attach-tenant to be specified"))
	}

	if in.Spec.AttachNetwork == "" && in.Spec.AttachTenant != "" {
		ret = append(ret, fmt.Errorf("attach-tenant needs attach-network to be specified"))
	}

	return ret
}

// createDefaultRoutingTable is a pre-commit hook to creates default RouteTable when a tenant is created
func (h *networkHooks) checkVirtualRouterMutableUpdate(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	existingObj := &network.VirtualRouter{}
	err := kv.Get(ctx, key, existingObj)
	if err != nil {
		log.Errorf("did not find obj [%v] on update (%s)", key, err)
		return i, true, err
	}
	curObj, ok := i.(network.VirtualRouter)
	if !ok {
		h.logger.ErrorLog("method", "checkVirtualRouterMutableUpdate", "msg", fmt.Sprintf("API server hook to create RouteTable called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}
	if curObj.Spec.Type != existingObj.Spec.Type {
		return i, true, fmt.Errorf("VirtualRouter Type cannot be modified [%v->%v]", existingObj.Spec.Type, curObj.Spec.Type)
	}
	if curObj.Spec.VxLanVNI != existingObj.Spec.VxLanVNI {
		return i, true, fmt.Errorf("VirtualRouter VxLanVNI cannot be updated")
	}
	return i, true, nil
}

// createDefaultRoutingTable is a pre-commit hook to creates default RouteTable when a tenant is created
func (h *networkHooks) createDefaultVRFRouteTable(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	r, ok := i.(network.VirtualRouter)
	if !ok {
		h.logger.ErrorLog("method", "createDefaultVRFRouteTable", "msg", fmt.Sprintf("API server hook to create RouteTable called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}
	rt := &network.RouteTable{}
	rt.Defaults("all")
	apiSrv := apisrvpkg.MustGetAPIServer()
	rt.APIVersion = apiSrv.GetVersion()
	rt.SelfLink = rt.MakeURI("configs", rt.APIVersion, string(apiclient.GroupNetwork))
	rt.Name = r.Name + "." + "default"
	rt.Tenant = r.Tenant
	rt.Namespace = r.Namespace
	rt.GenerationID = "1"
	rt.UUID = uuid.NewV4().String()
	ts, err := types.TimestampProto(time.Now())
	if err != nil {
		return i, true, err
	}
	rt.CreationTime, rt.ModTime = api.Timestamp{Timestamp: *ts}, api.Timestamp{Timestamp: *ts}
	rtk := rt.MakeKey(string(apiclient.GroupNetwork))
	err = txn.Create(rtk, rt)
	if err != nil {
		return r, true, errors.New("adding create operation to transaction failed")
	}
	return r, true, nil
}

// deleteDefaultRoutingTable is a pre-commit hook to delete default RouteTable when a tenant is deleted
func (h *networkHooks) deleteDefaultVRFRouteTable(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	r, ok := i.(network.VirtualRouter)
	if !ok {
		h.logger.ErrorLog("method", "deleteDefaultVRFRouteTable", "msg", fmt.Sprintf("API server hook to delete default RouteTable called for invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}
	rt := &network.RouteTable{}
	rt.Defaults("all")
	apiSrv := apisrvpkg.MustGetAPIServer()
	rt.APIVersion = apiSrv.GetVersion()
	rt.SelfLink = rt.MakeURI("configs", rt.APIVersion, string(apiclient.GroupNetwork))
	rt.Name = r.Name + "." + "default"
	rt.Tenant = r.Tenant
	rt.Namespace = r.Namespace
	rtk := rt.MakeKey(string(apiclient.GroupNetwork))
	err := txn.Delete(rtk)
	if err != nil {
		return r, true, errors.New("adding delete operation to transaction failed")
	}
	return r, true, nil
}

// checkNetworkInterfaceMutable is a pre-commit hook check for immutable fields
func (h *networkHooks) checkNetworkInterfaceMutable(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	r, ok := i.(network.NetworkInterface)
	if !ok {
		h.logger.ErrorLog("method", "checkNetworkInterfaceMutable", "msg", fmt.Sprintf("API server hook checkNetworkInterfaceMutable with invalid object type [%#v]", i))
		return i, true, errors.New("invalid input type")
	}
	if oper != apiintf.UpdateOper {
		return i, true, nil
	}
	pctx := apiutils.SetVar(ctx, apiutils.CtxKeyGetPersistedKV, true)
	existingObj := &network.NetworkInterface{}
	err := kv.Get(pctx, key, existingObj)
	if err != nil {
		return i, true, fmt.Errorf("Failed to get existing object: %s", err)
	}
	if existingObj.Spec.Type != r.Spec.Type {
		return i, true, fmt.Errorf("Interface Type cannot be changed")
	}
	return i, true, nil
}

func (h *networkHooks) updateAuthStatus(ctx context.Context, kvs kvstore.Interface, prefix string, in, old, resp interface{}, oper apiintf.APIOperType) (interface{}, error) {
	switch oper {
	case apiintf.CreateOper, apiintf.UpdateOper, apiintf.DeleteOper, apiintf.GetOper:
		var inRtCfg network.RoutingConfig
		if oper == apiintf.DeleteOper {
			inRtCfg = old.(network.RoutingConfig)
		} else {
			ino, ok := in.(network.RoutingConfig)
			if !ok {
				return resp, fmt.Errorf("invalid type")
			}
			err := kvs.Get(ctx, ino.MakeKey(prefix), &inRtCfg)
			if err != nil {
				return resp, err
			}
		}
		outRtCfg, ok := resp.(network.RoutingConfig)
		if ok {
			if inRtCfg.Spec.BGPConfig != nil {
				outRtCfg.Status.AuthConfigStatus = nil
				for _, n := range inRtCfg.Spec.BGPConfig.Neighbors {
					stat := &network.BGPAuthStatus{
						IPAddress: n.IPAddress,
						RemoteAS:  n.RemoteAS.ASNumber,
					}
					if n.Password != "" {
						stat.Status = network.BGPAuthStatus_Enabled.String()
					} else {
						stat.Status = network.BGPAuthStatus_Disabled.String()
					}
					outRtCfg.Status.AuthConfigStatus = append(outRtCfg.Status.AuthConfigStatus, stat)
				}
			}
			return outRtCfg, nil
		}
	case apiintf.ListOper:
		inMap := make(map[string]*network.RoutingConfig)
		outRtCfg, ok := resp.(network.RoutingConfigList)
		if ok {
			for _, r := range outRtCfg.Items {
				inRtCfg := new(network.RoutingConfig)
				err := kvs.Get(ctx, r.MakeKey(prefix), inRtCfg)
				if err != nil {
					return resp, err
				}
				inMap[r.Name] = inRtCfg
			}
		}

		if ok {
			for _, r := range outRtCfg.Items {
				r.Status.AuthConfigStatus = nil
				if r1, ok := inMap[r.Name]; ok {
					for _, n := range r1.Spec.BGPConfig.Neighbors {
						stat := &network.BGPAuthStatus{
							IPAddress: n.IPAddress,
							RemoteAS:  n.RemoteAS.ASNumber,
						}
						if n.Password != "" {
							stat.Status = network.BGPAuthStatus_Enabled.String()
						} else {
							stat.Status = network.BGPAuthStatus_Disabled.String()
						}
						r.Status.AuthConfigStatus = append(r.Status.AuthConfigStatus, stat)
					}
				}
			}
		}
		return outRtCfg, nil
	}
	return resp, nil
}

func (h *networkHooks) networkVNIReserve(ctx context.Context, i interface{}, kvs kvstore.Interface, key string, dryrun bool) (apiserver.ResourceRollbackFn, error) {
	n, ok := i.(network.Network)
	if !ok {
		log.Errorf("networkVNIReserve; invalid kind")
		return nil, fmt.Errorf("invalid kind received")
	}
	if n.Spec.VxlanVNI != 0 {
		defer h.vnidMapMu.Unlock()
		h.vnidMapMu.Lock()
		if v, ok := h.vnidMap[n.Spec.VxlanVNI]; ok {
			vals := strings.Split(v, "/")
			if len(vals) == 3 {
				if vals[1] == n.Tenant {
					return nil, fmt.Errorf("vxlan-vni [%d] already used by %v", n.Spec.VxlanVNI, v)
				}
			}
			return nil, fmt.Errorf("vxlan-vni [%d] already in use", n.Spec.VxlanVNI)
		}

		h.vnidMap[n.Spec.VxlanVNI] = "Network/" + n.Tenant + "/" + n.Name
		return func(ctx context.Context, i interface{}, _ kvstore.Interface, key string, dryrun bool) {
			delete(h.vnidMap, n.Spec.VxlanVNI)
		}, nil
	}
	return nil, nil
}

func (h *networkHooks) networkSubnetReserve(ctx context.Context, i interface{}, kvs kvstore.Interface, key string, dryrun bool) (apiserver.ResourceRollbackFn, error) {
	n, ok := i.(network.Network)
	if !ok {
		log.Errorf("networkSubnetReserve; invalid kind")
		return nil, fmt.Errorf("invalid kind received")
	}
	var err error
	var ret *net.IPNet

	if n.Spec.IPv4Subnet != "" {
		h.netwMapMu.Lock()
		_, ret, err = net.ParseCIDR(n.Spec.IPv4Subnet)
		if err != nil {
			log.Errorf("failed to parse [%v](%s)", n.Spec.IPv4Subnet, err)
			h.netwMapMu.Unlock()
			return nil, fmt.Errorf("could not parse network")
		}
		m, ok := h.netwMap[n.Tenant+"/"+n.Spec.VirtualRouter]
		if !ok {
			m = make(map[string]*net.IPNet)
			m[n.Name] = ret
			h.netwMap[n.Tenant+"/"+n.Spec.VirtualRouter] = m
		} else {
			for k, s := range m {
				if k != n.Name {
					if s.Contains(ret.IP) || ret.Contains(s.IP) {
						log.Errorf("overlapping subnet, existing[%v][%v] new[%v][%v]", k, s.IP.String(), n.Name, ret.IP.String())
						h.netwMapMu.Unlock()
						return nil, fmt.Errorf("IP subnet overlaps with %v", k)
					}
				}
			}
			m[n.Name] = ret
			h.netwMap[n.Tenant+"/"+n.Spec.VirtualRouter] = m
		}
		h.netwMapMu.Unlock()
	}
	return func(ctx context.Context, i interface{}, _ kvstore.Interface, key string, dryrun bool) {
		h.releaseNetworkIPNet(n)
	}, nil
}

func (h *networkHooks) vrouterVNIReserve(ctx context.Context, i interface{}, kvs kvstore.Interface, key string, dryrun bool) (apiserver.ResourceRollbackFn, error) {
	n, ok := i.(network.VirtualRouter)
	if !ok {
		log.Errorf("vrouterVNIReserve; invalid kind")
		return nil, fmt.Errorf("invalid kind received")
	}
	if n.Spec.VxLanVNI != 0 {
		defer h.vnidMapMu.Unlock()
		h.vnidMapMu.Lock()
		if v, ok := h.vnidMap[n.Spec.VxLanVNI]; ok {
			vals := strings.Split(v, "/")
			if len(vals) == 3 {
				if vals[1] == n.Tenant {
					return nil, fmt.Errorf("vxlan-vni [%d] already used by %v", n.Spec.VxLanVNI, v)
				}
			}
			return nil, fmt.Errorf("vxlan-vni [%d] already in use", n.Spec.VxLanVNI)
		}

		h.vnidMap[n.Spec.VxLanVNI] = "VirtualRouter/" + n.Tenant + "/" + n.Name
		return func(ctx context.Context, i interface{}, _ kvstore.Interface, key string, dryrun bool) {
			delete(h.vnidMap, n.Spec.VxLanVNI)
		}, nil
	}
	return nil, nil
}

func (h *networkHooks) releaseNetworkIPNet(n network.Network) {
	if n.Spec.IPv4Subnet != "" {
		h.netwMapMu.Lock()
		m, ok := h.netwMap[n.Tenant+"/"+n.Spec.VirtualRouter]
		if ok {
			delete(m, n.Name)
		}
		if len(m) == 0 {
			delete(h.netwMap, n.Tenant+"/"+n.Spec.VirtualRouter)
		}
		h.netwMapMu.Unlock()
	}
}

func (h *networkHooks) releaseNetworkResources(ctx context.Context, oper apiintf.APIOperType, i interface{}, dryRun bool) {
	n, ok := i.(network.Network)
	if !ok {
		log.Errorf("releaseNetworkResources; invalid kind")
		return
	}
	if n.Spec.VxlanVNI != 0 {
		h.vnidMapMu.Lock()
		if v, ok := h.vnidMap[n.Spec.VxlanVNI]; ok {
			k := "Network/" + n.Tenant + "/" + n.Name
			if v != k {
				log.Errorf("current resource did not match [%v][%v]", v, k)
				h.vnidMapMu.Unlock()
				return
			}
			delete(h.vnidMap, n.Spec.VxlanVNI)
		} else {
			log.Errorf("could not find VNI in map [%v/%v][%v]", n.Tenant, n.Name, n.Spec.VxlanVNI)
		}
		h.vnidMapMu.Unlock()
	}

	if n.Spec.IPv4Subnet != "" {
		h.releaseNetworkIPNet(n)
	}
}

func (h *networkHooks) releaseVRouterResources(ctx context.Context, oper apiintf.APIOperType, i interface{}, dryRun bool) {
	n, ok := i.(network.VirtualRouter)
	if !ok {
		log.Errorf("releaseVRouterResources; invalid kind")
		return
	}

	if n.Spec.VxLanVNI != 0 {
		defer h.vnidMapMu.Unlock()
		h.vnidMapMu.Lock()
		if v, ok := h.vnidMap[n.Spec.VxLanVNI]; ok {
			k := "VirtualRouter/" + n.Tenant + "/" + n.Name
			if v != k {
				log.Errorf("current resource did not match [%v][%v]", v, k)
				return
			}
			delete(h.vnidMap, n.Spec.VxLanVNI)
		} else {
			log.Errorf("could not find VNI in map [%v/%v][%v]", n.Tenant, n.Name, n.Spec.VxLanVNI)
		}
	}
}

func (h *networkHooks) restoreResourceMap(kvs kvstore.Interface, logger log.Logger) {
	defer h.vnidMapMu.Unlock()
	h.vnidMapMu.Lock()
	// list all VRFs across tenants
	vrList := network.VirtualRouterList{}
	key := vrList.MakeKey(string(apiclient.GroupNetwork))
	for strings.HasSuffix(key, "//") {
		key = strings.TrimSuffix(key, "//")
	}
	err := kvs.List(context.TODO(), key, &vrList)
	if err == nil {
		for _, v := range vrList.Items {
			if v.Spec.VxLanVNI != 0 {
				key = "VirtualRouter/" + v.Tenant + "/" + v.Name
				h.vnidMap[v.Spec.VxLanVNI] = key
			}
		}
	}

	netList := network.NetworkList{}
	key = netList.MakeKey(string(apiclient.GroupNetwork))
	for strings.HasSuffix(key, "//") {
		key = strings.TrimSuffix(key, "//")
	}
	err = kvs.List(context.TODO(), key, &netList)
	if err == nil {
		for _, v := range netList.Items {
			if v.Spec.VxlanVNI != 0 {
				key = "Network/" + v.Tenant + "/" + v.Name
				h.vnidMap[v.Spec.VxlanVNI] = key
			}
		}
	}
}

func registerNetworkHooks(svc apiserver.Service, logger log.Logger) {
	hooks := networkHooks{
		vnidMap: make(map[uint32]string),
		netwMap: make(map[string]map[string]*net.IPNet),
	}
	hooks.svc = svc
	hooks.logger = logger
	logger.InfoLog("Service", "NetworkV1", "msg", "registering networkAction hook")
	svc.GetCrudService("IPAMPolicy", apiintf.CreateOper).GetRequestType().WithValidate(hooks.validateIPAMPolicyConfig)
	svc.GetCrudService("IPAMPolicy", apiintf.UpdateOper).GetRequestType().WithValidate(hooks.validateIPAMPolicyConfig)
	svc.GetCrudService("Network", apiintf.CreateOper).WithPreCommitHook(hooks.checkNetworkCreateConfig).WithResourceAllocHook(hooks.networkVNIReserve).WithResourceAllocHook(hooks.networkSubnetReserve)
	svc.GetCrudService("Network", apiintf.UpdateOper).GetRequestType().WithValidate(hooks.validateNetworkConfig)
	svc.GetCrudService("Network", apiintf.UpdateOper).WithPreCommitHook(hooks.networkOrchConfigPrecommit).WithResourceAllocHook(hooks.networkSubnetReserve)
	svc.GetCrudService("Network", apiintf.UpdateOper).WithPreCommitHook(hooks.checkNetworkMutableFields)
	svc.GetCrudService("Network", apiintf.DeleteOper).WithPostCommitHook(hooks.releaseNetworkResources)
	svc.GetCrudService("NetworkInterface", apiintf.UpdateOper).GetRequestType().WithValidate(hooks.validateNetworkIntfConfig)
	svc.GetCrudService("NetworkInterface", apiintf.UpdateOper).WithPreCommitHook(hooks.checkNetworkInterfaceMutable)
	svc.GetCrudService("VirtualRouter", apiintf.CreateOper).WithPreCommitHook(hooks.createDefaultVRFRouteTable).WithResourceAllocHook(hooks.vrouterVNIReserve)
	svc.GetCrudService("VirtualRouter", apiintf.UpdateOper).GetRequestType().WithValidate(hooks.validateVirtualrouterConfig)
	svc.GetCrudService("VirtualRouter", apiintf.UpdateOper).WithPreCommitHook(hooks.checkVirtualRouterMutableUpdate)
	svc.GetCrudService("VirtualRouter", apiintf.DeleteOper).WithPreCommitHook(hooks.deleteDefaultVRFRouteTable).WithPostCommitHook(hooks.releaseVRouterResources)
	svc.GetCrudService("RoutingConfig", apiintf.CreateOper).WithPreCommitHook(hooks.routingConfigPreCommit).WithResponseWriter(hooks.updateAuthStatus)
	svc.GetCrudService("RoutingConfig", apiintf.UpdateOper).WithPreCommitHook(hooks.routingConfigPreCommit).WithResponseWriter(hooks.updateAuthStatus)
	svc.GetCrudService("RoutingConfig", apiintf.GetOper).WithResponseWriter(hooks.updateAuthStatus)
	svc.GetCrudService("RoutingConfig", apiintf.ListOper).WithResponseWriter(hooks.updateAuthStatus)
	svc.GetCrudService("RoutingConfig", apiintf.UpdateOper).GetRequestType().WithValidate(hooks.validateRoutingConfig)

	apisrv := apisrvpkg.MustGetAPIServer()
	apisrv.RegisterRestoreCallback(hooks.restoreResourceMap)
}

func init() {
	apisrv := apisrvpkg.MustGetAPIServer()
	apisrv.RegisterHooksCb("network.NetworkV1", registerNetworkHooks)
}
