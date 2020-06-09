// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/objstore/minio"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
	vospkg "github.com/pensando/sw/venice/vos/pkg"
)

var pkgName = globals.Vos

func main() {

	var (
		resolverURLs = flag.String("resolver-urls", ":"+globals.CMDResolverPort,
			"comma separated list of resolver URLs of the form 'ip:port'")
		clusterNodes    = flag.String("cluster-nodes", "", "comma seperated list of cluster nodes")
		logFile         = flag.String("logfile", fmt.Sprintf("%s.log", filepath.Join(globals.LogDir, globals.Vos)), "redirect logs to file")
		logToStdoutFlag = flag.Bool("logtostdout", false, "enable logging to stdout")
		debugFlag       = flag.Bool("debug", false, "enable debug mode")
		traceAPI        = flag.Bool("trace", false, "enable trace of APIs to stdout")
	)

	flag.Parse()

	// Fill logger config params
	logConfig := &log.Config{
		Module:      pkgName,
		Format:      log.JSONFmt,
		Filter:      log.AllowAllFilter,
		Debug:       *debugFlag,
		CtxSelector: log.ContextAll,
		LogToStdout: *logToStdoutFlag,
		LogToFile:   true,
		FileCfg: log.FileConfig{
			Filename:   *logFile,
			MaxSize:    10,
			MaxBackups: 3,
			MaxAge:     7,
		},
	}

	// Initialize logger config
	logger := log.SetConfig(logConfig)
	defer logger.Close()

	log.Infof("resolver-urls %+v", resolverURLs)
	log.Infof("cluster-nodes %v", *clusterNodes)
	log.Infof("starting object store with args : {%+v}", os.Args)
	nodes := strings.Split(*clusterNodes, ",")
	sort.Strings(nodes)
	endpoints := []string{}
	for _, h := range nodes {
		endpoints = append(endpoints, fmt.Sprintf("https://%s:%s"+vospkg.DiskPaths[0], h, globals.VosMinioPort))
		endpoints = append(endpoints, fmt.Sprintf("https://%s:%s"+vospkg.DiskPaths[1], h, globals.VosMinioPort))
	}
	localNode := k8s.GetPodIP()
	args := []string{}
	if len(nodes) > 1 {
		args = []string{pkgName,
			"server", "--address", fmt.Sprintf("%s:%s", localNode, globals.VosMinioPort)}
		args = append(args, endpoints...)
	} else {
		args = []string{pkgName,
			"server", "--address", fmt.Sprintf("%s:%s", localNode, globals.VosMinioPort), vospkg.DiskPaths[0]}
	}

	log.Infof("args for starting Minio %v", args)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	credsMgrChannel := make(chan interface{}, 1)
	go waitForAPIServerAndGetMinioCredsManager(resolverURLs, credsMgrChannel)

	// init obj store
	_, err := vospkg.New(ctx, *traceAPI, "",
		credsMgrChannel,
		vospkg.WithBootupArgs(args),
		vospkg.WithBucketDiskThresholds(vospkg.GetBucketDiskThresholds()))

	if err != nil {
		// let the scheduler restart obj store
		log.Fatalf("failed to init object store, %s", err)
	}

	select {}
}

func waitForAPIServerAndGetMinioCredsManager(resolverURLs *string, credsMgrChannel chan<- interface{}) {
	for {
		select {
		case <-time.After(5 * time.Second):
			resolver := resolver.New(&resolver.Config{Name: globals.Vos, Servers: strings.Split(*resolverURLs, ",")})
			apisrvURL := globals.APIServer
			// create the api client
			l := log.WithContext("Pkg", "VeniceObjectStore")
			apiClient, err := apiclient.NewGrpcAPIClient(globals.Vos, apisrvURL, l, rpckit.WithBalancer(balancer.New(resolver)))
			if err != nil {
				log.Errorf("Failed to connect to api gRPC server [%s], retrying...\n", apisrvURL)
				continue
			}
			credsMgrChannel <- minio.NewAPIServerBasedCredsManager(apiClient.ClusterV1(), minio.WithAPIRetries(5, 5*time.Second))
		}
	}
}
