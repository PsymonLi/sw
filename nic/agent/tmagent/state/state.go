package state

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/nic/agent/netagent/protos/netproto"
	"github.com/pensando/sw/venice/ctrler/tpm/rpcserver/protos"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/emstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/netutils"
	"github.com/pensando/sw/venice/utils/syslog"
	"github.com/pensando/sw/venice/utils/tsdb"
)

const maxRetry = 5
const maxCollectorsInVrf = 4

// PolicyState keeps the policy agent state
type PolicyState struct {
	ctx             context.Context
	cancel          context.CancelFunc
	emstore         emstore.Emstore
	netAgentURL     string
	fwLogCollectors sync.Map
	fwTable         tsdb.Obj
	hostName        string
	appName         string
	wg              sync.WaitGroup
}

type fwlogCollector struct {
	sync.Mutex
	vrf         uint64
	port        string
	proto       string
	destination string
	facility    syslog.Priority
	filter      uint32
	format      string
	syslogFd    syslog.Writer
}

// NewTpAgent creates new telemetry policy agent state
func NewTpAgent(pctx context.Context, agentPort string) (*PolicyState, error) {

	s, err := emstore.NewEmstore(emstore.MemStoreType, "")
	if err != nil {
		log.Errorf("failed to create db, %s", err)
		return nil, err
	}

	ctx, cancel := context.WithCancel(pctx)

	state := &PolicyState{
		ctx:             ctx,
		cancel:          cancel,
		emstore:         s,
		netAgentURL:     "http://127.0.0.1:" + agentPort,
		fwLogCollectors: sync.Map{},
	}

	state.connectSyslog()
	return state, nil
}

// Close frees all resources
func (s *PolicyState) Close() {
	s.cancel()
	// stop reconnect thread
	s.wg.Wait()
	// close db
	s.emstore.Close()

	// get all vrfs
	vrfList := []uint64{}
	s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {
		if key, ok := k.(uint64); ok {
			vrfList = append(vrfList, key)
		}
		return true
	})

	// delete all vrfs
	for _, k := range vrfList {
		s.deleteCollectors(k)
	}
}

// reconnect to syslog collectors
func (s *PolicyState) connectSyslog() error {
	s.wg.Add(1)
	go func() {
		defer s.wg.Done()
		for {
			select {
			case <-time.After(time.Second):
				s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {

					if c, ok := v.(*fwlogCollector); ok {
						c.Lock()
						if c.syslogFd == nil {
							// reconnect to collector that was never connected or had write error
							err := s.newSyslog(c)
							log.Infof("reconnect to collector %s:%s:%s, err:%v", c.proto, c.destination, c.port, err)
						}
						c.Unlock()
					} else {
						log.Errorf("invalid collector")
					}
					return true
				})

			case <-s.ctx.Done():
				log.Infof("stop connectSyslog thread")
				return

			}
		}
	}()

	return nil
}

// create a hash key to flatten collector config
func (s *PolicyState) getCollectorKey(vrf uint64, policy *tpmprotos.FwlogPolicy, m monitoring.ExportConfig) string {
	// keys that requires new connection to collector
	return fmt.Sprintf("%d:%s:%s:%s:%s:%s:%s", vrf, policy.ObjectMeta.Name, policy.Spec.Format, policy.Spec.Filter, policy.Spec.Config.FacilityOverride, m.Destination, m.Transport)
}

// get vrf from netagent
func (s *PolicyState) getvrf(tenant, namespace, vrfName string) (uint64, error) {
	var err error
	reqURL := fmt.Sprintf("%s/api/vrfs/", s.netAgentURL)

	// find vrf from netagent
	var vrfList []netproto.Vrf
	// TODO to use native netagent state's ValidateVrf method instead of repeated curls.
	if len(vrfName) == 0 {
		// Set the name to default. If we move to using ValidateVrf from netagent, it is automatically handled there.
		tenant = globals.DefaultTenant
		namespace = globals.DefaultVrf
		vrfName = globals.DefaultVrf

	}
	for i := 0; i < maxRetry; i++ {
		err = netutils.HTTPGet(reqURL, &vrfList)
		if err == nil {
			for _, vrf := range vrfList {
				if vrfName == vrfName && vrf.Tenant == tenant && vrf.Namespace == namespace {
					return vrf.Status.VrfID, nil
				}
			}
		}
		time.Sleep(time.Millisecond * 100)
	}

	return 0, fmt.Errorf("GET [%s], %s", reqURL, err)
}

