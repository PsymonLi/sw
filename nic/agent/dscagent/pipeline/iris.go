// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build iris

package pipeline

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/nic/agent/protos/dscagentproto"
	"github.com/pensando/sw/nic/agent/protos/tsproto"

	delphi "github.com/pensando/sw/nic/delphi/gosdk"
	sysmgr "github.com/pensando/sw/nic/sysmgr/golib"

	"github.com/gogo/protobuf/proto"
	protoTypes "github.com/gogo/protobuf/types"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/nic/agent/dscagent/common"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/utils/validator"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/log"
)

var knownUplinks = map[uint64]string{}

// IrisAPI implements PipelineAPI for Iris Pipeline
type IrisAPI struct {
	sync.Mutex
	SysmgrClient    *sysmgr.Client
	InfraAPI        types.InfraAPI
	ControllerAPI   types.ControllerAPI
	VrfClient       halapi.VrfClient
	L2SegmentClient halapi.L2SegmentClient
	EpClient        halapi.EndpointClient
	IntfClient      halapi.InterfaceClient
	NspClient       halapi.NwSecurityClient
	EventClient     halapi.EventClient
	PortClient      halapi.PortClient
	TelemetryClient halapi.TelemetryClient
	SystemClient    halapi.SystemClient
}

// NewPipelineAPI returns the implementer or PipelineAPI for Iris Pipeline
func NewPipelineAPI(infraAPI types.InfraAPI) (*IrisAPI, error) {

	conn, err := utils.CreateNewGRPCClient("HAL_GRPC_PORT", types.HalGRPCDefaultPort)
	if err != nil {
		log.Errorf("Failed to create GRPC Connection to HAL. Err: %v", err)
		return nil, err
	}

	i := IrisAPI{
		InfraAPI:        infraAPI,
		VrfClient:       halapi.NewVrfClient(conn),
		L2SegmentClient: halapi.NewL2SegmentClient(conn),
		EpClient:        halapi.NewEndpointClient(conn),
		IntfClient:      halapi.NewInterfaceClient(conn),
		NspClient:       halapi.NewNwSecurityClient(conn),
		EventClient:     halapi.NewEventClient(conn),
		PortClient:      halapi.NewPortClient(conn),
		TelemetryClient: halapi.NewTelemetryClient(conn),
		SystemClient:    halapi.NewSystemClient(conn),
	}

	delphiClient, err := delphi.NewClient(&i)
	if err != nil {
		log.Fatalf("delphi NewClient failed")
	}
	i.SysmgrClient = sysmgr.NewClient(delphiClient, types.Netagent)
	go delphiClient.Run()

	if err := i.PipelineInit(); err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineInit, "Iris Init: %v", err))
	}

	return &i, nil
}

// ############################################### SysMgr Methods  ###############################################

// OnMountComplete informs sysmgr that mount is done
func (i *IrisAPI) OnMountComplete() {
	i.SysmgrClient.InitDone()
}

// Name returns netagent svc name
func (i *IrisAPI) Name() string {
	return types.Netagent
}

// ############################################### PipelineAPI Methods  ###############################################

// PipelineInit does Iris Pipeline init. Creating default Vrfs, Untagged collector network and uplinks
func (i *IrisAPI) PipelineInit() error {
	log.Infof("Iris API: %s", types.InfoPipelineInit)
	c, _ := protoTypes.TimestampProto(time.Now())
	defaultVrf := netproto.Vrf{
		TypeMeta: api.TypeMeta{Kind: "Vrf"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      "default",
			CreationTime: api.Timestamp{
				Timestamp: *c,
			},
			ModTime: api.Timestamp{
				Timestamp: *c,
			},
		},
		Spec: netproto.VrfSpec{
			VrfType: "CUSTOMER",
		},
		Status: netproto.VrfStatus{
			VrfID: 65,
		},
	}

	defaultNetwork := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      types.InternalDefaultUntaggedNetwork,
			CreationTime: api.Timestamp{
				Timestamp: *c,
			},
			ModTime: api.Timestamp{
				Timestamp: *c,
			},
		},
		Spec: netproto.NetworkSpec{
			VlanID: types.UntaggedCollVLAN, // Untagged
		},
		Status: netproto.NetworkStatus{NetworkID: types.UntaggedCollVLAN},
	}

	// Clean up stale objects from store. This will be recomputed during PipelineInit
	if err := i.InfraAPI.Delete(defaultVrf.Kind, defaultVrf.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Vrf: %s | Err: %v", defaultVrf.GetKey(), err))
	}

	if err := i.InfraAPI.Delete(defaultNetwork.Kind, defaultNetwork.GetKey()); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Network: %s | Err: %v", defaultNetwork.GetKey(), err))
	}

	if _, err := i.HandleVrf(types.Create, defaultVrf); err != nil {
		log.Error(err)
		return err
	}
	log.Infof("Iris API: %s | %s", types.InfoPipelineInit, types.InfoDefaultVrfCreate)

	if _, err := i.HandleNetwork(types.Create, defaultNetwork); err != nil {
		log.Error(err)
		return err
	}
	log.Infof("Iris API: %s | %s", types.InfoPipelineInit, types.InfoDefaultUntaggedNetworkCreate)

	// parse the uuid
	resp, err := i.SystemClient.SystemUUIDGet(context.Background(), &halapi.Empty{})
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Failed to get system uuid, err %v", err))
		return err
	}
	if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
		return fmt.Errorf("HAL returned non OK status. %v", resp.ApiStatus.String())
	}

	// Remove the colon from UUID
	uid := utils.ConvertMAC(resp.Uuid)

	i.initLifStream(uid)

	if err := i.createPortsAndUplinks(uid); err != nil {
		log.Error(err)
		return err
	}

	// Attempt setting up config
	var obj types.DistributedServiceCardStatus
	if dat, err := i.InfraAPI.Read(types.VeniceConfigKind, types.VeniceConfigKey); err == nil {
		if err := json.Unmarshal(dat, &obj); err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Err: %v", err))
		} else {
			i.InfraAPI.StoreConfig(obj)
		}
	} else {
		log.Errorf("Could not read venice config key err: %v", err)
	}

	// Start the watch for bond0 IP so that ArpClient can be updated
	log.Infof("Starting the IP watch on bond0")
	iris.IPWatch(i.InfraAPI)

	// Start the ARP receive loop. Should be singleton.
	go iris.ResolveWatch()

	// Push default profile as a part of init sequence.
	// Replay Profile Object
	profiles, err := i.InfraAPI.List("Profile")
	if err == nil {
		for _, o := range profiles {
			var profile netproto.Profile
			err := profile.Unmarshal(o)
			if err != nil {
				log.Errorf("Failed to unmarshal object to Profile. Err: %v", err)
				continue
			}
			log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", profile.Kind, profile.GetKey())
			i.InfraAPI.Delete(profile.Kind, profile.GetKey())

			creator, ok := profile.ObjectMeta.Labels["CreatedBy"]
			if ok && creator == "Venice" {
				log.Info("Replaying persisted Profile object")
				if _, err := i.HandleProfile(types.Create, profile); err != nil {
					log.Errorf("Failed to recreate Profile: %v. Err: %v", profile.GetKey(), err)
				}
			}
		}
	}

	// Replay stored configs. This is a best-effort replay. Not marking errors as fatal since controllers will
	// eventually get the configs to a cluster-wide consistent state
	if err := i.ReplayConfigs(); err != nil {
		log.Error(err)
	}

	return nil
}

