package vospkg

import (
	"context"
	"crypto/tls"
	"fmt"
	"net/http"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	miniogo "github.com/minio/minio-go/v6"
	minio "github.com/minio/minio/cmd"
	"github.com/pkg/errors"

	vminio "github.com/pensando/sw/venice/utils/objstore/minio"

	"github.com/pensando/sw/api"
	apiintf "github.com/pensando/sw/api/interfaces"

	"github.com/pensando/sw/api/generated/objstore"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/log"
	minioclient "github.com/pensando/sw/venice/utils/objstore/minio/client"
	"github.com/pensando/sw/venice/utils/rpckit"
	"github.com/pensando/sw/venice/utils/tsdb"
	"github.com/pensando/sw/venice/utils/watchstream"
	"github.com/pensando/sw/venice/vos"
	"github.com/pensando/sw/venice/vos/plugins"
)

const (
	defaultLocation = "default"

	// Throttling is mainly used for firewall logs. Minio does not have the capability to add throttling for
	// specific buckets.
	// Assumptions:
	// Sustained rate: 500CPS/Card i.e. 500k logs per second entering into the venice cluster of 1000 DSCs
	// Each file (or batch) reported by card has maximum 6000 log entries
	// 500k/6000 = 83 requests/second entering into the Minio cluster
	// We have put a smaller number 60 then 83 for now, but we can change it accordingly.
	// This number gets set as an environment variable, so while testing it's possible to edit the kubespec
	// and add the MINIO_API_REQUESTS_MAX env variable and restart pen-vos daemonset.
	maxAPIRequests                 = 60
	cpuDivisionFactor              = 6
	defaultFlowlogsLifecycleConfig = `<LifecycleConfiguration><Rule><ID>expire-flowlogs</ID><Prefix></Prefix><Status>Enabled</Status>` +
		`<Expiration><Days>30</Days></Expiration></Rule></LifecycleConfiguration>`
	periodicDiskMonitorTime   = time.Minute * 30
	metaPrefix                = "X-Amz-Meta-"
	metaCreationTime          = "Creation-Time"
	metaFileName              = "file"
	metaContentType           = "content-type"
	fwlogsBucketName          = "fwlogs"
	diskUpdateWatchPath       = "diskupdates"
	disableNativeMinioBrowser = true
)

var (
	maxCreateBucketRetries = 1200
	maxSetLifecycleRetries = 1200

	// DiskPaths are the data folder locations for Minio
	DiskPaths = []string{"/disk1", "/disk2"}
)

// Option fills the optional params for Vos
type Option func(vos.Interface)

type instance struct {
	sync.RWMutex
	ctx                     context.Context
	cancel                  context.CancelFunc
	wg                      sync.WaitGroup
	pluginsMap              map[string]*pluginSet
	watcherMap              map[string]*storeWatcher
	store                   apiintf.Store
	pfxWatcher              watchstream.WatchedPrefixes
	client                  vos.BackendClient
	bootupArgs              []string
	bucketDiskThresholds    *sync.Map
	periodicDiskMonitorTime time.Duration
	metricsObjMap           map[string]tsdb.Obj
}

func (i *instance) Init(client vos.BackendClient) {
	defer i.Unlock()
	i.Lock()
	i.client = client
	i.pluginsMap = make(map[string]*pluginSet)
	l := log.WithContext("sub-module", "watcher")
	i.store = &storeImpl{client}
	i.pfxWatcher = watchstream.NewWatchedPrefixes(l, i.store, watchstream.WatchEventQConfig{})
	i.store = &storeImpl{client}
	i.watcherMap = make(map[string]*storeWatcher)
	i.ctx, i.cancel = context.WithCancel(context.Background())
	i.metricsObjMap = map[string]tsdb.Obj{}
}

func (i *instance) RegisterCb(bucket string, stage vos.OperStage, oper vos.ObjectOper, cb vos.CallBackFunc) {
	v, ok := i.pluginsMap[bucket]
	if !ok {
		v = newPluginSet(bucket)
		i.pluginsMap[bucket] = v
	}
	v.registerPlugin(stage, oper, cb)
}

