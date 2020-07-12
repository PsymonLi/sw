package workfarm

import (
	"context"

	"github.com/pensando/sw/venice/utils/log"
)

// Workers represents a worker pool
type Workers struct {
	ctx        context.Context
	numWorkers int
	workItems  chan func()
}

// PostWorkItem can be called for posting a work item to the worker pool
func (w *Workers) PostWorkItem(wi func()) {
	w.workItems <- wi
}

func doWork(ctx context.Context, workItems <-chan func()) {
	defer func() {
		if r := recover(); r != nil {
			log.Errorf("recovered in objstore's worker routine %s", r)
		}
	}()

	for {
		select {
		case <-ctx.Done():
			return
		case wi := <-workItems:
			wi()
		}
	}
}

// NewWorkers starts a new work farm
func NewWorkers(ctx context.Context, numWorkers int, bufferSize int) *Workers {
	w := &Workers{ctx: ctx, numWorkers: numWorkers, workItems: make(chan func(), bufferSize)}
	for i := 0; i < numWorkers; i++ {
		go doWork(ctx, w.workItems)
	}
	return w
}
