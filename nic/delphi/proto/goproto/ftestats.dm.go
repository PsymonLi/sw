// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package goproto

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/delphi/gosdk/gometrics"
	"github.com/pensando/sw/venice/utils/tsdb/metrics"
)

type FteCPSMetrics struct {
	ObjectMeta api.ObjectMeta

	key uint64

	ConnectionsPerSecond metrics.Counter

	MaxConnectionsPerSecond metrics.Counter

	// private state
	metrics gometrics.Metrics
}

func (mtr *FteCPSMetrics) GetKey() uint64 {
	return mtr.key
}

// Size returns the size of the metrics object
func (mtr *FteCPSMetrics) Size() int {
	sz := 0

	sz += mtr.ConnectionsPerSecond.Size()

	sz += mtr.MaxConnectionsPerSecond.Size()

	return sz
}

// Unmarshal unmarshal the raw counters from shared memory
func (mtr *FteCPSMetrics) Unmarshal() error {
	var offset int

	gometrics.DecodeScalarKey(&mtr.key, mtr.metrics.GetKey())

	mtr.ConnectionsPerSecond = mtr.metrics.GetCounter(offset)
	offset += mtr.ConnectionsPerSecond.Size()

	mtr.MaxConnectionsPerSecond = mtr.metrics.GetCounter(offset)
	offset += mtr.MaxConnectionsPerSecond.Size()

	return nil
}

// getOffset returns the offset for raw counters in shared memory
func (mtr *FteCPSMetrics) getOffset(fldName string) int {
	var offset int

	if fldName == "ConnectionsPerSecond" {
		return offset
	}
	offset += mtr.ConnectionsPerSecond.Size()

	if fldName == "MaxConnectionsPerSecond" {
		return offset
	}
	offset += mtr.MaxConnectionsPerSecond.Size()

	return offset
}

// SetConnectionsPerSecond sets cunter in shared memory
func (mtr *FteCPSMetrics) SetConnectionsPerSecond(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("ConnectionsPerSecond"))
	return nil
}

// SetMaxConnectionsPerSecond sets cunter in shared memory
func (mtr *FteCPSMetrics) SetMaxConnectionsPerSecond(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("MaxConnectionsPerSecond"))
	return nil
}

// FteCPSMetricsIterator is the iterator object
type FteCPSMetricsIterator struct {
	iter gometrics.MetricsIterator
}

// HasNext returns true if there are more objects
func (it *FteCPSMetricsIterator) HasNext() bool {
	return it.iter.HasNext()
}

// Next returns the next metrics
func (it *FteCPSMetricsIterator) Next() *FteCPSMetrics {
	mtr := it.iter.Next()
	if mtr == nil {
		return nil
	}

	tmtr := &FteCPSMetrics{metrics: mtr}
	tmtr.Unmarshal()
	return tmtr
}

// Find finds the metrics object by key

