// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package pipeline

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/nic/agent/protos/tsproto"

	"github.com/gogo/protobuf/proto"
	protoTypes "github.com/gogo/protobuf/types"
	"github.com/pkg/errors"
	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/dscagent/common"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu"
	apuluutils "github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	apuluValidator "github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/dscagentproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	halapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	msapi "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	operdapi "github.com/pensando/sw/nic/operd/daemon/gen/operd"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/log"
)

// ApuluAPI implements PipelineAPI for Apulu pipeline
type ApuluAPI struct {
	sync.Mutex
	InfraAPI                types.InfraAPI
	ControllerAPI           types.ControllerAPI
	VPCClient               halapi.VPCSvcClient
	SubnetClient            halapi.SubnetSvcClient
	DeviceSvcClient         halapi.DeviceSvcClient
	SecurityPolicySvcClient halapi.SecurityPolicySvcClient
	DHCPRelayClient         halapi.DHCPSvcClient
	InterfaceClient         halapi.IfSvcClient
	EventClient             halapi.EventSvcClient
	PortClient              halapi.PortSvcClient
	MirrorClient            halapi.MirrorSvcClient
	RouteSvcClient          halapi.RouteSvcClient
	RoutingClient           msapi.BGPSvcClient
	EvpnClient              msapi.EvpnSvcClient
	CPRouteSvcClient        msapi.CPRouteSvcClient
	TechSupportSvcClient    operdapi.TechSupportSvcClient
	OperSvcClient           operdapi.OperSvcClient
	MetricsSvcClient        operdapi.MetricsSvcClient
	LocalInterfaces         sync.Map
}

// NewPipelineAPI returns the implemetor of PipelineAPI
func NewPipelineAPI(infraAPI types.InfraAPI) (*ApuluAPI, error) {
	conn, err := utils.CreateNewGRPCClient("PDS_GRPC_PORT", types.PDSGRPCDefaultPort)
	if err != nil {
		log.Errorf("Failed to create GRPC Connection to HAL. Err: %v", err)
		return nil, err
	}

	operdconn, err := utils.CreateNewGRPCClient("OPERD_GRPC_PORT", types.OperdGRPCDefaultPort)
	if err != nil {
		log.Errorf("Failed to create GRPC Connection to Operd. Err: %v", err)
		return nil, err
	}

	penoperconn, err := utils.CreateNewGRPCClient("PEN_OPER_GRPC_PORT", types.PenOperGRPCDefaultPort)
	if err != nil {
		log.Errorf("Failed to create GRPC Connection to pen_oper plugin. Err: %v", err)
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
		TechSupportSvcClient:    operdapi.NewTechSupportSvcClient(operdconn),
		OperSvcClient:           operdapi.NewOperSvcClient(penoperconn),
		MetricsSvcClient:        operdapi.NewMetricsSvcClient(penoperconn),
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

	// handle alerts & alert policies
	a.startAlertPoliciesWatch()

	// handle all the metrics
	handleMetrics(a)

	// Ensure that the watches for all objects are set up since Apulu doesn't have a profile that dictates which objects to be watched
	a.startDynamicWatch(types.CloudPipelineKinds)

	// Start a tech support watcher. Ideally this should be handled via nimbus aggregate watch. In such a case we could have
	// reused the above code. Tech Support has a separate controller which prevents us from doing this.
	go func() {
		for {
			if a.ControllerAPI == nil {
				// Wait till Controller API is correctly registered
				time.Sleep(time.Minute)
				continue
			}
			log.Info("Staring Tech Support Watch")
			a.ControllerAPI.WatchTechSupport() // This blocks only on stream.Recv
			time.Sleep(time.Minute)
		}
	}()

	// Attempt setting up interfaces before replaying configs
	var obj types.DistributedServiceCardStatus
	if dat, err := a.InfraAPI.Read(types.VeniceConfigKind, types.VeniceConfigKey); err == nil {
		if err := json.Unmarshal(dat, &obj); err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Err: %v", err))
		} else {
			a.InfraAPI.StoreConfig(obj)
			a.HandleDevice(types.Update)
			a.HandleDSCInterfaceInfo(obj)
		}
	}

	// Replay stored configs. This is a best-effort replay. Not marking errors as fatal since controllers will
	// eventually get the configs to a cluster-wide consistent state
	if err := a.ReplayConfigs(); err != nil {
		log.Error(err)
	}

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
		Status: netproto.InterfaceStatus{
			DSC:   a.InfraAPI.GetDscName(),
			DSCID: a.InfraAPI.GetConfig().DSCID,
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

	if _, ok := a.LocalInterfaces.Load(lb.Name); ok {
		log.Infof("loopback interface already created [%v]", lb.Name)
		return
	}
	a.LocalInterfaces.Store(lb.Name, lb.UUID)
	if _, err := a.HandleInterface(types.Create, lb); err != nil {
		log.Errorf("Init: Failed to create loopback interface: Err: %s", err)
	}
	// inject loopback so venice can be updated
	dat, _ := lb.Marshal()
	if err := a.InfraAPI.Store(lb.Kind, lb.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Uplink: %s | Uplink: %v", lb.GetKey(), err))
		return
	}
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: lb,
	}
	a.InfraAPI.UpdateIfChannel(ifEvnt)
}

