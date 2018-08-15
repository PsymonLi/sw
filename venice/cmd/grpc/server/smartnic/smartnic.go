// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package smartnic

import (
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

	"golang.org/x/net/context"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	nmd "github.com/pensando/sw/nic/agent/nmd/protos"
	nmdstate "github.com/pensando/sw/nic/agent/nmd/state"
	"github.com/pensando/sw/venice/cmd/cache"
	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/grpc"
	perror "github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/netutils"
)

const (

	// HealthWatchInterval is default health watch interval
	HealthWatchInterval = 60 * time.Second

	// DeadInterval is default dead time interval, after
	// which NIC health status is declared UNKNOWN by CMD
	DeadInterval = 120 * time.Second

	// Max retry interval in seconds for Registration retries
	// Retry interval is initially exponential and is capped
	// at 30min.
	nicRegMaxInterval = (30 * 60)

	// ValidCertSignature is a Temporary definition, until CKM provides a API to validate Cert
	ValidCertSignature = "O=Pensando Systems, Inc., OU=Pensando Manufacturing, CN=Pensando Manufacturing CA/emailAddress=mfgca@pensando.io"
)

var (
	errAPIServerDown = fmt.Errorf("API Server not reachable or down")
)

// RPCServer implements SmartNIC gRPC service.
type RPCServer struct {
	sync.Mutex

	// ClientGetter is an interface to get the API client.
	ClientGetter APIClientGetter

	// HealthWatchIntvl is the health watch interval
	HealthWatchIntvl time.Duration

	// DeadIntvl is the dead time interval
	DeadIntvl time.Duration

	// REST port of NMD agent
	RestPort string

	// Map of smartNICs in active retry, which are
	// marked unreachable due to failure to post naples
	// config to the NMD agent.
	RetryNicDB map[string]*cluster.SmartNIC

	// reference to state manager
	stateMgr *cache.Statemgr
}

// APIClientGetter is an interface that returns an API Client.
type APIClientGetter interface {
	APIClient() cluster.ClusterV1Interface
}

// NewRPCServer returns a SmartNIC RPC server object
func NewRPCServer(clientGetter APIClientGetter, healthInvl, deadInvl time.Duration, restPort string, stateMgr *cache.Statemgr) (*RPCServer, error) {
	if clientGetter == nil {
		return nil, fmt.Errorf("Client getter is nil")
	}
	return &RPCServer{
		ClientGetter:     clientGetter,
		HealthWatchIntvl: healthInvl,
		DeadIntvl:        deadInvl,
		RestPort:         restPort,
		RetryNicDB:       make(map[string]*cluster.SmartNIC),
		stateMgr:         stateMgr,
	}, nil
}

func (s *RPCServer) getNicKey(nic *cluster.SmartNIC) string {
	return fmt.Sprintf("%s|%s", nic.Tenant, nic.Name)
}

// UpdateNicInRetryDB updates NIC entry in RetryDB
func (s *RPCServer) UpdateNicInRetryDB(nic *cluster.SmartNIC) {
	s.Lock()
	defer s.Unlock()
	s.RetryNicDB[s.getNicKey(nic)] = nic
}

// NicExistsInRetryDB checks whether NIC exists in RetryDB
func (s *RPCServer) NicExistsInRetryDB(nic *cluster.SmartNIC) bool {
	s.Lock()
	defer s.Unlock()
	_, exists := s.RetryNicDB[s.getNicKey(nic)]
	return exists
}

// DeleteNicFromRetryDB deletes NIC from RetryDB
func (s *RPCServer) DeleteNicFromRetryDB(nic *cluster.SmartNIC) {
	s.Lock()
	defer s.Unlock()
	delete(s.RetryNicDB, s.getNicKey(nic))
}

// GetNicInRetryDB returns NIC object with match key
func (s *RPCServer) GetNicInRetryDB(key string) *cluster.SmartNIC {
	s.Lock()
	defer s.Unlock()
	nicObj, ok := s.RetryNicDB[key]
	if ok {
		return nicObj
	}
	return nil
}

// IsValidFactoryCert inspects the certificate for validity and returns boolean
func (s *RPCServer) IsValidFactoryCert(cert []byte) bool {

	// TODO: For now, it just looks for a pensando string signature
	// Will use CKM APIs once it is available for checking validity
	str := string(cert)
	if strings.Contains(str, ValidCertSignature) {
		return true
	}

	return false
}