// HandleVeniceCoordinates initializes the pipeline when VeniceCoordinates are discovered
func (i *IrisAPI) HandleVeniceCoordinates(dsc types.DistributedServiceCardStatus) {
}

// RegisterControllerAPI ensures the handles for controller API is appropriately set up
func (i *IrisAPI) RegisterControllerAPI(controllerAPI types.ControllerAPI) {
	log.Info("Setting the controller api")
	i.ControllerAPI = controllerAPI
}

// HandleVrf handles CRUD Methods for Vrf Object
func (i *IrisAPI) HandleVrf(oper types.Operation, vrf netproto.Vrf) (vrfs []netproto.Vrf, err error) {
	i.Lock()
	defer i.Unlock()

	err = utils.ValidateMeta(oper, vrf.Kind, vrf.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Vrf
		)
		dat, err = i.InfraAPI.Read(vrf.Kind, vrf.GetKey())
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
		dat, err = i.InfraAPI.List(vrf.Kind)
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
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if vrf.Status.VrfID == 0 {
			vrf.Status.VrfID = i.InfraAPI.AllocateID(types.VrfID, types.VrfOffSet)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingVrf netproto.Vrf
		dat, err := i.InfraAPI.Read(vrf.Kind, vrf.GetKey())
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

		// Reuse ID from store
		vrf.Status.VrfID = existingVrf.Status.VrfID
	case types.Delete:
		var existingVrf netproto.Vrf
		dat, err := i.InfraAPI.Read(vrf.Kind, vrf.GetKey())
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
	err = iris.HandleVrf(i.InfraAPI, i.VrfClient, oper, vrf)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleNetwork handles CRUD Methods for Network Object
func (i *IrisAPI) HandleNetwork(oper types.Operation, network netproto.Network) (networks []netproto.Network, err error) {
	i.Lock()
	defer i.Unlock()

	networks, err = handleNetwork(i, oper, network)
	return
}

func handleNetwork(i *IrisAPI, oper types.Operation, network netproto.Network) (networks []netproto.Network, err error) {
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
		dat, err = i.InfraAPI.Read(network.Kind, network.GetKey())
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
		dat, err = i.InfraAPI.List(network.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Network: %s | Err: %v", network.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var network netproto.Network
			err = proto.Unmarshal(o, &network)
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
			network.Status.NetworkID = i.InfraAPI.AllocateID(types.NetworkID, types.NetworkOffSet)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingNetwork netproto.Network
		dat, err := i.InfraAPI.Read(network.Kind, network.GetKey())
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
		dat, err := i.InfraAPI.Read(network.Kind, network.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingNetwork.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Network: %s | Err: %v", network.GetKey(), err)
		}
		network = existingNetwork
	}
	// Perform object validations
	uplinkIDs, vrf, err := validator.ValidateNetwork(i.InfraAPI, oper, network)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	log.Infof("Network: %s | Op: %s | %s", network.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Network: %s | Op: %s | %s", network.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleL2Segment(i.InfraAPI, i.L2SegmentClient, oper, network, vrf.Status.VrfID, uplinkIDs)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleEndpoint handles CRUD Methods for Endpoint Object
func (i *IrisAPI) HandleEndpoint(oper types.Operation, endpoint netproto.Endpoint) (endpoints []netproto.Endpoint, err error) {
	i.Lock()
	defer i.Unlock()

	endpoints, err = handleEndpoint(i, oper, endpoint)
	return
}

func handleEndpoint(i *IrisAPI, oper types.Operation, endpoint netproto.Endpoint) (endpoints []netproto.Endpoint, err error) {
	err = utils.ValidateMeta(oper, endpoint.Kind, endpoint.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Endpoint
		)
		dat, err = i.InfraAPI.Read(endpoint.Kind, endpoint.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err)
		}
		endpoints = append(endpoints, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(endpoint.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var endpoint netproto.Endpoint
			err := proto.Unmarshal(o, &endpoint)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
				continue
			}
			endpoints = append(endpoints, endpoint)
		}

		return
	case types.Create:
		if i.isLocalEP(endpoint.Spec.NodeUUID) {
			endpoint.Status.EnicID = i.InfraAPI.AllocateID(types.EnicID, types.EnicOffset)
		}

	case types.Update:
		log.Infof("Handling EP update for %v", endpoint)
		// Get to ensure that the object exists
		var existingEndpoint netproto.Endpoint
		dat, err := i.InfraAPI.Read(endpoint.Kind, endpoint.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Endpoint: %s | Err: %v", endpoint.GetKey(), types.ErrObjNotFound)
		}
		err = existingEndpoint.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&endpoint.Spec, &existingEndpoint.Spec) {
			//log.Infof("Endpoint: %s | Info: %s ", endpoint.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		if i.isLocalEP(endpoint.Spec.NodeUUID) {
			endpoint.Status.EnicID = existingEndpoint.Status.EnicID
		}

	case types.Delete:
		var existingEndpoint netproto.Endpoint
		dat, err := i.InfraAPI.Read(endpoint.Kind, endpoint.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingEndpoint.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err)
		}
		endpoint = existingEndpoint
	}

	// Perform object validations
	network, vrf, err := validator.ValidateEndpoint(i.InfraAPI, endpoint)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("Endpoint: %s | Op: %s | %s", endpoint.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Endpoint: %s | Op: %s | %s", endpoint.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleEndpoint(i.InfraAPI, i.EpClient, i.IntfClient, oper, endpoint, vrf.Status.VrfID, network.Status.NetworkID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleInterface handles CRUD Methods for Interface Object
func (i *IrisAPI) HandleInterface(oper types.Operation, intf netproto.Interface) (intfs []netproto.Interface, err error) {
	i.Lock()
	defer i.Unlock()

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
		dat, err = i.InfraAPI.Read(intf.Kind, intf.GetKey())
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
		dat, err = i.InfraAPI.List(intf.Kind)
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
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays.
		if intf.Status.InterfaceID == 0 {
			intf.Status.InterfaceID = i.InfraAPI.AllocateID(types.InterfaceID, types.UplinkOffset)
		}
		fallthrough
	case types.Update:
		// Get to ensure that the object exists
		var existingInterface netproto.Interface
		dat, err := i.InfraAPI.Read(intf.Kind, intf.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Interface: %s | Err: %v", intf.GetKey(), types.ErrObjNotFound)
		}
		err = existingInterface.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&intf.Spec, &existingInterface.Spec) {
			//log.Infof("Interface: %s | Info: %s ", intf.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse Status from store (Shallow copy)
		intf.Status = existingInterface.Status
	case types.Delete:
		var existingInterface netproto.Interface
		dat, err := i.InfraAPI.Read(intf.Kind, intf.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingInterface.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Interface: %s | Err: %v", intf.GetKey(), err)
		}
		intf = existingInterface
	}
	// Perform object validations
	collectorMap := make(map[uint64]int)
	err = validator.ValidateInterface(i.InfraAPI, intf, collectorMap, iris.MirrorDestToIDMapping)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("Interface: %s | Op: %s | %s", intf.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Interface: %s | Op: %s | %s", intf.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleInterface(i.InfraAPI, i.IntfClient, oper, intf, collectorMap)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleTunnel handles CRUD Methods for Tunnel Object
func (i *IrisAPI) HandleTunnel(oper types.Operation, tunnel netproto.Tunnel) (tunnels []netproto.Tunnel, err error) {
	i.Lock()
	defer i.Unlock()

	err = utils.ValidateMeta(oper, tunnel.Kind, tunnel.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Tunnel
		)
		dat, err = i.InfraAPI.Read(tunnel.Kind, tunnel.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err)
		}
		tunnels = append(tunnels, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(tunnel.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var tunnel netproto.Tunnel
			err := tunnel.Unmarshal(o)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err))
				continue
			}
			tunnels = append(tunnels, tunnel)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if tunnel.Status.TunnelID == 0 {
			tunnel.Status.TunnelID = i.InfraAPI.AllocateID(types.TunnelID, types.TunnelOffset)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingTunnel netproto.Tunnel
		dat, err := i.InfraAPI.Read(tunnel.Kind, tunnel.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Tunnel: %s | Err: %v", tunnel.GetKey(), types.ErrObjNotFound)
		}
		err = existingTunnel.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&tunnel.Spec, &existingTunnel.Spec) {
			//log.Infof("Tunnel: %s | Info: %s ", tunnel.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		tunnel.Status.TunnelID = existingTunnel.Status.TunnelID
	case types.Delete:
		var existingTunnel netproto.Tunnel
		dat, err := i.InfraAPI.Read(tunnel.Kind, tunnel.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingTunnel.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Tunnel: %s | Err: %v", tunnel.GetKey(), err)
		}
		tunnel = existingTunnel
	}

	// Perform object validations
	vrf, err := validator.ValidateTunnel(i.InfraAPI, tunnel)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("Tunnel: %s | Op: %s | %s", tunnel.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Tunnel: %s | Op: %s | %s", tunnel.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleTunnel(i.InfraAPI, i.IntfClient, oper, tunnel, vrf.Status.VrfID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleApp handles CRUD Methods for App Object
func (i *IrisAPI) HandleApp(oper types.Operation, app netproto.App) (apps []netproto.App, err error) {
	i.Lock()
	defer i.Unlock()

	apps, err = handleApp(i, oper, app)
	return
}

func handleApp(i *IrisAPI, oper types.Operation, app netproto.App) (apps []netproto.App, err error) {
	apps, err = common.HandleApp(i.InfraAPI, oper, app)
	if err != nil {
		return
	}
	// TODO Trigger this from NPM's OnAppUpdate method
	if oper == types.Update {
		// Check if no actual update for App, then return
		var existingApp netproto.App
		d, err := i.InfraAPI.Read(app.Kind, app.GetKey())
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
		dat, err = i.InfraAPI.List("NetworkSecurityPolicy")
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
					vrf, ruleIDToAppMapping, err := validator.ValidateNetworkSecurityPolicy(i.InfraAPI, nsp)
					if err != nil {
						break
					}
					if err := iris.HandleNetworkSecurityPolicy(i.InfraAPI, i.NspClient, types.Update, nsp, vrf.Status.VrfID, ruleIDToAppMapping); err == nil {
						break
					}
				}
			}
		}
	}

	return
}

