package searchvos

import (
	"fmt"

	lru "github.com/hashicorp/golang-lru"

	"github.com/pensando/sw/venice/spyglass/searchvos/protos"
)

const (
	indexCacheSize = 20000
)

type indexCache struct {
	cache *lru.Cache
}

func newIndexCache() *indexCache {
	cache, err := lru.New(indexCacheSize)
	if err != nil {
		panic(fmt.Errorf("error in creating indexCache, err %s", err.Error()))
	}
	return &indexCache{
		cache: cache,
	}
}

func (rc *indexCache) addRawLogsShard(key string, value *protos.RawLogsShard) {
	rc.cache.Add(key, value)
}

func (rc *indexCache) getRawLogsShard(key string) *protos.RawLogsShard {
	v, ok := rc.cache.Get(key)
	if !ok {
		return nil
	}
	return v.(*protos.RawLogsShard)
}

func (rc *indexCache) addFlowPtrMap(key string, value *protos.FlowPtrMap) {
	rc.cache.Add(key, value)
}

func (rc *indexCache) getFlowPtrMap(key string) *protos.FlowPtrMap {
	v, ok := rc.cache.Get(key)
	if !ok {
		return nil
	}
	return v.(*protos.FlowPtrMap)
}

func (rc *indexCache) addShardIndex(key string, value *protos.CsvIndexShard) {
	rc.cache.Add(key, value)
}

func (rc *indexCache) getShardIndex(key string) *protos.CsvIndexShard {
	v, ok := rc.cache.Get(key)
	if !ok {
		return nil
	}
	return v.(*protos.CsvIndexShard)
}