// connect to syslog server
func (s *PolicyState) newSyslog(c *fwlogCollector) error {
	facility := syslog.LogUser // default facility
	if c.facility != 0 {
		facility = c.facility
	}

	priority := syslog.LogInfo // default priority

	switch c.format {
	case monitoring.MonitoringExportFormat_SYSLOG_BSD.String():
		fd, err := syslog.NewBsd(strings.ToLower(c.proto), fmt.Sprintf("%s:%s", c.destination, c.port), facility|priority, "mytag")
		if err != nil {
			return err
		}
		c.syslogFd = fd

	case monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String():
		fd, err := syslog.NewRfc5424(strings.ToLower(c.proto), fmt.Sprintf("%s:%s", c.destination, c.port), facility, s.hostName, s.appName)
		if err != nil {
			return err
		}
		c.syslogFd = fd

	default:
		return fmt.Errorf("invalid syslog format %v", c.format)
	}
	return nil
}

func (s *PolicyState) closeSyslog(c *fwlogCollector) {
	if c.syslogFd != nil {
		c.syslogFd.Close()
		c.syslogFd = nil
	}
}

// validate policy, can be received from REST/venice
func (s *PolicyState) validateFwLogPolicy(p *tpmprotos.FwlogPolicy) error {
	numColl := make(map[string]bool)

	// set default
	if p.Tenant == "" {
		p.Tenant = globals.DefaultTenant
	}

	if p.Namespace == "" {
		p.Namespace = globals.DefaultNamespace
	}

	if len(p.Spec.Targets) == 0 {
		return fmt.Errorf("no collectors configured")
	}

	for _, c := range p.Spec.Targets {
		if c.Destination == "" {
			return fmt.Errorf("destination can't be empty")
		}

		netIP := net.ParseIP(c.Destination)
		if netIP == nil {
			// treat it as hostname and resolve
			_, err := net.LookupHost(c.Destination)
			if err != nil {
				return fmt.Errorf("failed to resolve name {%s}, error: %s", c.Destination, err)
			}
		}

		tr := strings.Split(c.Transport, "/")
		if len(tr) != 2 {
			return fmt.Errorf("invalid protocol/port in %s", c.Transport)
		}

		if _, ok := map[string]bool{
			"tcp": true,
			"udp": true,
		}[strings.ToLower(tr[0])]; !ok {
			return fmt.Errorf("invalid protocol in %+v", c)
		}

		port, err := strconv.Atoi(tr[1])
		if err != nil {
			return fmt.Errorf("invalid port in %+v", c)
		}

		if uint(port) > uint(^uint16(0)) {
			return fmt.Errorf("invalid port (> %d) in %+v", ^uint16(0), c)
		}
		k := s.getCollectorKey(0, p, c)
		if _, ok := numColl[k]; ok {
			return fmt.Errorf("duplicate collector config %+v", c)
		}
		numColl[k] = true
	}

	if len(numColl) > maxCollectorsInVrf {
		return fmt.Errorf("too many collectors(%d) in %s", len(numColl), p.Name)
	}

	return nil
}

// get bitmap for the firewall action
func (s *PolicyState) getFilter(actions []string) uint32 {
	filter := uint32(0)

	for _, f := range actions {
		if f == monitoring.FwlogFilter_FIREWALL_ACTION_ALL.String() {
			// set all bits
			filter |= 0xFFFF

		} else {
			// set bit
			filter |= 1 << uint32(monitoring.FwlogFilter_value[f])
		}
	}

	return filter
}

// get collectors with matching vrf
func (s *PolicyState) getCollector(vrf uint64) (map[string]*fwlogCollector, bool) {
	c := map[string]*fwlogCollector{}

	s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {
		if key, ok := k.(string); ok {
			if val, ok := v.(*fwlogCollector); ok {
				val.Lock()
				if val.vrf == vrf {
					c[key] = val
				}
				val.Unlock()
			}
		}
		return true
	})

	if len(c) == 0 {
		return nil, false
	}

	return c, true
}