func (it *FteCPSMetricsIterator) Find(key uint64) (*FteCPSMetrics, error) {

	mtr, err := it.iter.Find(gometrics.EncodeScalarKey(key))

	if err != nil {
		return nil, err
	}
	tmtr := &FteCPSMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Create creates the object in shared memory

func (it *FteCPSMetricsIterator) Create(key uint64) (*FteCPSMetrics, error) {
	tmtr := &FteCPSMetrics{}

	mtr := it.iter.Create(gometrics.EncodeScalarKey(key), tmtr.Size())

	tmtr = &FteCPSMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Delete deletes the object from shared memory

func (it *FteCPSMetricsIterator) Delete(key uint64) error {

	return it.iter.Delete(gometrics.EncodeScalarKey(key))

}

// Free frees the iterator memory
func (it *FteCPSMetricsIterator) Free() {
	it.iter.Free()
}

// NewFteCPSMetricsIterator returns an iterator
func NewFteCPSMetricsIterator() (*FteCPSMetricsIterator, error) {
	iter, err := gometrics.NewMetricsIterator("FteCPSMetrics")
	if err != nil {
		return nil, err
	}
	// little hack to skip creating iterators on osx
	if iter == nil {
		return nil, nil
	}

	return &FteCPSMetricsIterator{iter: iter}, nil
}

type FteLifQMetrics struct {
	ObjectMeta api.ObjectMeta

	key uint64

	FlowMissPackets metrics.Counter

	L4RedirectPackets metrics.Counter

	AlgControlFlowPackets metrics.Counter

	TcpClosePackets metrics.Counter

	TlsProxyPackets metrics.Counter

	FteSpanPackets metrics.Counter

	SoftwareQueuePackets metrics.Counter

	QueuedTxPackets metrics.Counter

	// private state
	metrics gometrics.Metrics
}

func (mtr *FteLifQMetrics) GetKey() uint64 {
	return mtr.key
}

// Size returns the size of the metrics object
func (mtr *FteLifQMetrics) Size() int {
	sz := 0

	sz += mtr.FlowMissPackets.Size()

	sz += mtr.L4RedirectPackets.Size()

	sz += mtr.AlgControlFlowPackets.Size()

	sz += mtr.TcpClosePackets.Size()

	sz += mtr.TlsProxyPackets.Size()

	sz += mtr.FteSpanPackets.Size()

	sz += mtr.SoftwareQueuePackets.Size()

	sz += mtr.QueuedTxPackets.Size()

	return sz
}

// Unmarshal unmarshal the raw counters from shared memory
func (mtr *FteLifQMetrics) Unmarshal() error {
	var offset int

	gometrics.DecodeScalarKey(&mtr.key, mtr.metrics.GetKey())

	mtr.FlowMissPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.FlowMissPackets.Size()

	mtr.L4RedirectPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.L4RedirectPackets.Size()

	mtr.AlgControlFlowPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.AlgControlFlowPackets.Size()

	mtr.TcpClosePackets = mtr.metrics.GetCounter(offset)
	offset += mtr.TcpClosePackets.Size()

	mtr.TlsProxyPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.TlsProxyPackets.Size()

	mtr.FteSpanPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.FteSpanPackets.Size()

	mtr.SoftwareQueuePackets = mtr.metrics.GetCounter(offset)
	offset += mtr.SoftwareQueuePackets.Size()

	mtr.QueuedTxPackets = mtr.metrics.GetCounter(offset)
	offset += mtr.QueuedTxPackets.Size()

	return nil
}

// getOffset returns the offset for raw counters in shared memory
func (mtr *FteLifQMetrics) getOffset(fldName string) int {
	var offset int

	if fldName == "FlowMissPackets" {
		return offset
	}
	offset += mtr.FlowMissPackets.Size()

	if fldName == "L4RedirectPackets" {
		return offset
	}
	offset += mtr.L4RedirectPackets.Size()

	if fldName == "AlgControlFlowPackets" {
		return offset
	}
	offset += mtr.AlgControlFlowPackets.Size()

	if fldName == "TcpClosePackets" {
		return offset
	}
	offset += mtr.TcpClosePackets.Size()

	if fldName == "TlsProxyPackets" {
		return offset
	}
	offset += mtr.TlsProxyPackets.Size()

	if fldName == "FteSpanPackets" {
		return offset
	}
	offset += mtr.FteSpanPackets.Size()

	if fldName == "SoftwareQueuePackets" {
		return offset
	}
	offset += mtr.SoftwareQueuePackets.Size()

	if fldName == "QueuedTxPackets" {
		return offset
	}
	offset += mtr.QueuedTxPackets.Size()

	return offset
}

// SetFlowMissPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetFlowMissPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("FlowMissPackets"))
	return nil
}

// SetL4RedirectPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetL4RedirectPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("L4RedirectPackets"))
	return nil
}

// SetAlgControlFlowPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetAlgControlFlowPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("AlgControlFlowPackets"))
	return nil
}

// SetTcpClosePackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetTcpClosePackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("TcpClosePackets"))
	return nil
}

// SetTlsProxyPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetTlsProxyPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("TlsProxyPackets"))
	return nil
}

// SetFteSpanPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetFteSpanPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("FteSpanPackets"))
	return nil
}

// SetSoftwareQueuePackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetSoftwareQueuePackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("SoftwareQueuePackets"))
	return nil
}

// SetQueuedTxPackets sets cunter in shared memory
func (mtr *FteLifQMetrics) SetQueuedTxPackets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("QueuedTxPackets"))
	return nil
}

// FteLifQMetricsIterator is the iterator object
type FteLifQMetricsIterator struct {
	iter gometrics.MetricsIterator
}

// HasNext returns true if there are more objects
func (it *FteLifQMetricsIterator) HasNext() bool {
	return it.iter.HasNext()
}

