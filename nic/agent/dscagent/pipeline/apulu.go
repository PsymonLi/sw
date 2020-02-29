// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package pipeline

import (
	"context"
	"fmt"
	"io"
	"strconv"
	"sync"
	"time"

	"github.com/gogo/protobuf/proto"
	protoTypes "github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/common"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu"
	apuluutils "github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	apuluValidator "github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	msapi "github.com/pensando/sw/nic/metaswitch/gen/agent"
	"github.com/pensando/sw/venice/utils/log"
)

// ApuluAPI implements PipelineAPI for Apulu pipeline
type ApuluAPI struct {
	sync.Mutex
	InfraAPI                types.InfraAPI
	VPCClient               halapi.VPCSvcClient
	SubnetClient            halapi.SubnetSvcClient
	DeviceSvcClient         halapi.DeviceSvcClient
	RoutingClient           msapi.BGPSvcClient
	EvpnClient              msapi.EvpnSvcClient
	SecurityPolicySvcClient halapi.SecurityPolicySvcClient
	DHCPRelayClient         halapi.DHCPSvcClient
	InterfaceClient         halapi.IfSvcClient
	EventClient             halapi.EventSvcClient
	PortClient              halapi.PortSvcClient
	CPRouteSvcClient        msapi.CPRouteSvcClient
	MirrorClient            halapi.MirrorSvcClient
	RouteSvcClient          halapi.RouteSvcClient
	LocalInterfaces         map[string]string
}

// NewPipelineAPI returns the implemetor of PipelineAPI
func NewPipelineAPI(infraAPI types.InfraAPI) (*ApuluAPI, error) {
	conn, err := utils.CreateNewGRPCClient("PDS_GRPC_PORT", types.HalGRPCDefaultPort)
	if err != nil {
		log.Errorf("Failed to create GRPC Connection to HAL. Err: %v", err)
		return nil, err
	}

	a := ApuluAPI{
		InfraAPI:                infraAPI,
		VPCClient:               halapi.NewVPCSvcClient(conn),
		SubnetClient:            halapi.NewSubnetSvcClient(conn),
		DeviceSvcClient:         halapi.NewDeviceSvcClient(conn),
		SecurityPolicySvcClient: halapi.NewSecurityPolicySvcClient(conn),
		DHCPRelayClient:         halapi.NewDHCPSvcClient(conn),
		InterfaceClient:         halapi.NewIfSvcClient(conn),
		PortClient:              halapi.NewPortSvcClient(conn),
		EventClient:             halapi.NewEventSvcClient(conn),
		RouteSvcClient:          halapi.NewRouteSvcClient(conn),
		CPRouteSvcClient:        msapi.NewCPRouteSvcClient(conn),
		RoutingClient:           msapi.NewBGPSvcClient(conn),
		EvpnClient:              msapi.NewEvpnSvcClient(conn),
		MirrorClient:            halapi.NewMirrorSvcClient(conn),
		LocalInterfaces:         make(map[string]string),
	}

	if err := a.PipelineInit(); err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineInit, "Apulu Init: %v", err))
		return nil, errors.Wrapf(types.ErrPipelineInit, "Apulu Init: %v", err)
	}

	return &a, nil
}

// ############################################### PipelineAPI Methods  ###############################################

// PipelineInit does Apulu Pipeline init. Creating device, default security profile
func (a *ApuluAPI) PipelineInit() error {
	log.Infof("Apulu API: %s", types.InfoPipelineInit)
	if err := a.HandleDevice(types.Create); err != nil {
		log.Error(err)
		return err
	}
	log.Infof("Apulu API: %s | %s", types.InfoPipelineInit, types.InfoDeviceCreate)
	profile := netproto.SecurityProfile{
		TypeMeta: api.TypeMeta{
			Kind: "SecurityProfile",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "default",
			UUID:      uuid.NewV4().String(),
		},
	}
	if _, err := a.HandleSecurityProfile(types.Create, profile); err != nil {
		log.Error(err)
		return err
	}
	log.Infof("Apulu API: %s | %s", types.InfoPipelineInit, types.InfoSecurityProfileCreate)

	c, _ := protoTypes.TimestampProto(time.Now())
	defaultVrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    types.DefaultTenant,
			Namespace: types.DefaultNamespace,
			Name:      types.DefaulUnderlaytVrf,
			CreationTime: api.Timestamp{
				Timestamp: *c,
			},
			ModTime: api.Timestamp{
				Timestamp: *c,
			},
			UUID: uuid.NewV4().String(),
		},
		Spec: netproto.VrfSpec{
			VrfType: "INFRA",
		},
	}
	if _, err := a.HandleVrf(types.Create, defaultVrf); err != nil {
		log.Error(err)
		return err
	}

	// initialize stream for Lif events
	a.initEventStream()
	return nil
}