func (i *instance) RunPlugins(ctx context.Context, bucket string, stage vos.OperStage, oper vos.ObjectOper, in *objstore.Object, client vos.BackendClient) []error {
	defer i.RUnlock()
	i.RLock()
	v, ok := i.pluginsMap[bucket]
	if ok {
		errs := v.RunPlugins(ctx, stage, oper, in, client)
		return errs
	}
	return nil
}

func (i *instance) Close() {
	i.cancel()
	i.wg.Wait()
}

func (i *instance) createDefaultBuckets(client vos.BackendClient) error {
	log.Infof("creating default buckets in minio")
	defer i.Unlock()
	i.Lock()
	loop := true
	retryCount := 0
	var err error
	for loop && retryCount < maxCreateBucketRetries {
		loop = false
		for _, n := range objstore.Buckets_name {
			name := "default." + strings.ToLower(n)

			if err = i.createBucket(name, true); err != nil {
				log.Errorf("create bucket [%v] failed retry [%d] (%s)", name, retryCount, err)
				loop = true
			}

			if strings.Compare(strings.ToLower(n), globals.FwlogsBucketName) == 0 {
				metaBucketName := "default." + "meta-" + strings.ToLower(n)
				if err = i.createBucket(metaBucketName, false); err != nil {
					log.Errorf("create bucket [%v] failed retry [%d] (%s)", metaBucketName, retryCount, err)
					loop = true
				}
			}
		}
		if loop {
			time.Sleep(500 * time.Millisecond)
			retryCount++
		}
	}
	if retryCount >= maxCreateBucketRetries {
		return errors.Wrap(err, "failed after max retries")
	}
	return nil
}

func (i *instance) createBucket(bucket string, addWatcher bool) error {
	ok, err := i.client.BucketExists(strings.ToLower(bucket))
	if err != nil {
		return errors.Wrap(err, "client error")
	}
	if !ok {
		err = i.client.MakeBucket(strings.ToLower(bucket), defaultLocation)
		if err != nil {
			return errors.Wrap(err, fmt.Sprintf("MakeBucket operation[%s]", bucket))
		}
	}
	if addWatcher {
		if _, ok := i.watcherMap[bucket]; !ok {
			watcher := &storeWatcher{bucket: bucket, client: i.client, watchPrefixes: i.pfxWatcher}
			i.watcherMap[bucket] = watcher
			i.wg.Add(1)
			go watcher.Watch(i.ctx, func() {
				i.Lock()
				bucket := bucket
				delete(i.watcherMap, bucket)
				i.Unlock()
				i.wg.Done()
			})
		}
	}

	return nil
}

func (i *instance) setBucketLifecycle() error {
	var err error
outer:
	for _, n := range objstore.Buckets_name {
		buckets := map[string]string{}
		if strings.Compare(strings.ToLower(n), globals.FwlogsBucketName) == 0 {
			buckets["default."+strings.ToLower(n)] = defaultFlowlogsLifecycleConfig
			buckets["default."+globals.FwlogsMetaBucketName] = defaultFlowlogsLifecycleConfig
		}

		for bucket, lc := range buckets {
			retryCount := 0
		forever:
			for {
				err = i.client.SetBucketLifecycleWithContext(i.ctx, bucket, lc)
				if err != nil {
					retryCount++
					log.Errorf("setBucketLifecycle [%v] failed retry [%d] (%s)", bucket, retryCount, err)
					if retryCount >= maxSetLifecycleRetries {
						break outer
					}
					continue
				}
				break forever
			}
		}
	}

	if err != nil {
		return errors.Wrap(err, "failed after max retries")
	}

	return nil
}

func (i *instance) createDiskUpdateWatcher(monitorConfig *sync.Map,
	monitorPeriod time.Duration, paths []string,
	flowlogsdiskThresholdCriticialEventDuration time.Duration) (context.CancelFunc, error) {
	watcher := &storeWatcher{bucket: "",
		client:        nil,
		watchPrefixes: i.pfxWatcher,
		flowlogsdiskThresholdCriticialEventDuration: flowlogsdiskThresholdCriticialEventDuration}
	i.watcherMap[diskUpdateWatchPath] = watcher
	i.wg.Add(1)
	monitorCtx, cancelFunc := context.WithCancel(i.ctx)
	go watcher.monitorDisks(monitorCtx, monitorPeriod, &i.wg, monitorConfig, paths)
	return cancelFunc, nil
}

