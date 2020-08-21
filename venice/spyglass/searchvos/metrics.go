package searchvos

import (
	"expvar"
	"strconv"
)

var metric = newMetrics()

type metrics struct {
	metaIndexCountPerShard     *expvar.Map
	metaFlowIndexCountPerShard *expvar.Map
	metaIndexFileCountPerShard *expvar.Map
	srcIdxsCountPerShard       *expvar.Map
	destIdxsCountPerShard      *expvar.Map
	currentQueryCacheLength    *expvar.Int
}

func newMetrics() *metrics {
	return &metrics{
		metaIndexCountPerShard:     expvar.NewMap("metaIndexCountPerShard"),
		metaFlowIndexCountPerShard: expvar.NewMap("metaFlowIndexCountPerShard"),
		metaIndexFileCountPerShard: expvar.NewMap("metaIndexFileCountPerShard"),
		srcIdxsCountPerShard:       expvar.NewMap("srcIdxsCountPerShard"),
		destIdxsCountPerShard:      expvar.NewMap("destIdxsCountPerShard"),
		currentQueryCacheLength:    expvar.NewInt("queryCacheLength"),
	}
}

func (m *metrics) addMetaIndexShardCount(shard int, count int64) {
	m.metaIndexCountPerShard.Add(strconv.Itoa(shard), count)
}

func (m *metrics) addMetaFlowIndexShardCount(shard int, count int64) {
	m.metaFlowIndexCountPerShard.Add(strconv.Itoa(shard), count)
}

func (m *metrics) addMetaIndexShardFileCount(shard int, count int64) {
	m.metaIndexFileCountPerShard.Add(strconv.Itoa(shard), count)
}

func (m *metrics) addSrcIdxsCountPerShard(shard int, count int64) {
	m.srcIdxsCountPerShard.Add(strconv.Itoa(shard), count)
}

func (m *metrics) addDestIdxsCountPerShard(shard int, count int64) {
	m.destIdxsCountPerShard.Add(strconv.Itoa(shard), count)
}

func (m *metrics) setCurrentQueryCacheLength(length int64) {
	m.currentQueryCacheLength.Set(length)
}