// HandleVeniceCoordinates initializes the pipeline when VeniceCoordinates are discovered
func (a *ApuluAPI) HandleVeniceCoordinates(dsc types.DistributedServiceCardStatus) {
	// Create the Loopback interface
	lb := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			UUID:      uuid.NewV4().String(),
		},
		Spec: netproto.InterfaceSpec{
			Type:        netproto.InterfaceSpec_LOOPBACK.String(),
			AdminStatus: netproto.IFStatus_UP.String(),
		},
	}

	ifName, err := utils.GetIfName(a.InfraAPI.GetDscName(), utils.GetIfIndex(netproto.InterfaceSpec_LOOPBACK.String(), 0, 0, 1), "")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, loopback 0, err %v",
			lb.UUID, err))
		return
	}
	lb.Name = ifName

	// Find if we already have the interface in BoltDB (in case of stateful reboot)
	{
		curLB := netproto.Interface{}
		o, err := a.InfraAPI.Read("Interface", lb.GetKey())
		if err == nil {
			err = curLB.Unmarshal(o)
			if err != nil {
				log.Errorf("Could not parse existing Interface object")
			} else {
				lb = curLB
			}
		}
	}

	if _, ok := a.LocalInterfaces[lb.Name]; ok {
		log.Infof("loopback interface already created [%v]", lb.Name)
		return
	}
	a.LocalInterfaces[lb.Name] = lb.UUID
	if _, err := a.HandleInterface(types.Create, lb); err != nil {
		log.Errorf("Init: Failed to create loopback interface: Err: %s", err)
	}
	// inject loopback so venice can be updated
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: lb,
	}
	a.InfraAPI.UpdateIfChannel() <- ifEvnt
}

// HandleDevice handles CRUD methods for Device objects
func (a *ApuluAPI) HandleDevice(oper types.Operation) error {
	a.Lock()
	defer a.Unlock()
	log.Infof("Device Op: %s | %s", oper, types.InfoHandleObjBegin)
	defer log.Infof("Device Op: %s | %s", oper, types.InfoHandleObjEnd)
	cfg := a.InfraAPI.GetConfig()
	var lbip *halapi.IPAddress

	if cfg.LoopbackIP != "" {
		lbip = apuluutils.ConvertIPAddress(cfg.LoopbackIP)
	}
	return apulu.HandleDevice(oper, a.DeviceSvcClient, lbip)
}

// HandleVrf handles CRUD Methods for Vrf Object
func (a *ApuluAPI) HandleVrf(oper types.Operation, vrf netproto.Vrf) (vrfs []netproto.Vrf, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, vrf.Kind, vrf.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	log.Infof("Handle Vrf received [%s][%+v]", oper, vrf)
	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Vrf
		)
		dat, err = a.InfraAPI.Read(vrf.Kind, vrf.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err)
		}
		vrfs = append(vrfs, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(vrf.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var vrf netproto.Vrf
			err := proto.Unmarshal(o, &vrf)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err))
				continue
			}
			vrfs = append(vrfs, vrf)
		}

		return
	case types.Create:
	case types.Update:
		// Get to ensure that the object exists
		var existingVrf netproto.Vrf
		dat, err := a.InfraAPI.Read(vrf.Kind, vrf.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Vrf: %s | Err: %v", vrf.GetKey(), types.ErrObjNotFound)
		}
		err = existingVrf.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&vrf.Spec, &existingVrf.Spec) {
			//log.Infof("Vrf: %s | Info: %s ", vrf.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

	case types.Delete:
		var existingVrf netproto.Vrf
		dat, err := a.InfraAPI.Read(vrf.Kind, vrf.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingVrf.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", vrf.GetKey(), err)
		}
		vrf = existingVrf
	}
	log.Infof("Vrf: %+v | Op: %s | %s", vrf, oper, types.InfoHandleObjBegin)
	defer log.Infof("Vrf: %v | Op: %s | %s", vrf.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleVPC(a.InfraAPI, a.VPCClient, a.EvpnClient, oper, vrf)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleNetwork handles CRUD Methods for Network Object
func (a *ApuluAPI) HandleNetwork(oper types.Operation, network netproto.Network) (networks []netproto.Network, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, network.Kind, network.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Network
		)
		dat, err = a.InfraAPI.Read(network.Kind, network.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err)
		}
		networks = append(networks, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(network.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var network netproto.Network
			err := proto.Unmarshal(o, &network)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err))
				continue
			}
			networks = append(networks, network)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if network.Status.NetworkID == 0 {
			network.Status.NetworkID = a.InfraAPI.AllocateID(types.NetworkID, types.NetworkOffSet)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingNetwork netproto.Network
		dat, err := a.InfraAPI.Read(network.Kind, network.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound)
		}
		err = existingNetwork.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&network.Spec, &existingNetwork.Spec) {
			//log.Infof("Network: %s | Info: %s ", network.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		network.Status.NetworkID = existingNetwork.Status.NetworkID
	case types.Delete:
		var existingNetwork netproto.Network
		dat, err := a.InfraAPI.Read(network.Kind, network.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingNetwork.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", network.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Vrf: %s | Err: %v", network.GetKey(), err)
		}
		network = existingNetwork
	}
	// Perform object validations
	uplinkIDs, vrf, err := validator.ValidateNetwork(a.InfraAPI, oper, network)
	if err != nil {
		log.Errorf("Network: %v | Op: %s | Err: %s", network, oper, err)
		return nil, err
	}
	log.Infof("Network: %v | Op: %s | %s", network.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Network: %v | Op: %s | %s", network, oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleSubnet(a.InfraAPI, a.SubnetClient, a.EvpnClient, oper, network, vrf.Status.VrfID, uplinkIDs)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleEndpoint unimplemented
func (a *ApuluAPI) HandleEndpoint(oper types.Operation, ep netproto.Endpoint) ([]netproto.Endpoint, error) {
	return nil, errors.Wrapf(types.ErrNotImplemented, "Endpoint %s is not implemented by Apulu Pipeline", oper)
}

// HandleInterface handles CRUD Methods for Interface object
func (a *ApuluAPI) HandleInterface(oper types.Operation, intf netproto.Interface) (intfs []netproto.Interface, err error) {
	// Take a lock to ensure a single HAL API is active at any given point
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, intf.Kind, intf.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Interface
		)
		dat, err = a.InfraAPI.Read(intf.Kind, intf.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err)
		}
		intfs = append(intfs, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(intf.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var intf netproto.Interface
			err := proto.Unmarshal(o, &intf)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
				continue
			}
			intfs = append(intfs, intf)
		}

		return
	case types.Create:
		// Allow only L3 and loopback interfaces be created
		if intf.Spec.Type != netproto.InterfaceSpec_L3.String() && intf.Spec.Type != netproto.InterfaceSpec_LOOPBACK.String() {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrInvalidInterfaceType))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrInvalidInterfaceType)
		}
	case types.Update:
		// Get to ensure that the object exists
		var existingIntf netproto.Interface
		dat, err := a.InfraAPI.Read(intf.Kind, intf.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound)
		}
		err = existingIntf.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&intf.Spec, &existingIntf.Spec) {
			//log.Infof("Interface: %s | Info: %s ", intf.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}
	case types.Delete:
		var existingIntf netproto.Interface

		// Allow only L3 and loopback interfaces be deleted
		if intf.Spec.Type != netproto.InterfaceSpec_L3.String() && intf.Spec.Type != netproto.InterfaceSpec_LOOPBACK.String() {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrInvalidInterfaceType))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrInvalidInterfaceType)
		}
		dat, err := a.InfraAPI.Read(intf.Kind, intf.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingIntf.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err)
		}
		intf = existingIntf
	}

	uidStr, ok := a.LocalInterfaces[intf.Name]
	if !ok {
		log.Infof("Inteface: %s not local, ignoring", intf.GetKey())
		return nil, nil
	}

	// Use the UUID from the cache when interacting with PDS Agent
	intf.UUID = uidStr
	log.Infof("Interface: %v | Op: %s | %s", intf, oper, types.InfoHandleObjBegin)
	defer log.Infof("Interface: %v | Op: %s | Err: %v | %s", intf, oper, err, types.InfoHandleObjEnd)

	// Check for Loopback Update
	oldLoopbackIf := ""
	if intf.Spec.Type != netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := a.InfraAPI.GetConfig()
		oldLoopbackIf = cfg.LoopbackIP
	}

	err = apulu.HandleInterface(a.InfraAPI, a.InterfaceClient, a.SubnetClient, oper, intf)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := a.InfraAPI.GetConfig()
		if cfg.LoopbackIP != oldLoopbackIf {
			log.Infof("BGP RID change detected, deleting and recreating BGP config and updating Device")
			apulu.UpdateBGPLoopbackConfig(a.InfraAPI, a.RoutingClient)

			if cfg.LoopbackIP != "" {
				lbip := apuluutils.ConvertIPAddress(cfg.LoopbackIP)
				apulu.HandleDevice(types.Update, a.DeviceSvcClient, lbip)
			}
		}
	}
	return
}

