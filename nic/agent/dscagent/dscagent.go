// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package dscagent

import (
	"os"
	"sync"

	"github.com/pkg/errors"

	"github.com/pensando/sw/venice/utils/events"
	"github.com/pensando/sw/venice/utils/events/recorder"

	"github.com/pensando/sw/nic/agent/dscagent/controller"
	"github.com/pensando/sw/nic/agent/dscagent/infra"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/venice/utils/log"
)

// DSCAgent implements unified config agents
type DSCAgent struct {
	sync.Mutex
	ControllerAPI types.ControllerAPI
	PipelineAPI   types.PipelineAPI
	InfraAPI      types.InfraAPI
	Logger        log.Logger
	EvtsRecorder  events.Recorder
}

// NewDSCAgent returns a new instance of DSCAgent
func NewDSCAgent(logger log.Logger, npmURL, tpmURL, tsmURL, restURL string) (*DSCAgent, error) {
	var controllerAPI types.ControllerAPI
	evtsRecorder, err := recorder.NewRecorder(&recorder.Config{
		Component: types.Netagent}, logger)
	if err != nil {
		log.Errorf("failed to create events recorder, err: %v", err)
	}
	infraAPI, err := infra.NewInfraAPI(types.NetagentPrimaryDBPath, types.NetagentBackupDBPath)
	if err != nil {
		return nil, err
	}

	pipelineAPI, err := pipeline.NewPipelineAPI(infraAPI)
	if err != nil {
		return nil, err
	}

	// TODO controller needs to be pipeline aware to default to specific watch kinds
	pipelineKind, _ := os.LookupEnv("NAPLES_PIPELINE")
	if pipelineKind == "apollo" {
		controllerAPI = controller.NewControllerAPI(pipelineAPI, types.CloudPipelineKinds, infraAPI, npmURL, restURL)
	} else {
		controllerAPI = controller.NewControllerAPI(pipelineAPI, types.BaseNetKinds, infraAPI, npmURL, restURL)
	}

	d := DSCAgent{
		PipelineAPI:   pipelineAPI,
		ControllerAPI: controllerAPI,
		InfraAPI:      infraAPI,
		Logger:        logger,
		EvtsRecorder:  evtsRecorder,
	}

	// Ensure that the controller API is registered with the pipeline
	log.Infof("Controller API: %v", d.ControllerAPI)
	d.PipelineAPI.RegisterControllerAPI(d.ControllerAPI)

	return &d, nil
}

// Stop stops DSCAgent
func (ag *DSCAgent) Stop() {
	if ag.Logger != nil {
		ag.Logger.Close()
	}

	if ag.EvtsRecorder != nil {
		ag.EvtsRecorder.Close()
	}

	if err := ag.ControllerAPI.Stop(); err != nil {
		log.Error(errors.Wrapf(types.ErrControllerWatcherStop, "Controller API: %s", err))
	}

	ag.InfraAPI.Close()

	if ag.Logger != nil {
		ag.Logger.Close()
	}
}