// GetCluster fetches the Cluster object based on object meta
func (s *RPCServer) GetCluster() (*cluster.Cluster, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}
	opts := api.ListWatchOptions{}
	clusterObjs, err := cl.Cluster().List(context.Background(), &opts)
	if err != nil || len(clusterObjs) == 0 {
		return nil, perror.NewNotFound("Cluster", "")
	}

	// There should be only one Cluster object
	return clusterObjs[0], nil
}

// CreateHost creates the Host object based on object meta
func (s *RPCServer) CreateHost(ometa api.ObjectMeta) (*cluster.Host, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	host := &cluster.Host{}
	host.Defaults("v1")
	host.TypeMeta = api.TypeMeta{Kind: "Host"}
	host.ObjectMeta = ometa

	hostObj, err := cl.Host().Create(context.Background(), host)
	if err != nil || hostObj == nil {
		log.Errorf("Host create failed, host: %+v error: %+v", host, err)
		return nil, perror.NewNotFound("Host", ometa.Name)
	}

	return hostObj, nil
}

// GetHost fetches the Host object based on object meta
func (s *RPCServer) GetHost(om api.ObjectMeta) (*cluster.Host, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	hostObj, err := cl.Host().Get(context.Background(), &om)
	if err != nil || hostObj == nil {
		return nil, perror.NewNotFound("Host", om.Name)
	}

	return hostObj, nil
}

// UpdateHost updates the list of Nics in spec of Host object
// Host object is created if it does not exist.
func (s *RPCServer) UpdateHost(nic *cluster.SmartNIC) (*cluster.Host, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	// Check if object exists
	var hostObj *cluster.Host
	ometa := api.ObjectMeta{
		Name: nic.Spec.HostName,
	}
	refObj, err := s.GetHost(ometa)
	if err != nil || refObj == nil {

		// Create Host object
		host := &cluster.Host{}
		host.Defaults("v1")
		host.Kind = "Host"
		host.Name = nic.Spec.HostName
		host.Spec.Interfaces = map[string]cluster.HostIntfSpec{
			nic.Name: cluster.HostIntfSpec{
				MacAddrs: []string{nic.Name},
			},
		}

		hostObj, err = cl.Host().Create(context.Background(), host)
	} else {

		// Update the Nics list in Host Spec
		if refObj.Spec.Interfaces == nil {
			refObj.Spec.Interfaces = make(map[string]cluster.HostIntfSpec)
		}
		refObj.Spec.Interfaces[nic.Name] = cluster.HostIntfSpec{
			MacAddrs: []string{nic.Name},
		}
		log.Debugf("Updating Host: %+v", refObj)
		hostObj, err = cl.Host().Update(context.Background(), refObj)
	}

	return hostObj, err
}

// DeleteHost deletes the Host object based on object meta name
func (s *RPCServer) DeleteHost(om api.ObjectMeta) error {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return errAPIServerDown
	}

	_, err := cl.Host().Delete(context.Background(), &om)
	if err != nil {
		log.Errorf("Error deleting Host object name:%s err: %v", om.Name, err)
		return err
	}

	return nil
}

// GetSmartNIC fetches the SmartNIC object based on object meta
func (s *RPCServer) GetSmartNIC(om api.ObjectMeta) (*cluster.SmartNIC, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	nicObj, err := cl.SmartNIC().Get(context.Background(), &om)
	if err != nil || nicObj == nil {
		return nil, perror.NewNotFound("SmartNIC", om.Name)
	}

	return nicObj, nil
}

// UpdateSmartNIC creates or updates the smartNIC object
func (s *RPCServer) UpdateSmartNIC(obj *cluster.SmartNIC) (*cluster.SmartNIC, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	// Check if object exists
	var nicObj *cluster.SmartNIC
	refObj, err := s.GetSmartNIC(obj.ObjectMeta)
	if err != nil || refObj == nil {

		// Create smartNIC object
		nicObj, err = cl.SmartNIC().Create(context.Background(), obj)
	} else {

		// Update the object with CAS semantics, the phase and status conditions
		if obj.Spec.Phase != "" {
			refObj.Spec.Phase = obj.Spec.Phase
		}
		refObj.Status.Conditions = obj.Status.Conditions
		nicObj, err = cl.SmartNIC().Update(context.Background(), refObj)
	}

	log.Debugf("UpdateSmartNIC nic: %+v", nicObj)
	return nicObj, err
}