// HandleTunnel unimplemented
func (a *ApuluAPI) HandleTunnel(oper types.Operation, tun netproto.Tunnel) ([]netproto.Tunnel, error) {
	return nil, errors.Wrapf(types.ErrNotImplemented, "Tunnel %s is not implemented by Apulu Pipeline", oper)
}

// HandleApp handles CRUD Methods for App Object
func (a *ApuluAPI) HandleApp(oper types.Operation, app netproto.App) (apps []netproto.App, err error) {
	a.Lock()
	defer a.Unlock()

	apps, err = common.HandleApp(a.InfraAPI, oper, app)
	if err != nil {
		return
	}
	// TODO Trigger this from NPM's OnAppUpdate method
	if oper == types.Update {
		//
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List("NetworkSecurityPolicy")
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to list NetworkSecurityPolicies | Err: %v", err))
		}

		for _, o := range dat {
			var nsp netproto.NetworkSecurityPolicy
			err := nsp.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err))
				continue
			}
			for _, r := range nsp.Spec.Rules {
				if r.AppName == app.Name {
					// Update NetworkSecurity Policy here
					_, ruleIDToAppMapping, err := validator.ValidateNetworkSecurityPolicy(a.InfraAPI, nsp)
					if err != nil {
						break
					}
					if err := apuluValidator.ValidateNetworkSecurityPolicyApp(a.InfraAPI, nsp); err != nil {
						break
					}
					if err := apulu.HandleNetworkSecurityPolicy(a.InfraAPI, a.SecurityPolicySvcClient, oper, nsp, ruleIDToAppMapping); err == nil {
						break
					}
				}
			}
		}
	}
	return
}

