package module

import (
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	objstore "github.com/pensando/sw/venice/utils/objstore/client"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"

	. "github.com/pensando/sw/venice/utils/authn/testutils"

	_ "github.com/pensando/sw/api/generated/exports/apiserver"
)

const (
	clientName = "DiagnosticsTest"
)

type tInfo struct {
	apiSrv     apiserver.Server
	apiSrvAddr string
	apicl      apiclient.Services
	objstorecl objstore.Client
	logger     log.Logger
}

func (ti *tInfo) setup() {
	var err error

	config := log.GetDefaultConfig(clientName)
	config.Filter = log.AllowAllFilter
	ti.logger = log.GetNewLogger(config)
	recorder.Override(mockevtsrecorder.NewRecorder(clientName, ti.logger))

	// api server
	ti.apiSrv, ti.apiSrvAddr, err = serviceutils.StartAPIServer("localhost:0", clientName, ti.logger, []string{})
	if err != nil {
		panic("Unable to start API Server")
	}

	// api server client
	ti.apicl, err = apiclient.NewGrpcAPIClient(clientName, ti.apiSrvAddr, ti.logger)
	if err != nil {
		panic("Error creating api client")
	}
	// create cluster
	MustCreateCluster(ti.apicl)
	// create tenant
	MustCreateTenant(ti.apicl, globals.DefaultTenant)

}

func (ti *tInfo) shutdown() {
	MustDeleteTenant(ti.apicl, globals.DefaultTenant)
	MustDeleteCluster(ti.apicl)
	// stop api server
	ti.apiSrv.Stop()
}
