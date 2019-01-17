// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.
// watch and process telemetry policies

package tpm

import (
	"context"
	"fmt"
	"reflect"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	telemetry "github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/ctrler/tpm/rpcserver"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	debugStats "github.com/pensando/sw/venice/utils/debug/stats"
	"github.com/pensando/sw/venice/utils/kvstore"
	vLog "github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

// PolicyManager policy manager global info
type PolicyManager struct {
	// api client
	client apiclient.Services
	// cancel watch operation
	cancel context.CancelFunc
	// name server
	nsClient resolver.Interface
	// debug stats
	debugStats *debugStats.Stats
	// policyDB
	policyDb *memdb.Memdb
	// tpm rpc server
	rpcServer *rpcserver.PolicyRPCServer
}

const pkgName = "tpm"
const maxRetry = 15

// stats collection interval
const statsCollectionInterval = "30s"

var pmLog vLog.Logger

// NewPolicyManager creates a policy manager instance
func NewPolicyManager(listenURL string, nsClient resolver.Interface) (*PolicyManager, error) {

	pmLog = vLog.WithContext("pkg", pkgName)
	pm := &PolicyManager{nsClient: nsClient,
		policyDb: memdb.NewMemdb()}

	go pm.HandleEvents()
	server, err := rpcserver.NewRPCServer(listenURL, pm.policyDb, statsCollectionInterval)
	if err != nil {
		pmLog.Fatalf("failed to create rpc server, %s", err)
	}

	pm.rpcServer = server
	return pm, nil
}

// Stop shutdown policy watch
func (pm *PolicyManager) Stop() {
	pm.rpcServer.Stop()
	pm.cancel()
}

// HandleEvents handles policy events
func (pm *PolicyManager) HandleEvents() error {
	var err error

	ctx, cancel := context.WithCancel(context.Background())
	pm.cancel = cancel

	defer pm.cancel()

	for {
		if pm.client, err = pm.initGrpcClient(globals.APIServer, maxRetry); err != nil {
			pmLog.Fatalf("failed to init grpc client, error: %s", err)
		}
		pmLog.Infof("connected to {%s}", globals.APIServer)

		pm.processEvents(ctx)

		// close rpc services
		pm.client.Close()

		// context canceled, return
		if err := ctx.Err(); err != nil {
			pmLog.Warnf("policy watcher context error: %v, exit", err)
			return nil
		}
		time.Sleep(2 * time.Second)
	}
}

// init grpc client
func (pm *PolicyManager) initGrpcClient(serviceName string, retry int) (apiclient.Services, error) {
	for i := 0; i < retry; i++ {
		// create a grpc client
		client, apiErr := apiclient.NewGrpcAPIClient(globals.Tpm, serviceName, vLog.WithContext("pkg", "TPM-GRPC-API"),
			rpckit.WithBalancer(balancer.New(pm.nsClient)))
		if apiErr == nil {
			return client, nil
		}
		pmLog.Warnf("failed to connect to {%s}, error: %s, retry", globals.APIServer, apiErr)
		time.Sleep(2 * time.Second)
	}
	return nil, fmt.Errorf("failed to connect to {%s}, exhausted all attempts(%d)", serviceName, retry)
}

// processEvents watch & process telemetry policy events
func (pm *PolicyManager) processEvents(parentCtx context.Context) error {

	ctx, cancelWatch := context.WithCancel(parentCtx)
	// stop all watch channels
	defer cancelWatch()

	watchList := map[int]string{}
	opts := api.ListWatchOptions{}
	selCases := []reflect.SelectCase{}

	// stats
	watcher, err := pm.client.MonitoringV1().StatsPolicy().Watch(ctx, &opts)
	if err != nil {
		pmLog.Errorf("failed to watch stats policy, error: {%s}", err)
		return err
	}

	watchList[len(selCases)] = "stats"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// fwlog
	watcher, err = pm.client.MonitoringV1().FwlogPolicy().Watch(ctx, &opts)
	if err != nil {
		pmLog.Errorf("failed to watch fwlog policy, error: {%s}", err)
		return err
	}

	watchList[len(selCases)] = "fwlog"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// export
	watcher, err = pm.client.MonitoringV1().FlowExportPolicy().Watch(ctx, &opts)
	if err != nil {
		pmLog.Errorf("failed to watch export policy, error: {%s}", err)
		return err
	}

	watchList[len(selCases)] = "export"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// watch tenants
	watcher, err = pm.client.ClusterV1().Tenant().Watch(ctx, &opts)
	if err != nil {
		pmLog.Errorf("failed to watch tenant, error: {%s}", err)
		return err
	}

	watchList[len(selCases)] = "tenant"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// ctx done
	watchList[len(selCases)] = "ctx-canceled"
	selCases = append(selCases, reflect.SelectCase{Dir: reflect.SelectRecv, Chan: reflect.ValueOf(ctx.Done())})

	// event loop
	for {
		id, recVal, ok := reflect.Select(selCases)
		if !ok {
			pmLog.Errorf("{%s} channel closed", watchList[id])
			return fmt.Errorf("channel closed")
		}

		event, ok := recVal.Interface().(*kvstore.WatchEvent)
		if !ok {
			pmLog.Errorf("unknon policy object received from {%s}: %+v", watchList[id], recVal.Interface())
			return fmt.Errorf("unknown policy")
		}

		pmLog.Infof("received event %#v", event)

		switch polObj := event.Object.(type) {
		case *telemetry.StatsPolicy:
			pm.processStatsPolicy(event.Type, polObj)

		case *telemetry.FlowExportPolicy:
			pm.processExportPolicy(event.Type, polObj)

		case *telemetry.FwlogPolicy:
			pm.processFwlogPolicy(event.Type, polObj)

		case *cluster.Tenant:
			pm.processTenants(ctx, event.Type, polObj)

		default:
			pmLog.Errorf("invalid event type received from {%s}, %+v", watchList[id], event)
			return fmt.Errorf("invalid event type")
		}
	}
}

// process stats policy
func (pm *PolicyManager) processStatsPolicy(eventType kvstore.WatchEventType, policy *telemetry.StatsPolicy) error {
	pmLog.Infof("process stats policy event:{%s} {%#v} ", eventType, policy)

	switch eventType {
	case kvstore.Created:
		return pm.policyDb.AddObject(policy)

	case kvstore.Updated:
		return pm.policyDb.UpdateObject(policy)

	case kvstore.Deleted:
		return pm.policyDb.DeleteObject(policy)

	default:
		pmLog.Errorf("invalid stats event, type %s policy %+v", eventType, policy)
		return fmt.Errorf("invalid event")
	}
}

// process fwlog policy
func (pm *PolicyManager) processFwlogPolicy(eventType kvstore.WatchEventType, policy *telemetry.FwlogPolicy) error {
	pmLog.Infof("process fwlog policy event:{%s} {%#v} ", eventType, policy)

	switch eventType {
	case kvstore.Created:
		return pm.policyDb.AddObject(policy)

	case kvstore.Updated:
		return pm.policyDb.UpdateObject(policy)

	case kvstore.Deleted:
		return pm.policyDb.DeleteObject(policy)

	default:
		pmLog.Errorf("invalid fwlog event, type %s policy %+v", eventType, policy)
		return fmt.Errorf("invalid fwlog event")
	}
}

// process export policy
func (pm *PolicyManager) processExportPolicy(eventType kvstore.WatchEventType, policy *telemetry.FlowExportPolicy) error {
	pmLog.Infof("process export policy event:{%v} {%#v} ", eventType, policy)

	switch eventType {
	case kvstore.Created:
		return pm.policyDb.AddObject(policy)

	case kvstore.Updated:
		return pm.policyDb.UpdateObject(policy)

	case kvstore.Deleted:
		return pm.policyDb.DeleteObject(policy)

	default:
		pmLog.Errorf("invalid export event, type %s policy %+v", eventType, policy)
		return fmt.Errorf("invalid export event")
	}
}

// GetDefaultStatsSpec returns the default stats policy spec that is used
func GetDefaultStatsSpec() telemetry.StatsPolicySpec {
	defaultStatsSpec := telemetry.StatsPolicySpec{}
	defaultStatsSpec.Defaults("")
	return defaultStatsSpec
}

// process tenants
func (pm *PolicyManager) processTenants(ctx context.Context, eventType kvstore.WatchEventType, tenant *cluster.Tenant) error {
	pmLog.Infof("process tenant event:{%v} {%#v} ", eventType, tenant)

	switch eventType {
	case kvstore.Created:
		// create stats policy
		statsPolicy := &telemetry.StatsPolicy{
			ObjectMeta: api.ObjectMeta{
				Name:   tenant.GetName(),
				Tenant: tenant.GetName(),
			},
			Spec: GetDefaultStatsSpec(),
		}

		if _, err := pm.client.MonitoringV1().StatsPolicy().Get(ctx, &statsPolicy.ObjectMeta); err != nil {
			if _, err := pm.client.MonitoringV1().StatsPolicy().Create(ctx, statsPolicy); err != nil {
				pmLog.Errorf("failed to create stats policy for tenant %s, error: %s", tenant.GetName(), err)
				return err
			}
		}

	case kvstore.Updated: // no-op

	case kvstore.Deleted:
		// delete stats policy
		objMeta := &api.ObjectMeta{
			Name:   tenant.GetName(),
			Tenant: tenant.GetName(),
		}

		if _, err := pm.client.MonitoringV1().StatsPolicy().Delete(ctx, objMeta); err != nil {
			pmLog.Errorf("failed to delete stats policy for tenant %s, error: %s", tenant.GetName(), err)
			return err
		}

	default:
		return fmt.Errorf("invalid tenant event type %s", eventType)
	}

	return nil
}

// create/update database in Citadel
func (pm *PolicyManager) updateDatabase(tenantName string, dbName string,
	retentionTime string, downSampleRetention string) error {
	pmLog.Infof("update db tenant: %s, db: %s, retention: %s, dretention: %s",
		tenantName, dbName, retentionTime, downSampleRetention)
	return nil
}

// delete database in Citadel
func (pm *PolicyManager) deleteDatabase(tenantName string, dbName string) error {
	pmLog.Infof("delete db tenant: %s, db: %s, retention: %s, dretention: %s",
		tenantName, dbName)
	return nil
}

// Debug function dumps all policy cache for debug
func (pm *PolicyManager) Debug() interface{} {
	kind := []string{"StatsPolicy", "FwlogPolicy", "FlowExportPolicy"}
	dbgInfo := struct {
		Policy  map[string]map[string]memdb.Object
		Clients map[string]map[string]string
	}{
		Policy:  map[string]map[string]memdb.Object{},
		Clients: map[string]map[string]string{},
	}

	for _, key := range kind {
		dbgInfo.Policy[key] = map[string]memdb.Object{}

		pl := pm.policyDb.ListObjects(key)
		for _, p := range pl {
			dbgInfo.Policy[key][p.GetObjectMeta().GetName()] = p
		}
	}
	dbgInfo.Clients = pm.rpcServer.Debug()
	return dbgInfo
}