// HandleNetworkSecurityPolicy handles CRUD Methods for NetworkSecurityPolicy Object
func (a *ApuluAPI) HandleNetworkSecurityPolicy(oper types.Operation, nsp netproto.NetworkSecurityPolicy) (netSecPolicies []netproto.NetworkSecurityPolicy, err error) {
	a.Lock()
	defer a.Unlock()
	err = utils.ValidateMeta(oper, nsp.Kind, nsp.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.NetworkSecurityPolicy
		)
		dat, err = a.InfraAPI.Read(nsp.Kind, nsp.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err)
		}
		netSecPolicies = append(netSecPolicies, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(nsp.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var nsp netproto.NetworkSecurityPolicy
			err := nsp.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err))
				continue
			}
			netSecPolicies = append(netSecPolicies, nsp)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if nsp.Status.NetworkSecurityPolicyID == 0 {
			nsp.Status.NetworkSecurityPolicyID = a.InfraAPI.AllocateID(types.NetworkSecurityPolicyID, 0)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingNetworkSecurityPolicy netproto.NetworkSecurityPolicy
		dat, err := a.InfraAPI.Read(nsp.Kind, nsp.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), types.ErrObjNotFound)
		}

		err = existingNetworkSecurityPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err)
		}

		if proto.Equal(&nsp.Spec, &existingNetworkSecurityPolicy.Spec) {
			return nil, nil
		}

		// Reuse ID from store
		nsp.Status.NetworkSecurityPolicyID = existingNetworkSecurityPolicy.Status.NetworkSecurityPolicyID
		log.Infof("Existing NetworkSecurityPolicy : %v", existingNetworkSecurityPolicy)
		log.Infof("New NetworkSecurityPolicy: %v", nsp)

	case types.Delete:
		var existingNetworkSecurityPolicy netproto.NetworkSecurityPolicy
		dat, err := a.InfraAPI.Read(nsp.Kind, nsp.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingNetworkSecurityPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "NetworkSecurityPolicy: %s | Err: %v", nsp.GetKey(), err)
		}
		nsp = existingNetworkSecurityPolicy

	}

	// Perform object validations
	_, ruleIDToAppMapping, err := validator.ValidateNetworkSecurityPolicy(a.InfraAPI, nsp)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	if err := apuluValidator.ValidateNetworkSecurityPolicyApp(a.InfraAPI, nsp); err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("NetworkSecurityPolicy: %v | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("NetworkSecurityPolicy: %v | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleNetworkSecurityPolicy(a.InfraAPI, a.SecurityPolicySvcClient, oper, nsp, ruleIDToAppMapping)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleProfile unimplemented
func (a *ApuluAPI) HandleProfile(oper types.Operation, profile netproto.Profile) ([]netproto.Profile, error) {
	return nil, errors.Wrapf(types.ErrNotImplemented, "Profile %s is not implemented by Apulu Pipeline", oper)
}

// ValidateSecurityProfile validates the contents of SecurityProfile objects
func (a *ApuluAPI) ValidateSecurityProfile(profile netproto.SecurityProfile) (vrf netproto.Vrf, err error) {
	return vrf, nil
}

// HandleSecurityProfile handles CRUD methods for SecurityProfile objects
func (a *ApuluAPI) HandleSecurityProfile(oper types.Operation, profile netproto.SecurityProfile) (profiles []netproto.SecurityProfile, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, profile.Kind, profile.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.SecurityProfile
		)
		dat, err = a.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err)
		}
		profiles = append(profiles, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(profile.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var profile netproto.SecurityProfile
			err := proto.Unmarshal(o, &profile)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err))
				continue
			}
			profiles = append(profiles, profile)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if profile.Status.SecurityProfileID == 0 {
			profile.Status.SecurityProfileID = a.InfraAPI.AllocateID(types.SecurityProfileID, types.SecurityProfileOffSet)
		}
	case types.Update:
		// Get to ensure that the object exists
		var existingSecurityProfile netproto.SecurityProfile
		dat, err := a.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "SecurityProfile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}
		err = existingSecurityProfile.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&profile.Spec, &existingSecurityProfile.Spec) {
			//log.Infof("SecurityProfile: %s | Info: %s ", profile.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		profile.Status.SecurityProfileID = existingSecurityProfile.Status.SecurityProfileID
	case types.Delete:
		var existingSecurityProfile netproto.SecurityProfile
		dat, err := a.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingSecurityProfile.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "SecurityProfile: %s | Err: %v", profile.GetKey(), err)
		}
		profile = existingSecurityProfile
	}
	log.Infof("SecurityProfile: %v | Op: %s | %s", profile, oper, types.InfoHandleObjBegin)
	defer log.Infof("SecurityProfile: %v | Op: %s | %s", profile, oper, types.InfoHandleObjEnd)

	// Perform object validations
	// Currently security profile is singleton and not associated with any VPC
	_, err = a.ValidateSecurityProfile(profile)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleSecurityProfile(a.InfraAPI, a.SecurityPolicySvcClient, oper, profile)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleIPAMPolicy handles CRUD methods for IPAMPolicy objects
