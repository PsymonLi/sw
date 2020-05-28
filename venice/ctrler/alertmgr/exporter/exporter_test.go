package exporter

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
)

var (
	logConfig = log.GetDefaultConfig(fmt.Sprintf("%s.%s", globals.AlertMgr, "test"))
	logger    = log.SetConfig(logConfig)
	_         = recorder.Override(mockevtsrecorder.NewRecorder("alertmgr_test", logger))
	mr        = mockresolver.New()
	e         *exporter
)

func setup(t *testing.T) {
	// Create alert engine.
	ei, err := New(logger, mr)
	AssertOk(t, err, "New exporter engine failed")
	e = ei.(*exporter)

}

func TestRun(t *testing.T) {
	setup(t)
	Assert(t, e != nil, "exporter engine nil")
	e.ctx, e.cancel = context.WithCancel(context.Background())

	defer func() {
		e.Stop()
		time.Sleep(100 * time.Millisecond)
		Assert(t, !e.GetRunningStatus(), "running flag still set")
	}()

	inCh := make(chan *monitoring.Alert)
	errCh, err := e.Run(e.ctx, inCh)
	time.Sleep(100 * time.Millisecond)
	AssertOk(t, err, "Error running exporter engine")
	Assert(t, errCh != nil, "error channel nil")
	Assert(t, e.GetRunningStatus(), "running flag not set")

	a := &monitoring.Alert{}
	inCh <- a

	time.Sleep(1 * time.Second)
}