func (i *instance) Watch(ctx context.Context,
	path, peer string, handleFn apiintf.EventHandlerFn,
	opts *api.ListWatchOptions) error {
	wq := i.pfxWatcher.Add(path, peer)
	cleanupFn := func() {
		i.pfxWatcher.Del(path, peer)
	}
	wq.Dequeue(ctx, 0, false, handleFn, cleanupFn, opts)
	return nil
}

// New creaate an instance of obj store
//  This starts Minio server and starts a HTTP server handling multipart forms used or
//  uploading files to the object store and a gRPC frontend that frontends all other operations
//  for the objectstore.
// testURL = url for minio server for testing
// Vos is setup differently when testURL is provided. For example, tls is not used whil testing and
// insecure minio connection is initialized.
func New(ctx context.Context, trace bool, testURL string, resolverURLs string, credentialsManagerChannel <-chan interface{}, opts ...Option) (vos.Interface, error) {
	inst := &instance{periodicDiskMonitorTime: periodicDiskMonitorTime}

	// Run options
	for _, opt := range opts {
		if opt != nil {
			opt(inst)
		}
	}

	updateGoMaxProcs()
	disableMinioBrowser()

	var credentialsManager vminio.CredentialsManager

	select {
	case item := <-credentialsManagerChannel:
		if mgr, ok := item.(vminio.CredentialsManager); ok {
			credentialsManager = mgr
		} else {
			log.Errorf("Unexpected item found in credentials manager channel, item: %v", item)
			return nil, errors.New("unexpected item found in credentials manager channel")
		}
	}

	minioCreds, err := credentialsManager.GetCredentials()
	if err != nil {
		log.Errorf("Unable to get MINIO credentials, error: %s", err.Error())
		return nil, errors.Wrap(err, "unable to get MINIO credentials")
	}

	os.Setenv("MINIO_ACCESS_KEY", minioCreds.AccessKey)
	os.Setenv("MINIO_SECRET_KEY", minioCreds.SecretKey)
	var envVars string
	// remove secrets out from log info
	for _, envVar := range os.Environ() {
		if !strings.Contains(envVar, "MINIO_ACCESS_KEY") &&
			!strings.Contains(envVar, "MINIO_SECRET_KEY") {
			envVars = fmt.Sprintf("%s %s", envVars, envVar)
		}
	}
	log.Infof("minio env: %s", envVars)

	if trace {
		os.Setenv("MINIO_HTTP_TRACE", "/dev/stdout")
		log.Infof("minio enabled API tracing")
	}

	updateAPIThrottlingParams()

	log.Infof("minio args:  %+v", inst.bootupArgs)

	go minio.Main(inst.bootupArgs)
	time.Sleep(10 * time.Second)

	url := k8s.GetPodIP() + ":" + globals.VosMinioPort
	if testURL != "" {
		url = testURL + ":" + globals.VosMinioPort
	}

	log.Infof("connecting to minio at [%v]", url)

	secureMinio := true
	if testURL != "" {
		secureMinio = false
	}
	mclient, err := miniogo.New(url, minioCreds.AccessKey, minioCreds.SecretKey, secureMinio)
	if err != nil {
		log.Errorf("Failed to create client (%s)", err)
		return nil, errors.Wrap(err, "Failed to create Client")
	}
	defTr := http.DefaultTransport.(*http.Transport)
	tlcc, tlsc, err := getTLSConfig(testURL, false)
	if err != nil {
		return nil, err
	}
	defTr.TLSClientConfig = tlcc

	itlcc, _, err := getTLSConfig(testURL, true)
	if err != nil {
		return nil, err
	}

	mclient.SetCustomTransport(defTr)
	client := &storeImpl{BaseBackendClient: mclient}
	inst.Init(client)

	adminClient, err := minioclient.NewPinnedAdminClient(url,
		minioclient.WithCredentialsManager(credentialsManager), minioclient.WithTLSConfig(itlcc))

	if err != nil {
		log.Errorf("Failed to create admin client (%s)", err)
		return nil, errors.Wrap(err, "Failed to create admin client")
	}

	go monitorAndRecoverMinio(ctx, adminClient)

	grpcBackend, err := newGrpcServer(inst, client)
	if err != nil {
		return nil, errors.Wrap(err, "failed to start grpc listener")
	}

	httpBackend, err := newHTTPHandler(inst, client, adminClient)
	if err != nil {
		return nil, errors.Wrap(err, "failed to start http listener")
	}

	// For simplicity all nodes in the cluster check if the default buckets exist,
	//  if not, try to create the buckets. all nodes in the cluster try this till
	//  all default buckets are created. A little inefficient but simple and a rare
	//  operation (only on a create of a new cluster)
	err = inst.createDefaultBuckets(client)
	if err != nil {
		log.Errorf("failed to create buckets (%+v)", err)
		return nil, errors.Wrap(err, "failed to create buckets")
	}

	for {
		online, err := adminClient.IsClusterOnline(ctx)
		if err != nil {
			log.Errorf("minio cluster is not online yet (%+v)", err)
			continue
		}

		if online {
			err := inst.setBucketLifecycle()
			if err != nil {
				log.Errorf("failed to set bucket lifecycle (%+v)", err)
				return nil, errors.Wrap(err, "failed to set bucket lifecycle")
			}
			break
		}
	}

	_, err = inst.createDiskUpdateWatcher(inst.bucketDiskThresholds,
		inst.periodicDiskMonitorTime, DiskPaths, flowlogsDiskThresholdCriticalEventReportingPeriod)
	if err != nil {
		log.Errorf("failed to start disk watcher (%+v)", err)
		return nil, errors.Wrap(err, "failed to start disk watcher")
	}

	// Register all plugins
	plugins.RegisterPlugins(inst)
	grpcBackend.start(ctx)
	httpBackend.start(ctx, globals.VosHTTPPort, minioCreds.AccessKey, minioCreds.SecretKey, tlsc)
	inst.initTsdbClient(resolverURLs, minioCreds.AccessKey, minioCreds.SecretKey)
	log.Infof("Initialization complete")
	<-ctx.Done()
	return inst, nil
}