func (a *ApuluAPI) HandleIPAMPolicy(oper types.Operation, policy netproto.IPAMPolicy) (policies []netproto.IPAMPolicy, err error) {
	// Take a lock to ensure a single HAL API is active at any given point
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, policy.Kind, policy.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.IPAMPolicy
		)
		dat, err = a.InfraAPI.Read(policy.Kind, policy.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err)
		}
		policies = append(policies, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(policy.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var policy netproto.IPAMPolicy
			err := proto.Unmarshal(o, &policy)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
				continue
			}
			policies = append(policies, policy)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if policy.Status.IPAMPolicyID == 0 {
			policy.Status.IPAMPolicyID = a.InfraAPI.AllocateID(types.IPAMPolicyID, 0)
		}
	case types.Update:
		// Get to ensure that the object exists
		var existingPolicy netproto.IPAMPolicy
		dat, err := a.InfraAPI.Read(policy.Kind, policy.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: %s | Err: %v", policy.GetKey(), types.ErrObjNotFound)
		}
		err = existingPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&policy.Spec, &existingPolicy.Spec) {
			return nil, nil
		}

		// Reuse ID from store
		policy.Status.IPAMPolicyID = existingPolicy.Status.IPAMPolicyID
	case types.Delete:
		var existingPolicy netproto.IPAMPolicy
		dat, err := a.InfraAPI.Read(policy.Kind, policy.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err)
		}
		policy = existingPolicy
	}
	log.Infof("IPAMPolicy: %v | Op: %s | %s", policy, oper, types.InfoHandleObjBegin)
	defer log.Infof("IPAMPolicy: %v | Op: %s | %s", policy, oper, types.InfoHandleObjEnd)

	err = apulu.HandleIPAMPolicy(a.InfraAPI, a.DHCPRelayClient, oper, policy)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleMirrorSession handles CRUDs for MirrorSession object
func (a *ApuluAPI) HandleMirrorSession(oper types.Operation, mirror netproto.MirrorSession) (mirrors []netproto.MirrorSession, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, mirror.Kind, mirror.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.MirrorSession
		)
		dat, err = a.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
		mirrors = append(mirrors, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(mirror.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var mirror netproto.MirrorSession
			err := proto.Unmarshal(o, &mirror)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
				continue
			}
			mirrors = append(mirrors, mirror)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if mirror.Status.MirrorSessionID == 0 {
			mirror.Status.MirrorSessionID = a.InfraAPI.AllocateID(types.MirrorSessionID, 0)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingMirrorSession netproto.MirrorSession
		dat, err := a.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "MirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}
		err = existingMirrorSession.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&mirror.Spec, &existingMirrorSession.Spec) {
			//log.Infof("MirrorSession: %s | Info: %s ", mirror.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		mirror.Status.MirrorSessionID = existingMirrorSession.Status.MirrorSessionID
	case types.Delete:
		var existingMirrorSession netproto.MirrorSession
		dat, err := a.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingMirrorSession.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "MirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
		mirror = existingMirrorSession
	}
	log.Infof("MirrorSession: %v | Op: %s | %s", mirror, oper, types.InfoHandleObjBegin)
	defer log.Infof("MirrorSession: %v | Op: %s | %s", mirror, oper, types.InfoHandleObjEnd)

	// Perform object validations
	mirrorDestToKeys := map[string]int{}
	vrf, err := validator.ValidateMirrorSession(a.InfraAPI, mirror, oper, mirrorDestToKeys)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	// Update Mirror session ID to be under 8. TODO remove this once HAL doesn't rely on agents to provide its hardware ID
	mirror.Status.MirrorSessionID = mirror.Status.MirrorSessionID % types.MaxMirrorSessions
	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleMirrorSession(a.InfraAPI, a.MirrorClient, oper, mirror, vrf.Status.VrfID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleFlowExportPolicy unimplemented
func (a *ApuluAPI) HandleFlowExportPolicy(oper types.Operation, netflow netproto.FlowExportPolicy) (netflows []netproto.FlowExportPolicy, err error) {
	return nil, errors.Wrapf(types.ErrNotImplemented, "Mirror Session not implemented by Apulu Pipeline")
}

//func (a *ApuluAPI)HandleTelemetry(oper types.Operation, tm *netproto.Telemetry) ([]*netproto.Telemetry, error) {
//	return nil, errors.Wrapf(types.ErrNotImplemented, "Telemetry %s is not implemented by Apulu Pipeline",  oper)
//}

// HandleRoutingConfig handles CRUDs for NetworkSecurityPolicy object
func (a *ApuluAPI) HandleRoutingConfig(oper types.Operation, rtcfg netproto.RoutingConfig) (rtcfgs []netproto.RoutingConfig, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, rtcfg.Kind, rtcfg.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var obj netproto.RoutingConfig

		err = a.retrieveObject(rtcfg.Kind, rtcfg.GetKey(), obj.Unmarshal)
		if err == nil {
			rtcfgs = append(rtcfgs, obj)
		}

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(rtcfg.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "RoutingConfig: %s | Err: %v", rtcfg.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "RoutingConfig: %s | Err: %v", rtcfg.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var obj netproto.RoutingConfig
			err := proto.Unmarshal(o, &obj)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "RoutingConfig: %s | Err: %v", obj.GetKey(), err))
				continue
			}
			rtcfgs = append(rtcfgs, obj)
		}

		return
	case types.Create:
	case types.Update:
		// Get to ensure that the object exists
		var existingRtcfg netproto.RoutingConfig
		err = a.retrieveObject(rtcfg.Kind, rtcfg.GetKey(), existingRtcfg.Unmarshal)
		if err != nil {
			return
		}

		// Check for idempotency
		if proto.Equal(&rtcfg.Spec, &existingRtcfg.Spec) {
			return nil, nil
		}
	case types.Delete:
		var existingRtcfg netproto.RoutingConfig
		err = a.retrieveObject(rtcfg.Kind, rtcfg.GetKey(), existingRtcfg.Unmarshal)
		if err != nil {
			return
		}
		rtcfg = existingRtcfg
	}
	log.Infof("RoutingConfig: %v | Op: %s | %s", rtcfg, oper, types.InfoHandleObjBegin)
	defer log.Infof("RoutingConfig: %v | Op: %s | %s", rtcfg, oper, types.InfoHandleObjEnd)

	// Perform object validations
	err = validator.ValidateRoutingConfig(a.InfraAPI, rtcfg)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	err = apulu.HandleRouteConfig(a.InfraAPI, a.RoutingClient, oper, rtcfg)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleRouteTable handles CRUDs for RouteTable object