// RegisterControllerAPI ensures the handles for controller API is appropriately set up
func (a *ApuluAPI) RegisterControllerAPI(controllerAPI types.ControllerAPI) {
	a.ControllerAPI = controllerAPI
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
	log.Infof("Vrf: %s | Op: %s | %s", vrf.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Vrf: %s | Op: %s | %s", vrf.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleVPC(a.InfraAPI, a.VPCClient, a.EvpnClient, a.SubnetClient, oper, vrf)
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
	log.Infof("Network: %s | Op: %s | %s", network.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Network: %s | Op: %s | %s", network.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = apulu.HandleSubnet(a.InfraAPI, a.SubnetClient, a.InterfaceClient, a.EvpnClient, oper, network, vrf.Status.VrfID, uplinkIDs)
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

		// Ignore updates on UPLINK interfaces in apulu
		if intf.Spec.Type == netproto.InterfaceSpec_UPLINK_ETH.String() || intf.Spec.Type == netproto.InterfaceSpec_UPLINK_MGMT.String() {
			return nil, nil
		}
		// Retain the existing Status
		intf.Status = existingIntf.Status
	case types.Delete:
		var existingIntf netproto.Interface

		// Allow only L3 interfaces be deleted
		if intf.Spec.Type != netproto.InterfaceSpec_L3.String() {
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

	uidStr, ok := a.LocalInterfaces.Load(intf.Name)
	if !ok {
		log.Infof("Inteface: %s not local, ignoring", intf.GetKey())
		return nil, nil
	}

	// Use the UUID from the cache when interacting with PDS Agent
	intf.UUID = uidStr.(string)
	log.Infof("Interface: %s | Op: %s | %s", intf.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Interface: %s | Op: %s | Err: %v | %s", intf.GetKey(), oper, err, types.InfoHandleObjEnd)

	// Check for Loopback Update
	oldLoopbackIf := ""
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := a.InfraAPI.GetConfig()
		oldLoopbackIf = cfg.LoopbackIP
	}

	collectorMap := make(map[uint64]int)
	err = validator.ValidateInterface(a.InfraAPI, intf, collectorMap, apulu.MirrorKeyToSessionIdMapping)
	if err != nil {
		log.Error(err)
	}

	err = apulu.HandleInterface(a.InfraAPI, a.InterfaceClient, a.SubnetClient, oper, intf, collectorMap)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	if intf.Spec.Type == netproto.InterfaceSpec_LOOPBACK.String() {
		cfg := a.InfraAPI.GetConfig()
		if cfg.LoopbackIP != oldLoopbackIf {
			log.Infof("BGP RID change detected[%v->%v], deleting and recreating BGP config and updating Device", oldLoopbackIf, cfg.LoopbackIP)
			apulu.UpdateBGPLoopbackConfig(a.InfraAPI, a.RoutingClient, oldLoopbackIf, cfg.LoopbackIP)

			if cfg.LoopbackIP != "" {
				lbip := apuluutils.ConvertIPAddress(cfg.LoopbackIP)
				apulu.HandleDevice(types.Update, a.DeviceSvcClient, lbip)
			}
			var vrf netproto.Vrf
			vrf, err = validator.ValidateVrf(a.InfraAPI, types.DefaultTenant, types.DefaultNamespace, types.DefaulUnderlaytVrf)
			if err != nil {
				log.Errorf("Get VRF failed for %s", types.DefaulUnderlaytVrf)
				return
			}

			vrfuid, errr := uuid.FromString(vrf.UUID)
			if errr != nil {
				log.Errorf("Interface: | could not parse vrf UUID %s | Err: %v", vrf.UUID, err)
				err = errr
				return
			}

			var dat [][]byte
			dat, err = a.InfraAPI.List("IPAMPolicy")
			if err != nil {
				log.Error(errors.Wrapf(types.ErrBadRequest, "IPAMPolicy: | Err: %v", types.ErrObjNotFound))
				return
			}

			for _, o := range dat {
				var policy netproto.IPAMPolicy
				err := proto.Unmarshal(o, &policy)
				if err != nil {
					log.Error(errors.Wrapf(types.ErrUnmarshal, "IPAMPolicy: %s | Err: %v", policy.GetKey(), err))
					continue
				}
				log.Infof("IPAMPolicy: %s | Op: %s | %s", policy.GetKey(), types.Update, types.InfoHandleObjBegin)

				if err = apulu.HandleIPAMPolicy(a.InfraAPI, a.DHCPRelayClient, a.SubnetClient, types.Update, policy, vrfuid.Bytes()); err != nil {
					log.Error(err)
				}
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
		// Check if no actual update for App, then return
		var existingApp netproto.App
		d, err := a.InfraAPI.Read(app.Kind, app.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "App: %s | Err: %v", app.GetKey(), types.ErrObjNotFound)
		}
		err = existingApp.Unmarshal(d)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "App: %s | Err: %v", app.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "App: %s | Err: %v", app.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&app.Spec, &existingApp.Spec) {
			//log.Infof("App: %s | Info: %s ", app.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

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
					if err := apuluValidator.ValidateNetworkSecurityPolicy(a.InfraAPI, nsp); err != nil {
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

	if err := apuluValidator.ValidateNetworkSecurityPolicy(a.InfraAPI, nsp); err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("NetworkSecurityPolicy: %s | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("NetworkSecurityPolicy: %s | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjEnd)

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
	log.Infof("SecurityProfile: %s | Op: %s | %s", profile.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("SecurityProfile: %s | Op: %s | %s", profile.GetKey(), oper, types.InfoHandleObjEnd)

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
	// Perform object validations
	serverIPToKeys := map[string]int{}
	for ip, keys := range apulu.DHCPServerIPToUUID {
		serverIPToKeys[ip] = len(keys.PolicyKeys)
	}
	vrf, err := validator.ValidateIPAMPolicy(a.InfraAPI, policy, oper, serverIPToKeys)
	if err != nil {
		log.Errorf("Get VRF failed for %s", types.DefaulUnderlaytVrf)
		return nil, errors.Wrapf(err, "Get vrf failed")
	}

	vrfuid, err := uuid.FromString(vrf.UUID)
	if err != nil {
		log.Errorf("DHCPRelay: %s | could not parse vrf UUID | Err: %v", policy.GetKey(), err)
		return nil, errors.Wrapf(err, "parse vrf UUID")
	}

	log.Infof("IPAMPolicy: %s | Op: %s | %s", policy.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("IPAMPolicy: %s | Op: %s | %s", policy.GetKey(), oper, types.InfoHandleObjEnd)

	err = apulu.HandleIPAMPolicy(a.InfraAPI, a.DHCPRelayClient, a.SubnetClient, oper, policy, vrfuid.Bytes())
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

func getMirrorSessionObj(infraObj types.InfraAPI, mirror netproto.InterfaceMirrorSession) (mirrorSessionObj netproto.InterfaceMirrorSession, err error) {
	//read from BoltDB
	var (
		mirrorRawData []byte
	)
	mirrorRawData, err = infraObj.Read(mirror.Kind, mirror.GetKey())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
		return mirrorSessionObj, errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
	}
	err = mirrorSessionObj.Unmarshal(mirrorRawData)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
		return mirrorSessionObj, errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
	}
	return mirrorSessionObj, nil
}

// HandleInterfaceMirrorSession handles CRUD Methods for InterfaceMirrorSession Object
func (a *ApuluAPI) HandleInterfaceMirrorSession(oper types.Operation, mirror netproto.InterfaceMirrorSession) (mirrors []netproto.InterfaceMirrorSession, err error) {
	a.Lock()
	defer a.Unlock()

	err = utils.ValidateMeta(oper, mirror.Kind, mirror.ObjectMeta)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	var existingMirrorSession netproto.InterfaceMirrorSession
	switch oper {
	case types.Get:
		//read from BoltDB
		existingMirrorSession, err = getMirrorSessionObj(a.InfraAPI, mirror)
		if err != nil {
			mirrors = append(mirrors, existingMirrorSession)
			return mirrors, nil
		} else {
			return nil, err
		}
	case types.List:
		//read from BoltDB
		var mirrorRawArr [][]byte
		mirrorRawArr, err = a.InfraAPI.List(mirror.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}
		for _, mirrorSessionRaw := range mirrorRawArr {
			var mirrorSessionObj netproto.InterfaceMirrorSession
			err = proto.Unmarshal(mirrorSessionRaw, &mirrorSessionObj)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
				continue
			}
			mirrors = append(mirrors, mirrorSessionObj)
		}
		return mirrors, err
	case types.Create:
	case types.Update:
		existingMirrorSession, err = getMirrorSessionObj(a.InfraAPI, mirror)
		//check if existing session and new request are same and ignore
		if err != nil {
			return nil, err
		}
		if proto.Equal(&existingMirrorSession.Spec, &mirror.Spec) {
			return nil, nil
		}
	case types.Delete:
		existingMirrorSession, err = getMirrorSessionObj(a.InfraAPI, mirror)
		if err != nil {
			mirror = existingMirrorSession
		} else {
			return nil, err
		}
	}

	log.Infof("InterfaceMirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("InterfaceMirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjEnd)

	var halMirrorSessionCount int
	for _, sessionIds := range apulu.MirrorKeyToSessionIdMapping {
		halMirrorSessionCount += len(sessionIds)
	}

	vrf, err := validator.ValidateInterfaceMirrorSession(a.InfraAPI, mirror, oper, halMirrorSessionCount)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	err = apulu.HandleInterfaceMirrorSession(a.InfraAPI, a.MirrorClient, a.InterfaceClient, a.SubnetClient, oper, mirror, vrf.Status.VrfID)
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
	log.Infof("MirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("MirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjEnd)

	// Perform object validations
	mirrorSessions := 0
	vrf, err := validator.ValidateMirrorSession(a.InfraAPI, mirror, oper, mirrorSessions)
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
	log.Infof("RoutingConfig: %s | Op: %s | %s", rtcfg.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("RoutingConfig: %s | Op: %s | %s", rtcfg.GetKey(), oper, types.InfoHandleObjEnd)

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
	log.Infof("RouteTable: %s | Op: %s | %s", routetableObj.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("RouteTable: %s | Op: %s | %s", routetableObj.GetKey(), oper, types.InfoHandleObjEnd)

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

// HandleTechSupport requests techsupport to operd
func (a *ApuluAPI) HandleTechSupport(obj tsproto.TechSupportRequest) (string, error) {
	return apulu.HandleTechSupport(a.TechSupportSvcClient, obj.Spec.SkipCores, obj.Spec.InstanceID)
}

// HandleAlerts start consuming alerts from operd plugin & export
func (a *ApuluAPI) HandleAlerts(evtsDispatcher events.Dispatcher) {
	// handle all the alerts
	apulu.HandleAlerts(evtsDispatcher, a.OperSvcClient)
}

// ReplayConfigs replays last known configs from boltDB
func (a *ApuluAPI) ReplayConfigs() error {

	// Replay RoutingConfig Object
	rtCfgKind := netproto.RoutingConfig{
		TypeMeta: api.TypeMeta{Kind: "RoutingConfig"},
	}
	rtCfgs, err := a.HandleRoutingConfig(types.List, rtCfgKind)
	if err == nil {
		for _, rtcfg := range rtCfgs {
			creator, ok := rtcfg.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", rtcfg.Kind, rtcfg.GetKey())
				a.InfraAPI.Delete(rtcfg.Kind, rtcfg.GetKey())

				log.Info("Replaying persisted RoutingConfig object")
				if _, err := a.HandleRoutingConfig(types.Create, rtcfg); err != nil {
					log.Errorf("Failed to recreate RoutingConfig: %v. Err: %v", rtcfg.GetKey(), err)
				}
			}
		}
	}

	// Replay RouteTable Object
	rtTblKind := netproto.RouteTable{
		TypeMeta: api.TypeMeta{Kind: "RouteTable"},
	}

	rtTbls, err := a.HandleRouteTable(types.List, rtTblKind)
	if err == nil {
		for _, rtbl := range rtTbls {
			creator, ok := rtbl.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", rtbl.Kind, rtbl.GetKey())
				a.InfraAPI.Delete(rtbl.Kind, rtbl.GetKey())

				log.Info("Replaying persisted RouteTable object")
				if _, err := a.HandleRouteTable(types.Create, rtbl); err != nil {
					log.Errorf("Failed to recreate RouteTable: %v. Err: %v", rtbl.GetKey(), err)
				}
			}
		}
	}

	// Replay IPAMPolicy Object
	ipamKind := netproto.IPAMPolicy{
		TypeMeta: api.TypeMeta{Kind: "IPAMPolicy"},
	}

	ipams, err := a.HandleIPAMPolicy(types.List, ipamKind)
	if err == nil {
		for _, ipam := range ipams {
			creator, ok := ipam.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", ipam.Kind, ipam.GetKey())
				a.InfraAPI.Delete(ipam.Kind, ipam.GetKey())

				log.Info("Replaying persisted IPAM Policy object")
				if _, err := a.HandleIPAMPolicy(types.Create, ipam); err != nil {
					log.Errorf("Failed to recreate IPAM Policy: %v. Err: %v", ipam.GetKey(), err)
				}
			}
		}
	}

	// Replay Vrf objects
	vrfKind := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
	}
	vrfs, err := a.HandleVrf(types.List, vrfKind)
	if err == nil {
		for _, vrf := range vrfs {
			creator, ok := vrf.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", vrf.Kind, vrf.GetKey())
				a.InfraAPI.Delete(vrf.Kind, vrf.GetKey())

				log.Info("Replaying persisted Vrf object")
				if _, err := a.HandleVrf(types.Create, vrf); err != nil {
					log.Errorf("Failed to recreate Vrf: %v. Err: %v", vrf.GetKey(), err)
				}
			}
		}
	}

	// Replay NetworkSecurityPolicy Object
	nspKind := netproto.NetworkSecurityPolicy{
		TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
	}

	nsps, err := a.HandleNetworkSecurityPolicy(types.List, nspKind)
	if err == nil {
		for _, nsp := range nsps {
			creator, ok := nsp.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", nsp.Kind, nsp.GetKey())
				a.InfraAPI.Delete(nsp.Kind, nsp.GetKey())

				log.Info("Replaying persisted Network Security Policy object")
				if _, err := a.HandleNetworkSecurityPolicy(types.Create, nsp); err != nil {
					log.Errorf("Failed to recreate Network Security Policy: %v. Err: %v", nsp.GetKey(), err)
				}
			}
		}
	}

	//Replay InterfaceMirrorSession Object
	mirrorKind := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
	}

	mirrorSessions, err := a.HandleInterfaceMirrorSession(types.List, mirrorKind)
	if err == nil {
		for _, mirrorSession := range mirrorSessions {
			creator, ok := mirrorSession.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", mirrorSession.Kind, mirrorSession.GetKey())
				a.InfraAPI.Delete(mirrorSession.Kind, mirrorSession.GetKey())

				log.Info("Replaying persisted Interface Mirror Session object")
				if _, err := a.HandleInterfaceMirrorSession(types.Create, mirrorSession); err != nil {
					log.Errorf("Failed to recreate Interface Mirror Session: %v. Err: %v", mirrorSession.GetKey(), err)
				}
			}
		}
	}

	// Replay Network Object
	netKind := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
	}
	subnets, err := a.HandleNetwork(types.List, netKind)
	if err == nil {
		for _, netw := range subnets {
			creator, ok := netw.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", netw.Kind, netw.GetKey())
				a.InfraAPI.Delete(netw.Kind, netw.GetKey())

				log.Info("Replaying persisted Network object")
				if _, err := a.HandleNetwork(types.Create, netw); err != nil {
					log.Errorf("Failed to recreate Network: %v. Err: %v", netw.GetKey(), err)
				}
			}
		}
	}

	return nil
}

