package objects

import (
	"fmt"
	"strings"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/api/labels"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

// MirrorSession represents mirrorsession
type MirrorSession struct {
	VeniceMirrorSess *monitoring.MirrorSession
}

// MirrorSessionCollection is list of sessions
type MirrorSessionCollection struct {
	CollectionCommon
	err      error
	Sessions []*MirrorSession
}

var sessionIDTable map[uint32]bool

func allocateSpanID() uint32 {
	if sessionIDTable == nil {
		sessionIDTable = make(map[uint32]bool)
		var i uint32
		for i = 1; i < 1024; i++ {
			sessionIDTable[i] = true
		}
	}
	var key uint32
	for key, _ = range sessionIDTable {
		break
	}
	if key == 0 {
		panic("All SpanIDs used, deleteMirrorSessions to get unused SpanIDs")
	}
	delete(sessionIDTable, key)
	return key
}

func deallocateSpanID(key uint32) {
	if sessionIDTable == nil {
		return
	}
	if _, ok := sessionIDTable[key]; !ok {
		sessionIDTable[key] = true
	}
}

func NewMirrorSession(name string, client objClient.ObjClient, testbed *testbed.TestBed) *MirrorSessionCollection {
	return &MirrorSessionCollection{
		Sessions: []*MirrorSession{
			{
				VeniceMirrorSess: &monitoring.MirrorSession{
					TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
					ObjectMeta: api.ObjectMeta{
						Tenant:    "default",
						Namespace: "default",
						Name:      name,
					},
					Spec: monitoring.MirrorSessionSpec{
						PacketSize: 128,
						SpanID:     allocateSpanID(),
					},
				},
			},
		},
		CollectionCommon: CollectionCommon{Testbed: testbed, Client: client},
	}
}

// AddCollector adds a collector to the mirrorsession
func (msc *MirrorSessionCollection) AddCollector(wc *WorkloadCollection, transport string, wlnum int) *MirrorSessionCollection {
	if msc.HasError() {
		return msc
	}
	if wc.err != nil {
		return &MirrorSessionCollection{err: wc.err}
	}
	collector := monitoring.MirrorCollector{
		Type: monitoring.PacketCollectorType_ERSPAN_TYPE_3.String(),
		ExportCfg: &monitoring.MirrorExportConfig{
			Destination: strings.Split(wc.Workloads[wlnum].iotaWorkload.Interfaces[0].IpPrefix, "/")[0],
		},
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.Collectors = append(sess.VeniceMirrorSess.Spec.Collectors, collector)
	}

	return msc
}

// AddVeniceCollector adds venice collector
func (msc *MirrorSessionCollection) AddVeniceCollector(vnc *VeniceNodeCollection, transport string, wlnum int) *MirrorSessionCollection {
	return msc.AddVeniceCollectorWithOptions(vnc, transport, wlnum,
		monitoring.PacketCollectorType_ERSPAN_TYPE_3, false)
}

// AddVeniceCollectorWithOptions adds venice collector with type/vlan-hdr options
func (msc *MirrorSessionCollection) AddVeniceCollectorWithOptions(vnc *VeniceNodeCollection, transport string,
	wlnum int, ctype monitoring.PacketCollectorType, stripVlanHdr bool) *MirrorSessionCollection {
	if msc.HasError() {
		return msc
	}
	collector := monitoring.MirrorCollector{
		Type:         ctype.String(),
		StripVlanHdr: stripVlanHdr,
		ExportCfg: &monitoring.MirrorExportConfig{
			Destination: strings.Split(vnc.Nodes[0].iotaNode.IpAddress, "/")[0],
		},
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.Collectors = append(sess.VeniceMirrorSess.Spec.Collectors, collector)
	}

	return msc
}

// ClearCollectors adds a collector to the mirrorsession
func (msc *MirrorSessionCollection) ClearCollectors() *MirrorSessionCollection {
	if msc.HasError() {
		return msc
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.Collectors = nil
	}

	return msc
}

// Commit writes the mirrorsession to venice
func (msc *MirrorSessionCollection) Commit() error {
	if msc.err != nil {
		return msc.err
	}
	for _, sess := range msc.Sessions {
		err := msc.Client.CreateMirrorSession(sess.VeniceMirrorSess)
		if err != nil {
			// try updating it
			err = msc.Client.UpdateMirrorSession(sess.VeniceMirrorSess)
			if err != nil {
				msc.err = err
				log.Infof("Updating mirror session failed %v", err)
				return err
			}
		}

		log.Debugf("Created session: %#v", sess.VeniceMirrorSess)

	}

	return nil
}

// PropogationComplete checks whether propogation is complate.
func (msc *MirrorSessionCollection) PropogationComplete() error {
	if msc.err != nil {
		return msc.err
	}
	for _, sess := range msc.Sessions {
		ms, err := msc.Client.GetMirrorSession(&sess.VeniceMirrorSess.ObjectMeta)
		if err == nil {
			if ms.Status.PropagationStatus.GenerationID == "" || ms.Status.PropagationStatus.Updated == 0 || ms.Status.PropagationStatus.Pending != 0 {
				log.Infof("Propogation pending for mirror session %#v", ms.Status.PropagationStatus)
				return fmt.Errorf("Propogation pending for mirror session %v", ms.Name)
			}
		}
	}

	return nil
}

// AddRule adds a rule to the mirrorsession
func (msc *MirrorSessionCollection) AddRule(fromIP, toIP, port string) *MirrorSessionCollection {
	if msc.err != nil {
		return msc
	}

	// build the rule
	rule := monitoring.MatchRule{
		Src:         &monitoring.MatchSelector{IPAddresses: []string{fromIP}},
		Dst:         &monitoring.MatchSelector{IPAddresses: []string{toIP}},
		AppProtoSel: &monitoring.AppProtoSelector{ProtoPorts: []string{port}},
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.MatchRules = append(sess.VeniceMirrorSess.Spec.MatchRules, rule)
	}

	return msc
}

// AddRulesForWorkloadPairs adds rule for each workload pair into the sessions
func (msc *MirrorSessionCollection) AddRulesForWorkloadPairs(wpc *WorkloadPairCollection, port string) *MirrorSessionCollection {
	if msc.err != nil {
		return msc
	}
	if wpc.err != nil {
		return &MirrorSessionCollection{err: wpc.err}
	}

	// walk each workload pair
	for _, wpair := range wpc.Pairs {
		fromIP := wpair.Second.GetIP()
		toIP := wpair.First.GetIP()
		nmsc := msc.AddRule(fromIP, toIP, port)
		if nmsc.err != nil {
			return nmsc
		}
	}

	return msc
}

// SetInterfaceSelector add interface selector
func (msc *MirrorSessionCollection) SetInterfaceSelector(selector *labels.Selector) *MirrorSessionCollection {
	if msc.err != nil {
		return msc
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.Interfaces = &monitoring.InterfaceMirror{
			Selectors: []*labels.Selector{selector},
		}
	}

	return msc
}

// SetWorkloadSelector add workload selector
func (msc *MirrorSessionCollection) SetWorkloadSelector(selector *labels.Selector) *MirrorSessionCollection {
	if msc.err != nil {
		return msc
	}

	for _, sess := range msc.Sessions {
		sess.VeniceMirrorSess.Spec.Workloads = &monitoring.WorkloadMirror{
			Selectors: []*labels.Selector{selector},
		}
	}

	return msc
}

// Delete deletes all sessions in the collection
func (msc *MirrorSessionCollection) Delete() error {
	if msc.err != nil {
		return msc.err
	}

	// walk all sessions and delete them
	for _, sess := range msc.Sessions {
		deallocateSpanID(sess.VeniceMirrorSess.Spec.SpanID)
		err := msc.Client.DeleteMirrorSession(sess.VeniceMirrorSess)
		if err != nil {
			return err
		}
	}

	return nil
}