// DeleteSmartNIC deletes the SmartNIC object based on object meta name
func (s *RPCServer) DeleteSmartNIC(om api.ObjectMeta) error {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return errAPIServerDown
	}

	_, err := cl.SmartNIC().Delete(context.Background(), &om)
	if err != nil {
		log.Errorf("Error deleting smartNIC object name:%s err: %v", om.Name, err)
		return err
	}

	return nil
}

// RegisterNIC handles the register NIC request and upon validation creates SmartNIC object
// NMD starts the NIC registration process when the NIC is placed in managed mode.
// NMD is expected to retry with backoff if there are API errors or if the NIC is rejected.
func (s *RPCServer) RegisterNIC(ctx context.Context, req *grpc.RegisterNICRequest) (*grpc.RegisterNICResponse, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	mac := req.GetNic().Name
	cert := req.GetCert()

	log.Infof("Received NIC registration request for MAC:%s", mac)

	// Validate the factory cert obtained in the request
	if s.IsValidFactoryCert(cert) == false {

		// Reject NIC if the certificate is not valid
		// TODO: Add exact reason to the response once CKM api is used
		log.Errorf("Invalid certificate, NIC rejected mac: %v", mac)
		return &grpc.RegisterNICResponse{Phase: cluster.SmartNICSpec_REJECTED.String(), Reason: string("Invalid Cert")}, nil
	}

	// Get the Cluster object
	clusterObj, err := s.GetCluster()
	if err != nil {
		log.Errorf("Error getting Cluster object, err: %v", err)
		return &grpc.RegisterNICResponse{Phase: cluster.SmartNICSpec_UNKNOWN.String()}, err
	}

	// Process the request based on the configured admission mode
	var phase string
	if clusterObj.Spec.AutoAdmitNICs == true {

		// Admit NIC if autoAdmission is enabled
		phase = cluster.SmartNICSpec_ADMITTED.String()
	} else {

		// Set the NIC to pendingState for Manual Approval
		phase = cluster.SmartNICSpec_PENDING.String()
	}

	log.Infof("Validated NIC: %s, phase: %s", mac, phase)

	// Create SmartNIC object, if the status is either admitted or pending.
	if phase == cluster.SmartNICSpec_ADMITTED.String() || phase == cluster.SmartNICSpec_PENDING.String() {
		nic := req.GetNic()
		nic.Spec.Phase = phase
		nic.ObjectMeta.SelfLink = nic.MakeKey("cluster")

		_, err = s.UpdateSmartNIC(&nic)
		if err != nil {
			log.Errorf("Error updating smartNIC object: %+v err: %v", nic, err)
			return &grpc.RegisterNICResponse{Phase: phase}, err
		}

		// Create or Update the Host
		_, err = s.UpdateHost(&nic)
		if err != nil {
			log.Errorf("Error creating or updating Host object, host: %s mac: %s err: %v",
				nic.Spec.HostName, nic.ObjectMeta.Name, err)
		}
	}

	return &grpc.RegisterNICResponse{Phase: phase}, err
}

// UpdateNIC handles the update to smartNIC object
func (s *RPCServer) UpdateNIC(ctx context.Context, req *grpc.UpdateNICRequest) (*grpc.UpdateNICResponse, error) {
	cl := s.ClientGetter.APIClient()
	if cl == nil {
		return nil, errAPIServerDown
	}

	// Update smartNIC object with CAS semantics
	obj := req.GetNic()
	nicObj, err := s.UpdateSmartNIC(&obj)

	if err != nil || nicObj == nil {
		log.Errorf("Error updating SmartNIC object: %+v err: %v", obj, err)
		return &grpc.UpdateNICResponse{Nic: nil}, err
	}

	return &grpc.UpdateNICResponse{Nic: nicObj}, nil
}

//ListSmartNICs lists all smartNICs matching object selector
func (s *RPCServer) ListSmartNICs(ctx context.Context, sel *api.ObjectMeta) ([]cluster.SmartNIC, error) {
	var niclist []cluster.SmartNIC
	// get all smartnics
	nics, err := s.stateMgr.ListSmartNICs()
	if err != nil {
		return nil, err
	}

	// walk all smartnics and add it to the list
	for _, nic := range nics {
		niclist = append(niclist, nic.SmartNIC)
	}

	return niclist, nil
}