func (a *ApuluAPI) HandleRouteTable(oper types.Operation, routetableObj netproto.RouteTable) (retRts []netproto.RouteTable, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, routetableObj.Kind, routetableObj.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var obj netproto.RouteTable

		err = a.retrieveObject(routetableObj.Kind, routetableObj.GetKey(), obj.Unmarshal)
		if err == nil {
			retRts = append(retRts, obj)
		}

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = a.InfraAPI.List(routetableObj.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "RouteTable: %s | Err: %v", routetableObj.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "RouteTable: %s | Err: %v", routetableObj.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var obj netproto.RouteTable
			err := proto.Unmarshal(o, &obj)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "RouteTable: %s | Err: %v", obj.GetKey(), err))
				continue
			}
			retRts = append(retRts, obj)
		}

		return
	case types.Create:
	case types.Update:
		// Get to ensure that the object exists
		var existingRtcfg netproto.RouteTable
		err = a.retrieveObject(routetableObj.Kind, routetableObj.GetKey(), existingRtcfg.Unmarshal)
		if err != nil {
			return
		}

		// Check for idempotency
		if proto.Equal(&routetableObj.Spec, &existingRtcfg.Spec) {
			return nil, nil
		}
	case types.Delete:
		var existingRtcfg netproto.RouteTable
		err = a.retrieveObject(routetableObj.Kind, routetableObj.GetKey(), existingRtcfg.Unmarshal)
		if err != nil {
			return
		}
		routetableObj = existingRtcfg
	}
	log.Infof("RouteTable: %v | Op: %s | %s", routetableObj, oper, types.InfoHandleObjBegin)
	defer log.Infof("RouteTable: %v | Op: %s | %s", routetableObj, oper, types.InfoHandleObjEnd)

	// Perform object validations
	err = validator.ValidateRouteTable(a.InfraAPI, routetableObj)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	err = apulu.HandleRouteTable(a.InfraAPI, a.RouteSvcClient, oper, routetableObj)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// ReplayConfigs replays last known configs from boltDB
func (a *ApuluAPI) ReplayConfigs() error {

	// Replay RoutingConfig Object
	rtCfgs, err := a.InfraAPI.List("RoutingConfig")
	if err == nil {
		for _, o := range rtCfgs {
			var rtcfg netproto.RoutingConfig
			err := rtcfg.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to RoutingConfig, Err: %v", err)
				continue
			}
			creator, ok := rtcfg.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted RoutingConfig object")
				if _, err := a.HandleRoutingConfig(types.Create, rtcfg); err != nil {
					log.Errorf("Failed to recreate RoutingConfig: %v. Err: %v", rtcfg.GetKey(), err)
				}
			}
		}
	}

	rtTbls, err := a.InfraAPI.List("RouteTable")
	if err == nil {
		for _, o := range rtTbls {
			var rtbl netproto.RouteTable
			err := rtbl.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to RouteTable, Err: %v", err)
				continue
			}
			creator, ok := rtbl.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted RouteTable object")
				if _, err := a.HandleRouteTable(types.Create, rtbl); err != nil {
					log.Errorf("Failed to recreate RouteTable: %v. Err: %v", rtbl.GetKey(), err)
				}
			}
		}
	}

	ipams, err := a.InfraAPI.List("IPAMPolicy")
	if err == nil {
		for _, o := range ipams {
			var ipam netproto.IPAMPolicy
			err := ipam.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to IPAM Policy, Err: %v", err)
				continue
			}
			creator, ok := ipam.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted IPAM Policy object")
				if _, err := a.HandleIPAMPolicy(types.Create, ipam); err != nil {
					log.Errorf("Failed to recreate IPAM Policy: %v. Err: %v", ipam.GetKey(), err)
				}
			}
		}
	}

	// Replay vrf objects
	vrfs, err := a.InfraAPI.List("Vrf")
	if err == nil {
		for _, o := range vrfs {
			var vrf netproto.Vrf
			err := vrf.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to Vrf, Err: %v", err)
				continue
			}
			creator, ok := vrf.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted Vrf object")
				if _, err := a.HandleVrf(types.Create, vrf); err != nil {
					log.Errorf("Failed to recreate Vrf: %v. Err: %v", vrf.GetKey(), err)
				}
			}
		}
	}

	subnets, err := a.InfraAPI.List("Network")
	if err == nil {
		for _, o := range subnets {
			var netw netproto.Network
			err := netw.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to Network, Err: %v", err)
				continue
			}
			creator, ok := netw.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted Network object")
				if _, err := a.HandleNetwork(types.Create, netw); err != nil {
					log.Errorf("Failed to recreate Network: %v. Err: %v", netw.GetKey(), err)
				}
			}
		}
	}

	return errors.Wrapf(types.ErrNotImplemented, "Not all config replays not implemented by pipeline")
}

// PurgeConfigs unimplemented
func (a *ApuluAPI) PurgeConfigs() error {
	return errors.Wrapf(types.ErrNotImplemented, "Config Purges not implemented by Apulu Pipeline")
}