// delete collectors in a vrf
func (s *PolicyState) deleteCollectors(vrf uint64) error {
	delList := map[string]bool{}

	s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {
		if key, ok := k.(string); ok {
			if c, ok := v.(*fwlogCollector); ok {
				c.Lock()
				if c.vrf == vrf {
					s.closeSyslog(c)
					delList[key] = true
				}
				c.Unlock()
			} else {
				log.Errorf("invalid collector data")
			}
		} else {
			log.Errorf("invalid collector key")
		}
		return true
	})

	// remove keys
	for k := range delList {
		s.fwLogCollectors.Delete(k)
	}

	return nil
}

// CreateFwlogPolicy is the POST() entry point
func (s *PolicyState) CreateFwlogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) (err error) {
	if err = s.validateFwLogPolicy(p); err != nil {
		return err
	}

	if _, err := s.emstore.Read(p); err == nil {
		log.Warnf("policy %s/%s already exists", p.Kind, p.Name)
		return fmt.Errorf("policy %s already exists", p.Name)
	}

	vrf, err := s.getvrf(p.Tenant, p.Namespace, p.Spec.VrfName)
	if err != nil {
		return fmt.Errorf("failed to get vrf for %s/%s, %s", p.Tenant, p.Namespace, err)
	}

	filter := s.getFilter(p.Spec.Filter)
	for _, target := range p.Spec.Targets {
		key := s.getCollectorKey(vrf, p, target)
		transport := strings.Split(target.Transport, "/")
		fwcollector := &fwlogCollector{
			vrf:         vrf,
			filter:      filter,
			format:      p.Spec.Format,
			facility:    syslog.Priority(monitoring.SyslogFacility_value[p.Spec.Config.FacilityOverride]),
			destination: target.Destination,
			proto:       transport[0],
			port:        transport[1],
		}

		// we 'll keep the config & connect from the goroutine to avoid blocking
		s.fwLogCollectors.Store(key, fwcollector)
	}

	if err := s.emstore.Write(p); err != nil {
		s.DeleteFwlogPolicy(ctx, p)
		return fmt.Errorf("failed to save policy, %s", err)
	}

	return nil
}

// UpdateFwlogPolicy is the PUT entry point
func (s *PolicyState) UpdateFwlogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) error {
	if err := s.validateFwLogPolicy(p); err != nil {
		return err
	}

	if _, err := s.emstore.Read(p); err != nil {
		return fmt.Errorf("policy %s doesn't exist", p.Name)
	}

	vrf, err := s.getvrf(p.Tenant, p.Namespace, p.Spec.VrfName)
	if err != nil {
		return fmt.Errorf("failed to get tenant for %s/%s", p.Tenant, p.Namespace)
	}

	newCollector := make(map[string]*fwlogCollector)
	filter := s.getFilter(p.Spec.Filter)
	for _, target := range p.Spec.Targets {
		key := s.getCollectorKey(vrf, p, target)
		transport := strings.Split(target.Transport, "/")
		newCollector[key] = &fwlogCollector{
			vrf:         vrf,
			filter:      filter,
			format:      p.Spec.Format,
			facility:    syslog.Priority(monitoring.SyslogFacility_value[p.Spec.Config.FacilityOverride]),
			destination: target.Destination,
			proto:       transport[0],
			port:        transport[1],
		}
	}

	delList := map[string]bool{}
	// walk thru existing collectors
	s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {
		key := k.(string)
		col := v.(*fwlogCollector)
		col.Lock()
		if ncol, ok := newCollector[key]; ok {
			col.filter = ncol.filter
			delete(newCollector, key)
		} else {
			// remove it
			s.closeSyslog(col)
			delList[key] = true
		}
		col.Unlock()
		return true
	})

	// delete
	for k := range delList {
		s.fwLogCollectors.Delete(k)
	}

	// add new collectors
	for key := range newCollector {
		// connect() is done by connectSyslog() to avoid blocking
		s.fwLogCollectors.Store(key, newCollector[key])
	}

	if err := s.emstore.Write(p); err != nil {
		s.DeleteFwlogPolicy(ctx, p)
		return fmt.Errorf("failed to save policy, %s", err)
	}

	return nil
}

