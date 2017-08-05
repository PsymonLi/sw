// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package agent

import (
	log "github.com/Sirupsen/logrus"

	"github.com/pensando/sw/agent/netagent"
	"github.com/pensando/sw/agent/netagent/ctrlerif"
)

/* Rough Architecture for Pensando Agent
 * -------------------------------------------
 *
 *
 *
 *               +----------------------+----------------------+
 *               | REST Handler         |  gRPC handler        |
 *               +----------------------+----------------------+
 *                                      |
 *                            Controller Interface
 *                                      |
 *                                      V
 *  +---------+  +---------------------------------------------+
 *  |  K8s    +->+                    NetAgent                 |
 *  | Plugin  |  |                                    +--------+----+
 *  +---------+  |                                    | Embedded DB |
 *  +---------+  |                 Core Objects       +--------+----+
 *  | Docker  +->+                                             |
 *  +---------+  +-----------------------+---------------------+
 *                                       |
 *                               Datapath Interface
 *                                       |
 *                                       V
 *               +-----------------------+---------------------+
 *               |  Fake Datapath        |    HAL Interface    |
 *               +-----------------------+---------------------+
 */

// Agent contains agent state
type Agent struct {
	datapath  netagent.NetDatapathAPI
	Netagent  *netagent.NetAgent
	npmClient *ctrlerif.NpmClient
}

// NewAgent creates an agent instance
func NewAgent(dp netagent.NetDatapathAPI, ctrlerURL string) (*Agent, error) {

	// create new network agent
	nagent, err := netagent.NewNetAgent(dp)
	if err != nil {
		log.Errorf("Error creating network agent. Err: %v", err)
		return nil, err
	}

	// create the NPM client
	npmClient, err := ctrlerif.NewNpmClient(nagent, ctrlerURL)
	if err != nil {
		log.Errorf("Error creating NPM client. Err: %v", err)
		return nil, err
	}

	log.Infof("NPM client {%+v} is running", npmClient)

	// create the agent instance
	ag := Agent{
		Netagent:  nagent,
		datapath:  dp,
		npmClient: npmClient,
	}

	return &ag, nil
}

// Stop stops the agent
func (ag *Agent) Stop() {
	ag.npmClient.Stop()
}