// PurgeConfigs unimplemented
func (a *ApuluAPI) PurgeConfigs(deleteDB bool) error {
	// Apps, SGPolicies, Endpoints, Networks, RoutingConfig, Interface, Device
	rt := netproto.RouteTable{TypeMeta: api.TypeMeta{Kind: "RouteTable"}}
	rts, _ := a.HandleRouteTable(types.List, rt)
	for _, r := range rts {
		if _, err := a.HandleRouteTable(types.Delete, r); err != nil {
			log.Errorf("Failed to purge the RouteTable. Err: %v", err)
		}
	}

	rc := netproto.RoutingConfig{TypeMeta: api.TypeMeta{Kind: "RoutingConfig"}}
	rcs, _ := a.HandleRoutingConfig(types.List, rc)
	for _, r := range rcs {
		if _, err := a.HandleRoutingConfig(types.Delete, r); err != nil {
			log.Errorf("Failed to purge the RoutingConfig. Err: %v", err)
		}
	}

	ip := netproto.IPAMPolicy{TypeMeta: api.TypeMeta{Kind: "IPAMPolicy"}}
	ips, _ := a.HandleIPAMPolicy(types.List, ip)
	for _, i := range ips {
		if _, err := a.HandleIPAMPolicy(types.Delete, i); err != nil {
			log.Errorf("Failed to purge the IPAMPolicy. Err: %v", err)
		}
	}

	s := netproto.NetworkSecurityPolicy{TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"}}
	policies, _ := a.HandleNetworkSecurityPolicy(types.List, s)
	for _, policy := range policies {
		if _, err := a.HandleNetworkSecurityPolicy(types.Delete, policy); err != nil {
			log.Errorf("Failed to purge the NetworkSecurityPolicy. Err: %v", err)
		}
	}

	ap := netproto.App{TypeMeta: api.TypeMeta{Kind: "App"}}
	apps, _ := a.HandleApp(types.List, ap)
	for _, app := range apps {
		if _, err := a.HandleApp(types.Delete, app); err != nil {
			log.Errorf("Failed to purge the App. Err: %v", err)
		}
	}

	e := netproto.Endpoint{TypeMeta: api.TypeMeta{Kind: "Endpoint"}}
	endpoints, _ := a.HandleEndpoint(types.List, e)
	for _, endpoint := range endpoints {
		if strings.Contains(endpoint.Name, "_internal") {
			continue
		}
		if _, err := a.HandleEndpoint(types.Delete, endpoint); err != nil {
			log.Errorf("Failed to purge the Endpoint. Err: %v", err)
		}
	}

	n := netproto.Network{TypeMeta: api.TypeMeta{Kind: "Network"}}
	networks, _ := a.HandleNetwork(types.List, n)
	for _, network := range networks {
		if strings.Contains(network.Name, "_internal") {
			continue
		}
		if _, err := a.HandleNetwork(types.Delete, network); err != nil {
			log.Errorf("Failed to purge the Network. Err: %v", err)
		}
	}

	ms := netproto.MirrorSession{TypeMeta: api.TypeMeta{Kind: "MirrorSession"}}
	mirrors, _ := a.HandleMirrorSession(types.List, ms)
	for _, m := range mirrors {
		if _, err := a.HandleMirrorSession(types.Delete, m); err != nil {
			log.Errorf("Failed to purge the MirrorSession. Err: %v", err)
		}
	}

	ifs := netproto.Interface{TypeMeta: api.TypeMeta{Kind: "Interface"}}
	intfs, _ := a.HandleInterface(types.List, ifs)
	for _, i := range intfs {
		if _, err := a.HandleInterface(types.Delete, i); err != nil {
			log.Errorf("Failed to purge the Interface. Err: %v", err)
		}
	}

	sp := netproto.SecurityProfile{TypeMeta: api.TypeMeta{Kind: "SecurityProfile"}}
	profiles, _ := a.HandleSecurityProfile(types.List, sp)
	for _, profile := range profiles {
		if _, err := a.HandleSecurityProfile(types.Delete, profile); err != nil {
			log.Errorf("Failed to purge the SecurityProfile. Err: %v", err)
		}
	}

	v := netproto.Vrf{TypeMeta: api.TypeMeta{Kind: "Vrf"}}
	vrfs, _ := a.HandleVrf(types.List, v)
	for _, vrf := range vrfs {
		if _, err := a.HandleVrf(types.Delete, vrf); err != nil {
			log.Errorf("Failed to purge the Vrf. Err: %v", err)
		}
	}

	if err := a.HandleDevice(types.Delete); err != nil {
		log.Errorf("Failed to purge the Device. Err: %v", err)
	}
	return nil
}

// GetWatchOptions returns the options to be used while establishing a watch from this agent.
func (a *ApuluAPI) GetWatchOptions(ctx context.Context, kind string) (ret api.ListWatchOptions) {
	switch kind {
	case "Endpoint":
		str := fmt.Sprintf("(%s) infield (spec.node-uuid,status.node-uuid)", a.InfraAPI.GetDscName())
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

func (a *ApuluAPI) handleHostInterface(spec *halapi.InterfaceSpec, status *halapi.InterfaceStatus) error {
	// parse the uuid
	intfID := spec.GetId()
	uid, err := uuid.FromBytes(intfID)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse host if uuid %v, err %v", spec.GetId(), err))
		return err
	}
	// form the interface name
	ifName, err := utils.GetIfName(utils.ConvertMAC(uid.String()[24:]), status.GetIfIndex(), "")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, ifindex %x, err %v",
			spec.GetId(), status.GetIfIndex(), err))
		return err
	}
	// skip any non-host ifs
	if spec.GetHostIfSpec() == nil {
		log.Infof("Skipping IF_CREATE event for if %v", uid.String())
		return nil
	}
	log.Infof("Got host If update Spec:[%+v] Status [%+v]", spec, status)
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
			DSCID:       a.InfraAPI.GetConfig().DSCID,
			InterfaceID: uint64(status.GetIfIndex()),
			IFHostStatus: netproto.InterfaceHostStatus{
				MacAddress: utils.Uint64ToMac(status.GetHostIfStatus().MacAddress),
				HostIfName: status.GetHostIfStatus().GetName(),
			},
			OperStatus: status.GetHostIfStatus().GetStatus().String(),
		},
	}

	// Retain the subnet-PF attahment from boltDB
	curIntf := netproto.Interface{}
	o, err := a.InfraAPI.Read(i.Kind, i.GetKey())
	if err == nil {
		err = curIntf.Unmarshal(o)
		if err != nil {
			log.Errorf("Could not parse existing Interface object")
		} else {
			// Retain tenant and network
			i.Spec.VrfName = curIntf.Spec.VrfName
			i.Spec.Network = curIntf.Spec.Network
		}
	}

	log.Infof("Processing host interface [%+v]", i)
	a.LocalInterfaces.Store(i.Name, i.UUID)
	dat, _ := i.Marshal()
	if err := a.InfraAPI.Store(i.Kind, i.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Lif: %s | Lif: %v", i.GetKey(), err))
		return err
	}
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: i,
	}
	a.InfraAPI.UpdateIfChannel(ifEvnt)
	return nil
}

