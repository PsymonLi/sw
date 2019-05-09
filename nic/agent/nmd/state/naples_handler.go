// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"errors"
	"fmt"
	"time"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/nmd/cmdif"
	"github.com/pensando/sw/nic/agent/nmd/rolloutif"
	"github.com/pensando/sw/nic/agent/protos/nmd"
	"github.com/pensando/sw/venice/ctrler/rollout/rpcserver/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/certsproxy"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/rpckit"
)

const (
	// ConfigURL is URL to configure a nic in classic mode
	ConfigURL = "/api/v1/naples/"
	// RolloutURL is URL to configure SmartNICRollout object
	RolloutURL = "/api/v1/naples/rollout/"
	// MonitoringURL is URL to fetch logs and other diags from nic in classic mode
	MonitoringURL = "/monitoring/v1/naples/"
	// CoresURL is URL to fetch cores from nic in classic mode
	CoresURL = "/cores/v1/naples/"
	// CmdEXECUrl is URL to fetch output from running executables on Naples in classic mode
	CmdEXECUrl = "/cmd/v1/naples/"
	// UpdateURL is the URL to help with file upload
	UpdateURL = "/update/"

	// ProfileURL is the URL to create naples profiles
	ProfileURL = "/api/v1/naples/profiles/"

	// NaplesInfoURL is the URL to GET smartnic object
	NaplesInfoURL = "/api/v1/naples/info/"

	// NaplesVersionURL is the URL to GET all software version info from Naples
	NaplesVersionURL = "/api/v1/naples/version/"

	// Max retry interval in seconds for Registration retries
	// Retry interval is initially exponential and is capped
	// at 30min.
	nicRegMaxInterval = 30 * 60 * time.Second
)

// CreateNaplesProfile creates a Naples Profile
func (n *NMD) CreateNaplesProfile(profile nmd.NaplesProfile) error {
	log.Infof("Creating Naples Profile : %v", profile)
	// Validate the number of LIFs
	if !(profile.Spec.NumLifs == 1 || profile.Spec.NumLifs == 16) {
		return fmt.Errorf("requested lif number is not supported. Expecting either 1 or 16")
	}

	if profile.Spec.DefaultPortAdmin != nmd.PortAdminState_PORT_ADMIN_STATE_ENABLE.String() && profile.Spec.DefaultPortAdmin != nmd.PortAdminState_PORT_ADMIN_STATE_DISABLE.String() {
		log.Infof("Invalid port admin state set. Setting to default of Enabled.")
		profile.Spec.DefaultPortAdmin = nmd.PortAdminState_PORT_ADMIN_STATE_ENABLE.String()
	}

	n.Lock()
	defer n.Unlock()
	// ensure the profile name is unique.
	for _, p := range n.profiles {
		if profile.Name == p.Name {
			return fmt.Errorf("profile %v already exists", profile.Name)
		}
	}

	c, _ := types.TimestampProto(time.Now())
	profile.CreationTime = api.Timestamp{
		Timestamp: *c,
	}
	profile.ModTime = api.Timestamp{
		Timestamp: *c,
	}

	// persist profile
	if err := n.store.Write(&profile); err != nil {
		log.Errorf("Failed to persist naples profile. Err: %v", err)
		return err
	}

	// Update in memory state
	n.profiles = append(n.profiles, &profile)
	return nil
}

// UpdateNaplesConfig updates a local Naples Config object
func (n *NMD) UpdateNaplesConfig(cfg nmd.Naples) error {
	log.Infof("NAPLES Update: Old: %v  | New: %v ", n.config, cfg)

	// Remove this once we have a way to pick up the real mac address as hostname when
	// hostname is not passed as a parameter for mode update.
	if cfg.Spec.Mode == cmd.SmartNICSpec_NETWORK.String() && len(cfg.Spec.Hostname) == 0 {
		log.Errorf("Hostname cannot be empty. Cannot update NaplesConfig.")
		return errors.New("hostname cannot be empty")
	}

	n.SetNaplesConfig(cfg.Spec)
	if err := n.store.Write(&n.config); err != nil {
		log.Errorf("Failed to persist naples config to bolt db. Err:  %v", err)
	}

	return n.UpdateMgmtIP()
}