func updateGoMaxProcs() {
	cdf := cpuDivisionFactor
	cpuDivisionFactorEnv, ok := os.LookupEnv("CPU_DIVISION_FACTOR")
	if ok {
		log.Infof("cpuDivisionFactorEnv %s", cpuDivisionFactorEnv)
		temp, err := strconv.Atoi(cpuDivisionFactorEnv)
		if err != nil {
			log.Infof("error while parsing cpuDivisionFactorEnv %s, %+v", cpuDivisionFactorEnv, err)
		}
		if temp > 0 && temp <= 10 {
			cdf = temp
		}
	}
	numCPU := runtime.NumCPU()
	log.Infof("NumCPUs seen by this container %d", numCPU)
	if numCPU <= cdf {
		oldValue := runtime.GOMAXPROCS(1)
		log.Infof("GOMAXPROCS old value %d", oldValue)
	} else {
		oldValue := runtime.GOMAXPROCS(numCPU / cdf)
		log.Infof("GOMAXPROCS old value %d", oldValue)
	}
	log.Infof("GOMAXPROCS new value %d", runtime.GOMAXPROCS(-1))
}

func disableMinioBrowser() {
	// Set the variable only if its not overridden in the kubespec
	_, ok := os.LookupEnv("MINIO_BROWSER")
	if !ok {
		if disableNativeMinioBrowser {
			os.Setenv("MINIO_BROWSER", "off")
			log.Infof("disabled native minio browser: %s", os.Getenv("MINIO_BROWSER"))
		}
	}
}

func updateAPIThrottlingParams() {
	requestsMax, ok := os.LookupEnv("MINIO_API_REQUESTS_MAX")
	if !ok {
		os.Setenv("MINIO_API_REQUESTS_MAX", strconv.Itoa(maxAPIRequests))
	} else {
		os.Setenv("MINIO_API_REQUESTS_MAX", requestsMax)
	}
	log.Infof("minio enabled API throttling: %s", os.Getenv("MINIO_API_REQUESTS_MAX"))
}