var num int = 0

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
	ifName, err := utils.GetIfName(utils.ConvertMAC(uid.String()[24:]), status.GetIfIndex(), spec.GetType().String())
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
			DSCID:       a.InfraAPI.GetConfig().DSCID,
			InterfaceID: uint64(status.GetIfIndex()),
			IPAllocType: "DHCP",
			IFUplinkStatus: netproto.InterfaceUplinkStatus{
				PortID:       spec.GetPortNumber(),
				LLDPNeighbor: &netproto.LLDPNeighbor{},
			},
		},
	}

	switch status.GetLinkStatus().GetOperState() {
	case halapi.PortOperState_PORT_OPER_STATUS_UP:
		i.Status.OperStatus = netproto.IFStatus_UP.String()
	case halapi.PortOperState_PORT_OPER_STATUS_DOWN:
		i.Status.OperStatus = netproto.IFStatus_DOWN.String()
	default:
		i.Status.OperStatus = netproto.IFStatus_UP.String()
	}

	curIntf := netproto.Interface{}
	o, err := a.InfraAPI.Read(i.Kind, i.GetKey())
	if err == nil {
		err = curIntf.Unmarshal(o)
		if err != nil {
			log.Errorf("Could not parse existing Interface object")
			return err
		} else {
			i.Status.IFUplinkStatus.IPAddress = curIntf.Status.IFUplinkStatus.IPAddress
			i.Status.IFUplinkStatus.GatewayIP = curIntf.Status.IFUplinkStatus.GatewayIP
		}
	}

	// get interface spec to update LLDP neighbor info
	intfReqMsg := &halapi.InterfaceGetRequest{
		Id: [][]byte{},
	}
	intfs, err := a.InterfaceClient.InterfaceGet(context.Background(), intfReqMsg)
	if err != nil {
		log.Error("Could not get Interface for port %v", ifName)
	} else {
		for _, u := range intfs.Response {
			if u.Spec.GetUplinkSpec() != nil {
				pid, err := uuid.FromBytes(u.Spec.GetUplinkSpec().GetPortId())
				if err != nil {
					log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse intf's port uuid %v, err %v", u.Spec.GetUplinkSpec().GetPortId(), err))
					return err
				}
				if pid.String() != uid.String() {
					continue
				}
				var pStr, chsStr, ipStr string
				lldpNbrChs := u.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfChassisStatus()
				lldpNbrPort := u.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfPortStatus()
				if "LLDPID_SUBTYPE_NONE" != lldpNbrChs.GetChassisId().GetType().String() {
					cidType := strings.Replace(lldpNbrChs.GetChassisId().GetType().String(), "LLDPID_SUBTYPE_", "", -1)
					chsStr = fmt.Sprintf("%s %s", strings.ToLower(cidType), lldpNbrChs.GetChassisId().GetValue())
				}
				if "LLDPID_SUBTYPE_NONE" != lldpNbrPort.GetPortId().GetType().String() {
					pidType := strings.Replace(lldpNbrPort.GetPortId().GetType().String(), "LLDPID_SUBTYPE_", "", -1)
					i.Status.IFUplinkStatus.LLDPNeighbor.PortID = fmt.Sprintf("%s %s", strings.ToLower(pidType), lldpNbrPort.GetPortId().GetValue())
				}
				ipStr = apuluutils.HalIPToString(lldpNbrChs.GetMgmtIP())
				if ipStr == "0.0.0.0" {
					ipStr = "" // v6 anyways return ""
				}

				i.Status.IFUplinkStatus.LLDPNeighbor.ChassisID = chsStr
				i.Status.IFUplinkStatus.LLDPNeighbor.SysName = lldpNbrChs.GetSysName()
				i.Status.IFUplinkStatus.LLDPNeighbor.SysDescription = lldpNbrChs.GetSysDescr()
				i.Status.IFUplinkStatus.LLDPNeighbor.PortID = pStr
				i.Status.IFUplinkStatus.LLDPNeighbor.PortDescription = lldpNbrPort.GetPortDescr()
				i.Status.IFUplinkStatus.LLDPNeighbor.MgmtAddress = ipStr
				break
			}
		}
	}

	log.Infof("Processing uplink interface [%+v]", i)
	a.LocalInterfaces.Store(i.Name, i.UUID)

	dat, _ := i.Marshal()
	if err := a.InfraAPI.Store(i.Kind, i.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Port: %s | Port: %v", i.GetKey(), err))
		return err
	}
	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: i,
	}
	a.InfraAPI.UpdateIfChannel(ifEvnt)
	return nil
}