// HandleNetworkSecurityPolicy handles CRUD Methods for NetworkSecurityPolicy Object
func (i *IrisAPI) HandleNetworkSecurityPolicy(oper types.Operation, nsp netproto.NetworkSecurityPolicy) (netSecPolicies []netproto.NetworkSecurityPolicy, err error) {
	i.Lock()
	defer i.Unlock()

	netSecPolicies, err = handleNetworkSecurityPolicy(i, oper, nsp)
	return
}

func handleNetworkSecurityPolicy(i *IrisAPI, oper types.Operation, nsp netproto.NetworkSecurityPolicy) (netSecPolicies []netproto.NetworkSecurityPolicy, err error) {
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
		dat, err = i.InfraAPI.Read(nsp.Kind, nsp.GetKey())
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
		dat, err = i.InfraAPI.List(nsp.Kind)
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
			nsp.Status.NetworkSecurityPolicyID = i.InfraAPI.AllocateID(types.NetworkSecurityPolicyID, 0)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingNetworkSecurityPolicy netproto.NetworkSecurityPolicy
		dat, err := i.InfraAPI.Read(nsp.Kind, nsp.GetKey())
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
		dat, err := i.InfraAPI.Read(nsp.Kind, nsp.GetKey())
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
	vrf, ruleIDToAppMapping, err := validator.ValidateNetworkSecurityPolicy(i.InfraAPI, nsp)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	log.Infof("NetworkSecurityPolicy: %v | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("NetworkSecurityPolicy: %v | Op: %s | %s", nsp.GetKey(), oper, types.InfoHandleObjEnd)

	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleNetworkSecurityPolicy(i.InfraAPI, i.NspClient, oper, nsp, vrf.Status.VrfID, ruleIDToAppMapping)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// ValidateSecurityProfile validates the contents of SecurityProfile objects
func (i *IrisAPI) ValidateSecurityProfile(profile netproto.SecurityProfile) (vrf netproto.Vrf, err error) {
	vrf, err = validator.ValidateVrf(i.InfraAPI, profile.Tenant, profile.Namespace, types.DefaultVrf)
	return
}

// HandleSecurityProfile handles CRUD Methods for SecurityProfile Object
func (i *IrisAPI) HandleSecurityProfile(oper types.Operation, profile netproto.SecurityProfile) (profiles []netproto.SecurityProfile, err error) {
	i.Lock()
	defer i.Unlock()

	profiles, err = handleSecurityProfile(i, oper, profile)
	return
}

func handleSecurityProfile(i *IrisAPI, oper types.Operation, profile netproto.SecurityProfile) (profiles []netproto.SecurityProfile, err error) {
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
		dat, err = i.InfraAPI.Read(profile.Kind, profile.GetKey())
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
		dat, err = i.InfraAPI.List(profile.Kind)
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
			profile.Status.SecurityProfileID = i.InfraAPI.AllocateID(types.SecurityProfileID, types.SecurityProfileOffSet)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingSecurityProfile netproto.SecurityProfile
		dat, err := i.InfraAPI.Read(profile.Kind, profile.GetKey())
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
		dat, err := i.InfraAPI.Read(profile.Kind, profile.GetKey())
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
	vrf, err := i.ValidateSecurityProfile(profile)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleSecurityProfile(i.InfraAPI, i.NspClient, i.VrfClient, oper, profile, vrf)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleInterfaceMirrorSession handles CRUD Methods for MirrorSession Object
func (i *IrisAPI) HandleInterfaceMirrorSession(oper types.Operation, mirror netproto.InterfaceMirrorSession) (mirrors []netproto.InterfaceMirrorSession, err error) {
	i.Lock()
	defer i.Unlock()

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
			obj netproto.InterfaceMirrorSession
		)
		dat, err = i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
		mirrors = append(mirrors, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(mirror.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var mirror netproto.InterfaceMirrorSession
			err := proto.Unmarshal(o, &mirror)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
				continue
			}
			mirrors = append(mirrors, mirror)
		}

		return
	case types.Create:

	case types.Update:
		// Get to ensure that the object exists
		var existingMirrorSession netproto.InterfaceMirrorSession
		dat, err := i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), types.ErrObjNotFound)
		}
		err = existingMirrorSession.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&mirror.Spec, &existingMirrorSession.Spec) {
			//log.Infof("MirrorSession: %s | Info: %s ", mirror.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

	case types.Delete:
		var existingMirrorSession netproto.InterfaceMirrorSession
		dat, err := i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingMirrorSession.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "InterfaceMirrorSession: %s | Err: %v", mirror.GetKey(), err)
		}
		mirror = existingMirrorSession
	}
	log.Infof("InterfaceMirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("InterfaceMirrorSession: %s | Op: %s | %s", mirror.GetKey(), oper, types.InfoHandleObjEnd)

	// Perform object validations
	mirrorSessions := 0
	for _, collectors := range iris.MirrorDestToIDMapping {
		mirrorSessions += len(collectors)
	}
	vrf, err := validator.ValidateInterfaceMirrorSession(i.InfraAPI, mirror, oper, mirrorSessions)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	if oper == types.Create || oper == types.Update {
		if ip := utils.GetMgmtIP(iris.MgmtLink); ip == "" {
			log.Error(errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink))
			utils.RaiseEvent(
				eventtypes.BOND0_NO_IP,
				fmt.Sprintf("IPFIX/ERSPAN features will not work unless IP address is assigned to datapath bond0 interface on DSC %s using DHCP or Static Method using Penctl", i.InfraAPI.GetDscName()),
				i.InfraAPI.GetDscName())
			return nil, errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink)
		}
	}
	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleInterfaceMirrorSession(i.InfraAPI, i.TelemetryClient, i.IntfClient, i.EpClient, oper, mirror, vrf.Status.VrfID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleMirrorSession handles CRUD Methods for MirrorSession Object
func (i *IrisAPI) HandleMirrorSession(oper types.Operation, mirror netproto.MirrorSession) (mirrors []netproto.MirrorSession, err error) {
	i.Lock()
	defer i.Unlock()

	mirrors, err = handleMirrorSession(i, oper, mirror)
	return
}

func handleMirrorSession(i *IrisAPI, oper types.Operation, mirror netproto.MirrorSession) (mirrors []netproto.MirrorSession, err error) {
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
		dat, err = i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
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
		dat, err = i.InfraAPI.List(mirror.Kind)
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

	case types.Update:
		// Get to ensure that the object exists
		var existingMirrorSession netproto.MirrorSession
		dat, err := i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
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

	case types.Delete:
		var existingMirrorSession netproto.MirrorSession
		dat, err := i.InfraAPI.Read(mirror.Kind, mirror.GetKey())
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
	for _, collectors := range iris.MirrorDestToIDMapping {
		mirrorSessions += len(collectors)
	}
	vrf, err := validator.ValidateMirrorSession(i.InfraAPI, mirror, oper, mirrorSessions)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	if oper == types.Create || oper == types.Update {
		if ip := utils.GetMgmtIP(iris.MgmtLink); ip == "" {
			log.Error(errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink))
			utils.RaiseEvent(
				eventtypes.BOND0_NO_IP,
				fmt.Sprintf("IPFIX/ERSPAN features will not work unless IP address is assigned to datapath bond0 interface on DSC %s using DHCP or Static Method using Penctl", i.InfraAPI.GetDscName()),
				i.InfraAPI.GetDscName())
			return nil, errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink)
		}
	}
	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleMirrorSession(i.InfraAPI, i.TelemetryClient, i.IntfClient, i.EpClient, oper, mirror, vrf.Status.VrfID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleFlowExportPolicy handles CRUD Methods for FlowExportPolicy Object
func (i *IrisAPI) HandleFlowExportPolicy(oper types.Operation, netflow netproto.FlowExportPolicy) (netflows []netproto.FlowExportPolicy, err error) {
	i.Lock()
	defer i.Unlock()

	netflows, err = handleFlowExportPolicy(i, oper, netflow)
	return
}

func handleFlowExportPolicy(i *IrisAPI, oper types.Operation, netflow netproto.FlowExportPolicy) (netflows []netproto.FlowExportPolicy, err error) {
	err = utils.ValidateMeta(oper, netflow.Kind, netflow.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.FlowExportPolicy
		)
		dat, err = i.InfraAPI.Read(netflow.Kind, netflow.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
		}
		netflows = append(netflows, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(netflow.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var netflow netproto.FlowExportPolicy
			err := proto.Unmarshal(o, &netflow)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
				continue
			}
			netflows = append(netflows, netflow)
		}

		return
	case types.Create:
		// Alloc ID if ID field is empty. This will be pre-populated in case of config replays
		if netflow.Status.FlowExportPolicyID == 0 {
			netflow.Status.FlowExportPolicyID = i.InfraAPI.AllocateID(types.FlowExportPolicyID, 0)
		}

	case types.Update:
		// Get to ensure that the object exists
		var existingFlowExportPolicy netproto.FlowExportPolicy
		dat, err := i.InfraAPI.Read(netflow.Kind, netflow.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), types.ErrObjNotFound)
		}
		err = existingFlowExportPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&netflow.Spec, &existingFlowExportPolicy.Spec) {
			//log.Infof("FlowExportPolicy: %s | Info: %s ", netflow.GetKey(), types.InfoIgnoreUpdate)
			return nil, nil
		}

		// Reuse ID from store
		netflow.Status.FlowExportPolicyID = existingFlowExportPolicy.Status.FlowExportPolicyID
	case types.Delete:
		var existingFlowExportPolicy netproto.FlowExportPolicy
		dat, err := i.InfraAPI.Read(netflow.Kind, netflow.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingFlowExportPolicy.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "FlowExportPolicy: %s | Err: %v", netflow.GetKey(), err)
		}
		netflow = existingFlowExportPolicy
	}
	log.Infof("FlowExportPolicy: %s | Op: %s | %s", netflow.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("FlowExportPolicy: %s | Op: %s | %s", netflow.GetKey(), oper, types.InfoHandleObjEnd)

	// Perform object validations
	collectorToKeys := map[string]int{}
	for dest, keys := range iris.CollectorToNetflow {
		collectorToKeys[dest] = len(keys.NetflowKeys)
	}
	vrf, err := validator.ValidateFlowExportPolicy(i.InfraAPI, netflow, oper, collectorToKeys)
	if err != nil {
		log.Error(err)
		return nil, err
	}
	if oper == types.Create || oper == types.Update {
		if ip := utils.GetMgmtIP(iris.MgmtLink); ip == "" {
			log.Error(errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink))
			utils.RaiseEvent(
				eventtypes.BOND0_NO_IP,
				fmt.Sprintf("IPFIX/ERSPAN features will not work unless IP address is assigned to datapath bond0 interface on DSC %s using DHCP or Static Method using Penctl", i.InfraAPI.GetDscName()),
				i.InfraAPI.GetDscName())
			return nil, errors.Wrapf(types.ErrNoIpForMgmtIntf, "Could not get ip address for intf %v", iris.MgmtLink)
		}
	}
	// Take a lock to ensure a single HAL API is active at any given point
	err = iris.HandleFlowExportPolicy(i.InfraAPI, i.TelemetryClient, i.IntfClient, i.EpClient, oper, netflow, vrf.Status.VrfID)
	if err != nil {
		log.Error(err)
		return nil, err
	}

	return
}

