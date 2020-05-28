package exporter

import (
	"context"
	"fmt"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/resolver"
)

// TODO Add logger.info, debug

// Interface for exporter
type Interface interface {
	Run(ctx context.Context, inchan <-chan *monitoring.Alert) (<-chan error, error)
	Stop()
	GetRunningStatus() bool
}

type exporter struct {
	logger  log.Logger
	rslvr   resolver.Interface
	ctx     context.Context
	cancel  context.CancelFunc
	inCh    chan *monitoring.Alert
	errCh   chan error
	running bool
}

// New exporter.
func New(logger log.Logger, rslvr resolver.Interface) (Interface, error) {
	e := &exporter{
		logger: logger,
		rslvr:  rslvr,
	}

	return e, nil
}

// Spawns required goroutines and handles context.Done, and errors.
func (e *exporter) Run(ctx context.Context, inCh <-chan *monitoring.Alert) (<-chan error, error) {
	if e.running {
		return nil, fmt.Errorf("exporter already running")
	}

	e.ctx, e.cancel = context.WithCancel(ctx)
	e.errCh = make(chan error, 1)

	go func() {
		defer e.cleanup()

		for {
			select {
			case a, ok := <-inCh:
				if ok {
					err := e.export(a)
					if err != nil {
						// Fatal error.
						e.errCh <- err
					}
				}
			case <-e.ctx.Done():
				return
			}
		}
	}()

	e.running = true
	return e.errCh, nil
}

func (e *exporter) Stop() {
	e.cancel()
}

func (e *exporter) GetRunningStatus() bool {
	return e.running
}

func (e *exporter) cleanup() {
	if e.running {
		e.running = false
	}
}

func (e *exporter) export(alert *monitoring.Alert) error {
	return fmt.Errorf("unimplemented")
}