// WatchNICs watches smartNICs objects for changes and sends them as streaming rpc
func (s *RPCServer) WatchNICs(sel *api.ObjectMeta, stream grpc.SmartNICUpdates_WatchNICsServer) error {
	// watch for changes
	watchChan := make(chan memdb.Event, memdb.WatchLen)
	defer close(watchChan)
	s.stateMgr.WatchObjects("SmartNIC", watchChan)
	defer s.stateMgr.StopWatchObjects("SmartNIC", watchChan)

	// first get a list of all smartnics
	nics, err := s.ListSmartNICs(context.Background(), sel)
	if err != nil {
		log.Errorf("Error getting a list of smartnics. Err: %v", err)
		return err
	}

	ctx := stream.Context()

	// send the objects out as a stream
	for _, nic := range nics {
		watchEvt := grpc.SmartNICEvent{
			EventType: api.EventType_CreateEvent,
			Nic:       nic,
		}
		err = stream.Send(&watchEvt)
		if err != nil {
			log.Errorf("Error sending stream. Err: %v", err)
			return err
		}
	}

	// loop forever on watch channel
	for {
		select {
		// read from channel
		case evt, ok := <-watchChan:
			if !ok {
				log.Errorf("Error reading from channel. Closing watch")
				return errors.New("Error reading from channel")
			}

			// get event type from memdb event
			var etype api.EventType
			switch evt.EventType {
			case memdb.CreateEvent:
				etype = api.EventType_CreateEvent
			case memdb.UpdateEvent:
				etype = api.EventType_UpdateEvent
			case memdb.DeleteEvent:
				etype = api.EventType_DeleteEvent
			}

			// convert to smartnic object
			nic, err := cache.SmartNICStateFromObj(evt.Obj)
			if err != nil {
				return err
			}

			// construct the smartnic event object
			watchEvt := grpc.SmartNICEvent{
				EventType: etype,
				Nic:       nic.SmartNIC,
			}
			err = stream.Send(&watchEvt)
			if err != nil {
				log.Errorf("Error sending stream. Err: %v closing watch", err)
				return err
			}
		case <-ctx.Done():
			return ctx.Err()
		}
	}
}

// MonitorHealth periodically inspects that health status of
// smartNIC objects every 30sec. For NICs that haven't received
// health updates in over 120secs, CMD would mark the status as unknown.
func (s *RPCServer) MonitorHealth() {

	log.Info("Launching Monitor Health")
	for {
		select {

		// NIC health watch timer callback
		case <-time.After(s.HealthWatchIntvl):

			cl := s.ClientGetter.APIClient()
			if cl == nil {
				log.Errorf("Failed to get API client")
				continue
			}

			// Get a list of all existing smartNICs
			opts := api.ListWatchOptions{}
			nics, err := cl.SmartNIC().List(context.Background(), &opts)
			if err != nil {
				log.Errorf("Failed to getting a list of nics, err: %v", err)
				continue
			}

			log.Infof("Health watch timer callback, #nics: %d ", len(nics))

			// Iterate on smartNIC objects
			for _, nic := range nics {

				for i := 0; i < len(nic.Status.Conditions); i++ {

					condition := nic.Status.Conditions[i]

					// Inspect HEALTH condition with status that is marked healthy or unhealthy (i.e not unknown)
					if condition.Type == cluster.SmartNICCondition_HEALTHY.String() && condition.Status != cluster.ConditionStatus_UNKNOWN.String() {

						// parse the last reported time
						t, err := time.Parse(time.RFC3339, condition.LastTransitionTime)
						if err != nil {
							log.Errorf("Failed parsing last transition time for NIC health, nic: %+v, err: %v", nic, err)
							break
						}

						// if the time elapsed since last health update is over
						// the deadInterval, update the Health status to unknown
						if err == nil && time.Since(t) > s.DeadIntvl {

							// update the nic health status to unknown
							log.Infof("Updating NIC health to unknown, nic: %s DeadIntvl:%d", nic.Name, s.DeadIntvl)
							lastUpdateTime := nic.Status.Conditions[i].LastTransitionTime
							nic.Status.Conditions[i].Status = cluster.ConditionStatus_UNKNOWN.String()
							nic.Status.Conditions[i].LastTransitionTime = time.Now().Format(time.RFC3339)
							nic.Status.Conditions[i].Reason = fmt.Sprintf("NIC health update not received since %s", lastUpdateTime)
							_, err := cl.SmartNIC().Update(context.Background(), nic)
							if err != nil {
								log.Errorf("Failed updating the NIC health status to unknown, nic: %s err: %s", nic.Name, err)
							}
						}

						break
					}

				}
			}
		}
	}
}

