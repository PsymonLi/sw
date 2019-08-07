// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"path/filepath"

	"github.com/pensando/sw/venice/ctrler/ckm"
	"github.com/pensando/sw/venice/ctrler/ckm/rpcserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
)

// main function of the Cluster Key Manager
func main() {

	var (
		debugflag       = flag.Bool("debug", false, "Enable debug mode")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
		logToFile       = flag.String("logtofile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.Ckm)), "Redirect logs to file")
	)
	flag.Parse()

	// Fill logger config params
	logConfig := &log.Config{
		Module:      globals.Ckm,
		Format:      log.JSONFmt,
		Filter:      log.AllowAllFilter,
		Debug:       *debugflag,
		CtxSelector: log.ContextAll,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
		FileCfg: log.FileConfig{
			Filename:   *logToFile,
			MaxSize:    10,
			MaxBackups: 3,
			MaxAge:     7,
		},
	}

	// Initialize logger config
	logger := log.SetConfig(logConfig)
	defer logger.Close()

	// create a dummy channel to wait forever
	waitCh := make(chan bool)

	// TODO: This needs to be part of Service Config Object
	keyStoreDir, err := ioutil.TempDir("", "ckm")
	if err != nil {
		log.Fatal(err)
	}

	// create the controller
	ctrlr, err := ckm.NewCKMctrler(rpcserver.CKMctrlerURL, keyStoreDir)
	if err != nil {
		log.Fatalf("Error creating controller instance: %v", err)
	}

	log.Infof("%s is running {%+v}", globals.Ckm, ctrlr)

	// wait forever
	<-waitCh
}