// GetWatchOptions returns the options to be used while establishing a watch from this agent.
func (a *ApuluAPI) GetWatchOptions(ctx context.Context, kind string) (ret api.ListWatchOptions) {
	switch kind {
	case "Endpoint":
		str := fmt.Sprintf("spec.node-uuid=%s", a.InfraAPI.GetDscName())
		ret.FieldSelector = str
		// case "Interface":
		// 	ret.FieldSelector = fmt.Sprintf("status.dsc=%s", a.InfraAPI.GetDscName())
		// 	log.Infof("returning WatchOptions for Interface [%v]", ret.FieldSelector)
	}
	return ret
}

func (a *ApuluAPI) retrieveObject(kind, key string, umToFunc func([]byte) error) error {
	var dat []byte
	dat, err := a.InfraAPI.Read(kind, key)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "%v: %s | Err: %v", kind, key, types.ErrObjNotFound))
		return errors.Wrapf(types.ErrBadRequest, "%v: %s | Err: %v", kind, key, types.ErrObjNotFound)
	}
	err = umToFunc(dat)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "%v: %s | Err: %v", kind, key, err))
		return errors.Wrapf(types.ErrUnmarshal, "%v: %s | Err: %v", kind, key, err)
	}
	return nil
}

func (a *ApuluAPI) handleHostInterface(spec *halapi.LifSpec, status *halapi.LifStatus) error {
	// parse the uuid
	intfID := spec.GetId()
	uid, err := uuid.FromBytes(intfID)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse lif uuid %v, err %v", spec.GetId(), err))
		return err
	}
	// form the interface name
	ifName, err := utils.GetIfName(uid.String()[24:], status.GetIfIndex(), spec.GetType().String())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, ifindex %x, err %v",
			spec.GetId(), status.GetIfIndex(), err))
		return err
	}
	// skip any internal lifs
	if spec.GetType() != halapi.LifType_LIF_TYPE_HOST {
		log.Infof("Skipping LIF_CREATE event for lif %v, type %v", uid.String(), spec.GetType().String())
		return nil
	}
	// create host interface instance
	i := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      ifName,
			UUID:      uid.String(),
		},
		Spec: netproto.InterfaceSpec{
			Type:        netproto.InterfaceSpec_HOST_PF.String(),
			AdminStatus: netproto.IFStatus_UP.String(),
		},
		Status: netproto.InterfaceStatus{
			Name:        "",
			DSC:         a.InfraAPI.GetDscName(),
			InterfaceID: uint64(status.GetIfIndex()),
			IFHostStatus: netproto.InterfaceHostStatus{
				HostIfName: status.GetName(),
			},
			OperStatus: status.GetStatus().String(),
		},
	}
	log.Infof("Creating host interface [%+v]", i)
	a.LocalInterfaces[i.Name] = i.UUID
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: i,
	}
	a.InfraAPI.UpdateIfChannel() <- ifEvnt
	dat, _ := i.Marshal()
	if err := a.InfraAPI.Store(i.Kind, i.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Lif: %s | Lif: %v", i.GetKey(), err))
		return err
	}
	return nil
}

func (a *ApuluAPI) handleUplinkInterface(spec *halapi.PortSpec, status *halapi.PortStatus) error {
	var ifType string

	// parse the uuid
	intfID := spec.GetId()
	uid, err := uuid.FromBytes(intfID)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse port uuid %v, err %v", spec.GetId(), err))
		return err
	}
	// form the interface name
	ifName, err := utils.GetIfName(uid.String()[24:], status.GetIfIndex(), spec.GetType().String())
	if spec.GetType().String() == "PORT_TYPE_ETH" {
		ifType = netproto.InterfaceSpec_UPLINK_ETH.String()
	} else if spec.GetType().String() == "PORT_TYPE_ETH_MGMT" {
		ifType = netproto.InterfaceSpec_UPLINK_MGMT.String()
	}
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, ifindex %x, err %v",
			spec.GetId(), status.GetIfIndex(), err))
		return err
	}
	i := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      ifName,
			UUID:      uid.String(),
		},
		Spec: netproto.InterfaceSpec{
			Type:    ifType,
			VrfName: "default",
		},
		Status: netproto.InterfaceStatus{
			DSC:         a.InfraAPI.GetDscName(),
			InterfaceID: uint64(status.GetIfIndex()),
			OperStatus:  status.GetLinkStatus().GetOperState().String(),
			IFUplinkStatus: netproto.InterfaceUplinkStatus{
				PortID: spec.GetPortNumber(),
			},
		},
	}
	log.Infof("Creating Uplink interface [%+v]", i)
	a.LocalInterfaces[i.Name] = i.UUID
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: i,
	}
	a.InfraAPI.UpdateIfChannel() <- ifEvnt

	dat, _ := i.Marshal()
	if err := a.InfraAPI.Store(i.Kind, i.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Port: %s | Port: %v", i.GetKey(), err))
		return err
	}
	return nil
}

