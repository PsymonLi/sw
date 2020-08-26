package apiclient

import (
	"context"
	"fmt"
	"reflect"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/ctrler/turret/memdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/balancer"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/rpckit"
)

var (
	// maxRetries maximum number of retries for creating client.
	maxRetries = 10

	// delay between retries
	retryDelay = 2 * time.Second
)

// ConfigWatcher represents the api client that connects and watches config objects.
type ConfigWatcher struct {
	sync.Mutex
	resolverClient resolver.Interface
	apiClient      apiclient.Services // api client
	memDb          *memdb.MemDb       // in-memory db/cache
	wg             sync.WaitGroup
	ctx            context.Context
	cancel         context.CancelFunc
	logger         log.Logger
}

// NewConfigWatcher creates a new config watcher
func NewConfigWatcher(parentCtx context.Context, resolverClient resolver.Interface, memDb *memdb.MemDb,
	logger log.Logger) *ConfigWatcher {
	ctx, cancel := context.WithCancel(parentCtx)
	return &ConfigWatcher{
		ctx:            ctx,
		cancel:         cancel,
		resolverClient: resolverClient,
		memDb:          memDb,
		logger:         logger}
}

// APIClient returns the API client
func (c *ConfigWatcher) APIClient() apiclient.Services {
	return c.apiClient
}

// helper function to create api
func (c *ConfigWatcher) apiCl() apiclient.Services {
	logger := c.logger.WithContext("pkg", fmt.Sprintf("%s-%s", globals.Turret, "alert-engine-api-client"))
	cl, err := apiclient.NewGrpcAPIClient(globals.Turret, globals.APIServer, logger,
		rpckit.WithBalancer(balancer.New(c.resolverClient)), rpckit.WithLogger(logger))
	if err != nil {
		c.logger.Errorf("failed to create API client {API server URLs from resolver: %v}, err: %v",
			c.resolverClient.GetURLs(globals.APIServer), err)
		return nil
	}
	return cl
}

// Stop stops the watcher
func (c *ConfigWatcher) Stop() {
	c.Lock()
	defer c.Unlock()

	c.cancel()
	c.wg.Wait()

	if c.apiClient != nil {
		c.apiClient.Close()
		c.apiClient = nil
	}
}

// StartWatcher starts watching config objects and updates the memdb to be in sync with API server
func (c *ConfigWatcher) StartWatcher() {
	c.wg.Add(1)
	go func() {
		defer c.wg.Done()
		c.logger.Infof("starting config watcher")
		if err := c.startWatcher(); err != nil {
			c.logger.Errorf("exiting config watcher, err: %v", err)
		}
	}()
}

func (c *ConfigWatcher) startWatcher() error {
	for {
		if err := c.ctx.Err(); err != nil {
			return err
		}

		cl := c.apiCl()
		if cl == nil {
			time.Sleep(retryDelay)
			continue
		}

		if err := c.ctx.Err(); err != nil {
			return err
		}

		c.Lock()
		c.apiClient = cl
		c.Unlock()

		c.logger.Infof("created API server client: %p", c.apiClient)

		c.processEvents(c.ctx)
		c.apiClient.Close()
		if err := c.ctx.Err(); err != nil {
			return err
		}

		time.Sleep(retryDelay)
	}
}

// processEvents helper function to handle the API server watch events
func (c *ConfigWatcher) processEvents(parentCtx context.Context) error {
	ctx, cancelWatch := context.WithCancel(parentCtx)
	defer cancelWatch()

	watchList := map[int]string{}
	var selCases []reflect.SelectCase

	// watch stats policy
	watcher, err := c.apiClient.MonitoringV1().StatsAlertPolicy().Watch(ctx, &api.ListWatchOptions{FieldChangeSelector: []string{"Spec"}})
	if err != nil {
		c.logger.Errorf("failed to watch stats alert policy, err: %v", err)
		return err
	}
	watchList[len(selCases)] = "StatsAlertPolicy"
	selCases = append(selCases, reflect.SelectCase{
		Dir:  reflect.SelectRecv,
		Chan: reflect.ValueOf(watcher.EventChan())})

	// ctx done
	watchList[len(selCases)] = "ctx-canceled"
	selCases = append(selCases, reflect.SelectCase{Dir: reflect.SelectRecv, Chan: reflect.ValueOf(ctx.Done())})

	// event loop
	for {
		id, recVal, ok := reflect.Select(selCases)
		if id == len(selCases)-1 { // ctx-canceled
			c.logger.Errorf("context canceled, stopping the watchers")
			return fmt.Errorf("context canceled")
		}

		if !ok {
			c.logger.Errorf("{%s} channel closed", watchList[id])
			return fmt.Errorf("channel closed")
		}

		event, ok := recVal.Interface().(*kvstore.WatchEvent)
		if !ok {
			c.logger.Errorf("unknown object received from {%s}: %+v", watchList[id], recVal.Interface())
			return fmt.Errorf("unknown object received")
		}

		var err error
		c.logger.Infof("received watch event %#v", event)
		switch obj := event.Object.(type) {
		case *monitoring.StatsAlertPolicy:
			if err = c.processStatsAlertPolicy(event.Type, obj); err != nil {
				c.logger.Errorf("[StatsAlertPolicy] failed to add/update/delete memDb, err: %v", err)
			}
		default:
			c.logger.Errorf("invalid watch event type received from {%s}, %+v", watchList[id], event)
			return fmt.Errorf("invalid watch event type")
		}

		if err != nil {
			return err
		}
	}
}

// helper to process stats alert policies
func (c *ConfigWatcher) processStatsAlertPolicy(eventType kvstore.WatchEventType, statsAlertPolicy *monitoring.StatsAlertPolicy) error {
	c.logger.Debugf("processing stats alert policy watch event: {%s} {%#v} ", eventType, statsAlertPolicy)
	switch eventType {
	case kvstore.Created:
		return c.memDb.AddObject(statsAlertPolicy)
	case kvstore.Updated:
		return c.memDb.UpdateObject(statsAlertPolicy)
	case kvstore.Deleted:
		return c.memDb.DeleteObject(statsAlertPolicy)
	default:
		c.logger.Errorf("invalid stats alert policy watch event, type %s policy %+v", eventType, statsAlertPolicy)
		return fmt.Errorf("invalid stats alert policy watch event")
	}
}