// HandleRoutingConfig handles CRUDs for NetworkSecurityPolicy object
func (i *IrisAPI) HandleRoutingConfig(oper types.Operation, obj netproto.RoutingConfig) ([]netproto.RoutingConfig, error) {
	return nil, errors.Wrapf(types.ErrNotImplemented, "Routing Config not implemented by Iris Pipeline")
}

// HandleProfile handles CRUD Methods for Profile Object
func (i *IrisAPI) HandleProfile(oper types.Operation, profile netproto.Profile) (profiles []netproto.Profile, err error) {
	i.Lock()
	defer i.Unlock()

	profiles, err = handleProfile(i, oper, profile)
	return
}

func handleProfile(i *IrisAPI, oper types.Operation, profile netproto.Profile) (profiles []netproto.Profile, err error) {
	err = utils.ValidateMeta(oper, profile.Kind, profile.ObjectMeta)
	if err != nil {
		log.Error(err)
		return
	}

	var existingProfile netproto.Profile
	var isCreate bool
	// Handle Get and LIST. This doesn't need any pipeline specific APIs
	switch oper {
	case types.Get:
		var (
			dat []byte
			obj netproto.Profile
		)
		dat, err = i.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}
		err = obj.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err)
		}
		profiles = append(profiles, obj)

		return
	case types.List:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(profile.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}

		for _, o := range dat {
			var profile netproto.Profile
			err := proto.Unmarshal(o, &profile)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err))
				continue
			}
			profiles = append(profiles, profile)
		}

		return
	case types.Create:
		var (
			dat [][]byte
		)
		dat, err = i.InfraAPI.List(profile.Kind)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}

		log.Infof("Profile: Found %v profiles", len(dat))
		for _, o := range dat {
			err := proto.Unmarshal(o, &existingProfile)
			if err != nil {
				log.Error(errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", existingProfile.GetKey(), err))
				continue
			}
			log.Infof("Profile: %s | Existing profile found, profile move validation will be checked", existingProfile.GetKey())
			break
		}
		if len(dat) == 0 {
			isCreate = true
		}
	case types.Update:
		// Get to ensure that the object exists
		dat, err := i.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Error(errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound))
			return nil, errors.Wrapf(types.ErrBadRequest, "Profile: %s | Err: %v", profile.GetKey(), types.ErrObjNotFound)
		}
		err = existingProfile.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err)
		}

		// Check for idempotency
		if proto.Equal(&profile.Spec, &existingProfile.Spec) {
			log.Infof("Profile: %s | Info: %s ", profile.GetKey(), types.Update)
			return nil, nil
		}

	case types.Delete:
		dat, err := i.InfraAPI.Read(profile.Kind, profile.GetKey())
		if err != nil {
			log.Infof("Controller API: %s | Err: %s", types.InfoIgnoreDelete, err)
			return nil, nil
		}
		err = existingProfile.Unmarshal(dat)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err))
			return nil, errors.Wrapf(types.ErrUnmarshal, "Profile: %s | Err: %v", profile.GetKey(), err)
		}

		// Take a lock to ensure a single HAL API is active at any given point
		if err := iris.HandleProfile(i.InfraAPI, i.SystemClient, oper, profile); err != nil {
			log.Error(err)
			return nil, err
		}

		return nil, nil

	}
	log.Infof("Profile: %s | Op: %s | %s", profile.GetKey(), oper, types.InfoHandleObjBegin)
	defer log.Infof("Profile: %s | Op: %s | %s", profile.GetKey(), oper, types.InfoHandleObjEnd)

	// Perform object validations
	if err := validator.ValidateProfile(profile); err != nil {
		log.Error(err)
		return nil, err
	}

	var kinds *[]string
	if strings.ToLower(profile.Spec.FwdMode) == strings.ToLower(netproto.ProfileSpec_INSERTION.String()) {
		kinds = &types.InsertionKinds
	} else {
		switch strings.ToLower(profile.Spec.PolicyMode) {
		case strings.ToLower(netproto.ProfileSpec_ENFORCED.String()):
			kinds = &types.EnforcedKinds
		case strings.ToLower(netproto.ProfileSpec_FLOWAWARE.String()):
			kinds = &types.FlowAwareKinds
		case strings.ToLower(netproto.ProfileSpec_BASENET.String()):
			kinds = &types.BaseNetKinds
		default:
			log.Error("Unknown Profile CREATE/UPDATE received")
			return
		}
	}

	// Profile changes to restricted profiles warrant a purge of all configs. Clean up the watches first to prevent
	// updates on objects after they are purged. Also, take a lock to ensure a single HAL API is active at any given point.
	// Check profile move only if we already have a valid profile on DSC. In case there are no profile on DSC,
	// we should create the profile with purging of watchers and objects.
	if !isCreate && utils.IsSafeProfileMove(existingProfile, profile) != true {
		i.startDynamicWatch(*kinds)
		purgeConfigs(i, false)
		if err := iris.HandleProfile(i.InfraAPI, i.SystemClient, types.Create, profile); err != nil {
			log.Error(err)
			return nil, err
		}
	} else {
		if err := iris.HandleProfile(i.InfraAPI, i.SystemClient, oper, profile); err != nil {
			log.Error(err)
			return nil, err
		}
		i.startDynamicWatch(*kinds)
	}

	return
}

