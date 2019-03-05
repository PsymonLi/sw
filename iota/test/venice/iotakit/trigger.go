// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package iotakit

import (
	"context"
	"fmt"

	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/svcs/common"
	"github.com/pensando/sw/venice/utils/log"
)

// Trigger is an instance of trigger
type Trigger struct {
	cmds       []*iota.Command
	tb         *TestBed
	iotaClient *common.GRPCClient
}

// NewTrigger returns a new trigger instance
func (tb *TestBed) NewTrigger() *Trigger {
	return &Trigger{
		cmds:       []*iota.Command{},
		tb:         tb,
		iotaClient: tb.iotaClient,
	}
}

// AddCommand adds a command to trigger
func (tr *Trigger) AddCommand(command, entity, node string) error {
	cmd := iota.Command{
		Mode:       iota.CommandMode_COMMAND_FOREGROUND,
		Command:    command,
		EntityName: entity,
		NodeName:   node,
	}
	tr.cmds = append(tr.cmds, &cmd)
	return nil
}

// AddBackgroundCommand adds a background command
func (tr *Trigger) AddBackgroundCommand(command, entity, node string) error {
	cmd := iota.Command{
		Mode:       iota.CommandMode_COMMAND_BACKGROUND,
		Command:    command,
		EntityName: entity,
		NodeName:   node,
	}
	tr.cmds = append(tr.cmds, &cmd)
	return nil
}

// Run runs trigger commands serially within a node but, parallely across nodes
func (tr *Trigger) Run() ([]*iota.Command, error) {
	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: iota.TriggerMode_TRIGGER_PARALLEL,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    tr.cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tr.iotaClient.Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return nil, fmt.Errorf("Trigger failed. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	log.Debugf("Got Trigger resp: %+v", triggerResp)

	return triggerResp.Commands, nil
}

// RunParallel runs all commands parallely
func (tr *Trigger) RunParallel() ([]*iota.Command, error) {
	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: iota.TriggerMode_TRIGGER_NODE_PARALLEL,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    tr.cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tr.iotaClient.Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return nil, fmt.Errorf("Trigger failed. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	log.Debugf("Got Trigger resp: %+v", triggerResp)

	return triggerResp.Commands, nil
}

// RunSerial runs commands serially
func (tr *Trigger) RunSerial() ([]*iota.Command, error) {
	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_EXEC_CMDS,
		TriggerMode: iota.TriggerMode_TRIGGER_SERIAL,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    tr.cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tr.iotaClient.Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return nil, fmt.Errorf("Trigger failed. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	log.Debugf("Got Trigger resp: %+v", triggerResp)

	return triggerResp.Commands, nil
}

// StopCommands stop all commands using previously returned command handle
func (tr *Trigger) StopCommands(cmds []*iota.Command) ([]*iota.Command, error) {
	trigMsg := &iota.TriggerMsg{
		TriggerOp:   iota.TriggerOp_TERMINATE_ALL_CMDS,
		TriggerMode: iota.TriggerMode_TRIGGER_PARALLEL,
		ApiResponse: &iota.IotaAPIResponse{},
		Commands:    cmds,
	}

	// Trigger App
	topoClient := iota.NewTopologyApiClient(tr.iotaClient.Client)
	triggerResp, err := topoClient.Trigger(context.Background(), trigMsg)
	if err != nil || triggerResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return nil, fmt.Errorf("Trigger failed. API Status: %+v | Err: %v", triggerResp.ApiResponse, err)
	}

	log.Debugf("Got StopCommands Trigger resp: %+v", triggerResp)

	return triggerResp.Commands, nil
}

// CopyToHost copies a file to host
func (tb *TestBed) CopyToHost(nodeName string, files []string, destDir string) error {
	// copy message
	copyMsg := iota.EntityCopyMsg{
		Direction:   iota.CopyDirection_DIR_IN,
		NodeName:    nodeName,
		EntityName:  nodeName + "_host",
		Files:       files,
		DestDir:     destDir,
		ApiResponse: &iota.IotaAPIResponse{},
	}

	// send it to iota
	topoClient := iota.NewTopologyApiClient(tb.iotaClient.Client)
	copyResp, err := topoClient.EntityCopy(context.Background(), &copyMsg)
	if err != nil {
		log.Errorf("Copy failed: Err: %v", err)
		return fmt.Errorf("Copy files failed.  Err: %v", err)
	} else if copyResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return fmt.Errorf("Copy files failed. API Status: %+v ", copyResp.ApiResponse)
	}

	log.Debugf("Got Copy resp: %+v", copyResp)

	return nil
}

// CopyFromHost copies a file from host
func (tb *TestBed) CopyFromHost(nodeName string, files []string, destDir string) error {
	// copy message
	copyMsg := iota.EntityCopyMsg{
		Direction:   iota.CopyDirection_DIR_OUT,
		NodeName:    nodeName,
		EntityName:  nodeName + "_host",
		Files:       files,
		DestDir:     destDir,
		ApiResponse: &iota.IotaAPIResponse{},
	}

	// send it to iota
	topoClient := iota.NewTopologyApiClient(tb.iotaClient.Client)
	copyResp, err := topoClient.EntityCopy(context.Background(), &copyMsg)
	if err != nil {
		log.Errorf("Copy failed: Err: %v", err)
		return fmt.Errorf("Copy files failed.  Err: %v", err)
	} else if copyResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Copy failed: Resp: %v", copyResp)
		return fmt.Errorf("Copy files failed. API Status: %+v ", copyResp.ApiResponse)
	}

	log.Debugf("Got Copy resp: %+v", copyResp)

	return nil
}

