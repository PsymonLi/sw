package types

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
	tstype "github.com/pensando/sw/nic/agent/troubleshooting/state/types"
	"github.com/pensando/sw/venice/ctrler/tpm/rpcserver/protos"
)

// CtrlerIntf provides all CRUD operations on telemetry policy objects
type CtrlerIntf interface {
	CreateFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) error
	GetFlowExportPolicy(tx context.Context, p *tpmprotos.FlowExportPolicy) (*tpmprotos.FlowExportPolicy, error)
	ListFlowExportPolicy(tx context.Context) ([]*tpmprotos.FlowExportPolicy, error)
	UpdateFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) error
	DeleteFlowExportPolicy(ctx context.Context, p *tpmprotos.FlowExportPolicy) error

	CreateFwLogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) error
	GetFwLogPolicy(tx context.Context, p *tpmprotos.FwlogPolicy) (*tpmprotos.FwlogPolicy, error)
	ListFwLogPolicy(tx context.Context) ([]*tpmprotos.FwlogPolicy, error)
	UpdateFwLogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) error
	DeleteFwLogPolicy(ctx context.Context, p *tpmprotos.FwlogPolicy) error
	Debug(w http.ResponseWriter, r *http.Request)
}

// CollectorKey is the key to collector hash table
type CollectorKey struct {
	// VrfID verf id for the collector
	VrfID uint64
	// Interval in seconds
	Interval uint32
	// Format is the export format, IPFIX only now
	Format halproto.ExportFormat
	// Destination is the collector address
	Destination string
	// Protocol is tcp/udp
	Protocol halproto.IPProtocol
	// Port is the port number
	Port uint32
}

// String function converts key to string
func (c *CollectorKey) String() string {
	return fmt.Sprintf("%d:%d:%d:%s:%d:%d", c.VrfID, c.Interval, c.Format, c.Destination, c.Protocol, c.Port)
}

// ParseCollectorKey parses string to collector key, used for debug
func ParseCollectorKey(buff string) CollectorKey {
	var c CollectorKey
	fmt.Sscanf(buff, "%d:%d:%d:%s:%d:%d", &c.VrfID, &c.Interval, &c.Format, &c.Destination, &c.Protocol, &c.Port)
	return c
}

// CollectorData is the data stored in collector hash table
type CollectorData struct {
	// CollectorID is the key to hal apis
	CollectorID uint64
	// PolicyNames are policies referring to this collector
	PolicyNames map[string]bool
}

// CollectorTable is the object saved in db to track hal object
type CollectorTable struct {
	api.TypeMeta
	api.ObjectMeta
	// Collector table uses CollectorKey.string as key
	Collector map[string]*CollectorData
}

// Marshal provides data marshalling before storing in emdb
func (m *CollectorTable) Marshal() ([]byte, error) {
	return json.Marshal(m)
}

// Unmarshal ro retrieve data frpom emdb
func (m *CollectorTable) Unmarshal(data []byte) error {
	return json.Unmarshal(data, m)
}

// FlowMonitorRuleKey is the rule key for hash tables
type FlowMonitorRuleKey tstype.FlowMonitorRuleSpec

// String function converts key to string
func (c *FlowMonitorRuleKey) String() string {
	return fmt.Sprintf("%s:%s:%d:%d:%d:%d:%d:%d:%d:%d:%d", c.SourceIP, c.DestIP,
		c.SourceMac, c.DestMac, c.EtherType, c.Protocol,
		c.SourceL4Port, c.DestL4Port, c.SourceGroupID, c.DestGroupID, c.VrfID)
}

// ParseFlowMonitorRuleKey parses string to flow rule key, used for debug
func ParseFlowMonitorRuleKey(buff string) FlowMonitorRuleKey {
	var c FlowMonitorRuleKey

	atoi := func(s string) int {
		if v, err := strconv.Atoi(s); err == nil {
			return v
		}
		return 0
	}
	s := strings.Split(buff, ":")
	if len(s) == 11 {
		c.SourceIP = s[0]
		c.DestIP = s[1]
		c.SourceMac = uint64(atoi(s[2]))
		c.DestMac = uint64(atoi(s[3]))
		c.EtherType = uint32(atoi(s[4]))
		c.Protocol = uint32(atoi(s[5]))
		c.SourceL4Port = uint32(atoi(s[6]))
		c.DestL4Port = uint32(atoi(s[7]))
		c.SourceGroupID = uint64(atoi(s[8]))
		c.DestGroupID = uint64(atoi(s[9]))
		c.VrfID = uint64(atoi(s[10]))
	}

	return c
}

// FlowMonitorData is stored in flowmonitor hash table
type FlowMonitorData struct {
	// RuleID is the hal key
	RuleID  uint64
	RuleKey FlowMonitorRuleKey
	// Collectors contains per policy collectors referred by this rule
	Collectors map[string]map[string]bool
	// PolicyNames are policies referring to this rule
	PolicyNames map[string]bool
}

// FlowMonitorTable is saved in agent to track hal object
type FlowMonitorTable struct {
	api.TypeMeta
	api.ObjectMeta
	// FlowRules contains all rules, key is FlowMonitorRuleKey.string()
	FlowRules map[string]*FlowMonitorData
}

// Marshal provides data marshalling before storing in emdb
func (m *FlowMonitorTable) Marshal() ([]byte, error) {
	return json.Marshal(m)
}

// Unmarshal ro retrieve data frpom emdb
func (m *FlowMonitorTable) Unmarshal(data []byte) error {
	return json.Unmarshal(data, m)
}

// FlowExportPolicyTable is saved in agent to track hal object
type FlowExportPolicyTable struct {
	*tpmprotos.FlowExportPolicy
	// Vrf is the vrf id
	Vrf uint64
	// CollectorKeys stores collectors with CollectorKey.string() as key
	CollectorKeys map[string]bool
	// FlowRuleKeys stores rules with FlowMonitorRuleKey.string() as key
	FlowRuleKeys map[string]bool
}

// Marshal provides data marshalling before storing in emdb
func (m *FlowExportPolicyTable) Marshal() ([]byte, error) {
	return json.Marshal(m)
}

// Unmarshal ro retrieve data frpom emdb
func (m *FlowExportPolicyTable) Unmarshal(data []byte) error {
	return json.Unmarshal(data, m)
}
