package shardworkers

import (
	"context"
	"errors"
	"hash/fnv"
	"sync"
	"sync/atomic"
)

type workerState int

const (
	notRunning workerState = iota
	running
)

//WorkFunc Prototype of work function.
type WorkFunc func(ctx context.Context, workObj WorkObj) error

//WorkObj interface
type WorkObj interface {
	GetKey() string
}

type worker struct {
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

func (w *worker) Run(ctx context.Context) {
	w.workMaster.StartWg.Done()
	defer w.workMaster.DoneWg.Done()
	for true {
		select {
		case workContext := <-w.queue:
			workContext.workFunc(ctx, workContext.workObj)
			atomic.AddUint32(&w.doneCnt, 1)
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
func (wp *WorkerPool) RunJob(workObj WorkObj, workerFunc WorkFunc) error {

	if !wp.Running() {
		return errors.New("workers are not started")
	}

	workerID := wp.getWorkerID(workObj)

	wp.workers[workerID].queue <- workCtx{workFunc: workerFunc, workObj: workObj}

	return nil
}