// HandleIPAMPolicy handles CRUD methods for IPAMPolicy
func (i *IrisAPI) HandleIPAMPolicy(oper types.Operation, policy netproto.IPAMPolicy) (policies []netproto.IPAMPolicy, err error) {
	return nil, types.ErrNotImplemented
}

// HandleRouteTable handles CRUDs for RouteTable object
func (i *IrisAPI) HandleRouteTable(oper types.Operation, routetableObj netproto.RouteTable) ([]netproto.RouteTable, error) {
	return nil, types.ErrNotImplemented
}

// ReplayConfigs replays last known configs from boltDB
func (i *IrisAPI) ReplayConfigs() error {
	// TODO: Ideally we would love to have all these objects in a data structure that would provide some sort of relationship modeling.
	// This would help in replaying all configs and purging an object and all its dependents cleanly. This could come in handy when
	// changing profiles too (specially moving to more restrictive profiles).

	// Purge/Replay/Reconcile interfaces first. Since this gets used in subsequent configs.
	intfKind := netproto.Interface{
		TypeMeta: api.TypeMeta{Kind: "Interface"},
	}
	interfaces, _ := i.HandleInterface(types.List, intfKind)
	for _, intf := range interfaces {
		// Check for key incompat issues. Objects retured via LIST must return OK when we do a GET.
		// If this doesn't happen it means that boltDB has stale keys or key naming changes have gone in the interim.
		if _, err := i.HandleInterface(types.Get, intf); err != nil {
			// This is a stale interface object with inconsitent naming convention. Deleting this since we need to keep
			// the state consistent
			key := fmt.Sprintf("%s-%s-%s", intfKind.Kind, intf.Tenant, intf.Name)
			log.Infof("Deleting inconsistent key: %v", key)
			if err := i.InfraAPI.Delete(intfKind.Kind, key); err != nil {
				log.Error(errors.Wrapf(types.ErrInconsistentInterfaceDelete, "Interface: %s | Err: %v", key, err))
			}
			continue
		}

		if intf.Spec.Type != netproto.InterfaceSpec_UPLINK_ETH.String() &&
			intf.Spec.Type != netproto.InterfaceSpec_UPLINK_MGMT.String() {
			log.Infof("Not purging Interface %v", intf.Spec.Type)
			continue
		}

		if _, ok := knownUplinks[intf.Status.InterfaceID]; ok {
			log.Infof("Not purging known Interface %v", intf.GetKey())
			continue
		}

		log.Infof("Purging unknown uplink Interface %v", intf.GetKey())
		if err := i.InfraAPI.Delete(intf.Kind, intf.GetKey()); err != nil {
			log.Error(errors.Wrapf(types.ErrBoltDBStoreDelete, "Interface: %s | Err: %v", intf.GetKey(), err))
		}
	}

	// Replay Network Object
	nwKind := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
	}
	networks, _ := i.HandleNetwork(types.List, nwKind)
	for _, nt := range networks {
		// Do not delete default network created in PipelineInit
		if nt.ObjectMeta.Name != types.InternalDefaultUntaggedNetwork {
			log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", nt.Kind, nt.GetKey())
			i.InfraAPI.Delete(nt.Kind, nt.GetKey())
		}

		creator, ok := nt.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted Network Object: %v", nt.GetKey())
			if _, err := i.HandleNetwork(types.Create, nt); err != nil {
				log.Errorf("Failed to recreate Network: %v. Err: %v", nt.GetKey(), err)
			}
		}
	}

	// Replay Endpoint Object
	epKind := netproto.Endpoint{
		TypeMeta: api.TypeMeta{Kind: "Endpoint"},
	}
	endpoints, _ := i.HandleEndpoint(types.List, epKind)
	for _, ep := range endpoints {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", ep.Kind, ep.GetKey())
		i.InfraAPI.Delete(ep.Kind, ep.GetKey())

		creator, ok := ep.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted Network Object: %v", ep.GetKey())
			if _, err := i.HandleEndpoint(types.Create, ep); err != nil {
				log.Errorf("Failed to recreate Endpoint: %v. Err: %v", ep.GetKey(), err)
			}
		}
	}

	// Replay Tunnel Object Tunnel Replay is not needed since its not created by Venice. Uncomment this when Venice supports tunnel creations. Tunnel objects are deleted from the DB on config replay.
	tlKind := netproto.Tunnel{
		TypeMeta: api.TypeMeta{Kind: "Tunnel"},
	}
	tunnels, _ := i.HandleTunnel(types.List, tlKind)
	for _, tl := range tunnels {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", tl.Kind, tl.GetKey())
		i.InfraAPI.Delete(tl.Kind, tl.GetKey())

		//	creator, ok := ep.ObjectMeta.Labels["CreatedBy"]
		//	if ok && creator == "Venice" {
		//		log.Infof("Replaying persisted Tunnel Object: %v", tl.GetKey())
		//		if _, err := i.HandleTunnel(types.Create, tl); err != nil {
		//			log.Errorf("Failed to recreate Tunnel: %v. Err: %v", tl.GetKey(), err)
		//		}
		//	}
	}

	// Replay NetworkSecurityPolicy Object
	nspKind := netproto.NetworkSecurityPolicy{
		TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
	}
	policies, _ := i.HandleNetworkSecurityPolicy(types.List, nspKind)
	for _, policy := range policies {
		creator, ok := policy.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted NetworkSecurityPolicy Object: %v", policy.GetKey())
			if _, err := i.HandleNetworkSecurityPolicy(types.Create, policy); err != nil {
				log.Errorf("Failed to recreate NetworkSecurityPolicy: %v. Err: %v", policy.GetKey(), err)
			}
		}
	}

	// Replay SecurityProfile Object
	secProfileKind := netproto.SecurityProfile{
		TypeMeta: api.TypeMeta{Kind: "SecurityProfile"},
	}
	secProfiles, _ := i.HandleSecurityProfile(types.List, secProfileKind)
	for _, profile := range secProfiles {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", profile.Kind, profile.GetKey())
		i.InfraAPI.Delete(profile.Kind, profile.GetKey())

		creator, ok := profile.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted SecurityProfile Object: %v", profile.GetKey())
			if _, err := i.HandleSecurityProfile(types.Create, profile); err != nil {
				log.Errorf("Failed to recreate SecurityProfile: %v. Err: %v", profile.GetKey(), err)
			}
		}
	}

	// Replay InterfaceMirrorSession Object
	intfMirrorKind := netproto.InterfaceMirrorSession{
		TypeMeta: api.TypeMeta{Kind: "InterfaceMirrorSession"},
	}
	intfMirrors, _ := i.HandleInterfaceMirrorSession(types.List, intfMirrorKind)
	for _, intfMirror := range intfMirrors {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", intfMirror.Kind, intfMirror.GetKey())
		i.InfraAPI.Delete(intfMirror.Kind, intfMirror.GetKey())

		creator, ok := intfMirror.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted InterfaceMirrorSession Object: %v", intfMirror.GetKey())
			if _, err := i.HandleInterfaceMirrorSession(types.Create, intfMirror); err != nil {
				log.Errorf("Failed to recreate InterfaceMirrorSession: %v. Err: %v", intfMirror.GetKey(), err)
			}
		}
	}

	// Replay MirrorSession Object
	mrKind := netproto.MirrorSession{
		TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
	}
	mirrors, _ := i.HandleMirrorSession(types.List, mrKind)
	for _, mr := range mirrors {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", mr.Kind, mr.GetKey())
		i.InfraAPI.Delete(mr.Kind, mr.GetKey())

		creator, ok := mr.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted MirrorSession Object: %v", mr.GetKey())
			if _, err := i.HandleMirrorSession(types.Create, mr); err != nil {
				log.Errorf("Failed to recreate MirrorSession: %v. Err: %v", mr.GetKey(), err)
			}
		}
	}

	// Replay Flowexport policies
	feKind := netproto.FlowExportPolicy{
		TypeMeta: api.TypeMeta{Kind: "FlowExportPolicy"},
	}
	fepolicies, _ := i.HandleFlowExportPolicy(types.List, feKind)
	for _, fe := range fepolicies {
		log.Infof("Purging from internal DB for idempotency. Kind: %v | Key: %v", fe.Kind, fe.GetKey())
		i.InfraAPI.Delete(fe.Kind, fe.GetKey())

		creator, ok := fe.ObjectMeta.Labels["CreatedBy"]
		if ok && creator == "Venice" {
			log.Infof("Replaying persisted FlowExportPolicy Object: %v", fe.GetKey())
			if _, err := i.HandleFlowExportPolicy(types.Create, fe); err != nil {
				log.Errorf("Failed to recreate FlowExportPolicy: %v. Err: %v", fe.GetKey(), err)
			}
		}
	}

	return nil
}