// StartManagedMode starts the tasks required for managed mode
func (n *NMD) StartManagedMode() error {
	log.Info("Starting Managed Mode")

	n.modeChange.Lock()
	if n.cmd == nil {
		// create the CMD client
		cmdClient, err := cmdif.NewCmdClient(n, n.cmdRegURL, n.resolverClient)
		if err != nil {
			log.Errorf("Error creating CMD client. Err: %v", err)
			n.modeChange.Unlock()
			return err
		}
		log.Infof("CMD client {%+v} is running", cmdClient)
		// CMD client sets n.cmd when it's ready
	}

	if n.rollout == nil {
		roClient, err := rolloutif.NewRoClient(n, n.resolverClient)
		if err != nil {
			log.Errorf("Error creating Rollout Controller client. Err: %v", err)
			n.modeChange.Unlock()
			return err
		}
		log.Infof("Rollout client {%+v} is running", roClient)
		n.rollout = roClient
	}

	err := n.initTLSProvider()
	if err != nil {
		n.modeChange.Unlock()
		return fmt.Errorf("error initializing TLS provider: %v", err)
	}

	// Set Registration in progress flag
	log.Infof("NIC in managed mode, mac: %v", n.config.Status.Fru.MacStr)
	n.setRegStatus(true)

	// The mode change is completed when we start the registration loop.
	n.modeChange.Unlock()

	for {
		select {

		// Check if registration loop should be stopped
		case <-n.stopNICReg:

			log.Infof("Registration stopped, exiting.")

			// Clear Registration in progress flag
			n.setRegStatus(false)
			return nil

		// Register NIC
		case <-time.After(n.nicRegInterval):

			// For the NIC in Naples Config, start the registration
			mac := n.config.Status.Fru.MacStr

			n.UpdateNaplesInfoFromConfig()

			nicObj, _ := n.GetSmartNIC()
			// Send NIC register request to CMD
			log.Infof("Registering NIC with CMD : %+v", nicObj)
			msg, err := n.RegisterSmartNICReq(nicObj)

			// Cache it in nicDB
			if msg.AdmissionResponse != nil {
				nicObj.Status.AdmissionPhase = msg.AdmissionResponse.Phase
				nicObj.Status.AdmissionPhaseReason = msg.AdmissionResponse.Reason
				n.config.Status.AdmissionPhase = msg.AdmissionResponse.Phase
				n.config.Status.AdmissionPhaseReason = msg.AdmissionResponse.Reason
			}
			n.SetSmartNIC(nicObj)

			// Error and Phase response is handled according to the following rules.
			//
			// 1. If there are RPC errors (in connecting to CMD) we should retry at
			//    regular interval.
			// 2. If the factory cert is invalid, phase is REJECTED and reason indicates
			//    why it is rejected. In this case, there is no retry done.
			// 3. If the phase is PENDING, it indicates that the certificate is valid, but it
			//    is either not auto-admitted or not manually approved. In such cases
			//    the retry is done at exponential interval and capped at 30min retry.
			// 4. If is the phase is ADMITTED, move on to next stage of sending periodic
			//    NIC updates.
			//
			if err != nil {

				// Rule #1 - continue retry at regular interval
				log.Errorf("Error registering nic, mac: %s err: %+v", mac, err)
			} else {
				resp := msg.AdmissionResponse
				if resp == nil {
					log.Errorf("Protocol error: no AdmissionResponse in message, mac: %s", mac)
				}
				log.Infof("Received register response, phase: %+v", resp.Phase)
				switch resp.Phase {

				case cmd.SmartNICStatus_REJECTED.String():

					// Rule #2 - abort retry, clear registration status flag
					log.Errorf("Invalid NIC, Admission rejected, mac: %s reason: %s", mac, resp.Reason)
					n.setRegStatus(false)
					return err

				case cmd.SmartNICStatus_PENDING.String():

					// Rule #3 - needs slower exponential retry
					// Cap the retry interval at 30mins
					if 2*n.nicRegInterval <= nicRegMaxInterval {
						n.nicRegInterval = 2 * n.nicRegInterval
					} else {
						n.nicRegInterval = nicRegMaxInterval
					}
					if len(resp.RolloutVersion) > 0 {
						log.Infof("NIC (mac %s) running version is incompatible. Request rollout to version %s", mac, resp.RolloutVersion)
						// Create rollout object for version
						snicRollout := protos.SmartNICRollout{
							TypeMeta: api.TypeMeta{
								Kind: "SmartNICRollout"},
							ObjectMeta: api.ObjectMeta{
								Name:   n.config.Status.Fru.MacStr,
								Tenant: n.config.Tenant,
							},
							Spec: protos.SmartNICRolloutSpec{
								Ops: []*protos.SmartNICOpSpec{
									{
										Op:      protos.SmartNICOp_SmartNICImageDownload,
										Version: resp.RolloutVersion,
									},
									{
										Op:      protos.SmartNICOp_SmartNICPreCheckForUpgOnNextHostReboot,
										Version: resp.RolloutVersion,
									},
									{
										Op:      protos.SmartNICOp_SmartNICUpgOnNextHostReboot,
										Version: resp.RolloutVersion,
									},
								},
							}}

						err := n.CreateUpdateSmartNICRollout(&snicRollout)
						if err != nil {
							log.Errorf("Error creating smartNICRollout during NIC Version check {%+v}", err)
						}
					} else {
						log.Infof("NIC waiting for manual approval of admission into cluster, mac: %s reason: %s",
							mac, resp.Reason)
					}

				case cmd.SmartNICStatus_ADMITTED.String():

					// Rule #4 - registration is success, clear registration status
					// and move on to next stage
					log.Infof("NIC admitted into cluster, mac: %s", mac)
					n.setRegStatus(false)
					n.nicRegInterval = n.nicRegInitInterval

					err = n.setClusterCredentials(resp)
					if err != nil {
						log.Errorf("Error processing cluster credentials: %v", err)
					}

					// Start certificates proxy
					if n.certsListenURL != "" {
						certsProxy, err := certsproxy.NewCertsProxy(n.certsListenURL, n.remoteCertsURL,
							rpckit.WithTLSProvider(n.tlsProvider), rpckit.WithRemoteServerName(globals.Cmd))
						if err != nil {
							log.Errorf("Error starting certificates proxy at %s: %v", n.certsListenURL, err)
							// cannot proceed without certs proxy, retry after nicRegInterval
							continue
						} else {
							log.Infof("Started certificates proxy at %s, forwarding to: %s", n.certsListenURL, n.remoteCertsURL)
							n.certsProxy = certsProxy
							n.certsProxy.Start()
						}
					}

					_ = stopNtpClient()
					err = startNtpClient(n.config.Status.Controllers)
					if err != nil {
						log.Infof("start NTP client returned %v", err)
					}

					// start watching objects
					go n.cmd.WatchSmartNICUpdates()
					go n.rollout.WatchSmartNICRolloutUpdates()

					// Start goroutine to send periodic NIC updates
					n.Add(1)
					go func() {
						defer n.Done()
						n.SendNICUpdates()
					}()

					return nil

				case cmd.SmartNICStatus_UNKNOWN.String():
					// Not an expected response
					log.Errorf("Unknown response, nic: %+v phase: %v", nicObj, resp)

				} // end of switch statement
			} // end of if err != nil statement
		} // end of select statement
	}
}

