package searchvos

import (
	"context"
	"sync"
	"sync/atomic"
	"time"
)

type searchCache struct {
	lock sync.RWMutex

	// query cache
	qc map[string]*queryCache // string is the query id

	// index cache
	indexCache *indexCache

	// query results cache
	resultsCache *resultsCache
}

type queryCache struct {
	id             string
	cancel         func()
	logsChannel    chan []string
	objectsChannel chan string
	purgeDuration  time.Duration
	lastQueried    int64 // stored in seconds for doing atomic operations
}

func newSearchCache() *searchCache {
	sc := &searchCache{
		lock:         sync.RWMutex{},
		qc:           map[string]*queryCache{},
		indexCache:   newIndexCache(),
		resultsCache: newResultsCache(),
	}
	go sc.initQueryCachePurgeRoutine()
	return sc
}

func newQueryCache(id string, cancelFunc func(), purgeDuration time.Duration) *queryCache {
	qc := &queryCache{
		id:             id,
		cancel:         cancelFunc,
		logsChannel:    make(chan []string, logsCacheLimit),
		objectsChannel: make(chan string),
		purgeDuration:  defaultQueryContextPurgeDuration,
	}
	if purgeDuration != 0 {
		qc.purgeDuration = purgeDuration
	}
	return qc
}

func (sc *searchCache) addorGetQueryCache(ctx context.Context,
	cancelFunc func(), id string, addIfNotPresent bool,
	purgeDuration time.Duration) *queryCache {
	sc.lock.Lock()
	defer sc.lock.Unlock()
	qc, ok := sc.qc[id]
	if !ok && addIfNotPresent {
		qc = newQueryCache(id, cancelFunc, purgeDuration)
		sc.qc[id] = qc
		metric.setCurrentQueryCacheLength(int64(len(sc.qc)))
	}
	return qc
}

func (qc *queryCache) updateLastQueried() {
	// Update last queried time, its used for purging the querycache after timeout
	atomic.StoreInt64(&qc.lastQueried, time.Now().Unix())
}

func (sc *searchCache) initQueryCachePurgeRoutine() {
	for {
		select {
		case <-time.After(time.Second * 10):
			sc.lock.Lock()
			for k, q := range sc.qc {
				lq := atomic.LoadInt64(&q.lastQueried)
				if time.Now().Unix()-lq > int64(q.purgeDuration.Seconds()) {
					// purge the query cache and cancel its context
					q.cancel()
					delete(sc.qc, k)
					metric.setCurrentQueryCacheLength(int64(len(sc.qc)))
				}
			}
			sc.lock.Unlock()
		}
	}
}
