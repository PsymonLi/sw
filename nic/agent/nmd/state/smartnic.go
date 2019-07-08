// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"context"
	"errors"
	"fmt"
	"time"

	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/nmd/cmdif"
	"github.com/pensando/sw/nic/agent/nmd/rolloutif"
	"github.com/pensando/sw/nic/agent/nmd/utils"
	"github.com/pensando/sw/nic/agent/protos/nmd"
	"github.com/pensando/sw/venice/cmd/grpc"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/tsdb"
)

// RegisterSmartNICReq registers a NIC with CMD
func (n *NMD) RegisterSmartNICReq(nic *cmd.SmartNIC) (grpc.RegisterNICResponse, error) {

	if n.cmd == nil {
		log.Errorf("Failed to register NIC, mac: %s cmd not ready", nic.ObjectMeta.Name)
		return grpc.RegisterNICResponse{}, errors.New("cmd not ready yet")
	}
	resp, err := n.cmd.RegisterSmartNICReq(nic)
	if err != nil {
		log.Errorf("Failed to register NIC, mac: %s err: %v", nic.ObjectMeta.Name, err)
		return resp, err
	}
	return resp, nil
}

// UpdateSmartNICReq registers a NIC with CMD/Venice cluster
func (n *NMD) UpdateSmartNICReq(nic *cmd.SmartNIC) error {

	if n.cmd == nil {
		log.Errorf("Failed to update NIC, mac: %s cmd not ready", nic.ObjectMeta.Name)
		return errors.New("cmd not ready yet")
	}
	err := n.cmd.UpdateSmartNICReq(nic)
	if err != nil || nic == nil {
		log.Errorf("Failed to update NIC, mac: %s err: %v", nic.ObjectMeta.Name, err)
		return err
	}

	log.Infof("Update NIC response mac: %s", nic.ObjectMeta.Name)
	return nil
}

// CreateSmartNIC creates a local smartNIC object
func (n *NMD) CreateSmartNIC(nic *cmd.SmartNIC) error {

	log.Infof("SmartNIC create, mac: %s", nic.ObjectMeta.Name)

	// When we re-connect to CMD we may get "create" notifications for the NIC.
	// In that case  we need to treat the notification as an update, because
	// (instead of just overriding what we have) because otherwise we may miss
	// a transition.
	if n.nic == nil {
		// add the nic to database
		n.SetSmartNIC(nic)
		err := n.store.Write(nic)
		return err
	}
	return n.UpdateSmartNIC(nic)
}