func (a *ApuluAPI) handleUplinkLldpUpdate(portID []byte) error {

	uid, err := uuid.FromBytes(portID)
	if err != nil {
		log.Errorf("handleUplinkLldpUpdate: Failed to parse port uuid %v, err %v", uid.String(), err)
		return err
	}
	log.Infof("handleUplinkLldpUpdate: portID [%+v]", uid.String())

	// get physical port
	portReqMsg := &halapi.PortGetRequest{
		Id: [][]byte{portID},
	}

	ports, err := a.PortClient.PortGet(context.Background(), portReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelinePortGet, "handleUplinkLldpUpdate: %v", err))
		return err
	}

	// handle the ports
	for _, port := range ports.Response {
		a.handleUplinkInterface(port.Spec, port.Status)
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
				EventId: halapi.EventId_EVENT_ID_HOST_IF_CREATE,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_HOST_IF_UPDATE,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_HOST_IF_UP,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
			{
				EventId: halapi.EventId_EVENT_ID_HOST_IF_DOWN,
				Action:  halapi.EventOp_EVENT_OP_SUBSCRIBE,
			},
		},
	}

	// get all the host IFs known at this time
	ifReqMsg := &halapi.InterfaceGetRequest{
		Id: [][]byte{},
	}
	intfs, err := a.InterfaceClient.InterfaceGet(context.Background(), ifReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineInterfaceGet, "Init: %v", err))
	}

	// get all the physical ports
	portReqMsg := &halapi.PortGetRequest{
		Id: [][]byte{},
	}
	ports, err := a.PortClient.PortGet(context.Background(), portReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelinePortGet, "Init: %v", err))
	}

	go func() {
		// subscribe to event stream
		var stream halapi.EventSvc_EventSubscribeClient
		for {
			stream, err = a.EventClient.EventSubscribe(context.Background())
			if err != nil {
				time.Sleep(time.Second * 1)
				continue
			}
			stream.Send(evtReqMsg)
			break
		}

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

			intf := resp.GetIfEventInfo()
			port := resp.GetPortEventInfo()
			switch resp.EventId {
			case halapi.EventId_EVENT_ID_HOST_IF_CREATE:
				fallthrough
			case halapi.EventId_EVENT_ID_HOST_IF_UPDATE:
				fallthrough
			case halapi.EventId_EVENT_ID_HOST_IF_UP:
				fallthrough
			case halapi.EventId_EVENT_ID_HOST_IF_DOWN:
				if intf != nil {
					err = a.handleHostInterface(intf.Spec, intf.Status)
				}
			case halapi.EventId_EVENT_ID_PORT_CREATE:
				fallthrough
			case halapi.EventId_EVENT_ID_PORT_UP:
				fallthrough
			case halapi.EventId_EVENT_ID_PORT_DOWN:
				if port != nil {
					err = a.handleUplinkInterface(port.Spec, port.Status)
				}
			}
		}
	}()

	if intfs != nil {
		// Store initial host ifs
		for _, intf := range intfs.Response {
			a.handleHostInterface(intf.Spec, intf.Status)
		}
	}

	if ports != nil {
		// handle the ports
		for _, port := range ports.Response {
			a.handleUplinkInterface(port.Spec, port.Status)
		}
	}

	// list of uplink interfaces and store the key/lldp neighbor info
	m := make(map[string]*netproto.LLDPNeighbor)
	intReqMsg := &halapi.InterfaceGetRequest{
		Id: [][]byte{},
	}
	intfs, err = a.InterfaceClient.InterfaceGet(context.Background(), intReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineInterfaceGet, "Init: could not get interfaces %v", err))
	} else {
		for _, i := range intfs.Response {
			uid, err := uuid.FromBytes(i.Spec.GetId())
			if err != nil {
				log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse port uuid %v, err %v", i.Spec.GetId(), err))
				continue
			}

			// move this check to top
			if i.Spec.GetUplinkSpec() == nil {
				continue
			}
			lldpNbrChs := i.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfChassisStatus()
			lldpNbrPort := i.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfPortStatus()
			m[uid.String()] = &netproto.LLDPNeighbor{
				ChassisID:       lldpNbrChs.GetChassisId().String(),
				SysName:         lldpNbrChs.GetSysName(),
				SysDescription:  lldpNbrChs.GetSysDescr(),
				MgmtAddress:     apuluutils.HalIPToString(lldpNbrChs.GetMgmtIP()),
				PortID:          lldpNbrPort.GetPortId().String(),
				PortDescription: lldpNbrPort.GetPortDescr(),
			}
		}
	}

	go func() {
		// miute loop to check lldp updates on uplink interfaces
		ticker := time.Tick(time.Minute)
		for {
			select {
			case <-ticker:
				//get all uplink interfaces
				uplinkReqMsg := &halapi.InterfaceGetRequest{
					Id: [][]byte{},
				}
				intf, err := a.InterfaceClient.InterfaceGet(context.Background(), uplinkReqMsg)
				if err != nil {
					log.Error(errors.Wrapf(types.ErrPipelineInterfaceGet, "LLDPPeriodicTimer: could not get interfaces %v", err))
					continue
				}

				// loop through the response and check for LLDP neighbor update
				for _, i := range intf.Response {
					if i.Spec.GetUplinkSpec() == nil {
						continue
					}
					uid, err := uuid.FromBytes(i.Spec.GetId())
					if err != nil {
						log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse port uuid %v, err %v", i.Spec.GetId(), err))
						continue
					}

					val, ok := m[uid.String()]
					if ok == false {
						log.Errorf("LLDPPeriodicTimer: [%+v] doesnt have value", uid.String())
						continue
					}
					lChs := i.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfChassisStatus()
					lPort := i.Status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus().GetLldpIfPortStatus()

					if lChs.GetChassisId().String() != val.ChassisID ||
						lChs.GetSysName() != val.SysName ||
						lChs.GetSysDescr() != val.SysDescription ||
						apuluutils.HalIPToString(lChs.GetMgmtIP()) != val.MgmtAddress ||
						lPort.GetPortId().String() != val.PortID ||
						lPort.GetPortDescr() != val.PortDescription {

						// update lldp neighbor in the key -> value
						val.ChassisID = lChs.GetChassisId().String()
						val.SysName = lChs.GetSysName()
						val.SysDescription = lChs.GetSysDescr()
						val.PortID = lPort.GetPortId().String()
						val.PortDescription = lPort.GetPortDescr()
						val.MgmtAddress = apuluutils.HalIPToString(lChs.GetMgmtIP())

						log.Infof("LLDPPeriodicTimer: Update LLDP Neighbor of Uplink Intf [%+v]", uid.String())
						// update uplink interface's lldp neighbor info
						a.handleUplinkLldpUpdate(i.Spec.GetUplinkSpec().GetPortId())
					}
				}
			}
		}
	}()
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