// PurgeConfigs deletes all configs on Naples Decommission
func (i *IrisAPI) PurgeConfigs(deleteDB bool) error {
	i.Lock()
	defer i.Unlock()

	err := purgeConfigs(i, deleteDB)
	return err
}

func purgeConfigs(i *IrisAPI, deleteDB bool) error {
	// SGPolicies, Apps, Endpoints,  Networks, MirrorSessions, FlowExportPolicies
	log.Info("Starting Decomission workflow")

	// Populuate in memory state
	s := netproto.NetworkSecurityPolicy{TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"}}
	a := netproto.App{TypeMeta: api.TypeMeta{Kind: "App"}}
	e := netproto.Endpoint{TypeMeta: api.TypeMeta{Kind: "Endpoint"}}
	n := netproto.Network{TypeMeta: api.TypeMeta{Kind: "Network"}}
	m := netproto.MirrorSession{TypeMeta: api.TypeMeta{Kind: "MirrorSession"}}
	f := netproto.FlowExportPolicy{TypeMeta: api.TypeMeta{Kind: "FlowExportPolicy"}}
	secprof := netproto.SecurityProfile{TypeMeta: api.TypeMeta{Kind: "SecurityProfile"}}
	p := netproto.Profile{TypeMeta: api.TypeMeta{Kind: "Profile"}}

	policies, _ := handleNetworkSecurityPolicy(i, types.List, s)
	apps, _ := handleApp(i, types.List, a)
	endpoints, _ := handleEndpoint(i, types.List, e)
	networks, _ := handleNetwork(i, types.List, n)
	mirrorSessions, _ := handleMirrorSession(i, types.List, m)
	flowExportPolicies, _ := handleFlowExportPolicy(i, types.List, f)
	sp, _ := handleSecurityProfile(i, types.List, secprof)
	profiles, _ := handleProfile(i, types.List, p)

	// Clean up the DB
	if deleteDB {
		i.InfraAPI.Purge()
	}

	log.Info("Stores purged. Ensured state consistency")

	for _, policy := range policies {
		if _, err := handleNetworkSecurityPolicy(i, types.Delete, policy); err != nil {
			log.Errorf("Failed to purge the NetworkSecurityPolicy. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(policy.Kind, policy.GetKey())
		}
	}

	for _, app := range apps {
		if _, err := handleApp(i, types.Delete, app); err != nil {
			log.Errorf("Failed to purge the App. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(app.Kind, app.GetKey())
		}
	}

	for _, endpoint := range endpoints {
		if strings.Contains(endpoint.Name, "_internal") {
			continue
		}
		if _, err := handleEndpoint(i, types.Delete, endpoint); err != nil {
			log.Errorf("Failed to purge the Endpoint. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(endpoint.Kind, endpoint.GetKey())
		}
	}

	for _, network := range networks {
		if strings.Contains(network.Name, "_internal") {
			continue
		}
		if _, err := handleNetwork(i, types.Delete, network); err != nil {
			log.Errorf("Failed to purge the Network. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(network.Kind, network.GetKey())
		}
	}

	for _, secProfile := range sp {
		if _, err := handleSecurityProfile(i, types.Delete, secProfile); err != nil {
			log.Errorf("Failed to purge the secProfile. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(secProfile.Kind, secProfile.GetKey())
		}
	}

	for _, mirrorSession := range mirrorSessions {
		if _, err := handleMirrorSession(i, types.Delete, mirrorSession); err != nil {
			log.Errorf("Failed to purge the MirrorSession. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(mirrorSession.Kind, mirrorSession.GetKey())
		}
	}

	for _, flowExportPolicy := range flowExportPolicies {
		if _, err := handleFlowExportPolicy(i, types.Delete, flowExportPolicy); err != nil {
			log.Errorf("Failed to purge the FlowExportPolicy. Err: %v", err)
		}
		if deleteDB != true {
			i.InfraAPI.Delete(flowExportPolicy.Kind, flowExportPolicy.GetKey())
		}
	}

	for _, profile := range profiles {
		if _, err := handleProfile(i, types.Delete, profile); err != nil {
			log.Errorf("Failed to purge the Profiles. Err: %v", err)
		}
	}

	log.Info("Decomission workflow successful")

	return nil
}

// GetWatchOptions returns the options to be used while establishing a watch from this agent.
func (i *IrisAPI) GetWatchOptions(ctx context.Context, kind string) (ret api.ListWatchOptions) {
	switch kind {
	case "Endpoint":
		str := fmt.Sprintf("spec.node-uuid=%s", i.InfraAPI.GetDscName())
		log.Info("WatchOptions for: ", kind, " ", str)
		ret.FieldSelector = str
	}
	return ret
}

// ############################################### Helper Methods  ###############################################
func (i *IrisAPI) createHostInterface(uid string, spec *halapi.LifSpec, status *halapi.LifStatus) error {
	var ifStatus string
	var ifAdminStatus string
	// skip any internal lifs
	if spec.GetType() != halapi.LifType_LIF_TYPE_HOST {
		log.Infof("Skipping LIF_CREATE event for lif %v, type %v", uid, spec.GetType().String())
		return nil
	}

	if status.GetLifStatus().String() == "IF_STATUS_UP" {
		ifStatus = strings.ToLower(netproto.IFStatus_UP.String())
	} else {
		ifStatus = strings.ToLower(netproto.IFStatus_DOWN.String())
	}

	if spec.GetAdminStatus().String() == "IF_STATUS_UP" {
		ifAdminStatus = strings.ToLower(netproto.IFStatus_UP.String())
	} else {
		ifAdminStatus = strings.ToLower(netproto.IFStatus_DOWN.String())
	}

	lifIndex := utils.GetLifIndex(status.HwLifId)
	// form the interface name
	ifName, err := utils.GetIfName(uid, lifIndex, spec.GetType().String())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, ifindex %x, err %v",
			uid, lifIndex, err))
		return err
	}
	// create host interface instance
	l := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      ifName,
			UUID:      uid,
		},
		Spec: netproto.InterfaceSpec{
			Type:        netproto.InterfaceSpec_HOST_PF.String(),
			AdminStatus: ifAdminStatus,
		},
		Status: netproto.InterfaceStatus{
			InterfaceID: uint64(lifIndex),
			IFHostStatus: netproto.InterfaceHostStatus{
				MacAddress: utils.Uint64ToMac(spec.MacAddress),
				HostIfName: spec.GetName(),
			},
			OperStatus: ifStatus,
			DSC:        uid,
			DSCID:      i.InfraAPI.GetConfig().DSCID,
		},
	}

	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: l,
	}
	dat, _ := l.Marshal()
	if err := i.InfraAPI.Store(l.Kind, l.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Lif: %s | Lif: %v", l.GetKey(), err))
		return err
	}
	i.InfraAPI.UpdateIfChannel(ifEvnt)
	knownUplinks[l.Status.InterfaceID] = ifName
	return nil
}

func (i *IrisAPI) createUplinkInterface(uid string, spec *halapi.PortSpec, status *halapi.PortStatus) error {
	var ifType, portStatus string
	// form the interface name
	ifName, err := utils.GetIfName(uid, status.GetIfIndex(), spec.GetPortType().String())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest,
			"Failed to form interface name, uuid %v, ifindex %x, err %v",
			uid, status.GetIfIndex(), err))
		return err
	}
	if spec.GetPortType().String() == "PORT_TYPE_ETH" {
		ifType = netproto.InterfaceSpec_UPLINK_ETH.String()
	} else if spec.GetPortType().String() == "PORT_TYPE_MGMT" {
		ifType = netproto.InterfaceSpec_UPLINK_MGMT.String()
	}

	if status.GetLinkStatus().GetOperState().String() == "PORT_OPER_STATUS_UP" {
		portStatus = strings.ToLower(netproto.IFStatus_UP.String())
	} else {
		portStatus = strings.ToLower(netproto.IFStatus_DOWN.String())
	}

	uplink := netproto.Interface{
		TypeMeta: api.TypeMeta{
			Kind: "Interface",
		},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "default",
			Name:      ifName,
			UUID:      uid,
		},
		Spec: netproto.InterfaceSpec{
			Type:        ifType,
			AdminStatus: strings.ToLower(netproto.IFStatus_UP.String()),
		},
		Status: netproto.InterfaceStatus{
			InterfaceID: utils.EthIfIndexToUplinkIfIndex(uint64(status.GetIfIndex())),
			OperStatus:  portStatus,
			IFUplinkStatus: netproto.InterfaceUplinkStatus{
				PortID: spec.KeyOrHandle.GetPortId(),
			},
			DSC:   uid,
			DSCID: i.InfraAPI.GetConfig().DSCID,
		},
	}

	ifEvnt := types.UpdateIfEvent{
		Oper: types.Create,
		Intf: uplink,
	}
	dat, _ := uplink.Marshal()
	if err := i.InfraAPI.Store(uplink.Kind, uplink.GetKey(), dat); err != nil {
		log.Error(errors.Wrapf(types.ErrBoltDBStoreCreate, "Uplink: %s | Uplink: %v", uplink.GetKey(), err))
		return err
	}
	i.InfraAPI.UpdateIfChannel(ifEvnt)
	knownUplinks[uplink.Status.InterfaceID] = ifName
	return nil
}