// CopyFromNaples copies files from naples
func (tb *TestBed) CopyFromNaples(nodeName string, files []string, destDir string) error {
	// copy message
	copyMsg := iota.EntityCopyMsg{
		Direction:   iota.CopyDirection_DIR_OUT,
		NodeName:    nodeName,
		EntityName:  nodeName + "_naples",
		Files:       files,
		DestDir:     destDir,
		ApiResponse: &iota.IotaAPIResponse{},
	}

	// send it to iota
	topoClient := iota.NewTopologyApiClient(tb.iotaClient.Client)
	copyResp, err := topoClient.EntityCopy(context.Background(), &copyMsg)
	if err != nil {
		log.Errorf("Copy failed: Err: %v", err)
		return fmt.Errorf("Copy files failed.  Err: %v", err)
	} else if copyResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Copy failed: Resp: %v", copyResp)
		return fmt.Errorf("Copy files failed. API Status: %+v ", copyResp.ApiResponse)
	}

	log.Debugf("Got Copy resp: %+v", copyResp)

	return nil
}

// CopyFromVenice copies a file from venice node
func (tb *TestBed) CopyFromVenice(nodeName string, files []string, destDir string) error {
	// copy message
	copyMsg := iota.EntityCopyMsg{
		Direction:   iota.CopyDirection_DIR_OUT,
		NodeName:    nodeName,
		EntityName:  nodeName + "_venice",
		Files:       files,
		DestDir:     destDir,
		ApiResponse: &iota.IotaAPIResponse{},
	}

	// send it to iota
	topoClient := iota.NewTopologyApiClient(tb.iotaClient.Client)
	copyResp, err := topoClient.EntityCopy(context.Background(), &copyMsg)
	if err != nil {
		log.Errorf("Copy failed: Err: %v", err)
		return fmt.Errorf("Copy files failed.  Err: %v", err)
	} else if copyResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Copy failed: Resp: %v", copyResp)
		return fmt.Errorf("Copy files failed. API Status: %+v ", copyResp.ApiResponse)
	}

	log.Debugf("Got Copy resp: %+v", copyResp)

	return nil
}