func (a *ApuluAPI) initEventStream() {

	evtReqMsg := &halapi.EventRequest{
		Request: []*halapi.EventRequest_EventSpec{
			{
				EventId: halapi.EventId_EVENT_ID_PORT_CREATE,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_PORT_UP,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_PORT_DOWN,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_LIF_CREATE,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_LIF_UPDATE,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_LIF_UP,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_LIF_DOWN,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
		},
	}

	// subscribe to event stream
	eventStream, err := a.EventClient.EventSubscribe(context.Background())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineEventListen, "Init: %v", err))
	}
	eventStream.Send(evtReqMsg)

	// get all the LIFs known at this time
	lifReqMsg := &halapi.LifGetRequest{}
	lifs, err := a.InterfaceClient.LifGet(context.Background(), lifReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineLifGet, "Init: %v", err))
	}

	// get all the physical ports
	portReqMsg := &halapi.PortGetRequest{
		Id: [][]byte{},
	}
	ports, err := a.PortClient.PortGet(context.Background(), portReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelinePortGet, "Init: %v", err))
	}

	go func(stream halapi.EventSvc_EventSubscribeClient) {
		for {
			resp, err := stream.Recv()
			if err == io.EOF {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "PDS Event stream closed"))
				break
			}
			if err != nil {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "Init: %v", err))
				break
			}

			if resp.Status != halapi.ApiStatus_API_STATUS_OK {
				log.Error(errors.Wrapf(types.ErrDatapathHandling, "Iris: %v", err))
			}

			lif := resp.GetLifEventInfo()
			port := resp.GetPortEventInfo()
			switch resp.EventId {
			case halapi.EventId_EVENT_ID_LIF_CREATE:
				fallthrough
			case halapi.EventId_EVENT_ID_LIF_UPDATE:
				fallthrough
			case halapi.EventId_EVENT_ID_LIF_UP:
				fallthrough
			case halapi.EventId_EVENT_ID_LIF_DOWN:
				err = a.handleHostInterface(lif.Spec, lif.Status)
			case halapi.EventId_EVENT_ID_PORT_CREATE:
				fallthrough
			case halapi.EventId_EVENT_ID_PORT_UP:
				fallthrough
			case halapi.EventId_EVENT_ID_PORT_DOWN:
				err = a.handleUplinkInterface(port.Spec, port.Status)
			}
		}
	}(eventStream)

	// Store initial Lifs
	for _, lif := range lifs.Response {
		a.handleHostInterface(lif.Spec, lif.Status)
	}

	// handle the ports
	for _, port := range ports.Response {
		a.handleUplinkInterface(port.Spec, port.Status)
	}
}

// HandleCPRoutingConfig handles creation of control plane route objects
func (a *ApuluAPI) HandleCPRoutingConfig(obj types.DSCStaticRoute) error {
	a.Lock()
	defer a.Unlock()

	staticRouteSpec := &msapi.CPStaticRouteSpec{
		DestAddr:    apuluutils.ConvertIPAddresses(obj.DestAddr)[0],
		PrefixLen:   obj.DestPrefixLen,
		NextHopAddr: apuluutils.ConvertIPAddresses(obj.NextHop)[0],
		State:       1,
		AdminDist:   250,
		Override:    true}

	staticRouteRequest := &msapi.CPStaticRouteRequest{
		Request: []*msapi.CPStaticRouteSpec{staticRouteSpec}}

	resp, err := a.CPRouteSvcClient.CPStaticRouteCreate(context.Background(), staticRouteRequest)
	log.Infof("CPStaticRoute Response: %v. Err: %v", resp, err)
	if resp != nil {
		if err := apuluutils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("Create failed for static route")); err != nil {
			return err
		}
	}

	return nil
}

// HandleDSCL3Interface handles configuring L3 interface on DSC interfaces
func (a *ApuluAPI) HandleDSCL3Interface(obj types.DSCInterfaceIP) error {
	iDat, err := a.InfraAPI.List("Interface")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
		return err
	}

	var uplinkInterface *netproto.Interface
	for _, o := range iDat {
		var intf netproto.Interface
		err = intf.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			continue
		}
		if intf.Spec.Type == netproto.InterfaceSpec_UPLINK_ETH.String() &&
			intf.Status.IFUplinkStatus.PortID == uint32(obj.IfID)+1 {
			uplinkInterface = &intf
			break
		}
	}

	log.Infof("Found uplink interface %v", uplinkInterface)
	if uplinkInterface != nil {
		l3uuid := uuid.NewV4().String()
		i := netproto.Interface{
			TypeMeta: api.TypeMeta{
				Kind: "Interface",
			},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      uplinkInterface.Name,
				UUID:      l3uuid,
			},
			Spec: netproto.InterfaceSpec{
				Type:        netproto.InterfaceSpec_L3.String(),
				VrfName:     "underlay-vpc",
				IPAddress:   fmt.Sprintf("%s/%d", obj.IPAddress, obj.DestPrefixLen),
				Network:     strconv.Itoa(int(obj.DestPrefixLen)),
				AdminStatus: "UP",
			},
			Status: netproto.InterfaceStatus{
				InterfaceUUID: uplinkInterface.UUID,
			},
		}

		a.LocalInterfaces[i.Name] = i.UUID

		if _, err := a.HandleInterface(types.Create, i); err != nil {
			log.Error(err)
			return err
		}
		// Configure the default route
		err := a.HandleCPRoutingConfig(types.DSCStaticRoute{DestAddr: "0.0.0.0", DestPrefixLen: 0, NextHop: obj.GatewayIP})
		if err != nil {
			return err
		}
	}
	return nil
}

// HandleCollector handles CRUD Methods for Collector Object
func (a *ApuluAPI) HandleCollector(oper types.Operation, col netproto.Collector) (cols []netproto.Collector, err error) {
	return nil, nil
}