func (i *IrisAPI) initLifStream(uid string) {

	lifReqMsg := &halapi.LifGetRequestMsg{
		Request: []*halapi.LifGetRequest{
			{},
		},
	}
	evtReqMsg := &halapi.EventRequest{
		EventId:        halapi.EventId_EVENT_ID_LIF_ADD_UPDATE,
		EventOperation: halapi.EventOp_EVENT_OP_SUBSCRIBE,
	}

	lifs, err := i.IntfClient.LifGet(context.Background(), lifReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineLifGet, "Iris Init: %v", err))
	}
	log.Infof("Iris API: %s | %s", types.InfoPipelineInit, types.InfoSingletonLifGet)

	lifStream, err := i.EventClient.EventListen(context.Background())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineEventListen, "Iris Init: %v", err))
	}

	lifStream.Send(evtReqMsg)

	go func(stream halapi.Event_EventListenClient) {
		for {
			resp, err := stream.Recv()
			if err == io.EOF {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "HAL Event stream closed"))
				break
			}
			if err != nil {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "Init: %v", err))
				break
			}
			if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
				log.Error(errors.Wrapf(types.ErrDatapathHandling, "Iris Init: %v", err))
			}

			lif := resp.GetLifEvent()

			err = i.createHostInterface(uid, lif.Spec, lif.Status)

		}

	}(lifStream)

	// Store initial Lifs
	for _, lif := range lifs.Response {
		i.createHostInterface(uid, lif.Spec, lif.Status)
	}
}

