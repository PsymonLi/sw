// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package goproto

import (
	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/delphi/gosdk/gometrics"
	"github.com/pensando/sw/venice/utils/tsdb/metrics"
)

type MirrorMetrics struct {
	ObjectMeta api.ObjectMeta

	key uint64

	Pkts metrics.Counter

	Bytes metrics.Counter

	Pps metrics.Counter

	Bytesps metrics.Counter

	// private state
	metrics gometrics.Metrics
}

func (mtr *MirrorMetrics) GetKey() uint64 {
	return mtr.key
}

// Size returns the size of the metrics object
func (mtr *MirrorMetrics) Size() int {
	sz := 0

	sz += mtr.Pkts.Size()

	sz += mtr.Bytes.Size()

	sz += mtr.Pps.Size()

	sz += mtr.Bytesps.Size()

	return sz
}

// Unmarshal unmarshal the raw counters from shared memory
func (mtr *MirrorMetrics) Unmarshal() error {
	var offset int

	gometrics.DecodeScalarKey(&mtr.key, mtr.metrics.GetKey())

	mtr.Pkts = mtr.metrics.GetCounter(offset)
	offset += mtr.Pkts.Size()

	mtr.Bytes = mtr.metrics.GetCounter(offset)
	offset += mtr.Bytes.Size()

	mtr.Pps = mtr.metrics.GetCounter(offset)
	offset += mtr.Pps.Size()

	mtr.Bytesps = mtr.metrics.GetCounter(offset)
	offset += mtr.Bytesps.Size()

	return nil
}

// getOffset returns the offset for raw counters in shared memory
func (mtr *MirrorMetrics) getOffset(fldName string) int {
	var offset int

	if fldName == "Pkts" {
		return offset
	}
	offset += mtr.Pkts.Size()

	if fldName == "Bytes" {
		return offset
	}
	offset += mtr.Bytes.Size()

	if fldName == "Pps" {
		return offset
	}
	offset += mtr.Pps.Size()

	if fldName == "Bytesps" {
		return offset
	}
	offset += mtr.Bytesps.Size()

	return offset
}

// SetPkts sets cunter in shared memory
func (mtr *MirrorMetrics) SetPkts(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("Pkts"))
	return nil
}

// SetBytes sets cunter in shared memory
func (mtr *MirrorMetrics) SetBytes(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("Bytes"))
	return nil
}

// SetPps sets cunter in shared memory
func (mtr *MirrorMetrics) SetPps(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("Pps"))
	return nil
}

// SetBytesps sets cunter in shared memory
func (mtr *MirrorMetrics) SetBytesps(val metrics.Counter) error {
	mtr.metrics.SetCounter(val, mtr.getOffset("Bytesps"))
	return nil
}

// MirrorMetricsIterator is the iterator object
type MirrorMetricsIterator struct {
	iter gometrics.MetricsIterator
}

// HasNext returns true if there are more objects
func (it *MirrorMetricsIterator) HasNext() bool {
	return it.iter.HasNext()
}

// Next returns the next metrics
func (it *MirrorMetricsIterator) Next() *MirrorMetrics {
	mtr := it.iter.Next()
	if mtr == nil {
		return nil
	}

	tmtr := &MirrorMetrics{metrics: mtr}
	tmtr.Unmarshal()
	return tmtr
}

// Find finds the metrics object by key

func (it *MirrorMetricsIterator) Find(key uint64) (*MirrorMetrics, error) {

	mtr, err := it.iter.Find(gometrics.EncodeScalarKey(key))

	if err != nil {
		return nil, err
	}
	tmtr := &MirrorMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Create creates the object in shared memory

func (it *MirrorMetricsIterator) Create(key uint64) (*MirrorMetrics, error) {
	tmtr := &MirrorMetrics{}

	mtr := it.iter.Create(gometrics.EncodeScalarKey(key), tmtr.Size())

	tmtr = &MirrorMetrics{metrics: mtr, key: key}
	tmtr.Unmarshal()
	return tmtr, nil
}

// Delete deletes the object from shared memory

func (it *MirrorMetricsIterator) Delete(key uint64) error {

	return it.iter.Delete(gometrics.EncodeScalarKey(key))

}

// Free frees the iterator memory
func (it *MirrorMetricsIterator) Free() {
	it.iter.Free()
}

// NewMirrorMetricsIterator returns an iterator
func NewMirrorMetricsIterator() (*MirrorMetricsIterator, error) {
	iter, err := gometrics.NewMetricsIterator("MirrorMetrics")
	if err != nil {
		return nil, err
	}
	// little hack to skip creating iterators on osx
	if iter == nil {
		return nil, nil
	}

	return &MirrorMetricsIterator{iter: iter}, nil
}