// HandleDSCInterfaceInfo handles configuring DSC interfaces served from the DB
func (a *ApuluAPI) HandleDSCInterfaceInfo(obj types.DistributedServiceCardStatus) {
	// Note: If handleDSCL3Interface fails for one of the IPs in obj.DSCInterfaceIPs,
	// we log error and continue.
	if strings.Contains(strings.ToLower(obj.DSCMode), "network") {
		log.Infof("Pipeline API: handleDSCInterfaceInfo | Obj: %v", obj)
		if len(obj.DSCInterfaceIPs) != 0 {
			for _, intf := range obj.DSCInterfaceIPs {
				if err := handleDSCL3Interface(a, intf, obj); err != nil {
					log.Error(err)
				}
			}
		}
	}
	return
}

func (a *ApuluAPI) updateUplinkIntfHostname(intf *netproto.Interface, obj types.DistributedServiceCardStatus) {
	log.Infof("Pipeline API: updateUplinkIntfHostname %v (key=%v) | DSCID: %v", intf, intf.UUID, obj.DSCID)
	intfId, err := uuid.FromString(intf.UUID)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse netproto port uuid %v, err %v", intf.UUID, err))
		return
	}
	intReqMsg := &halapi.InterfaceGetRequest{
		Id: [][]byte{},
	}

	u, err := a.InterfaceClient.InterfaceGet(context.Background(), intReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineInterfaceGet, "updateUplinkIntfHostname: could not get interface %v | err %v", intf.UUID, err))
		return
	}

	for _, i := range u.Response {
		if i.Spec.GetUplinkSpec() == nil {
			continue
		}
		pid, err := uuid.FromBytes(i.Spec.GetUplinkSpec().GetPortId())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse pds port uuid %v, err %v", i.Spec.GetUplinkSpec().GetPortId(), err))
			return
		}
		if intfId.String() != pid.String() {
			continue
		}

		// Found uplink interface
		uid, err := uuid.FromBytes(i.Spec.Id)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to parse pds intf uuid %v, err %v", i.Spec.Id, err))
			return
		}

		intUpdateReq := &halapi.InterfaceRequest{
			BatchCtxt: nil,
			Request: []*halapi.InterfaceSpec{
				{
					Id:          i.Spec.Id,
					AdminStatus: i.Spec.AdminStatus,
					Ifinfo: &halapi.InterfaceSpec_UplinkSpec{
						UplinkSpec: &halapi.UplinkSpec{
							PortId: i.Spec.GetUplinkSpec().GetPortId(),
							LldpSpec: &halapi.LldpSpec{
								LldpIfChassisSpec: &halapi.LldpIfChassisInfo{
									SysName: obj.DSCID,
								},
							},
						},
					},
				},
			},
		}

		// TODO: uplink Interface updates fail for now intentionally.
		resp, err := a.InterfaceClient.InterfaceUpdate(context.Background(), intUpdateReq)
		if err != nil {
			log.Errorf("Uplink interface: %v update failed | Err: %v", uid.String(), err)
			return
		}
		if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
			log.Errorf("Uplink interface: %s update failed  | Status: %s", uid.String(), resp.ApiStatus)
			return
		}
		log.Infof("Uplink interface: %s update | Status: %s | Resp: %v", uid.String(), resp.ApiStatus, resp.Response)
		break
	}
}

