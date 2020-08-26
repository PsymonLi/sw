package turret

import (
	"context"
	"errors"
	"fmt"
	"sync"
	"time"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/generated/telemetry_query"
	"github.com/pensando/sw/venice/ctrler/turret/alertengine"
	"github.com/pensando/sw/venice/ctrler/turret/apiclient"
	alertmemdb "github.com/pensando/sw/venice/ctrler/turret/memdb"
	"github.com/pensando/sw/venice/ctrler/turret/policyhdr"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

var (
	maxConnRetries = 20
)

//
// StatsAlertMgr is responsible for watching alert policies from memdb and creating policy handlers for them.
// There is a policy handler created for each stats alert policy. And, it is responsible for converting
// the policy to metrics query, passing the metrics query result to alert engine for further action (
// create/update/resolve).
//

// StatsAlertMgr represents the stats alert manager.
type StatsAlertMgr struct {
	sync.RWMutex
	memDb            *alertmemdb.MemDb                        // in-memory db/cache
	configWatcher    *apiclient.ConfigWatcher                 // api client
	resolverClient   resolver.Interface                       // resolver
	metricsRPCClient *rpckit.RPCClient                        // RPC client to connect to the metrics/telemetry service
	metricsClient    telemetry_query.TelemetryV1Client        // metrics/telemetry query service client
	policyHandlers   map[string]*policyhdr.StatsPolicyHandler // map of policy id to handlers
	alertEngine      *alertengine.StatsAlertEngine            // stats alert engine
	logger           log.Logger                               // logger
	ctx              context.Context                          // context to cancel goroutines
	cancelFunc       context.CancelFunc                       // context to cancel goroutines
	wg               sync.WaitGroup
}

// AlertsGCConfig contains GC related config
// Note: this needs to be moved to a common place once we start supporting object-based & threshold-based alerts
type AlertsGCConfig struct {
	Interval                      time.Duration // garbage collect every x seconds
	ResolvedAlertsRetentionPeriod time.Duration // delete resolved alerts after this retention period
}

// Stop stops the stats alert manager.
func (alertmgr *StatsAlertMgr) Stop() {
	alertmgr.logger.Infof("stopping stats alert mgr")
	alertmgr.cancelFunc()
	alertmgr.wg.Wait()

	// stop all the policy handlers
	alertmgr.Lock()
	for _, ph := range alertmgr.policyHandlers {
		ph.Stop()
		delete(alertmgr.policyHandlers, ph.Name())
	}
	alertmgr.Unlock()

	// stop alert engine
	if alertmgr.alertEngine != nil {
		alertmgr.alertEngine.Stop()
		alertmgr.alertEngine = nil
	}

	// stop config watcher
	if alertmgr.configWatcher != nil {
		alertmgr.configWatcher.Stop()
		alertmgr.configWatcher = nil
	}

	// close the metrics client
	if alertmgr.metricsRPCClient != nil {
		alertmgr.metricsRPCClient.Close()
		alertmgr.metricsRPCClient = nil
	}

	alertmgr.logger.Infof("stopped stats alert mgr")
}

// Watches the stats alert policies by establishing a watch with memdb.
func (alertmgr *StatsAlertMgr) watchStatsAlertPolicies() {
	alertmgr.logger.Infof("starting stats alert policy watcher (memdb)")
	defer alertmgr.wg.Done()

	watcher := alertmgr.memDb.WatchStatsAlertPolicy()
	defer alertmgr.memDb.StopWatchStatsAlertPolicy(watcher)

	for {
		select {
		case <-alertmgr.ctx.Done():
			alertmgr.logger.Infof("context cancelled; exiting from stats policy watcher (memdb)")
			return
		case evt, ok := <-watcher.Channel:
			if !ok {
				alertmgr.logger.Errorf("error reading stats alert policy from the memdb channel, exiting")
				return
			}
			sapObj := evt.Obj.(*monitoring.StatsAlertPolicy)
			alertmgr.handleStatsAlertPolicy(evt.EventType, sapObj)
		}
	}
}

// Creates/Deletes the policy handler based on the watch event type. Currently, we do not support
// updates on the stats alert policy. Any update on the policy will have to go through delete + create.
func (alertmgr *StatsAlertMgr) handleStatsAlertPolicy(evtType memdb.EventType, sap *monitoring.StatsAlertPolicy) {
	alertmgr.Lock()
	defer alertmgr.Unlock()

	policyName := fmt.Sprintf("%s/%s", sap.GetName(), sap.GetUUID())
	switch evtType {
	case memdb.CreateEvent: // create new policy handler
		ph, err := policyhdr.NewStatsPolicyHandler(alertmgr.ctx, policyName, sap, alertmgr.metricsClient, alertmgr.alertEngine, alertmgr.logger.WithContext("stats-pol-handler", policyName))
		if err != nil {
			alertmgr.logger.Errorf("failed to create stats policy handler, err: %v", err)
			return
		}
		ph.Start()
		alertmgr.policyHandlers[policyName] = ph
		alertmgr.logger.Infof("created stats policy handler for the policy: %s", policyName)
	case memdb.DeleteEvent: // delete existing policy handler
		if ph, found := alertmgr.policyHandlers[policyName]; found {
			ph.Stop()
			delete(alertmgr.policyHandlers, policyName)
			alertmgr.logger.Infof("deleted existing stats policy handler for the policy: %s", policyName)
		}
	case memdb.UpdateEvent:
		alertmgr.logger.Errorf("update operation not supported for stats alert policy, " +
			"but received an update event. something went wrong")
	}
}

// open a connection to citadel service with retries
func (alertmgr *StatsAlertMgr) createMetricClient() error {
	rpcOptions := []rpckit.Option{
		rpckit.WithLogger(alertmgr.logger),
		rpckit.WithBalancer(balancer.New(alertmgr.resolverClient))}

	client, err := utils.ExecuteWithRetry(func(ctx context.Context) (interface{}, error) {
		return rpckit.NewRPCClient("statsalertmgr", globals.Citadel, rpcOptions...)
	}, 2*time.Second, maxConnRetries)
	if err != nil {
		return err
	}

	alertmgr.metricsRPCClient = client.(*rpckit.RPCClient)
	alertmgr.metricsClient = telemetry_query.NewTelemetryV1Client(alertmgr.metricsRPCClient.ClientConn)
	return nil
}

// NewStatsAlertMgr creates the new stats alert mgr with the given parameters.
func NewStatsAlertMgr(parentCtx context.Context, memDb *alertmemdb.MemDb,
	resolverClient resolver.Interface, logger log.Logger) (*StatsAlertMgr, error) {
	if nil == logger {
		return nil, errors.New("all parameters are required")
	}

	ctx, cancelFunc := context.WithCancel(parentCtx)
	statsAlertMgr := &StatsAlertMgr{
		memDb:          memDb,
		resolverClient: resolverClient,
		policyHandlers: make(map[string]*policyhdr.StatsPolicyHandler),
		ctx:            ctx,
		cancelFunc:     cancelFunc,
		logger:         logger,
	}

	// create metrics client
	if err := statsAlertMgr.createMetricClient(); err != nil {
		statsAlertMgr.logger.Errorf("failed to create metrics RPC client, err: %v", err)
		return nil, err
	}

	statsAlertMgr.alertEngine = alertengine.NewStatsAlertEngine(ctx, statsAlertMgr.memDb, resolverClient, logger)
	statsAlertMgr.configWatcher = apiclient.NewConfigWatcher(ctx, resolverClient, statsAlertMgr.memDb, logger)

	// start watcher to do CRUD operation on memDB for obj from apiserver's obj
	statsAlertMgr.configWatcher.StartWatcher()

	// start watcher to watch stats alert policy from local memDB copy
	statsAlertMgr.wg.Add(1)
	go statsAlertMgr.watchStatsAlertPolicies()

	return statsAlertMgr, nil
}
