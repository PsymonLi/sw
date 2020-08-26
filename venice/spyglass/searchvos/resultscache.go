package searchvos

import (
	"fmt"

	lru "github.com/hashicorp/golang-lru"

	"github.com/pensando/sw/venice/spyglass/searchvos/protos"
)

const (
	resultsCacheSize = 40000
)

type resultsCache struct {
	cache *lru.Cache
}

func newResultsCache() *resultsCache {
	cache, err := lru.New(resultsCacheSize)
	if err != nil {
		panic(fmt.Errorf("error in creating resultsCache, err %s", err.Error()))
	}
	return &resultsCache{
		cache: cache,
	}
}

func (rc *resultsCache) addFlowsProto(key string, value *protos.FlowRec) {
	rc.cache.Add(key, value)
}

func (rc *resultsCache) getFlowsProto(key string) (*protos.FlowRec, bool) {
	v, ok := rc.cache.Get(key)
	if !ok {
		return nil, false
	}
	return v.(*protos.FlowRec), true
}

func (rc *resultsCache) add(key string, value [][]string) {
	rc.cache.Add(key, value)
}

func (rc *resultsCache) get(key string) ([][]string, bool) {
	v, ok := rc.cache.Get(key)
	if !ok {
		return [][]string{}, false
	}
	return v.([][]string), true
}
