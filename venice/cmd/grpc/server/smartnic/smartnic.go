// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package smartnic

import (
	"crypto/x509"
	"fmt"
	"net"
	"sync"
	"time"

	"github.com/pkg/errors"
	"golang.org/x/net/context"

	"github.com/pensando/sw/api"
	apierrors "github.com/pensando/sw/api/errors"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/events"
	evtsapi "github.com/pensando/sw/api/generated/events"
	nmdstate "github.com/pensando/sw/nic/agent/nmd/state"
	nmd "github.com/pensando/sw/nic/agent/protos/nmd"
	"github.com/pensando/sw/venice/cmd/cache"
	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/cmd/grpc/server/certificates/certapi"
	cmdcertutils "github.com/pensando/sw/venice/cmd/grpc/server/certificates/utils"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/ctxutils"
	"github.com/pensando/sw/venice/utils/events/recorder"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/runtime"
)

const (

	// Max retry interval in seconds for Registration retries
	// Retry interval is initially exponential and is capped
	// at 2 min.
	nicRegMaxInterval = (2 * 60)

	// Subject for a valid platform certificate
	platformCertOrg            = "Pensando Systems"
	platformCertOrgUnit        = "Pensando Manufacturing CA"
	platformSerialNumberFormat = "PID=%s SN=%s"
)

var (
	errAPIServerDown = fmt.Errorf("API Server not reachable or down")

	// HealthWatchInterval is default health watch interval
	HealthWatchInterval = 60 * time.Second

	// DeadInterval is default dead time interval, after
	// which NIC health status is declared UNKNOWN by CMD
	DeadInterval = 120 * time.Second

	// Max time (in milliseconds) to complete the entire registration sequence,
	// after which the server will cancel the request
	nicRegTimeout = 3000 * time.Millisecond
)

func getNICCondition(nic *cluster.SmartNIC, condType cluster.SmartNICCondition_ConditionType) *cluster.SmartNICCondition {
	for _, cond := range nic.Status.Conditions {
		if cond.Type == condType.String() {
			return &cond
		}
	}
	return nil
}

// RPCServer implements SmartNIC gRPC service.
type RPCServer struct {
	sync.Mutex

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

	versionChecker NICAdmissionVersionChecker // checks version of NIC for admission
}

// NICAdmissionVersionChecker is an interface that checks whether the nicVersion for the given SKU is allowed for admission
type NICAdmissionVersionChecker interface {
	CheckNICVersionForAdmission(nicSku string, nicVersion string) (string, string)
}