// WithBootupArgs sets the args to bootup Minio
func WithBootupArgs(args []string) func(vos.Interface) {
	return func(i vos.Interface) {
		if inst, ok := i.(*instance); ok {
			inst.bootupArgs = args
		}
	}
}

// WithBucketDiskThresholds sets the disk threshold for Minio buckets
func WithBucketDiskThresholds(th *sync.Map) func(vos.Interface) {
	return func(i vos.Interface) {
		if inst, ok := i.(*instance); ok {
			inst.bucketDiskThresholds = th
		}
	}
}

// WithDiskMonitorDuration sets custom disk monitor time
func WithDiskMonitorDuration(t time.Duration) func(vos.Interface) {
	return func(i vos.Interface) {
		if inst, ok := i.(*instance); ok {
			inst.periodicDiskMonitorTime = t
		}
	}
}

// GetBucketDiskThresholds returns the bucket disk thresholds
func GetBucketDiskThresholds() *sync.Map {
	// Threshold set to -1 will force dynamic threshold calculation based on current disk size
	// Dynamic threshold calculation is needed for supporting dynamic disk expansion.
	// Debug API "/debug/config" can be used for overriding threshold percent.
	m := new(sync.Map)
	c := newDiskMonitorConfig("", float64(-1), "fwlogs", "meta-fwlogs")
	m.Store("", c)
	return m
}

func recoverMinioHelper(ctx context.Context, mAdminC minioclient.AdminClient) error {
	log.Infof("recovering minio")

	// 1. Delete backend_encrypted and config.json from all disks
	// 2. Restart the services
	for _, p := range DiskPaths {
		path := p + "/.minio.sys/backend_encrypted"
		if err := os.RemoveAll(path); err != nil {
			log.Errorf("error in removing path %s", path)
		}

		path = p + "/.minio.sys/config/config.json"
		if err := os.RemoveAll(path); err != nil {
			log.Errorf("error in removing path %s", path)
		}
	}
	return mAdminC.ServiceRestart(ctx)
}

func monitorAndRecoverMinio(ctx context.Context, mAdminC minioclient.AdminClient) {
	log.Infof("starting monitoring & recovery routine")
	errCount := 0
	maxErrCount := 3
	for {
		select {
		case <-ctx.Done():
			return
		case <-time.After(30 * time.Second):
			_, err := mAdminC.ClusterInfo(ctx)
			if err == nil {
				continue
			}

			log.Errorf("minio cluster error %+v", err)
			if strings.Contains(err.Error(), "Storage resources are insufficient for the read operation") {
				errCount++
				if errCount == maxErrCount {
					errCount = 0
					err := recoverMinioHelper(ctx, mAdminC)
					if err != nil {
						log.Errorf("error in recovering minio %+v", err)
					}
				}
			}
		}
	}
}

func getTLSConfig(testURL string, insecure bool) (*tls.Config, *tls.Config, error) {
	var tlsc *tls.Config
	if testURL == "" {
		tlsp, err := rpckit.GetDefaultTLSProvider(globals.Vos)
		if err != nil {
			log.Errorf("failed to get tls provider (%s)", err)
			return nil, nil, errors.Wrap(err, "failed GetDefaultTLSProvider()")
		}

		tlsClientConfig, err := tlsp.GetClientTLSConfig(globals.Vos)
		if err != nil {
			log.Errorf("failed to get client tls config (%s)", err)
			return nil, nil, errors.Wrap(err, "client tls config")
		}

		tlsc, err = tlsp.GetServerTLSConfig(globals.Vos)
		if err != nil {
			log.Errorf("failed to get tls config (%s)", err)
			return nil, nil, errors.Wrap(err, "failed GetDefaultTLSProvider()")
		}
		tlsc.ServerName = globals.Vos
		tlsc.InsecureSkipVerify = insecure
		return tlsClientConfig, tlsc, nil
	}
	return nil, nil, nil
}