// UpdateSmartNIC updates the local smartNIC object
// Only meant to be called when receiving events from SmartNIC watcher
func (n *NMD) UpdateSmartNIC(nic *cmd.SmartNIC) error {

	log.Infof("SmartNIC update, mac: %s, phase: %s, mgmt mode: %s", nic.ObjectMeta.Name, nic.Status.AdmissionPhase, nic.Spec.MgmtMode)

	// get current state from db
	oldNic, err := n.GetSmartNIC()
	if err != nil {
		log.Errorf("Error retrieving state for nic %+v: %v", nic, err)
	}

	// update nic in the DB
	n.SetSmartNIC(nic)
	err = n.store.Write(nic)
	if err != nil {
		log.Errorf("Error updating NMD state %+v: %v", nic, err)
	}

	// Handle de-admission and decommission

	// We need to check the old spec and status values because we may receive multiple
	// notifications from CMD before we actually shut down the updates channel and we
	// don't want to do react to each of them.
	if oldNic != nil {
		decommission := oldNic.Spec.MgmtMode == cmd.SmartNICSpec_NETWORK.String() &&
			nic.Spec.MgmtMode == cmd.SmartNICSpec_HOST.String()

		deAdmission := oldNic.Status.AdmissionPhase == cmd.SmartNICStatus_ADMITTED.String() &&
			nic.Status.AdmissionPhase == cmd.SmartNICStatus_PENDING.String()

		if decommission || deAdmission {
			n.metrics = nil
			// Spawn a goroutine that will wait for cleanup to finish and then restart managed or classic mode.
			// This has to be done in a separate goroutine because this function is executing in the context of
			// the watcher and the watcher has to terminate for the cleanup to be complete
			go func() {
				// wipe out existing roots of trust
				err = utils.ClearNaplesTrustRoots()
				if err != nil {
					log.Errorf("Error removing trust roots: %v", err)
				}

				// restart rev proxy so that it can go back to HTTP and no client auth
				n.StopReverseProxy()
				n.StartReverseProxy()

				if decommission {
					// NIC has been decommissioned by user. Go back to classic mode.
					log.Infof("SmartNIC %s has been decommissioned, triggering change to HOST managed mode", nic.ObjectMeta.Name)
					cfg := nmd.Naples{}
					// Update config status to reflect the mode change
					cfg.Spec.Mode = nmd.MgmtMode_HOST.String()
					cfg.Spec.NaplesProfile = "default"
					err = n.StartNMDRestServer()
					if err != nil {
						log.Errorf("Error starting NIC managed mode: %v", err)
					}
					if err := n.UpdateNaplesConfig(cfg); err != nil {
						log.Errorf("Failed to revert to host managed mode during decommissioning")
					}
					n.config.Status.AdmissionPhase = cmd.SmartNICStatus_DECOMMISSIONED.String()
					n.config.Status.AdmissionPhaseReason = "SmartNIC management mode changed to HOST"
					log.Infof("Naples successfully decommissioned and moved to HOST mode.")
				} else {
					err = n.StopManagedMode()
					if err != nil {
						log.Errorf("Failed to stop managed mode. Err : %v", err)
					}

					n.Add(1)
					defer n.Done()

					// Re-init admission. TODO move this as native fsm state
					log.Infof("SmartNIC %s has been de-admitted from cluster", nic.ObjectMeta.Name)
					// Update CMD Client with the new resolvers obtained statically
					veniceIPs := n.config.Status.Controllers
					if len(veniceIPs) == 0 {
						log.Error("Missing venice co-ordinates")
					}
					var resolverURLs []string
					registrationURL := fmt.Sprintf("%s:%s", veniceIPs[0], globals.CMDSmartNICRegistrationPort)
					for _, v := range veniceIPs {
						resolverURLs = append(resolverURLs, fmt.Sprintf("%s:%s", v, globals.CMDGRPCAuthPort))
					}
					// Update NMD's resolver client.
					n.resolverClient = resolver.New(&resolver.Config{
						Name:    fmt.Sprintf("%s-%s", globals.Nmd, n.GetAgentID()),
						Servers: resolverURLs,
					})

					// Pick the first resolver URL for remote certs
					n.remoteCertsURL = fmt.Sprintf("%s:%s", veniceIPs[0], globals.CMDAuthCertAPIPort)

					// Init TSDB
					// initialize netagent's tsdb client
					opts := &tsdb.Opts{
						ClientName:              "nmd_" + n.GetAgentID(),
						ResolverClient:          n.resolverClient,
						Collector:               globals.Collector,
						DBName:                  "default",
						SendInterval:            time.Duration(30) * time.Second,
						ConnectionRetryInterval: 100 * time.Millisecond,
					}
					ctx, cancel := context.WithCancel(context.Background())
					tsdb.Init(ctx, opts)
					n.tsdbCancel = cancel

					cmdAPI, err := cmdif.NewCmdClient(n, registrationURL, n.resolverClient)
					if err != nil {
						log.Errorf("Failed to instantiate CMD Client. Err: %v", err)
					}
					roClient, err := rolloutif.NewRoClient(n, n.resolverClient)
					if err != nil {
						log.Errorf("Failed to instantiate Rollout Client. Err: %v", err)
					}

					n.modeChange.Lock()

					n.cmd = cmdAPI
					n.rollout = roClient

					err = n.initTLSProvider()
					if err != nil {
						log.Errorf("Error initializing TLS provider: %v", err)
						return
					}
					n.modeChange.Unlock()

					go n.AdmitNaples()
				}
			}()
		}
	}

	return err
}

// DeleteSmartNIC deletes the local smartNIC object
func (n *NMD) DeleteSmartNIC(nic *cmd.SmartNIC) error {

	log.Infof("SmartNIC delete, mac: %s", nic.ObjectMeta.Name)

	// remove nic from DB
	n.SetSmartNIC(nil)
	err := n.store.Delete(nic)

	return err
}