// InitiateNICRegistration does the naples config POST for managed mode
// to the NMD REST endpoint using the configured Mgmt-IP. Failures will
// be retried for maxIters and after that NIC status will be updated to
// UNREACHABLE.
// Further retries for UNREACHABLE nics will be handled by
// NIC health watcher which runs periodically.
func (s *RPCServer) InitiateNICRegistration(nic *cluster.SmartNIC) {

	var retryInterval time.Duration
	retryInterval = 1

	var err error
	var resp nmdstate.NaplesConfigResp

	// check if Nic exists in RetryDB
	if s.NicExistsInRetryDB(nic) == true {
		s.UpdateNicInRetryDB(nic)
		log.Debugf("Nic registration retry is ongoing, nic: %s", nic.Name)
		return
	}

	// Add Nic to the RetryDB
	s.UpdateNicInRetryDB(nic)
	log.Infof("Initiating nic registration for Naples, MAC: %s IP:%+v", nic.Name, nic.Spec.MgmtIp)

	for {
		select {
		case <-time.After(retryInterval * time.Second):

			if s.NicExistsInRetryDB(nic) == false {
				// If NIC is deleted stop the retry
				return
			}

			nicObj := s.GetNicInRetryDB(s.getNicKey(nic))

			// Config to switch to Managed mode
			naplesCfg := nmd.Naples{
				ObjectMeta: api.ObjectMeta{Name: "NaplesConfig"},
				TypeMeta:   api.TypeMeta{Kind: "Naples"},
				Spec: nmd.NaplesSpec{
					Mode:           nmd.NaplesMode_MANAGED_MODE,
					PrimaryMac:     nicObj.Name,
					ClusterAddress: []string{env.UnauthRPCServer.GetListenURL()},
					HostName:       nicObj.Spec.HostName,
					MgmtIp:         nicObj.Spec.MgmtIp,
				},
			}

			nmdURL := fmt.Sprintf("http://%s:%s%s", nicObj.Spec.MgmtIp, s.RestPort, nmdstate.ConfigURL)
			log.Infof("Posting Naples config: %+v to Naples-Ip: %s", naplesCfg, nmdURL)

			err = netutils.HTTPPost(nmdURL, &naplesCfg, &resp)
			if err == nil {
				log.Infof("Nic registration post request to Naples node successful, nic:%s", nic.Name)
				s.DeleteNicFromRetryDB(nic)
				return
			}

			// Update NIC status condition.
			// Naples may be unreachable if the configured Mgmt-IP is either invalid
			// or if it is valid but part of another Venice cluster and in that case
			// the REST port would have been shutdown (hence unreachable) after it is
			// admitted in managed mode.
			log.Errorf("Retrying, failed to post naples config, nic: %s err: %+v resp: %+v", nic.Name, err, resp)
			nic := cluster.SmartNIC{
				TypeMeta:   nicObj.TypeMeta,
				ObjectMeta: nicObj.ObjectMeta,
				Status: cluster.SmartNICStatus{
					Conditions: []*cluster.SmartNICCondition{
						{
							Type:               cluster.SmartNICCondition_UNREACHABLE.String(),
							Status:             cluster.ConditionStatus_TRUE.String(),
							LastTransitionTime: time.Now().Format(time.RFC3339),
							Reason:             fmt.Sprintf("Failed to post naples config after several attempts, response: %+v", resp),
							Message:            fmt.Sprintf("Naples REST endpoint: %s:%s is not reachable", nic.Spec.MgmtIp, s.RestPort),
						},
					},
				},
			}
			_, err = s.UpdateSmartNIC(&nic)
			if err != nil {
				log.Errorf("Error updating the NIC status as unreachable nic:%s err:%v", nicObj.Name, err)
			}

			// Retry with backoff, capped at nicRegMaxInterval
			if 2*retryInterval <= nicRegMaxInterval {
				retryInterval = 2 * retryInterval
			} else {
				retryInterval = nicRegMaxInterval
			}
		}
	}
}