// Next returns the next metrics
func (it *FteLifQMetricsIterator) Next() *FteLifQMetrics {
	mtr := it.iter.Next()
	if mtr == nil {
		return nil
	}

	tmtr := &FteLifQMetrics{metrics: mtr}
	tmtr.Unmarshal()
	return tmtr
}

// Find finds the metrics object by key

func (it *FteLifQMetricsIterator) Find(key uint64) (*FteLifQMetrics, error) {

	mtr, err := it.iter.Find(gometrics.EncodeScalarKey(key))

	if err != nil {
		return nil, err
	}
	tmtr := &FteLifQMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Create creates the object in shared memory

func (it *FteLifQMetricsIterator) Create(key uint64) (*FteLifQMetrics, error) {
	tmtr := &FteLifQMetrics{}

	mtr := it.iter.Create(gometrics.EncodeScalarKey(key), tmtr.Size())

	tmtr = &FteLifQMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Delete deletes the object from shared memory

func (it *FteLifQMetricsIterator) Delete(key uint64) error {

	return it.iter.Delete(gometrics.EncodeScalarKey(key))

}

// Free frees the iterator memory
func (it *FteLifQMetricsIterator) Free() {
	it.iter.Free()
}

// NewFteLifQMetricsIterator returns an iterator
func NewFteLifQMetricsIterator() (*FteLifQMetricsIterator, error) {
	iter, err := gometrics.NewMetricsIterator("FteLifQMetrics")
	if err != nil {
		return nil, err
	}
	// little hack to skip creating iterators on osx
	if iter == nil {
		return nil, nil
	}

	return &FteLifQMetricsIterator{iter: iter}, nil
}

type SessionSummaryMetrics struct {
	ObjectMeta api.ObjectMeta

	key uint64

	NumL2Sessions metrics.Counter

	NumTcpSessions metrics.Counter

	NumUdpSessions metrics.Counter

	NumIcmpSessions metrics.Counter

	NumDropSessions metrics.Counter

	NumAgedSessions metrics.Counter

	NumTcpResets metrics.Counter

	NumIcmpErrors metrics.Counter

	NumTcpCxnsetupTimeouts metrics.Counter

	NumSessionCreateErrors metrics.Counter

	// private state
	metrics gometrics.Metrics
}

func (mtr *SessionSummaryMetrics) GetKey() uint64 {
	return mtr.key
}

// Size returns the size of the metrics object
func (mtr *SessionSummaryMetrics) Size() int {
	sz := 0

	sz += mtr.NumL2Sessions.Size()

	sz += mtr.NumTcpSessions.Size()

	sz += mtr.NumUdpSessions.Size()

	sz += mtr.NumIcmpSessions.Size()

	sz += mtr.NumDropSessions.Size()

	sz += mtr.NumAgedSessions.Size()

	sz += mtr.NumTcpResets.Size()

	sz += mtr.NumIcmpErrors.Size()

	sz += mtr.NumTcpCxnsetupTimeouts.Size()

	sz += mtr.NumSessionCreateErrors.Size()

	return sz
}

// Unmarshal unmarshal the raw counters from shared memory
func (mtr *SessionSummaryMetrics) Unmarshal() error {
	var offset int

	gometrics.DecodeScalarKey(&mtr.key, mtr.metrics.GetKey())

	mtr.NumL2Sessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumL2Sessions.Size()

	mtr.NumTcpSessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumTcpSessions.Size()

	mtr.NumUdpSessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumUdpSessions.Size()

	mtr.NumIcmpSessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumIcmpSessions.Size()

	mtr.NumDropSessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumDropSessions.Size()

	mtr.NumAgedSessions = mtr.metrics.GetCounter(offset)
	offset += mtr.NumAgedSessions.Size()

	mtr.NumTcpResets = mtr.metrics.GetCounter(offset)
	offset += mtr.NumTcpResets.Size()

	mtr.NumIcmpErrors = mtr.metrics.GetCounter(offset)
	offset += mtr.NumIcmpErrors.Size()

	mtr.NumTcpCxnsetupTimeouts = mtr.metrics.GetCounter(offset)
	offset += mtr.NumTcpCxnsetupTimeouts.Size()

	mtr.NumSessionCreateErrors = mtr.metrics.GetCounter(offset)
	offset += mtr.NumSessionCreateErrors.Size()

	return nil
}

// getOffset returns the offset for raw counters in shared memory
func (mtr *SessionSummaryMetrics) getOffset(fldName string) int {
	var offset int

	if fldName == "NumL2Sessions" {
		return offset
	}
	offset += mtr.NumL2Sessions.Size()

	if fldName == "NumTcpSessions" {
		return offset
	}
	offset += mtr.NumTcpSessions.Size()

	if fldName == "NumUdpSessions" {
		return offset
	}
	offset += mtr.NumUdpSessions.Size()

	if fldName == "NumIcmpSessions" {
		return offset
	}
	offset += mtr.NumIcmpSessions.Size()

	if fldName == "NumDropSessions" {
		return offset
	}
	offset += mtr.NumDropSessions.Size()

	if fldName == "NumAgedSessions" {
		return offset
	}
	offset += mtr.NumAgedSessions.Size()

	if fldName == "NumTcpResets" {
		return offset
	}
	offset += mtr.NumTcpResets.Size()

	if fldName == "NumIcmpErrors" {
		return offset
	}
	offset += mtr.NumIcmpErrors.Size()

	if fldName == "NumTcpCxnsetupTimeouts" {
		return offset
	}
	offset += mtr.NumTcpCxnsetupTimeouts.Size()

	if fldName == "NumSessionCreateErrors" {
		return offset
	}
	offset += mtr.NumSessionCreateErrors.Size()

	return offset
}

// SetNumL2Sessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumL2Sessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumL2Sessions"))
	return nil
}

// SetNumTcpSessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumTcpSessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumTcpSessions"))
	return nil
}

// SetNumUdpSessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumUdpSessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumUdpSessions"))
	return nil
}

// SetNumIcmpSessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumIcmpSessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumIcmpSessions"))
	return nil
}

// SetNumDropSessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumDropSessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumDropSessions"))
	return nil
}

// SetNumAgedSessions sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumAgedSessions(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumAgedSessions"))
	return nil
}

// SetNumTcpResets sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumTcpResets(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumTcpResets"))
	return nil
}

// SetNumIcmpErrors sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumIcmpErrors(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumIcmpErrors"))
	return nil
}

// SetNumTcpCxnsetupTimeouts sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumTcpCxnsetupTimeouts(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumTcpCxnsetupTimeouts"))
	return nil
}

// SetNumSessionCreateErrors sets cunter in shared memory
func (mtr *SessionSummaryMetrics) SetNumSessionCreateErrors(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NumSessionCreateErrors"))
	return nil
}

// SessionSummaryMetricsIterator is the iterator object
type SessionSummaryMetricsIterator struct {
	iter gometrics.MetricsIterator
}

// HasNext returns true if there are more objects
func (it *SessionSummaryMetricsIterator) HasNext() bool {
	return it.iter.HasNext()
}

// Next returns the next metrics
func (it *SessionSummaryMetricsIterator) Next() *SessionSummaryMetrics {
	mtr := it.iter.Next()
	if mtr == nil {
		return nil
	}

	tmtr := &SessionSummaryMetrics{metrics: mtr}
	tmtr.Unmarshal()
	return tmtr
}

// Find finds the metrics object by key

func (it *SessionSummaryMetricsIterator) Find(key uint64) (*SessionSummaryMetrics, error) {

	mtr, err := it.iter.Find(gometrics.EncodeScalarKey(key))

	if err != nil {
		return nil, err
	}
	tmtr := &SessionSummaryMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Create creates the object in shared memory

func (it *SessionSummaryMetricsIterator) Create(key uint64) (*SessionSummaryMetrics, error) {
	tmtr := &SessionSummaryMetrics{}

	mtr := it.iter.Create(gometrics.EncodeScalarKey(key), tmtr.Size())

	tmtr = &SessionSummaryMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Delete deletes the object from shared memory

func (it *SessionSummaryMetricsIterator) Delete(key uint64) error {

	return it.iter.Delete(gometrics.EncodeScalarKey(key))

}

// Free frees the iterator memory
func (it *SessionSummaryMetricsIterator) Free() {
	it.iter.Free()
}

// NewSessionSummaryMetricsIterator returns an iterator
func NewSessionSummaryMetricsIterator() (*SessionSummaryMetricsIterator, error) {
	iter, err := gometrics.NewMetricsIterator("SessionSummaryMetrics")
	if err != nil {
		return nil, err
	}
	// little hack to skip creating iterators on osx
	if iter == nil {
		return nil, nil
	}

	return &SessionSummaryMetricsIterator{iter: iter}, nil
}