// SendNICUpdates sends periodic updates about NIC to CMD
func (n *NMD) SendNICUpdates() error {

	n.setUpdStatus(true)
	for {
		select {

		// Check if NICUpdate loop should be stopped
		case <-n.stopNICUpd:

			log.Infof("NICUpdate stopped, exiting.")
			n.setUpdStatus(false)
			return nil

		// NIC update timer callback
		case <-time.After(n.nicUpdInterval):
			nicObj := n.nic

			// Skip until NIC is admitted
			if nicObj.Status.AdmissionPhase != cmd.SmartNICStatus_ADMITTED.String() {
				log.Infof("Skipping health update, phase %v", nicObj.Status.AdmissionPhase)
				continue
			}

			// TODO : Get status from platform and fill nic Status
			nicObj.Status = cmd.SmartNICStatus{
				AdmissionPhase: cmd.SmartNICStatus_ADMITTED.String(),
				Conditions: []cmd.SmartNICCondition{
					{
						Type:               cmd.SmartNICCondition_HEALTHY.String(),
						Status:             cmd.ConditionStatus_TRUE.String(),
						LastTransitionTime: time.Now().UTC().Format(time.RFC3339),
					},
				},
			}

			// Send nic status
			log.Infof("Sending NIC health update: %+v", nicObj)
			err := n.UpdateSmartNICReq(nicObj)
			if err != nil {
				log.Errorf("Error updating nic, name:%s  err: %+v",
					nicObj.Name, err)
			}

		}
	}
}

