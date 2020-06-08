package shardworkers

import (
	"context"
	"errors"
	"hash/fnv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/pensando/sw/venice/utils/kvstore"
)

type workerState int

const (
	notRunning workerState = iota
	running
	idle
)

// number of times to retry the operation
const maxRetryCount = 40

//WorkFunc Prototype of work function.
type WorkFunc func(ctx context.Context, workObj WorkObj) error

//WorkObj interface
type WorkObj interface {
	GetKey() string
	WorkFunc(context context.Context) error
}

type worker struct {
	sync.Mutex
	id         int
	queue      chan workCtx
	state      workerState
	workMaster *WorkerPool
	doneCnt    uint32
}

type workCtx struct {
	workFunc WorkFunc
	workObj  WorkObj
}

func (w *worker) doneCount() uint32 {
	return atomic.LoadUint32(&w.doneCnt)
}

func (w *worker) pendingCount() uint32 {
	return (uint32)(len(w.queue))
}

func (w *worker) running() bool {
	w.Lock()
	defer w.Unlock()
	return w.state == running
}

func (w *worker) retryError(err error) bool {

	return kvstore.IsKeyNotFoundError(err) || kvstore.IsTxnFailedError(err)
}

func (w *worker) Run(ctx context.Context) {
	w.workMaster.StartWg.Done()
	defer w.workMaster.DoneWg.Done()
	w.Lock()
	w.state = idle
	w.Unlock()
	ticker := time.NewTicker((1 * time.Second))
	for true {
		select {
		case workContext := <-w.queue:
			w.Lock()
			w.state = running
			w.Unlock()
			var err error
			for i := 0; i < maxRetryCount; i++ {
				if workContext.workFunc != nil {
					err = workContext.workFunc(ctx, workContext.workObj)
				} else {
					err = workContext.workObj.WorkFunc(ctx)
				}
				if w.retryError(err) {
					// retry after a some time
					time.Sleep(20 * time.Millisecond * time.Duration(i+1))
					continue
				} else {
					// we are done
					break
				}

			}
			atomic.AddUint32(&w.doneCnt, 1)
		case <-ticker.C:
			if len(w.queue) == 0 {
				w.Lock()
				w.state = idle
				w.Unlock()
			}
		case <-ctx.Done():
			return
		}
	}

}

//WorkerPool structure
type WorkerPool struct {
	numberOfWorkers uint32
	workers         []*worker
	state           workerState
	StartWg         sync.WaitGroup
	DoneWg          sync.WaitGroup
	cancelCtx       context.Context
	cancelFunc      context.CancelFunc
}

//NewWorkerPool initialize new worker pool
func NewWorkerPool(numOfWorkers uint32) *WorkerPool {
	cancelCtx, cancel := context.WithCancel(context.Background())
	return &WorkerPool{numberOfWorkers: numOfWorkers, state: notRunning,
		cancelFunc: cancel, cancelCtx: cancelCtx}
}

//Start start all the workers.
func (wp *WorkerPool) Start() error {

	for i := 0; i < int(wp.numberOfWorkers); i++ {
		wp.workers = append(wp.workers, &worker{id: i,
			queue: make(chan workCtx, 16384), workMaster: wp})
		wp.StartWg.Add(1)
		wp.DoneWg.Add(1)
		go wp.workers[i].Run(wp.cancelCtx)
	}

	//Wait untill all workers are started
	wp.StartWg.Wait()
	wp.state = running

	return nil
}

//Running are worker pool running
func (wp *WorkerPool) Running() bool {
	return wp.state == running
}

//Stop all the workers
func (wp *WorkerPool) Stop() error {

	if wp.state == notRunning {
		return nil
	}
	wp.cancelFunc()
	wp.DoneWg.Wait()
	wp.state = notRunning
	return nil
}

//GetCompletedJobsCount get completed job
func (wp *WorkerPool) GetCompletedJobsCount(workerID uint32) (uint32, error) {
	if !wp.Running() {
		return 0, errors.New("workers are not started")
	}

	if workerID >= wp.numberOfWorkers {
		return 0, errors.New("worker ID exceeds number of workers")
	}

	return wp.workers[workerID].doneCount(), nil
}

func (wp *WorkerPool) getWorkerID(obj WorkObj) uint32 {
	h := fnv.New32a()
	h.Write([]byte(obj.GetKey()))
	return h.Sum32() % wp.numberOfWorkers
}

//GetPendingJobsCount get pending job
func (wp *WorkerPool) GetPendingJobsCount(workerID uint32) (uint32, error) {
	if !wp.Running() {
		return 0, errors.New("workers are not started")
	}

	if workerID >= wp.numberOfWorkers {
		return 0, errors.New("worker ID exceeds number of workers")
	}

	return wp.workers[workerID].pendingCount(), nil
}

//RunJob runs with user context
func (wp *WorkerPool) RunJob(workObj WorkObj) error {

	if !wp.Running() {
		return errors.New("workers are not started")
	}

	workerID := wp.getWorkerID(workObj)

	wp.workers[workerID].queue <- workCtx{workFunc: nil, workObj: workObj}

	return nil
}

//RunFunction runs jobs with specified function
func (wp *WorkerPool) RunFunction(workObj WorkObj, workerFunc WorkFunc) error {

	if !wp.Running() {
		return errors.New("workers are not started")
	}

	workerID := wp.getWorkerID(workObj)

	wp.workers[workerID].queue <- workCtx{workFunc: workerFunc, workObj: workObj}

	return nil
}

//IsIdle tells whether worker pool is idle
func (wp *WorkerPool) IsIdle() (bool, error) {

	if !wp.Running() {
		return false, errors.New("workers are not started")
	}

	for _, w := range wp.workers {
		if w.running() {
			return false, nil
		}
	}
	return true, nil
}
