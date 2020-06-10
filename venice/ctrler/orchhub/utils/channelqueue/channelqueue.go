// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package channelqueue

import (
	"container/list"
	"context"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/kvstore"
)

/**
* ChannelQueue is a utility class for having an object queue using channel semantics.
* Writers can send objects without worrying about getting blocked by the receiver not reading.
* Receivers can select on the read channel to be sent values when available.
 */

const (
	chanSize = 50
)

// Item is the items that are sent on the queue
type Item struct {
	EvtType kvstore.WatchEventType
	ObjMeta *api.ObjectMeta
	Kind    string
}

// ChQueue is a queue object that uses channels for pushing and popping
type ChQueue struct {
	lock     sync.RWMutex
	active   bool
	ctx      context.Context
	cancelFn context.CancelFunc
	// Doubly linked list of events.
	evtQ    *list.List
	inbox   chan Item
	outbox  chan Item
	notifCh chan bool
}

// NewChQueue returns a new channel queue object
func NewChQueue() *ChQueue {
	q := &ChQueue{
		evtQ:   list.New(),
		inbox:  make(chan Item, chanSize),
		outbox: make(chan Item),
	}
	return q
}

// Start launches the queue's goroutine
func (q *ChQueue) Start(parentCtx context.Context) {
	q.lock.Lock()
	if !q.active {
		q.active = true
		q.ctx, q.cancelFn = context.WithCancel(parentCtx)
		go q.run()
	}
	q.lock.Unlock()
}

// Stop stops the queue
func (q *ChQueue) Stop() {
	q.lock.Lock()
	if q.active {
		q.cancelFn()
		q.cancelFn = nil
		q.active = false
	}
	q.lock.Unlock()
}

// Send adds an item to the queue, or blackholes the value if the queue is not active
func (q *ChQueue) Send(item Item) {
	q.lock.RLock()
	if q.active {
		q.inbox <- item
	}
	q.lock.RUnlock()
}

// ReadCh returns a channel for consumers to read sent events
func (q *ChQueue) ReadCh() <-chan Item {
	return q.outbox
}

func (q *ChQueue) run() {
	for {
		item := q.evtQ.Front()
		if item == nil {
			select {
			case <-q.ctx.Done():
				return
			case obj := <-q.inbox:
				q.evtQ.PushBack(obj)
			}
		} else {
			select {
			case <-q.ctx.Done():
				return
			case obj := <-q.inbox:
				q.evtQ.PushBack(obj)
			case q.outbox <- item.Value.(Item):
				q.evtQ.Remove(item)
			}
		}
	}
}