// NewRPCServer returns a SmartNIC RPC server object
func NewRPCServer(healthInvl, deadInvl time.Duration, restPort string, stateMgr *cache.Statemgr, nicVersionChecker NICAdmissionVersionChecker) (*RPCServer, error) {
	return &RPCServer{
		HealthWatchIntvl: healthInvl,
		DeadIntvl:        deadInvl,
		RestPort:         restPort,
		RetryNicDB:       make(map[string]*cluster.SmartNIC),
		stateMgr:         stateMgr,
		versionChecker:   nicVersionChecker,
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

// validateNICPlatformCert validates the certificate provided by NAPLES in the admission request
// It only checks the certificate. It does not check possession of the private key.
func validateNICPlatformCert(cert *x509.Certificate, nic *cluster.SmartNIC) error {
	if cert == nil {
		return fmt.Errorf("No certificate provided")
	}

	if nic == nil {
		return fmt.Errorf("No SmartNIC object provided")
	}

	// TODO: use only Pensando PKI certs, do not accept self-signed!
	verifyOpts := x509.VerifyOptions{
		Roots: certs.NewCertPool([]*x509.Certificate{cert}),
	}

	chains, err := cert.Verify(verifyOpts)
	if err != nil || len(chains) != 1 {
		return fmt.Errorf("Certificate validation failed, err: %v", err)
	}

	if len(cert.Subject.Organization) != 1 || cert.Subject.Organization[0] != platformCertOrg {
		return fmt.Errorf("Invalid organization field in subject: %v", cert.Subject.Organization)
	}

	if len(cert.Subject.OrganizationalUnit) != 1 || cert.Subject.OrganizationalUnit[0] != platformCertOrgUnit {
		return fmt.Errorf("Invalid organizational unit field in subject: %v", cert.Subject.OrganizationalUnit)
	}

	var pid, sn string
	n, err := fmt.Sscanf(cert.Subject.SerialNumber, platformSerialNumberFormat, &pid, &sn)
	if n != 2 {
		return fmt.Errorf("Invalid PID/SN: %s", cert.Subject.SerialNumber)
	}
	if err != nil {
		return fmt.Errorf("Invalid PID/SN: %s. Error: %v", cert.Subject.SerialNumber, err)
	}

	if sn != nic.Status.SerialNum {
		return fmt.Errorf("Serial number mismatch. AdmissionRequest: %s, Certificate: %s", sn, nic.Status.SerialNum)
	}

	if cert.Subject.CommonName != nic.Status.PrimaryMAC {
		return fmt.Errorf("Name mismatch. AdmissionRequest: %s, Certificate: %s", nic.Status.PrimaryMAC, cert.Subject.CommonName)
	}

	return nil
}

// UpdateSmartNIC creates or updates the smartNIC object
func (s *RPCServer) UpdateSmartNIC(updObj *cluster.SmartNIC) (*cluster.SmartNIC, error) {
	var err error
	var refObj *cluster.SmartNIC // reference object (before update)
	var retObj *cluster.SmartNIC // return object (after update)

	// Check if object exists.
	// If it doesn't exist, do not create it, as it might have been deleted by user.
	// NIC object creation (for example during NIC registration) should always be explicit.
	nicState, err := s.stateMgr.FindSmartNIC(updObj.GetName())
	if nicState != nil && err == nil {
		nicState.Lock()
		defer nicState.Unlock()
		refObj = nicState.SmartNIC
	} else {
		return nil, fmt.Errorf("Error retrieving reference object for NIC update. Err: %v, update: %+v", err, updObj)
	}

	nicName := updObj.Name

	// decide whether to send to ApiServer or not before we make any adjustment
	updateAPIServer := !runtime.FilterUpdate(refObj.Status, updObj.Status, []string{"LastTransitionTime"}, nil)

	refHealthCond := getNICCondition(refObj, cluster.SmartNICCondition_HEALTHY)
	updHealthCond := getNICCondition(updObj, cluster.SmartNICCondition_HEALTHY)

	// generate event if there was a health transition
	if updHealthCond != nil && refHealthCond != nil && refHealthCond.Status != updHealthCond.Status {
		var evtType string
		var severity events.SeverityLevel
		var msg string

		switch updHealthCond.Status {
		case cluster.ConditionStatus_TRUE.String():
			evtType = cluster.NICHealthy
			severity = evtsapi.SeverityLevel_INFO
			msg = fmt.Sprintf("Healthy condition for SmartNIC %s is now %s", nicName, cluster.ConditionStatus_TRUE.String())

		case cluster.ConditionStatus_FALSE.String():
			evtType = cluster.NICUnhealthy
			severity = evtsapi.SeverityLevel_CRITICAL
			msg = fmt.Sprintf("Healthy condition for SmartNIC %s is now %s", nicName, cluster.ConditionStatus_FALSE.String())

		default:
			// this should not happen
			log.Errorf("NIC reported unknown health condition: %+v", updHealthCond)
		}

		if evtType != "" {
			recorder.Event(evtType, severity, msg, nil)
			log.Infof("Generated event, type: %v, sev: %v, msg: %s", evtType, severity.String(), msg)
		}

		// Ignore the time-stamp provided by NMD and replace it with our own.
		// This will help mitigating issues due to clock misalignments between Venice and NAPLES
		// As long as it gets periodic updates, CMD is happy.
		// If it happens to process an old message by mistake, the next one will correct it.
		updHealthCond.LastTransitionTime = time.Now().UTC().Format(time.RFC3339)
	}

	// Store the update in local cache
	refObj.Status.Conditions = updObj.Status.Conditions
	err = s.stateMgr.UpdateSmartNIC(refObj, updateAPIServer)
	if err != nil {
		log.Errorf("Error updating statemgr state for NIC %s: %v", nicName, err)
	}

	log.Debugf("UpdateSmartNIC nic: %+v", retObj)
	return refObj, err
}

func (s *RPCServer) isHostnameUnique(subj *cluster.SmartNIC) (bool, error) {
	nics, err := s.stateMgr.ListSmartNICs()
	if err != nil {
		return false, err
	}

	for _, n := range nics {
		n.Lock()
		if subj.Spec.Hostname == n.Spec.Hostname && subj.Name != n.Name {
			n.Unlock()
			return false, nil
		}
		n.Unlock()
	}
	return true, nil
}

// RegisterNIC handles the register NIC request and upon validation creates SmartNIC object.
// NMD starts the NIC registration process when the NIC is placed in managed mode.
// NMD is expected to retry with backoff if there are API errors or if the NIC is rejected.
func (s *RPCServer) RegisterNIC(stream grpc.SmartNICRegistration_RegisterNICServer) error {
	var req *grpc.NICAdmissionRequest
	var name string

	// Canned responses in case of error
	authErrResp := &grpc.RegisterNICResponse{
		AdmissionResponse: &grpc.NICAdmissionResponse{
			Phase:  cluster.SmartNICStatus_REJECTED.String(),
			Reason: string("Authentication error"),
		},
	}

	intErrResp := &grpc.RegisterNICResponse{
		AdmissionResponse: &grpc.NICAdmissionResponse{
			Phase:  cluster.SmartNICStatus_UNKNOWN.String(),
			Reason: string("Internal error"),
		},
	}

	noClusterErrResp := &grpc.RegisterNICResponse{
		AdmissionResponse: &grpc.NICAdmissionResponse{
			Phase:  cluster.SmartNICStatus_UNKNOWN.String(),
			Reason: string("Controller node is not part of a cluster"),
		},
	}

	protoErrResp := &grpc.RegisterNICResponse{
		AdmissionResponse: &grpc.NICAdmissionResponse{
			Phase:  cluster.SmartNICStatus_UNKNOWN.String(),
			Reason: string("Internal error"),
		},
	}

	// There is no way to specify timeouts for individual Send/Recv calls in Golang gRPC,
	// so we execute the entire registration sequence under a single timeout.
	// https://github.com/grpc/grpc-go/issues/445
	// https://github.com/grpc/grpc-go/issues/1229
	procRequest := func() (interface{}, error) {

		msg, err := stream.Recv()
		if err != nil {
			return nil, errors.Wrapf(err, "Error receiving admission request")
		}

		if msg.AdmissionRequest == nil {
			return protoErrResp, fmt.Errorf("Protocol error: no AdmissionRequest in message")
		}

		req = msg.AdmissionRequest
		naplesNIC := req.GetNic()
		name = naplesNIC.Name

		cert, err := x509.ParseCertificate(req.GetCert())
		if err != nil {
			return authErrResp, errors.Wrapf(err, "Invalid certificate")
		}

		// Validate the factory cert obtained in the request
		err = validateNICPlatformCert(cert, &naplesNIC)
		if err != nil {
			return authErrResp, errors.Wrapf(err, "Invalid certificate, name: %v, cert subject: %v", naplesNIC.Name, cert.Subject)
		}

		// check the CSR
		csr, err := x509.ParseCertificateRequest(req.GetClusterCertSignRequest())
		if err != nil {
			return protoErrResp, errors.Wrapf(err, "Invalid certificate request")
		}
		err = csr.CheckSignature()
		if err != nil {
			return protoErrResp, errors.Wrapf(err, "Certificate request has invalid signature")
		}

		// Send challenge
		challenge, err := certs.GeneratePoPNonce()
		if err != nil {
			return intErrResp, errors.Wrapf(err, "Error generating challenge")
		}

		authReq := &grpc.RegisterNICResponse{
			AuthenticationRequest: &grpc.AuthenticationRequest{
				Challenge: challenge,
			},
		}

		err = stream.Send(authReq)
		if err != nil {
			return nil, errors.Wrapf(err, "Error sending auth request")
		}

		// Receive challenge response
		msg, err = stream.Recv()
		if err != nil {
			return nil, errors.Wrapf(err, "Error receiving auth response")
		}

		authResp := msg.AuthenticationResponse
		if authResp == nil {
			return protoErrResp, fmt.Errorf("Protocol error: no AuthenticationResponse in msg")
		}

		err = certs.VerifyPoPChallenge(challenge, authResp.ClaimantRandom, authResp.ChallengeResponse, cert)
		if err != nil {
			return authErrResp, errors.Wrapf(err, "Authentication failed")
		}

		// NAPLES is genuine, sign the CSR
		clusterCert, err := env.CertMgr.Ca().Sign(csr)
		if err != nil {
			return intErrResp, errors.Wrap(err, "Error signing certificate request")
		}

		// NAPLES must be in network-managed mode
		if naplesNIC.Spec.MgmtMode != cluster.SmartNICSpec_NETWORK.String() {
			return intErrResp, fmt.Errorf("Cannot admit SmartNIC because it is not in network-managed mode")
		}

		log.Infof("Validated NIC: %s", name)

		// If we make it to this point, we know the NAPLES is authentic.
		// However, there are still reasons why the admission may fail.
		// We create or update the object in ApiServer so that user knows
		// if something went wrong and can take action.

		// Get the Cluster object
		clusterObj, err := s.stateMgr.GetCluster()
		if err != nil {
			return intErrResp, errors.Wrapf(err, "Error getting Cluster object")
		}

		hostnameUnique, err := s.isHostnameUnique(&naplesNIC)
		if err != nil {
			return intErrResp, errors.Wrapf(err, "Error getting SmartNIC list")
		}

		var nicObj *cluster.SmartNIC
		var smartNICObjExists bool // does the SmartNIC object already exist ?
		nicObjState, err := s.stateMgr.FindSmartNIC(name)
		if err != nil {
			if err != memdb.ErrObjectNotFound {
				return intErrResp, errors.Wrapf(err, "Error reading NIC object")
			}
			// NIC object is not present. Create the NIC object based on the template provided by NAPLES and update it accordingly.
			smartNICObjExists = false
			nicObj = &naplesNIC
		} else {
			nicObjState.Lock()
			defer nicObjState.Unlock()
			nicObj = nicObjState.SmartNIC
			smartNICObjExists = true
		}

		// If SmartNIC object does not exist, we initialize Spec.Admit using the value of cluster.Spec.AutoAdmitNICs
		// If it exists, we honor the value that is currently there
		// Note that even if Spec.Admit = true, the SmartNIC can end up in rejected state if it fails subsequent checks.
		if !smartNICObjExists {
			nicObj.Spec.Admit = clusterObj.Spec.AutoAdmitNICs
		}

		// If hostname supplied by NAPLES is not unique, reject but still create SmartNIC object
		if !hostnameUnique {
			nicObj.Status.AdmissionPhase = cluster.SmartNICStatus_REJECTED.String()
			nicObj.Status.AdmissionPhaseReason = "Hostname is not unique"
			nicObj.Status.Conditions = nil
		} else {
			// Re-initialize status as it might have changed
			nicObj.Status = naplesNIC.Status
			// clear out host pairing so that it will be recomputed
			nicObj.Status.Host = ""
			// mark as admitted or pending based on current value of Spec.Admit
			if nicObj.Spec.Admit == true {
				nicObj.Status.AdmissionPhase = cluster.SmartNICStatus_ADMITTED.String()
				nicObj.Status.AdmissionPhaseReason = ""
				// If the NIC is admitted, override all spec parameters with the new supplied values,
				// except those that are owned by Venice.
				nicObj.Spec = naplesNIC.Spec
				nicObj.Spec.Admit = true
			} else {
				nicObj.Status.AdmissionPhase = cluster.SmartNICStatus_PENDING.String()
				nicObj.Status.AdmissionPhaseReason = "SmartNIC waiting for manual admission"
				nicObj.Status.Conditions = nil
			}
		}

		// If NIC was decommissioned from Venice, override previous Spec.MgmtMode
		nicObj.Spec.MgmtMode = cluster.SmartNICSpec_NETWORK.String()

		// Create or update SmartNIC object in ApiServer
		if smartNICObjExists {
			err = s.stateMgr.UpdateSmartNIC(nicObj, true)
		} else {
			var sns *cache.SmartNICState
			sns, err = s.stateMgr.CreateSmartNIC(nicObj, true)
			sns.Lock()
			defer sns.Unlock()
		}
		if err != nil {
			status := apierrors.FromError(err)
			log.Errorf("Error creating or updating smartNIC object. Create:%v, obj:%+v err:%v status:%v", !smartNICObjExists, nicObj, err, status)
			return intErrResp, errors.Wrapf(err, "Error updating smartNIC object")
		}

		if nicObj.Status.AdmissionPhase == cluster.SmartNICStatus_REJECTED.String() {
			recorder.Event(cluster.NICRejected, evtsapi.SeverityLevel_WARNING,
				fmt.Sprintf("Admission for SmartNIC %s was rejected, reason: %s", nicObj.Name, nicObj.Status.AdmissionPhaseReason), nil)

			return &grpc.RegisterNICResponse{
				AdmissionResponse: &grpc.NICAdmissionResponse{
					Phase:  cluster.SmartNICStatus_REJECTED.String(),
					Reason: nicObj.Status.AdmissionPhaseReason,
				},
			}, nil
		}

		// ADMITTED or PENDING
		okResp := &grpc.RegisterNICResponse{
			AdmissionResponse: &grpc.NICAdmissionResponse{
				Phase: nicObj.Status.AdmissionPhase,
			},
		}

		if nicObj.Status.AdmissionPhase == cluster.SmartNICStatus_ADMITTED.String() {
			okResp.AdmissionResponse.ClusterCert = &certapi.CertificateSignResp{
				Certificate: &certapi.Certificate{
					Certificate: clusterCert.Raw,
				},
			}
			okResp.AdmissionResponse.CaTrustChain = cmdcertutils.GetCaTrustChain(env.CertMgr)
			okResp.AdmissionResponse.TrustRoots = cmdcertutils.GetTrustRoots(env.CertMgr)
		}

		status, version := s.versionChecker.CheckNICVersionForAdmission(nicObj.Status.GetSmartNICSku(), nicObj.Status.GetSmartNICVersion())
		if status != "" {
			log.Infof("NIC %s with SKU %s and version %s is requested to rollout to version %s because %s. Skip temporarily.", name, nicObj.Status.GetSmartNICSku(), nicObj.Status.GetSmartNICVersion(), version, status)
			/*
				okResp.AdmissionResponse.Phase = cluster.SmartNICStatus_PENDING.String()
				okResp.AdmissionResponse.Reason = status
				okResp.AdmissionResponse.RolloutVersion = version
			*/
		}
		return okResp, nil
	}

	// if we are not part of a cluster yet we cannot process admission requests
	clusterObj, err := s.stateMgr.GetCluster()
	if err != nil || clusterObj == nil {
		retErr := fmt.Errorf("Rejecting RegisterNIC request from %s, cluster not formed yet, err: %v", ctxutils.GetPeerAddress(stream.Context()), err)
		log.Errorf("%v", retErr)
		stream.Send(noClusterErrResp)
		return retErr
	}

	ctx, cancel := context.WithTimeout(stream.Context(), nicRegTimeout)
	defer cancel()
	resp, err := utils.ExecuteWithContext(ctx, procRequest)

	// if we have a response we send it back to the client, otherwise
	// it means we timed out and can just terminate the request
	if resp != nil {
		regNICResp := resp.(*grpc.RegisterNICResponse)
		stream.Send(regNICResp)
		if regNICResp.AdmissionResponse.Phase == cluster.SmartNICStatus_ADMITTED.String() {
			log.Infof("SmartNIC %s is admitted to the cluster", name)
		}
	}

	if err != nil {
		log.Errorf("Error processing NIC admission request, name: %v, err: %v", name, err)
	}

	return err
}

// UpdateNIC is the handler for the UpdateNIC() RPC invoked by NMD
func (s *RPCServer) UpdateNIC(ctx context.Context, req *grpc.UpdateNICRequest) (*grpc.UpdateNICResponse, error) {
	// Update smartNIC object with CAS semantics
	obj := req.GetNic()
	nicObj, err := s.UpdateSmartNIC(&obj)

	if err != nil || nicObj == nil {
		log.Errorf("Error updating SmartNIC object: %+v err: %v", obj, err)
		return &grpc.UpdateNICResponse{}, err
	}

	return &grpc.UpdateNICResponse{}, nil // no need to send back the full update
}

//ListSmartNICs lists all smartNICs matching object selector
func (s *RPCServer) ListSmartNICs(ctx context.Context, sel *api.ObjectMeta) ([]*cluster.SmartNIC, error) {
	var niclist []*cluster.SmartNIC
	// get all smartnics
	nics, err := s.stateMgr.ListSmartNICs()
	if err != nil {
		return nil, err
	}

	// walk all smartnics and add it to the list
	for _, nic := range nics {
		if sel.GetName() == nic.GetName() {
			niclist = append(niclist, nic.SmartNIC)
		}
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
			Nic:       *nic,
		}
		err = stream.Send(&watchEvt)
		if err != nil {
			log.Errorf("Error sending stream. Err: %v", err)
			return err
		}
	}

	// loop forever on watch channel
	log.Infof("WatchNICs entering watch loop for SmartNIC: %s", sel.GetName())
	var sentEvent grpc.SmartNICEvent
	for {
		select {
		// read from channel
		case evt, ok := <-watchChan:
			if !ok {
				log.Errorf("Error reading from channel. Closing watch")
				return errors.New("Error reading from channel")
			}

			// convert to smartnic object
			nic, err := cache.SmartNICStateFromObj(evt.Obj)
			if err != nil {
				return err
			}

			if sel.GetName() != nic.GetName() {
				continue
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

			nic.Lock()
			// construct the smartnic event object
			watchEvt := grpc.SmartNICEvent{
				EventType: etype,
				Nic:       *nic.SmartNIC,
			}
			nic.Unlock()
			send := !runtime.FilterUpdate(sentEvent, watchEvt, []string{"LastTransitionTime"}, nil)
			if send {
				log.Infof("Sending watch event for SmartNIC: %+v", watchEvt)
				err = stream.Send(&watchEvt)
				if err != nil {
					log.Errorf("Error sending stream. Err: %v closing watch", err)
					return err
				}
				sentEvent = watchEvt
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

		case <-time.After(s.HealthWatchIntvl):
			if env.LeaderService == nil || !env.LeaderService.IsLeader() {
				// only leader gets updates from the NIC and so can detect when a NIC becomes unresponsive
				continue
			}

			// Get a list of all existing smartNICs
			// Take it from the cache, not ApiServer, because we do not propagate state
			// back to ApiServer unless there is a real change.
			// condition.LastTransitionTime on ApiServer actually represents the last time
			// there was a transition, not the last time the condition was reported.
			// In the cache instead we set LastTransitionTime for each update.
			nicStates, err := s.stateMgr.ListSmartNICs()
			if err != nil {
				log.Errorf("Failed to getting a list of nics, err: %v", err)
				continue
			}

			log.Infof("Health watch timer callback, #nics: %d ", len(nicStates))

			// Iterate on smartNIC objects
			for _, nicState := range nicStates {
				nicState.Lock()
				nic := nicState.SmartNIC
				if nic.Status.AdmissionPhase == cluster.SmartNICStatus_ADMITTED.String() {
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
								nic.Status.Conditions[i].LastTransitionTime = time.Now().UTC().Format(time.RFC3339)
								nic.Status.Conditions[i].Reason = fmt.Sprintf("NIC health update not received since %s", lastUpdateTime)
								// push the update back to ApiServer
								err := s.stateMgr.UpdateSmartNIC(nic, true)
								if err != nil {
									log.Errorf("Failed updating the NIC health status to unknown, nic: %s err: %s", nic.Name, err)
								}
								recorder.Event(cluster.NICHealthUnknown, evtsapi.SeverityLevel_WARNING,
									fmt.Sprintf("Healthy condition for SmartNIC %s is now %s", nic.Name, cluster.ConditionStatus_UNKNOWN.String()), nil)
							}
							break
						}
					}
				}
				nicState.Unlock()
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

	var resp nmdstate.NaplesConfigResp

	// check if Nic exists in RetryDB
	if s.NicExistsInRetryDB(nic) == true {
		s.UpdateNicInRetryDB(nic)
		log.Debugf("Nic registration retry is ongoing, nic: %s", nic.Name)
		return
	}

	// Add Nic to the RetryDB
	s.UpdateNicInRetryDB(nic)
	log.Infof("Initiating nic registration for Naples, MAC: %s IP:%+v", nic.Name, nic.Spec.IPConfig.IPAddress)

	for {
		select {
		case <-time.After(retryInterval * time.Second):

			if s.NicExistsInRetryDB(nic) == false {
				// If NIC is deleted stop the retry
				return
			}

			nicObj := s.GetNicInRetryDB(s.getNicKey(nic))
			controller, _, err := net.SplitHostPort(env.SmartNICRegRPCServer.GetListenURL())
			if err != nil {
				log.Errorf("Error parsing unauth RPC server URL %s: %v", env.SmartNICRegRPCServer.GetListenURL(), err)
				// retry
				continue
			}

			// Config to switch to Managed mode
			naplesCfg := nmd.Naples{
				ObjectMeta: api.ObjectMeta{Name: "NaplesConfig"},
				TypeMeta:   api.TypeMeta{Kind: "Naples"},
				Spec: nmd.NaplesSpec{
					Mode:        nmd.MgmtMode_NETWORK.String(),
					NetworkMode: nicObj.Spec.NetworkMode,
					PrimaryMAC:  nicObj.Name,
					Controllers: []string{controller},
					Hostname:    nicObj.Spec.Hostname,
					IPConfig:    nicObj.Spec.IPConfig,
				},
			}

			ip, _, _ := net.ParseCIDR(nicObj.Spec.IPConfig.IPAddress)
			nmdURL := fmt.Sprintf("http://%s:%s%s", ip, s.RestPort, nmdstate.ConfigURL)
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
					Conditions: []cluster.SmartNICCondition{
						{
							Type:               cluster.SmartNICCondition_UNREACHABLE.String(),
							Status:             cluster.ConditionStatus_TRUE.String(),
							LastTransitionTime: time.Now().UTC().Format(time.RFC3339),
							Reason:             fmt.Sprintf("Failed to post naples config after several attempts, response: %+v", resp),
							Message:            fmt.Sprintf("Naples REST endpoint: %s:%s is not reachable", nic.Spec.IPConfig.IPAddress, s.RestPort),
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
