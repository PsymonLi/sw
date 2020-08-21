/*
Package workfarm Pinned workers can be used when the usecase requires processing items in the following fashion:
1. Items belonging to different keys should be processed concurrently.
2. Items belonging to the same key should be processed sequentially.

The PostWorkItem requires posting work item against a Key, all the items belonging to the same key are processed sequentially.

The dispatcher does a round-robin selection of keys for picking the next work item to be processed.
*/
package workfarm

import (
	"context"
)

type workChannel chan WorkItem
type workItems map[string]workChannel
type availableWorkers chan workChannel

// PinnedWorker represents a pinned worker
type PinnedWorker interface {
	PostWorkItem(string, func()) error
	Stop()
}

// WorkItem represents a work item to be processed by the worker
type WorkItem struct {
	pin string
	wi  func()
}

type worker struct {
	id                int
	workCh            workChannel
	indicateAvailable availableWorkers
}

type dispatcher struct {
	ctx               context.Context
	cancel            func()
	workers           []*worker // this is the list of workers that dispatcher tracks
	availableW        availableWorkers
	pins              []string
	workCh            workChannel      // client submits a job to this channel
	wis               workItems        // dispatcher submits a job to this channel
	indicateAvailable availableWorkers // this is the shared JobPool between the workers
	pendingWis        int              // pending work items
	lastServedPin     int
}

func newWorker(id int, wc workChannel, aw availableWorkers) *worker {
	return &worker{
		id:                id,
		workCh:            wc,
		indicateAvailable: aw,
	}
}

// NewPinnedWorker creates a new pinned worker
func NewPinnedWorker(ctx context.Context, num int, bufferSize int) PinnedWorker {
	ctxNew, cancel := context.WithCancel(ctx)
	d := &dispatcher{
		ctx:               ctxNew,
		cancel:            cancel,
		pins:              []string{},
		workers:           make([]*worker, num),
		wis:               make(workItems),
		workCh:            make(workChannel, 10000),
		indicateAvailable: make(availableWorkers, num),
		availableW:        make(availableWorkers, num),
		lastServedPin:     -1,
	}
	d.start(bufferSize)
	return d
}

func (d *dispatcher) PostWorkItem(id string, wi func()) error {
	d.workCh <- WorkItem{pin: id, wi: wi}
	return nil
}

func (d *dispatcher) Stop() {
	d.cancel()
}

func (d *dispatcher) start(bufferSize int) {
	for i := 1; i <= len(d.workers); i++ {
		d.workers[i-1] = newWorker(i, make(workChannel), d.indicateAvailable)
		d.workers[i-1].start(d.ctx)
	}
	go d.process(bufferSize)
}

func (wr *worker) start(ctx context.Context) {
	go func() {
		for {
			wr.indicateAvailable <- wr.workCh
			select {
			case <-ctx.Done():
				close(wr.workCh)
				return
			case wi := <-wr.workCh:
				wi.wi()
			}
		}
	}()
}

func (d *dispatcher) process(bufferSize int) {
	for {
		select {
		case wi := <-d.workCh:
			ch, ok := d.wis[wi.pin]
			if !ok {
				ch = make(chan WorkItem, bufferSize)
				d.wis[wi.pin] = ch
				d.pins = append(d.pins, wi.pin)
			}
			d.wis[wi.pin] <- wi
		loop:
			for {
				select {
				case aw := <-d.availableW:
					nextPin := d.getNextPin()
					if nextPin == "" {
						d.availableW <- aw
						break loop
					}
					aw <- <-d.wis[nextPin]
				default:
					break loop
				}
			}

		case aw := <-d.indicateAvailable:
			nextPin := d.getNextPin()
			if nextPin == "" {
				d.availableW <- aw
				continue
			}
			aw <- <-d.wis[nextPin]
		}
	}
}

func (d *dispatcher) getNextPin() string {
	original := d.lastServedPin
	start := d.lastServedPin
	for {
		start++
		if start < len(d.pins) {
			if len(d.wis[d.pins[start]]) > 0 {
				d.lastServedPin = start
				return d.pins[start]
			}
			delete(d.wis, d.pins[start])
			d.pins = append(d.pins[:start], d.pins[start+1:]...)
		} else {
			start = -1
		}
		if start == original || len(d.pins) == 0 {
			// we have made one full round, there are no wis available
			d.lastServedPin = -1
			return ""
		}
	}
}
