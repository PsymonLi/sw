package rest

import (
	"context"
	"encoding/json"
	"expvar"
	"net/http"
	"net/http/pprof"
	"path"

	"github.com/go-martini/martini"

	cmd "github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/cmd/env"
	"github.com/pensando/sw/venice/cmd/installer"
	"github.com/pensando/sw/venice/cmd/ops"
	"github.com/pensando/sw/venice/cmd/services"
	"github.com/pensando/sw/venice/cmd/utils"
	"github.com/pensando/sw/venice/globals"
	// Import utils/debug pkg to publish runtime stats as part of its pkg init
	_ "github.com/pensando/sw/venice/utils/debug"
	"github.com/pensando/sw/venice/utils/errors"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
)

// constants used by REST interface
const (
	uRLPrefix   = "/api/v1"
	clusterURL  = "/cluster"
	servicesURL = "/services"
	debugPrefix = "/debug"
	expvarURL   = "/vars"
)

// NewRESTServer creates REST server endpoints for cluster create/get. These ops
// are handled directly by CMD before the cluster is created. Once the cluster
// is created, all ops come through API Gateway over gRPC.
func NewRESTServer() *martini.ClassicMartini {
	m := martini.Classic()

	m.Post(uRLPrefix+clusterURL, ClusterCreateHandler)
	m.Get(uRLPrefix+clusterURL+"/:id", ClusterGetHandler)
	m.Get(uRLPrefix+servicesURL, ServiceListHandler)
	m.Get(uRLPrefix+"/debugSrvUpgrade", DebugUpgradeHandler)      // TODO: Remove after upgrade development is complete
	m.Get(uRLPrefix+"/debugNodeUpgrade", DebugNodeUpgradeHandler) // TODO: Remove after upgrade development is complete
	m.Get(debugPrefix+expvarURL, expvar.Handler())

	m.Group("/debug/pprof", func(r martini.Router) {
		r.Any("/", pprof.Index)
		r.Any("/cmdline", pprof.Cmdline)
		r.Any("/profile", pprof.Profile)
		r.Any("/symbol", pprof.Symbol)
		r.Any("/trace", pprof.Trace)
		r.Any("/block", pprof.Handler("block").ServeHTTP)
		r.Any("/heap", pprof.Handler("heap").ServeHTTP)
		r.Any("/mutex", pprof.Handler("mutex").ServeHTTP)
		r.Any("/goroutine", pprof.Handler("goroutine").ServeHTTP)
		r.Any("/threadcreate", pprof.Handler("threadcreate").ServeHTTP)
	})

	return m
}

// ClusterCreateHandler handles the REST call for cluster creation.
func ClusterCreateHandler(w http.ResponseWriter, req *http.Request) {
	env.Mutex.Lock()
	defer env.Mutex.Unlock()

	decoder := json.NewDecoder(req.Body)
	defer req.Body.Close()

	cluster := cmd.Cluster{}
	if err := decoder.Decode(&cluster); err != nil {
		errors.SendBadRequest(w, err.Error())
		return
	}

	log.Infof("Cluster create args: %+v", cluster)

	ops.RunHTTP(w, ops.NewClusterCreateOp(&cluster))
}

// ClusterGetHandler returns the cluster information.
func ClusterGetHandler(w http.ResponseWriter, params martini.Params) {
	cluster := cmd.Cluster{}
	id := params["id"]

	if id == "" {
		errors.SendNotFound(w, "Cluster", "")
		return
	}

	if env.KVStore == nil {
		errors.SendNotFound(w, "Cluster", "")
		return
	}

	if err := env.KVStore.Get(context.Background(), path.Join(globals.ClusterKey, id), &cluster); err != nil {
		if kvstore.IsKeyNotFoundError(err) {
			errors.SendNotFound(w, "Cluster", id)
			return
		}
		errors.SendInternalError(w, err)
		return
	}

	cluster.Status.Leader = env.LeaderService.Leader()

	encoder := json.NewEncoder(w)

	if err := encoder.Encode(&cluster); err != nil {
		log.Errorf("Failed to encode with error: %v", err)
	}
}

// ServiceListHandler returns the services running in the cluster.
func ServiceListHandler(w http.ResponseWriter, req *http.Request) {
	if env.ResolverService == nil {
		errors.SendNotFound(w, "ServiceList", "")
		return
	}

	encoder := json.NewEncoder(w)

	if err := encoder.Encode(env.ResolverService.List()); err != nil {
		log.Errorf("Failed to encode with error: %v", err)
	}
}

// DebugUpgradeHandler is a debug handler during development of upgrade
func DebugUpgradeHandler(w http.ResponseWriter, req *http.Request) {
	// read the file for the updated list of services
	services.ContainerInfoMap = utils.GetContainerInfo()
	err := env.K8sService.UpgradeServices(utils.GetUpgradeOrder())
	log.Debugf("UpgradeServices returned %s", err)
}

// DebugNodeUpgradeHandler is a debug handler for installing+upgrade of node services
func DebugNodeUpgradeHandler(w http.ResponseWriter, req *http.Request) {

	imageName, err := installer.DownloadImage("ignore")
	log.Infof("DownloadImage returned %s %v", imageName, err)
	err = installer.ExtractImage(imageName)
	log.Infof("extractImage returned %v ", err)
	err = installer.PreLoadImage()
	log.Infof("preLoadImage returned %v ", err)
	err = installer.LoadAndInstallImage()
	log.Infof("loadAndInstallImage returned %v ", err)
}
