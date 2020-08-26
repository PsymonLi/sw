package main

import (
	"context"
	"flag"
	"fmt"
	"path/filepath"
	"strings"

	"github.com/pensando/sw/venice/ctrler/turret"
	"github.com/pensando/sw/venice/ctrler/turret/memdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

var (
	debugflag       = flag.Bool("debug", false, "Enable debug mode")
	logToFile       = flag.String("logtofile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.Turret)), "Redirect logs to file")
	logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
)

func main() {
	// Logger config.
	logConfig := &log.Config{
		Module:      globals.Turret,
		Format:      log.JSONFmt,
		Filter:      log.AllowInfoFilter,
		Debug:       *debugflag,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
		CtxSelector: log.ContextAll,
		FileCfg: log.FileConfig{
			Filename:   *logToFile,
			MaxSize:    10,
			MaxBackups: 3,
			MaxAge:     7,
		},
	}

	logger := log.SetConfig(logConfig)
	defer logger.Close()

	// Create resolver client.
	rslvr := resolver.New(&resolver.Config{
		Name:    "turret",
		Servers: strings.Split(*flag.String("resolver-addrs", ":"+globals.CMDResolverPort, "comma separated list of resolver URLs <IP:Port>"), ",")},
	)

	// Create the controller.
	ctx := context.Background()
	amgr, err := turret.NewStatsAlertMgr(ctx, memdb.NewMemDb(), rslvr, logger)
	if err != nil {
		log.Fatalf("Failed to create threshold alertmgr, err %v", err)
	}
	defer amgr.Stop()
	<-ctx.Done()
}