// handleDSCL3Interface handles configuring L3 interface on DSC interfaces
func handleDSCL3Interface(a *ApuluAPI, obj types.DSCInterfaceIP, dscStatus types.DistributedServiceCardStatus) error {
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
	// push DSC ID (hostname) to pds-agent as Uplink interface SysName
	//a.updateUplinkIntfHostname(uplinkInterface, dscStatus)

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
				IPAddress:   obj.IPAddress,
				AdminStatus: "UP",
			},
			Status: netproto.InterfaceStatus{
				InterfaceUUID: uplinkInterface.UUID,
			},
		}

		a.LocalInterfaces.Store(i.Name, i.UUID)

		if _, err := a.HandleInterface(types.Create, i); err != nil {
			log.Error(err)
			return err
		}
		// Configure the default route
		err := a.HandleCPRoutingConfig(types.DSCStaticRoute{DestAddr: "0.0.0.0", DestPrefixLen: 0, NextHop: obj.GatewayIP})
		if err != nil {
			return err
		}

		// Update the uplink interface status
		uplinkInterface.Status.IFUplinkStatus.IPAddress = obj.IPAddress
		uplinkInterface.Status.IFUplinkStatus.GatewayIP = obj.GatewayIP
		dat, _ := uplinkInterface.Marshal()
		if err := a.InfraAPI.Store(uplinkInterface.Kind, uplinkInterface.GetKey(), dat); err != nil {
			log.Error(errors.Wrapf(types.ErrBoltDBStoreUpdate, "Port: %s | Port: %v", uplinkInterface.GetKey(), err))
			return err
		}
	}
	return nil
}