// DeleteFwlogPolicy is the DELETE entry point
func (s *PolicyState) DeleteFwlogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) error {
	// set default
	if p.Tenant == "" {
		p.Tenant = globals.DefaultTenant
	}

	if p.Namespace == "" {
		p.Namespace = globals.DefaultNamespace
	}

	if _, err := s.emstore.Read(p); err != nil {
		return fmt.Errorf("policy %s doesn't exist", p.Name)
	}

	vrf, err := s.getvrf(p.Tenant, p.Namespace, p.Spec.VrfName)
	if err != nil {
		return fmt.Errorf("failed to get vrf for %s/%s", p.Tenant, p.Namespace)
	}

	delList := map[string]bool{}
	for _, target := range p.Spec.Targets {
		key := s.getCollectorKey(vrf, p, target)
		delList[key] = true
	}

	// Close syslogs that will be deleted
	s.fwLogCollectors.Range(func(k interface{}, v interface{}) bool {
		key := k.(string)
		col := v.(*fwlogCollector)
		col.Lock()
		if _, ok := delList[key]; ok {
			// remove it
			s.closeSyslog(col)
		}
		col.Unlock()
		return true
	})

	// delete
	for k := range delList {
		s.fwLogCollectors.Delete(k)
	}

	s.emstore.Delete(p)
	return nil
}

// GetFwlogPolicy is the GET entry point
func (s *PolicyState) GetFwlogPolicy(tx context.Context, p *tpmprotos.FwlogPolicy) (*tpmprotos.FwlogPolicy, error) {

	obj, err := s.emstore.Read(p)
	if err != nil {
		return nil, err
	}
	if pol, ok := obj.(*tpmprotos.FwlogPolicy); ok {
		return pol, nil
	}
	return nil, fmt.Errorf("failed to find policy")
}

// ListFwlogPolicy is the LIST all entry point
func (s *PolicyState) ListFwlogPolicy(tx context.Context) ([]*tpmprotos.FwlogPolicy, error) {
	fwlogPol := []*tpmprotos.FwlogPolicy{}

	objList, err := s.emstore.List(&tpmprotos.FwlogPolicy{
		TypeMeta: api.TypeMeta{
			Kind: "fwLogPolicy",
		},
	})

	if err != nil {
		return fwlogPol, nil
	}

	for _, obj := range objList {
		pol, ok := obj.(*tpmprotos.FwlogPolicy)
		if !ok {
			log.Errorf("invalid fwlog policy type %+v", obj)
			return fwlogPol, nil
		}
		fwlogPol = append(fwlogPol, pol)
	}
	return fwlogPol, nil
}

// send fwlog to collector
func (s *PolicyState) sendFwLog(c *fwlogCollector, data map[string]string) {
	if c.format == monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String() {
		if err := c.syslogFd.Info(&syslog.Message{
			MsgID: data["rule-id"], // set rule-id
			StructuredData: syslog.StrData{
				"firewall@Pensando": data,
			},
		}); err != nil {
			s.closeSyslog(c)
		}
	} else {
		d, err := json.Marshal(data)
		if err != nil {
			log.Errorf("marshal error [%v] %s", data, err)
			return
		}

		if err := c.syslogFd.Info(&syslog.Message{
			MsgID: data["rule-id"], // set rule-id
			Msg:   string(d),
		}); err != nil {
			s.closeSyslog(c)
		}
	}
}

// dummy functions. these polcies are handled in netagent

// CreateFlowExportPolicy is the POST() entry point
func (s *PolicyState) CreateFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) (err error) {
	return nil
}

// UpdateFlowExportPolicy is the PUT entry point
func (s *PolicyState) UpdateFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) error {
	return nil
}

// DeleteFlowExportPolicy is the DELETE entry point
func (s *PolicyState) DeleteFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) error {
	return nil
}

// GetFlowExportPolicy is the GET entry point
func (s *PolicyState) GetFlowExportPolicy(tx context.Context, p *tpmprotos.FlowExportPolicy) (*tpmprotos.FlowExportPolicy, error) {

	return nil, nil
}

// ListFlowExportPolicy is the LIST all entry point
func (s *PolicyState) ListFlowExportPolicy(tx context.Context) ([]*tpmprotos.FlowExportPolicy, error) {
	return []*tpmprotos.FlowExportPolicy{}, nil
}

// Debug is the debug entry point from REST
func (s *PolicyState) Debug(w http.ResponseWriter, r *http.Request) {
}