// StopManagedMode stop the ongoing tasks meant for managed mode
func (n *NMD) StopManagedMode() error {
	log.Info("Stopping Managed Mode.")
	n.modeChange.Lock()
	defer n.modeChange.Unlock()

	// stop accepting certificate requests
	n.Lock()
	if n.certsProxy != nil {
		n.certsProxy.Stop()
		n.certsProxy = nil
	}
	n.Unlock()

	// stop ongoing NIC registration, if any
	if n.GetRegStatus() {
		n.stopNICReg <- true
	}

	// stop ongoing NIC updates, if any
	if n.GetUpdStatus() {
		n.stopNICUpd <- true
	}

	// Wait for goroutines launched in managed mode
	// to complete
	n.Wait()

	// cmd, rollout and tlsProvider are protected by modeChange lock

	if n.cmd != nil {
		n.cmd.Stop()
		n.cmd = nil
	}

	if n.rollout != nil {
		n.rollout.Stop()
		n.rollout = nil
	}

	// release TLS provider resources
	if n.tlsProvider != nil {
		n.tlsProvider.Close()
		n.tlsProvider = nil
	}

	return nil
}

// StartClassicMode start the tasks required for classic mode
func (n *NMD) StartClassicMode() error {
	log.Infof("Starting Classic Mode.")
	if !n.GetRestServerStatus() {
		// Start RestServer
		log.Infof("NIC in classic mode, mac: %v", n.config.Status.Fru.MacStr)
		return n.StartRestServer()
	}

	return nil
}

// StopClassicMode stops the ongoing tasks meant for classic mode
func (n *NMD) StopClassicMode(shutdown bool) error {
	log.Infof("Stopping Classic Mode.")
	// Stop RestServer
	return n.StopRestServer(shutdown)
}

// GetPlatformCertificate returns the certificate containing the NIC identity and public key
func (n *NMD) GetPlatformCertificate(nic *cmd.SmartNIC) ([]byte, error) {
	return n.platform.GetPlatformCertificate(nic)
}

// GenChallengeResponse returns the response to a challenge issued by CMD to authenticate this NAPLES
func (n *NMD) GenChallengeResponse(nic *cmd.SmartNIC, challenge []byte) ([]byte, []byte, error) {
	signer, err := n.platform.GetPlatformSigner(nic)
	if err != nil {
		return nil, nil, fmt.Errorf("error getting platform signer: %v", err)
	}
	return certs.GeneratePoPChallengeResponse(signer, challenge)
}