// TODO Move this into InfraAPI to avoid code duplication
func (a *ApuluAPI) startDynamicWatch(kinds []string) {
	log.Infof("Starting Dynamic Watches for kinds: %v", kinds)
	startWatcher := func() {
		ticker := time.NewTicker(time.Second * 30)
		for {
			select {
			case <-ticker.C:
				if a.ControllerAPI == nil {
					log.Info("Waiting for controller registration")
				} else {
					log.Infof("AggWatchers Start for kinds %s", kinds)
					a.ControllerAPI.Start(kinds)
					return
				}
			}
		}

	}
	go startWatcher()
}

func (a *ApuluAPI) startAlertPoliciesWatch() {
	log.Infof("Initiating alert policy watch")
	startAlertPolicyWatcher := func() {
		ticker := time.NewTicker(5 * time.Second)
		for {
			select {
			case <-ticker.C:
				if a.ControllerAPI == nil {
					log.Info("Waiting for controller registration before starting alert policy watch")
				} else {
					err := a.ControllerAPI.WatchAlertPolicies()
					if err == nil {
						log.Infof("Started alert policy watch")
						return
					}
				}
			}
		}
	}
	go startAlertPolicyWatcher()
}

// GetDSCAgentStatus returns the current agent status
func (a *ApuluAPI) GetDSCAgentStatus(status *dscagentproto.DSCAgentStatus) {
	req := &halapi.BGPPeerGetRequest{}
	respMsg, err := a.RoutingClient.BGPPeerGet(context.Background(), req)
	peerTracker := make(map[string]*cluster.PeerStatus)
	peers := make([]*cluster.PeerStatus, 0)
	if err == nil && respMsg.ApiStatus == halapi.ApiStatus_API_STATUS_OK {
		for _, peer := range respMsg.Response {
			peerStatus := &cluster.PeerStatus{
				PeerAddress: apuluutils.HalIPToString(peer.Spec.PeerAddr),
				State:       peer.Status.Status.String(),
				RemoteASN:   peer.Spec.RemoteASN,
			}
			peers = append(peers, peerStatus)
			peerTracker[apuluutils.HalIPToString(peer.Spec.PeerAddr)] = peerStatus
		}
	}

	peerAfRequest := &halapi.BGPPeerAfGetRequest{}
	resp, err := a.RoutingClient.BGPPeerAfGet(context.Background(), peerAfRequest)
	if err == nil && resp.ApiStatus == halapi.ApiStatus_API_STATUS_OK {
		for _, peerAf := range resp.Response {
			if peer, ok := peerTracker[apuluutils.HalIPToString(peerAf.Spec.PeerAddr)]; ok {
				peer.AddressFamilies = append(peer.AddressFamilies, peerAf.Spec.Afi.String())
			}
		}
	}

	status.ControlPlaneStatus = &cluster.DSCControlPlaneStatus{BGPStatus: peers}
	status.IsConnectedToVenice = a.InfraAPI.GetConfig().IsConnectedToVenice
	status.UnhealthyServices = apulu.UnhealthyServices
}