// HandleCPRoutingConfig unimplemented
func (i *IrisAPI) HandleCPRoutingConfig(obj types.DSCStaticRoute) error {
	return errors.Wrapf(types.ErrNotImplemented, "Handle CP Routing Config not implemented by Iris Pipeline")
}

// HandleTechSupport unimplemented
func (i *IrisAPI) HandleTechSupport(obj tsproto.TechSupportRequest) (string, error) {
	return "", errors.Wrapf(types.ErrNotImplemented, "Tech Support Capture not implemented by Iris Pipeline")
}

// HandleAlerts unimplemented
func (i *IrisAPI) HandleAlerts(ctx context.Context, evtsDispatcher events.Dispatcher) {
	return
}

// TODO Remove PortCreates once the linkmgr changes are stable
func (i *IrisAPI) createPortsAndUplinks(uid string) error {
	portReqMsg := &halapi.PortGetRequestMsg{
		Request: []*halapi.PortGetRequest{{}},
	}
	evtReqMsg := &halapi.EventRequest{
		EventId:        halapi.EventId_EVENT_ID_PORT_STATE,
		EventOperation: halapi.EventOp_EVENT_OP_SUBSCRIBE,
	}

	ports, err := i.PortClient.PortGet(context.Background(), portReqMsg)
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelinePortGet, "Iris Init: %v", err))
		return errors.Wrapf(types.ErrPipelinePortGet, "Iris Init: %v", err)
	}

	portStream, err := i.EventClient.EventListen(context.Background())
	if err != nil {
		log.Error(errors.Wrapf(types.ErrPipelineEventListen, "Iris Init: %v", err))
	}
	portStream.Send(evtReqMsg)

	go func(stream halapi.Event_EventListenClient) {
		for {
			resp, err := stream.Recv()
			if err == io.EOF {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "HAL Event stream closed"))
				break
			}
			if err != nil {
				log.Error(errors.Wrapf(types.ErrPipelineEventStreamClosed, "Init: %v", err))
				break
			}
			if resp.ApiStatus != halapi.ApiStatus_API_STATUS_OK {
				log.Error(errors.Wrapf(types.ErrDatapathHandling, "Iris Init: %v", err))
			}

			port := resp.GetPortEvent()

			if port != nil {
				err = i.createUplinkInterface(uid, port.Spec, port.Status)
			}

		}

	}(portStream)

	// Store initial uplinks
	for _, port := range ports.Response {
		i.createUplinkInterface(uid, port.Spec, port.Status)
	}

	return nil
}

func (i *IrisAPI) isLocalEP(nodeuuid string) bool {
	log.Infof("Node UUID: %s | Self Node UUID: %s", nodeuuid, i.InfraAPI.GetDscName())
	epNodeUUID, _ := net.ParseMAC(nodeuuid)
	selfNodeUUID, _ := net.ParseMAC(i.InfraAPI.GetDscName())
	return epNodeUUID.String() == selfNodeUUID.String()
}

// HandleDSCInterfaceInfo handles configuring DSC interfaces served from the DB
func (i *IrisAPI) HandleDSCInterfaceInfo(obj types.DistributedServiceCardStatus) {
	log.Info("Handle CP Routing Config not implemented by Iris Pipeline")
	return
}

// TODO Move this into InfraAPI to avoid code duplication
func (i *IrisAPI) startDynamicWatch(kinds []string) {
	log.Infof("Starting Dynamic Watches for kinds: %v", kinds)
	startWatcher := func() {
		start := func() bool {
			if i.ControllerAPI == nil {
				return false
			}
			log.Infof("AggWatchers Start for kinds %s", kinds)
			i.ControllerAPI.Start(kinds)
			return true
		}
		if start() {
			return
		}
		ticker := time.NewTicker(time.Second * 30)
		for {
			select {
			case <-ticker.C:
				if start() {
					return
				}
			}
		}

	}
	go startWatcher()
}

func (a *IrisAPI) StartAlertPoliciesWatch(ctx context.Context) {
	log.Info("StartAlertPoliciesWatch not implemented by Iris Pipeline")
	return
}

// GetDSCAgentStatus returns the current agent status
func (i *IrisAPI) GetDSCAgentStatus(status *dscagentproto.DSCAgentStatus) {
	log.Info("GetControlPlaneStatus not implemented by Iris Pipeline")
	return
}
