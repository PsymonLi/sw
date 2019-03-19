// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package goproto

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/delphi/gosdk/gometrics"
	"github.com/pensando/sw/venice/utils/tsdb/metrics"
)

type UpgradeMetrics struct {
	ObjectMeta api.ObjectMeta

	key uint32

	IsUpgPossible metrics.Counter

	DisruptiveUpg metrics.Counter

	NonDisruptiveUpg metrics.Counter

	SuccessfulUpg metrics.Counter

	FailedUpg metrics.Counter

	AbortedUpg metrics.Counter

	NumRegApps metrics.Gauge

	UpgPossible metrics.Counter

	UpgNotPossible metrics.Counter

	// private state
	metrics gometrics.Metrics
}

func (mtr *UpgradeMetrics) GetKey() uint32 {
	return mtr.key
}

// Size returns the size of the metrics object
func (mtr *UpgradeMetrics) Size() int {
	sz := 0

	sz += mtr.IsUpgPossible.Size()

	sz += mtr.DisruptiveUpg.Size()

	sz += mtr.NonDisruptiveUpg.Size()

	sz += mtr.SuccessfulUpg.Size()

	sz += mtr.FailedUpg.Size()

	sz += mtr.AbortedUpg.Size()

	sz += mtr.NumRegApps.Size()

	sz += mtr.UpgPossible.Size()

	sz += mtr.UpgNotPossible.Size()

	return sz
}

// Unmarshal unmarshal the raw counters from shared memory
func (mtr *UpgradeMetrics) Unmarshal() error {
	var offset int

	gometrics.DecodeScalarKey(&mtr.key, mtr.metrics.GetKey())

	mtr.IsUpgPossible = mtr.metrics.GetCounter(offset)
	offset += mtr.IsUpgPossible.Size()

	mtr.DisruptiveUpg = mtr.metrics.GetCounter(offset)
	offset += mtr.DisruptiveUpg.Size()

	mtr.NonDisruptiveUpg = mtr.metrics.GetCounter(offset)
	offset += mtr.NonDisruptiveUpg.Size()

	mtr.SuccessfulUpg = mtr.metrics.GetCounter(offset)
	offset += mtr.SuccessfulUpg.Size()

	mtr.FailedUpg = mtr.metrics.GetCounter(offset)
	offset += mtr.FailedUpg.Size()

	mtr.AbortedUpg = mtr.metrics.GetCounter(offset)
	offset += mtr.AbortedUpg.Size()

	mtr.NumRegApps = mtr.metrics.GetGauge(offset)
	offset += mtr.NumRegApps.Size()

	mtr.UpgPossible = mtr.metrics.GetCounter(offset)
	offset += mtr.UpgPossible.Size()

	mtr.UpgNotPossible = mtr.metrics.GetCounter(offset)
	offset += mtr.UpgNotPossible.Size()

	return nil
}

// getOffset returns the offset for raw counters in shared memory
func (mtr *UpgradeMetrics) getOffset(fldName string) int {
	var offset int

	if fldName == "IsUpgPossible" {
		return offset
	}
	offset += mtr.IsUpgPossible.Size()

	if fldName == "DisruptiveUpg" {
		return offset
	}
	offset += mtr.DisruptiveUpg.Size()

	if fldName == "NonDisruptiveUpg" {
		return offset
	}
	offset += mtr.NonDisruptiveUpg.Size()

	if fldName == "SuccessfulUpg" {
		return offset
	}
	offset += mtr.SuccessfulUpg.Size()

	if fldName == "FailedUpg" {
		return offset
	}
	offset += mtr.FailedUpg.Size()

	if fldName == "AbortedUpg" {
		return offset
	}
	offset += mtr.AbortedUpg.Size()

	if fldName == "NumRegApps" {
		return offset
	}
	offset += mtr.NumRegApps.Size()

	if fldName == "UpgPossible" {
		return offset
	}
	offset += mtr.UpgPossible.Size()

	if fldName == "UpgNotPossible" {
		return offset
	}
	offset += mtr.UpgNotPossible.Size()

	return offset
}

// SetIsUpgPossible sets cunter in shared memory
func (mtr *UpgradeMetrics) SetIsUpgPossible(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("IsUpgPossible"))
	return nil
}

// SetDisruptiveUpg sets cunter in shared memory
func (mtr *UpgradeMetrics) SetDisruptiveUpg(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("DisruptiveUpg"))
	return nil
}

// SetNonDisruptiveUpg sets cunter in shared memory
func (mtr *UpgradeMetrics) SetNonDisruptiveUpg(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("NonDisruptiveUpg"))
	return nil
}

// SetSuccessfulUpg sets cunter in shared memory
func (mtr *UpgradeMetrics) SetSuccessfulUpg(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("SuccessfulUpg"))
	return nil
}

// SetFailedUpg sets cunter in shared memory
func (mtr *UpgradeMetrics) SetFailedUpg(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("FailedUpg"))
	return nil
}

// SetAbortedUpg sets cunter in shared memory
func (mtr *UpgradeMetrics) SetAbortedUpg(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("AbortedUpg"))
	return nil
}

// SetNumRegApps sets gauge in shared memory
func (mtr *UpgradeMetrics) SetNumRegApps(val metrics.Gauge) error {
	mtr.metrics.SetGauge(val, mtr.getOffset("NumRegApps"))
	return nil
}

// SetUpgPossible sets cunter in shared memory
func (mtr *UpgradeMetrics) SetUpgPossible(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("UpgPossible"))
	return nil
}

// SetUpgNotPossible sets cunter in shared memory
func (mtr *UpgradeMetrics) SetUpgNotPossible(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("UpgNotPossible"))
	return nil
}

// UpgradeMetricsIterator is the iterator object
type UpgradeMetricsIterator struct {
	iter gometrics.MetricsIterator
}

// HasNext returns true if there are more objects
func (it *UpgradeMetricsIterator) HasNext() bool {
	return it.iter.HasNext()
}

// Next returns the next metrics
func (it *UpgradeMetricsIterator) Next() *UpgradeMetrics {
	mtr := it.iter.Next()
	if mtr == nil {
		return nil
	}

	tmtr := &UpgradeMetrics{metrics: mtr}
	tmtr.Unmarshal()
	return tmtr
}

// Find finds the metrics object by key

func (it *UpgradeMetricsIterator) Find(key uint32) (*UpgradeMetrics, error) {

	mtr, err := it.iter.Find(gometrics.EncodeScalarKey(key))

	if err != nil {
		return nil, err
	}
	tmtr := &UpgradeMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Create creates the object in shared memory

func (it *UpgradeMetricsIterator) Create(key uint32) (*UpgradeMetrics, error) {
	tmtr := &UpgradeMetrics{}

	mtr := it.iter.Create(gometrics.EncodeScalarKey(key), tmtr.Size())

	tmtr = &UpgradeMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Delete deletes the object from shared memory

func (it *UpgradeMetricsIterator) Delete(key uint32) error {

	return it.iter.Delete(gometrics.EncodeScalarKey(key))

}

// Free frees the iterator memory
func (it *UpgradeMetricsIterator) Free() {
	it.iter.Free()
}

// NewUpgradeMetricsIterator returns an iterator
func NewUpgradeMetricsIterator() (*UpgradeMetricsIterator, error) {
	iter, err := gometrics.NewMetricsIterator("UpgradeMetrics")
	if err != nil {
		return nil, err
	}
	// little hack to skip creating iterators on osx
	if iter == nil {
		return nil, nil
	}

	return &UpgradeMetricsIterator{iter: iter}, nil
}
