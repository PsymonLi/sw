package pcache

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/bookstore"
	smmock "github.com/pensando/sw/venice/ctrler/orchhub/statemgr"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
	. "github.com/pensando/sw/venice/utils/testutils"
)

var (
	logConfig = log.GetDefaultConfig("pcache_test")
	logger    = log.SetConfig(logConfig)
	orderKind = string(bookstore.KindOrder)
)

func TestPcache(t *testing.T) {
	logConfig.LogToStdout = true
	logConfig.Filter = log.AllowAllFilter
	logger := log.SetConfig(logConfig)
	// Set should check validator before writing to statemgr
	// Call to statemgr should be update/create
	sm, _, err := smmock.NewMockStateManager()
	AssertOk(t, err, "failed to create statemgr")
	pCache := NewPCache(sm, logger)
	pCache.retryInterval = 10 * time.Second

	ctx, cancel := context.WithCancel(context.Background())
	wg := &sync.WaitGroup{}
	wg.Add(1)
	go pCache.Run(ctx, wg)
	defer func() {
		cancel()
		wg.Wait()
	}()

	// Register bookstore kind
	writeCalled := false
	deleteCalled := false
	pCache.RegisterKind(orderKind, &KindOpts{
		WriteToApiserver: true,
		WriteObj: func(in interface{}) error {
			writeCalled = true
			return nil
		},
		DeleteObj: func(in interface{}) error {
			deleteCalled = true
			return nil
		},
		Validator: func(in interface{}) (bool, bool) {
			w := in.(*bookstore.Order)
			valid := false
			push := false
			if w.Labels != nil {
				logger.Infof("----------- %v", w.Labels)
				if len(w.Labels["valid"]) != 0 {
					valid = true
				}
				if len(w.Labels["push"]) != 0 {
					push = true
				}
			}
			return valid, push
		},
	})

	order := &bookstore.Order{
		ObjectMeta: api.ObjectMeta{
			Name: "order1",
		},
	}
	err = pCache.Set(orderKind, order, true)
	AssertOk(t, err, "Failed to write workload")
	Assert(t, !writeCalled, "write shouldn't have been called")
	Assert(t, !deleteCalled, "delete shouldn't have been called")

	// Get
	temp := ref.DeepCopy(*order).(bookstore.Order)
	order = &temp
	ret := pCache.Get(orderKind, order.GetObjectMeta())
	AssertEquals(t, order, ret, "pcache entry didn't match")

	// Make item valid
	order.Labels = map[string]string{
		"valid": "true",
	}
	err = pCache.Set(orderKind, order, true)
	Assert(t, !writeCalled, "write shouldn't have been called")
	Assert(t, !deleteCalled, "delete shouldn't have been called")

	// Set without writing to statemgr
	order.Labels = map[string]string{
		"valid": "true",
		"push":  "true",
	}
	err = pCache.Set(orderKind, order, false)
	Assert(t, !writeCalled, "write shouldn't have been called")
	Assert(t, !deleteCalled, "delete shouldn't have been called")

	// Test isValid
	Assert(t, pCache.IsValid(orderKind, order.GetObjectMeta()), "order should have been valid")

	// Revalidate should push the object
	pCache.RevalidateKind(orderKind)
	Assert(t, writeCalled, "write should have been called")
	Assert(t, !deleteCalled, "delete shouldn't have been called")

	writeCalled = false

	// Set to trigger delete
	order.Labels = map[string]string{
		"push": "true",
	}
	err = pCache.Set(orderKind, order, true)
	AssertOk(t, err, "failed to set")
	Assert(t, !writeCalled, "write shouldn't have been called")
	Assert(t, deleteCalled, "delete should have been called")

	deleteCalled = false

	// Delete object
	err = pCache.Delete(orderKind, order)
	AssertOk(t, err, "failed to delete")
	Assert(t, !writeCalled, "write shouldn't have been called")
	Assert(t, deleteCalled, "delete should have been called")
	ret = pCache.Get(orderKind, order.GetObjectMeta())
	AssertEquals(t, nil, ret, "pcache entry should be gone")

	// Add back and test List and Debug
	err = pCache.Set(orderKind, order, true)
	AssertOk(t, err, "failed to set")

	order2 := &bookstore.Order{
		ObjectMeta: api.ObjectMeta{
			Name: "order2",
		},
	}
	err = pCache.Set(orderKind, order2, true)
	AssertOk(t, err, "failed to set")

	// Debug should return value in cache
	debugInfoResp, err := pCache.Debug(map[string]string{"kind": orderKind})
	AssertOk(t, err, "Failed to get debug info")
	debugMap := debugInfoResp.(map[string]interface{})
	debugEntryMap := debugMap[orderKind].(map[string]interface{})
	AssertEquals(t, 2, len(debugEntryMap), "expected 2 entries")

	debugInfoResp1, err := pCache.Debug(nil)
	AssertOk(t, err, "Failed to get debug info")
	AssertEquals(t, debugInfoResp, debugInfoResp1, "Debugs were note equal")

	AssertEquals(t, 2, len(pCache.List(orderKind)), "List expected to have 2 entries")
}
